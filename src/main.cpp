#include <atomic>
#include <csignal>
#include <cstdlib>
#include <print>
#include <string>

#include "config_loader.h"
#include "simple_dns_server.h"

namespace {
std::atomic<bool> g_shutdown_requested{false};

void SignalHandler(int signal) {
  g_shutdown_requested.store(true);
  std::println("\nReceived signal {}, shutting down gracefully...", signal);
}
}  // namespace

int main(int argc, char* argv[]) {
  std::string config_file = [&]() -> std::string {
    if (argc > 1) {
      return argv[1];
    }
    return "config.json";
  }();

  ConfigLoader config_loader;
  if (!config_loader.LoadFromFile(config_file)) {
    std::println(stderr, "Failed to load configuration from: {}", config_file);
    return 1;
  }

  SimpleDnsServer dns_server;
  if (!dns_server.Initialize(config_loader)) {
    std::println(stderr, "Failed to initialize DNS server");
    return 1;
  }

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  auto shutdown_guard = [&dns_server]<typename T>(T&&) { dns_server.Stop(); };

  try {
    std::println("Server starting up... Press Ctrl+C to stop.");
    dns_server.Start();
  } catch (const std::exception& e) {
    std::println(stderr, "Error: {}", e.what());
    return 1;
  }

  shutdown_guard(0);  // Ensure cleanup
  return 0;
}