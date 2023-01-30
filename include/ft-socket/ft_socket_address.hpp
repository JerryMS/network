#pragma once

#include <arpa/inet.h>
#include <memory>
#include <netdb.h>
#include <string>

namespace FtTCP {
class Address;

using AddressPtr = std::shared_ptr<Address>;

class Address {
protected:
  static constexpr unsigned int MAX_BUFFER_LENGTH = 128;

  std::string m_host;
  int m_port;
  std::string m_presentation;
  sockaddr_in m_address;

  bool m_isValid;
  bool m_isListener;
  bool m_isAsync;

  void FillPresentation();

public:
  Address(const char* host, const int port);
  Address(const int port, const bool async);
  ~Address() = default;

  bool IsEmpty() const noexcept;
  bool IsValid() const noexcept;
  bool IsListener() const noexcept;
  std::string toString() const;
  const sockaddr_in* GetAddress() const;

  static AddressPtr CreateClientAddress(const char* host, const int port);
  static AddressPtr CreateListenerAddress(const int port, const bool isAsync);
};
} // namespace FtTCP
