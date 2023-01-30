#include "telnet_callbacks.hpp"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

static constexpr std::string_view prompt = "user@system:$ ";
static constexpr std::string_view responce = "Ok\n";

static constexpr std::string_view bye = "shutting down\n";

static char close_cmd[] = "close";
static constexpr std::string_view close_msg = "Connection closed.\n";

static char stop_cmd[] = "stop";
static char stop_msg[] = "Server stopped.\n";

static char shutdown_cmd[] = "shutdown";
static char shutdown_msg[] = "Server shuting down.\n";

static char testout_cmd[] = "test out";
static char testout[] =
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n"
  "Big text\n";

static std::string reason_names[] = {
  "InitiallBindFail",   "InitiallListenFail", "ConnectionAccepted",
  "ConnectionRejected", "ConnectionDeleted",  "ServerStarted",
  "ServerStopSignal",   "ServerStopped"};

static constexpr std::string_view client_password = "123";

void TelnetCallbacks::OnStartListening(FtTCP::Server& server)
{
  std::cout << "Listener started" << std::endl;
}

void TelnetCallbacks::OnClientConnect(FtTCP::Server& server,
                                      FtTCP::ClientHandle clientHandle)
{
  std::cout << "Client connected: " << clientHandle << std::endl;
  std::string_view sw = prompt;
  server.SendToClient(clientHandle, sw);
}

void TelnetCallbacks::OnClientDisconnect(FtTCP::Server& server,
                                         FtTCP::ClientHandle clientHandle)
{
  std::cout << "Client diconnected: " << clientHandle << std::endl;
}

void TelnetCallbacks::OnClientReceiveData(FtTCP::Server& server,
                                          FtTCP::ClientHandle clientHandle,
                                          const void* data, const size_t size)
{
  char* strdata = (char*)data;
  strdata[size] = 0;
  std::cout << "Client: " << clientHandle << " received: " << strdata
            << std::endl;
  server.SendToClient(clientHandle, responce);
  if (0 == memcmp(data, close_cmd, std::min(size, strlen(close_cmd)))) {
    server.SendToClient(clientHandle, close_msg);
    server.CloseClient(clientHandle);
  }
  else if (0 == memcmp(data, stop_cmd, std::min(size, strlen(stop_cmd)))) {
    server.SendToClient(clientHandle, stop_msg);
    m_stopping = true;
  }
  else if (0 ==
           memcmp(data, shutdown_cmd, std::min(size, strlen(shutdown_cmd)))) {
    server.SendToClient(clientHandle, shutdown_msg);
    m_stopping = true;
    m_shutingdown = true;
  }
  else if (0 ==
           memcmp(data, testout_cmd, std::min(size, strlen(shutdown_cmd)))) {
    server.SendToClient(clientHandle, testout);
    server.SendToClient(clientHandle, prompt);
  }
  else {
    server.SendToClient(clientHandle, prompt);
  }
}

void TelnetCallbacks::OnUpdate(
  FtTCP::Server& server, FtTCP::ServerReason reason, FtTCP::PlatformError err)
{
  std::cout << "Server updated: " << reason_names[reason]
            << " error: " << strerror(err) << std::endl;
  if (98 == err) // connection already used
    m_stopping = true;
}

bool TelnetCallbacks::OnClientPasswordEntered(
  FtTCP::Server& server, FtTCP::ClientHandle clientHandle, const void* data,
  const size_t size)
{
  std::string psw((const char*)data, size);
  while (psw.length() && psw.back() < 32)
  {
    psw.pop_back();
  }
  return (0 == client_password.compare(psw.c_str()));
}
