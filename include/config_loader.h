#ifndef CONFIG_LOADER_H_
#define CONFIG_LOADER_H_

#include <string>
#include <string_view>
#include <map>

struct ServerConfig {
  int port;
  std::string default_ip;
  bool log_queries;

  constexpr ServerConfig()
      : port{53}, default_ip{"127.0.0.1"}, log_queries{true} {}
};

class ConfigLoader {
 public:
  constexpr ConfigLoader() = default;
  ~ConfigLoader() = default;

  ConfigLoader(const ConfigLoader&) = delete;
  ConfigLoader& operator=(const ConfigLoader&) = delete;

  bool LoadFromFile(this const ConfigLoader& self, std::string_view filename);

  constexpr const ServerConfig& GetServerConfig() const noexcept {
    return server_config_;
  }

  constexpr const std::map<std::string, std::string>&
  GetDnsRecords() const noexcept {
    return dns_records_;
  }

  [[nodiscard("Don't ignore config validation result")]]
  bool ValidateConfig() const;

 private:
  ServerConfig server_config_;
  std::map<std::string, std::string> dns_records_;
};

#endif  // CONFIG_LOADER_H_