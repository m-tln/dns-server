#include <expected>
#include <fstream>
#include <print>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include "config_loader.h"
#include <nlohmann/json.hpp>

namespace cfg {
enum class ValidationError {
  InvalidPort,
  InvalidIpAddress,
  FileNotFound,
  JsonParseError
};

using ValidationResult = std::expected<void, ValidationError>;
}  // namespace cfg

bool ConfigLoader::LoadFromFile(this const ConfigLoader& self,
                                std::string_view filename) {
  try {
    auto& mutable_self = const_cast<ConfigLoader&>(self);

    std::ifstream config_file(std::string{filename});
    if (!config_file.is_open()) {
      std::println(stderr, "Error: Cannot open config file: {}", filename);
      return false;
    }

    nlohmann::json config_json;
    config_file >> config_json;

    if (const auto& server_json = config_json["server"];
        config_json.contains("server")) {
      if (auto it = server_json.find("port"); it != server_json.end()) {
        mutable_self.server_config_.port = it->get<int>();
      }
      if (auto it = server_json.find("default_ip"); it != server_json.end()) {
        mutable_self.server_config_.default_ip = it->get<std::string>();
      }
      if (auto it = server_json.find("log_queries"); it != server_json.end()) {
        mutable_self.server_config_.log_queries = it->get<bool>();
      }
    }

    if (config_json.contains("dns_records")) {
      for (const auto& [domain, ip] : config_json["dns_records"].items()) {
        mutable_self.dns_records_.emplace(domain, ip.get<std::string>());
      }
    }

    config_file.close();

    if (!mutable_self.ValidateConfig()) {
      return false;
    }

    std::println("Configuration loaded from: {}", filename);
    std::println("Loaded {} DNS records", mutable_self.dns_records_.size());
    return true;

  } catch (const nlohmann::json::exception& e) {
    std::println(stderr, "JSON error: {}", e.what());
    return false;
  } catch (const std::exception& e) {
    std::println(stderr, "Error loading config: {}", e.what());
    return false;
  }
}

bool ConfigLoader::ValidateConfig() const {
  if (int port = server_config_.port; port < 1 || port > 65535) {
    std::println(stderr, "Error: Invalid port number: {}", port);
    return false;
  }

  // Validate IP addresses using inet_pton
  struct sockaddr_in sa;

  if (inet_pton(AF_INET, std::string_view{server_config_.default_ip}.data(),
                &(sa.sin_addr)) != 1) {
    std::println(stderr, "Error: Invalid default IP address: {}",
                 server_config_.default_ip);
    return false;
  }

  for (const auto& [domain, ip] : dns_records_) {
    if (inet_pton(AF_INET, std::string_view{ip}.data(), &(sa.sin_addr)) != 1) {
      std::println(stderr, "Error: Invalid IP address for domain {}: {}",
                   domain, ip);
      return false;
    }
  }

  std::println("Configuration validated successfully");
  return true;
}