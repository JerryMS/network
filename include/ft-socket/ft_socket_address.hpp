#pragma once

#include <arpa/inet.h>
#include <memory>
#include <netdb.h>
#include <string>

namespace FtTCP {
class Address;

using AddressPtr = std::shared_ptr<Address>;

enum IPProto { eTCP = IPPROTO_TCP, eUDP = IPPROTO_UDP };

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
  IPProto m_proto;
  int m_sockType;

  void FillPresentation();

public:
  Address(const char* host, const int port, IPProto proto = IPProto::eTCP);
  Address(const int port, const bool async, IPProto proto = IPProto::eTCP);
  ~Address() = default;

  bool IsEmpty() const noexcept;
  bool IsValid() const noexcept;
  bool IsListener() const noexcept;
  std::string toString() const;
  const sockaddr_in* GetAddress() const;
  IPProto GetProto() const;
  int GetSocketType() const;

  static AddressPtr CreateBroadcastAddress(const char* self_ip, const int port);
  static AddressPtr CreateClientAddress(const char* host, const int port);
  static AddressPtr CreateListenerAddress(const int port, const bool isAsync);
};
} // namespace FtTCP
