#include <algorithm>
#include <chrono>
#include <print>
#include <thread>
#include <utility>

#include "simple_dns_server.h"

SimpleDnsServer::~SimpleDnsServer() {
  Stop();
}

bool SimpleDnsServer::Initialize(const ConfigLoader& config_loader) {
  const ServerConfig& server_config = config_loader.GetServerConfig();

  default_ip_ = server_config.default_ip;
  log_queries_ = server_config.log_queries;
  dns_records_ = config_loader.GetDnsRecords();

#ifdef _WIN32
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    std::println(stderr, "WSAStartup failed");
    return false;
  }
#endif

  sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd_ == INVALID_SOCKET) {
    std::println(stderr, "Error creating socket");
    return false;
  }

  if constexpr (INVALID_SOCKET != -1) {  // Windows
    const char yes = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  } else {  // Unix-like
    const int yes = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  }

  struct sockaddr_in server_addr{.sin_len = 0,
                                 .sin_family = AF_INET,
                                 .sin_port = htons(server_config.port),
                                 .sin_addr = {.s_addr = INADDR_ANY},
                                 .sin_zero = {0}};

  if (bind(sockfd_, reinterpret_cast<struct sockaddr*>(&server_addr),
           sizeof(server_addr)) == SOCKET_ERROR) {
    std::println(stderr, "Error binding socket to port {}", server_config.port);

#ifdef _WIN32
    closesocket(sockfd_);
    WSACleanup();
#else
    closesocket(sockfd_);
#endif
    sockfd_ = INVALID_SOCKET;
    return false;
  }

  std::println("DNS server started on port {}", server_config.port);
  std::println("Loaded {} DNS records", dns_records_.size());
  std::println("Default IP for unknown domains: {}", default_ip_);

  return true;
}

std::string
SimpleDnsServer::ExtractDomainName(std::span<const unsigned char> buffer,
                                   int& position) const {
  std::string domain;
  int len = buffer[position];

  while (len != 0) {
    if ((len & 0xC0) == 0xC0) {
      position += 2;
      break;
    }

    ++position;
    auto label_view = buffer.subspan(position, len);
    domain.append(label_view.begin(), label_view.end());
    domain += '.';
    position += len;

    len = buffer[position];
  }

  if (!domain.empty() && domain.back() == '.') {
    domain.pop_back();
  }

  ++position;
  return domain;
}

std::optional<std::string>
SimpleDnsServer::FindIpForDomain(const std::string& domain) const {
  if (auto it = dns_records_.find(domain); it != dns_records_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<unsigned char>
SimpleDnsServer::CreateDnsResponse(std::span<const unsigned char> query,
                                   std::string_view ip) const {

  std::vector<unsigned char> response;
  response.reserve(query.size() + 16);

  std::ranges::copy(query, std::back_inserter(response));

  response[2] = 0x81;
  response[3] = 0x80;
  response[7] = 1;

  constexpr auto answer = std::to_array<unsigned char>({
      0xC0, 0x0C,              // Compressed name pointer
      0x00, 0x01,              // Type A
      0x00, 0x01,              // Class IN
      0x00, 0x00, 0x01, 0x2C,  // TTL 300
      0x00, 0x04               // Data length
  });

  response.insert(response.end(), answer.begin(), answer.end());

  // IP address
  struct in_addr addr;
  if (inet_pton(AF_INET, ip.data(), &addr) != 1) {
    inet_pton(AF_INET, default_ip_.c_str(), &addr);
  }

  uint32_t ip_num = ntohl(addr.s_addr);

  auto bytes = std::bit_cast<std::array<unsigned char, 4>>(ip_num);
  response.insert(response.end(), bytes.begin(), bytes.end());

  return response;
}

void SimpleDnsServer::HandleDnsQuery() {
  std::array<unsigned char, 512> buffer;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  int bytes_received = recvfrom(
      sockfd_, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0,
      reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);

  if (bytes_received < 12)
    return;

  auto query_span = std::span{buffer}.first(bytes_received);
  int position = 12;
  std::string domain = ExtractDomainName(query_span, position);

  if (log_queries_) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    std::println("DNS query from {} for: {}", client_ip, domain);
  }

  std::string ip =
      FindIpForDomain(domain)
          .and_then(
              [](const std::string& found_ip) -> std::optional<std::string> {
                return found_ip;
              })
          .value_or(default_ip_);

  if (log_queries_) {
    if (ip != default_ip_) {
      std::println("Found IP: {} for domain: {}", ip, domain);
    } else {
      std::println("Domain not found: {}, returning: {}", domain, ip);
    }
  }

  auto response = CreateDnsResponse(query_span, ip);
  sendto(sockfd_, reinterpret_cast<const char*>(response.data()),
         response.size(), 0, reinterpret_cast<struct sockaddr*>(&client_addr),
         client_len);
}

void SimpleDnsServer::Start() {
  is_running_.store(true);
  std::println("DNS server started. Waiting for requests...\n");

  while (IsRunning()) {
    HandleDnsQuery();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void SimpleDnsServer::Stop() {
  is_running_.store(false);
  if (sockfd_ != INVALID_SOCKET) {
    closesocket(sockfd_);
    sockfd_ = INVALID_SOCKET;
  }

#ifdef _WIN32
  WSACleanup();
#endif
}

void SimpleDnsServer::AddRecord(std::string_view domain, std::string_view ip) {
  dns_records_.insert_or_assign(std::string{domain}, std::string{ip});
  std::println("Added record: {} -> {}", domain, ip);
}