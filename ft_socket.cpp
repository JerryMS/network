#include "ft-socket/ft_socket.hpp"

#include "esp_log.h"

#include <errno.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace FtTCP {

Socket::Socket(AddressPtr address)
  : m_socket{INVALID_SOCKET}, m_address(address)
{
  if (!m_address) {
    return;
  }
  if (!m_address->IsValid()) {
    return;
  }
  const sockaddr_in* addr = m_address->GetAddress();
  m_socket = socket(addr->sin_family, m_address->GetSocketType(), m_address->GetProto());
}

Socket::Socket(AddressPtr address, PlatformSocket sock)
  : m_socket(sock), m_address(address)
{
}

Socket::~Socket()
{
  if (m_socket != INVALID_SOCKET) {
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
  }
}

std::string Socket::ErrorsToStr() const
{
  std::string res;
  while (!m_errors.empty()) {
    res.append(std::to_string(m_errors.front()));
    res.append("\n\0");
    m_errors.pop();
  }
  return res;
}

bool Socket::IsInvalid() const
{
  if (m_socket == INVALID_SOCKET)
    return true;

  fd_set socketSetErr;
  timeval tmval{0, TCP_NODELAY_US};

  FD_ZERO(&socketSetErr);
  FD_SET(m_socket, &socketSetErr);
  PlatformError ErrDescr = select(static_cast<int>(m_socket + 1), (fd_set*)0,
                                  (fd_set*)0, &socketSetErr, &tmval);
  if (ErrDescr != 0) // invalid
  {
    return true;
  }

  fd_set socketSetWr;
  FD_ZERO(&socketSetWr);
  FD_SET(m_socket, &socketSetWr);
  PlatformError WrDescr = select(static_cast<int>(m_socket + 1), (fd_set*)0,
                                 &socketSetWr, (fd_set*)0, &tmval);
  if (WrDescr < 1) // invalid
  {
    PlatformError lastError = errno;
    m_errors.push(lastError);
    return true;
  }
  return false; // socket is valid
}

void Socket::SetNonBlocking(bool nonBlocking)
{
  u_long mode = nonBlocking ? 1 : 0;
  ioctl(m_socket, FIONBIO, &mode);
}

bool Socket::Connect()
{
  if (m_socket == INVALID_SOCKET) {
    return false;
  }

  struct timeval tmval {
    0, TCP_NODELAY_US
  };
  PlatformError result =
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &tmval, sizeof(tmval));
  if (SOCKET_ERROR == result) {
    PlatformError lastError = errno;
    m_errors.push(lastError);
    return false;
  }
  const sockaddr_in* addr = m_address->GetAddress();
  result =
    connect(m_socket, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
  if (SOCKET_ERROR == result) {
    PlatformError lastError = errno;
    m_errors.push(lastError);
    return false;
  }

  return true;
}

bool Socket::Bind()
{
  const sockaddr_in* addr = m_address->GetAddress();
  ESP_LOGI("TELNET", "socket bind: %s", m_address->toString().c_str());
  // reuse address in case server socket
  PlatformSocket option = 1;
  setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  PlatformError result =
    bind(m_socket, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
  if (result == SOCKET_ERROR) {
    PlatformError lastError = errno;
    if (lastError != EAGAIN && lastError != EINPROGRESS) {
      m_errors.push(lastError);
      return false;
    }
  }
  return true;
}

bool Socket::Listen()
{
  PlatformError result = listen(m_socket, MAX_BACKLOG);
  if (result == SOCKET_ERROR) {
    PlatformError lastError = errno;
    if (lastError != EAGAIN && lastError != EINPROGRESS) {
      m_errors.push(lastError);
      return false;
    }
  }
  return true;
}

SocketPtr Socket::Accept(std::chrono::milliseconds timeout)
{
  struct timeval tmval {
    0, static_cast<suseconds_t>(
         std::chrono::duration_cast<std::chrono::microseconds>(timeout).count())
  };
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(m_socket, &rfds);
  PlatformError selectresult = select(
    static_cast<int>(m_socket + 1), &rfds, (fd_set*)0, (fd_set*)0, &tmval);
  if (selectresult > 0) {
    PlatformSocket newSocket = INVALID_SOCKET;
    newSocket = accept(m_socket, nullptr, nullptr);
    return CreateSocket(m_address, newSocket);
  }

  return nullptr;
}

bool Socket::IsReadyForRead(std::chrono::milliseconds timeout)
{
  fd_set socketSet;
  FD_ZERO(&socketSet);
  FD_SET(m_socket, &socketSet);
  struct timeval tmval {
    0, static_cast<suseconds_t>(
         std::chrono::duration_cast<std::chrono::microseconds>(timeout).count())
  };
  PlatformError result = select(
    static_cast<int>(m_socket + 1), &socketSet, (fd_set*)0, (fd_set*)0, &tmval);
  if (result == SOCKET_ERROR) {
    PlatformError lastError = errno;
    m_errors.push(lastError);
    return false;
  }
  return (result == 1) ? true : false;
}

bool Socket::IsReadyForWrite(std::chrono::milliseconds timeout)
{
  fd_set socketSet;
  FD_ZERO(&socketSet);
  FD_SET(m_socket, &socketSet);
  struct timeval tmval {
    0, static_cast<suseconds_t>(
         std::chrono::duration_cast<std::chrono::microseconds>(timeout).count())
  };
  PlatformError result = select(
    static_cast<int>(m_socket + 1), (fd_set*)0, &socketSet, (fd_set*)0, &tmval);

  if (result == SOCKET_ERROR) {
    PlatformError lastError = errno;
    m_errors.push(lastError);
    return false;
  }
  return (result == 1) ? true : false;
}

size_t Socket::Receive(void* data, size_t bytes, uint32_t flags)
{
  auto received =
    recv(m_socket, static_cast<char*>(data), static_cast<int>(bytes), flags);
  if (received == SOCKET_ERROR) {
    PlatformError lastError = errno;
    m_errors.push(lastError);
    return 0;
  }
  return static_cast<size_t>(received);
}

bool Socket::Send(void* data, size_t bytes, uint32_t flags, size_t* bytesSent)
{
  if (nullptr == bytesSent) {
    return false;
  }
  ssize_t sent = send(m_socket, static_cast<const char*>(data),
                      static_cast<int>(bytes), flags);
  if (sent == SOCKET_ERROR || sent <= 0) {
    PlatformError lastError = errno;
    ESP_LOGE("TELNET", "Error send %d", lastError);
    m_errors.push(lastError);
    return false;
  }
  *bytesSent += static_cast<size_t>(sent);
  return true;
}

bool Socket::SendDatagram(const void* data, size_t bytes)
{
  if (nullptr == data || IPProto::eUDP != m_address->GetProto()) {
    return false;
  }
  const sockaddr_in* addr = m_address->GetAddress();
  int sended = sendto(m_socket, data, bytes, MSG_NOSIGNAL, (struct sockaddr*)addr, sizeof(sockaddr_in));
  bool res = (sended > -1);
  if (!res)
  {
    PlatformError lastError = errno;
    m_errors.push(lastError);
  }
  return res;
}

SocketPtr Socket::CreateSocket(AddressPtr address)
{
  return std::make_shared<Socket>(address);
}

SocketPtr Socket::CreateSocket(AddressPtr address, PlatformSocket sock)
{
  return std::make_shared<Socket>(address, sock);
}
} // namespace FtTCP