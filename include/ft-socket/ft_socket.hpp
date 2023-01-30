#pragma once

#include "ft_socket_address.hpp"

#include <chrono>
#include <queue>

namespace FtTCP {
class Socket;

using SocketPtr = std::shared_ptr<Socket>;
using PlatformSocket = int;
using PlatformError = int;

static constexpr PlatformSocket INVALID_SOCKET = -1;
static constexpr PlatformError SOCKET_ERROR = -1;

static constexpr int TCP_NODELAY_US = 100;
static constexpr int MAX_BACKLOG = 2;

class Socket {
private:
  PlatformSocket m_socket;
  AddressPtr m_address;
  mutable std::queue<PlatformError> m_errors;

public:
  Socket(AddressPtr address);
  Socket(AddressPtr address, PlatformSocket sock);
  ~Socket();

  bool IsInvalid() const;

  std::string ErrorsToStr() const;
  void SetNonBlocking(bool nonBlocking);
  bool Connect();
  bool Bind();
  bool Listen();
  bool IsReadyForRead(std::chrono::milliseconds timeout);
  bool IsReadyForWrite(std::chrono::milliseconds timeout);
  SocketPtr Accept(std::chrono::milliseconds timeout);
  size_t Receive(void* data, size_t bytes, uint32_t flags);
  bool Send(void* data, size_t bytes, uint32_t flags, size_t* bytesSent);

  static SocketPtr CreateSocket(AddressPtr address);
  static SocketPtr CreateSocket(AddressPtr address, PlatformSocket sock);
};
} // namespace FtTCP
