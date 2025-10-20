#ifndef SIMPLE_DNS_SERVER_H_
#define SIMPLE_DNS_SERVER_H_

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close
#endif

#include <atomic>
#include <cstring>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "config_loader.h"

class SimpleDnsServer {
 public:
  constexpr SimpleDnsServer() noexcept = default;
  ~SimpleDnsServer();

  SimpleDnsServer(const SimpleDnsServer&) = delete;
  SimpleDnsServer& operator=(const SimpleDnsServer&) = delete;

  bool Initialize(const ConfigLoader& config_loader);
  void Start();
  void Stop();

  [[nodiscard("Check if server is running before operations")]]
  constexpr bool IsRunning() const noexcept {
    return is_running_.load();
  }

  void AddRecord(std::string_view domain, std::string_view ip);

 private:
  std::string ExtractDomainName(std::span<const unsigned char> buffer,
                                int& position) const;
  std::vector<unsigned char>
  CreateDnsResponse(std::span<const unsigned char> query,
                    std::string_view ip) const;
  void HandleDnsQuery();
  std::optional<std::string> FindIpForDomain(const std::string& domain) const;

  SOCKET sockfd_{INVALID_SOCKET};
  std::map<std::string, std::string> dns_records_;
  std::atomic<bool> is_running_{false};
  std::string default_ip_{"127.0.0.1"};
  bool log_queries_{true};
};

#endif  // SIMPLE_DNS_SERVER_H_