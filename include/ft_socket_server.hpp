#pragma once

#include "ft_socket.hpp"
#include "ft_socket_queues.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <thread>

namespace FtTCP {

class Server;

using ServerPtr = std::shared_ptr<Server>;

using ClientHandle = long unsigned int;

enum ServerReason {
  InitiallBindFail,
  InitiallListenFail,
  ConnectionAccepted,
  ConnectionRefused,
  ConnectionDeleted,
  ServerStarted,
  ServerStopSignal,
  ServerStopped
};
struct ServerParameters {
  unsigned short int port;
  unsigned short int maxConnections;
  std::chrono::seconds clientTimeOut;
};

using OnStartListeningFnType = std::function<void(Server&)>;
using OnClientConnectFnType = std::function<void(Server&, ClientHandle)>;
using OnClientDisconnectFnType = std::function<void(Server&, ClientHandle)>;
using OnClientReceiveDataFnType =
  std::function<void(Server&, ClientHandle, const void*, size_t)>;
using OnUpdateFnType =
  std::function<void(Server&, ServerReason, PlatformError)>;
using OnPasswordEntered =
  std::function<bool(Server&, ClientHandle, const void*, size_t)>;

class Server {
  enum Stage { Initializing, Listening, Shutingdown };
  static constexpr char TAG[] = "TELNET";

private:
  struct Client {
    Client(const Server& svr) : server(svr), clientHandle(-1), connected(false)
    {
    }
    const Server& server;
    std::thread thread;
    SocketPtr socket;
    ClientHandle clientHandle;
    std::atomic_bool connected;
    SocketSendQueue forSend;
  };

  using ClientPtr = std::shared_ptr<Client>;

  static constexpr std::chrono::milliseconds START_SERVER{500};
  static constexpr std::chrono::milliseconds CLIENT_THROTTLE_TIME{5};
  static constexpr std::chrono::milliseconds LISTENER_THROTTLE_TIME{5};
  static constexpr std::chrono::milliseconds ACCEPT_TIMEOUT{1000};
  static constexpr std::chrono::milliseconds NOWAIT{0};
  static constexpr std::size_t RECEIVE_BUFFER_SIZE{256};
  static constexpr std::string_view PASSWORD_PROMPT = "password: ";
  static constexpr std::string_view WRONG_PASSWORD_MESSAGE = "wrong password\n";

  std::thread m_listenerThread;
  std::mutex m_listenerMutex;
  std::mutex m_notifierMutex;
  std::condition_variable m_listenerStageCond;
  std::atomic<Stage> m_stage;
  ServerParameters m_parameters;
  SocketPtr m_listenerSocket;
  std::map<ClientHandle, ClientPtr> m_clients;
  mutable std::queue<ClientHandle> m_clientsForDelete;
  ClientHandle m_clientHandlesCounter{0};
  std::string m_commandPrompt = "@";

  OnStartListeningFnType m_onStartListening = nullptr;
  OnClientConnectFnType m_onConnect = nullptr;
  OnClientDisconnectFnType m_onDisconnect = nullptr;
  OnClientReceiveDataFnType m_onReceiveData = nullptr;
  OnUpdateFnType m_onUpdate = nullptr;
  OnPasswordEntered m_onPasswordEntered = nullptr;

  void Run();
  void RunClient(ClientPtr client);
  bool ProcessClientPassword(const ClientHandle clientHandle, const void* data,
                             const size_t size);
  bool DoInitializing();
  bool DoListening();
  void CleanupClients();

public:
  Server(const ServerParameters& params);
  ~Server();

  static ServerPtr CreateServer(unsigned short int port,
                                unsigned short int maxConnection);

  void Start();
  void Stop();

  void SendToClient(ClientHandle clientHandle, const void* data, size_t size);
  void CloseClient(ClientHandle clientHandle);

  bool SetOnStartListeningCallback(OnStartListeningFnType onStartListening);
  bool SetOnClientConnectCallback(OnClientConnectFnType onConnect);
  bool SetOnClientDisconnectCallback(OnClientDisconnectFnType onDisconnect);
  bool SetOnReceiveDataCallback(OnClientReceiveDataFnType onReceiveData);
  bool SetOnServerUpdate(OnUpdateFnType onUpdate);
  bool SetOnPasswordEntered(OnPasswordEntered onPasswordEntered);
};

} // namespace FtTCP