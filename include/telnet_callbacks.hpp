#pragma once

#include "ft-socket/ft_socket_server.hpp"

class TelnetCallbacks {
public:
  std::atomic_bool m_stopping{false};
  std::atomic_bool m_shutingdown{false};

  void OnStartListening(FtTCP::Server& server);
  void OnClientConnect(FtTCP::Server& server, FtTCP::ClientHandle clientHandle);
  void OnClientDisconnect(FtTCP::Server& server,
                          FtTCP::ClientHandle clientHandle);
  void OnClientReceiveData(FtTCP::Server& server,
                           FtTCP::ClientHandle clientHandle, const void* data,
                           const size_t size);
  void OnUpdate(FtTCP::Server& server, FtTCP::ServerReason reason,
                FtTCP::PlatformError err);
  bool OnClientPasswordEntered(FtTCP::Server& server,
                               FtTCP::ClientHandle clientHandle,
                               const void* data, const size_t size);
};
