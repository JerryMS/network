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
  std::function<void(Server&, ClientHandle, const void*, const size_t)>;
using OnUpdateFnType =
  std::function<void(Server&, ServerReason, PlatformError)>;
using OnPasswordEntered =
  std::function<bool(Server&, ClientHandle, const void*, const size_t)>;

class Server {
  enum Stage { Initializing, Listening, Shutingdown }; 

private:
  struct Client {
    /*
    Client(const Server& svr) : server(svr), clientHandle(-1), connected(false)
    {
    }
    */
    Client(const Server& svr, ClientHandle client, bool conn, SocketPtr sock): 
      server(svr), socket(sock), clientHandle(client), connected(conn)
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
  // guard the updates of client and containers
  std::mutex m_listenerMutex;
  // guard the set/use event function like OnUpdate/OnConnect
  std::mutex m_notifierMutex;
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
  static constexpr char TAG[] = "TELNET";

  Server(const ServerParameters& params);
  ~Server();

  static ServerPtr CreateServer(unsigned short int port,
                                unsigned short int maxConnection);

  void Start();
  void Stop();

  void SetPrompt(const char* prompt);

  void SendToClient(ClientHandle clientHandle, const std::string_view& msg);
  void ShowPrompt(ClientHandle clientHandle);
  void CloseClient(ClientHandle clientHandle);  

  template<class T>
  bool SetOnStartListeningCallback(T* const object,
                                   void (T::*const onStartListening)(Server&))
  {
    if (Stage::Initializing != m_stage)
      return false;
    using namespace std::placeholders;
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_onStartListening = static_cast<OnStartListeningFnType>(
      std::bind(onStartListening, object, _1));
    return true;
  };

  template<class T>
  bool SetOnClientConnectCallback(
    T* const object, void (T::*const onClientConnect)(Server&, ClientHandle))
  {
    if (Stage::Initializing != m_stage)
      return false;
    using namespace std::placeholders;
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_onConnect = static_cast<OnClientConnectFnType>(
      std::bind(onClientConnect, object, _1, _2));
    return true;
  }

  template<class T>
  bool SetOnClientDisconnectCallback(
    T* const object, void (T::*const onClientDisconnect)(Server&, ClientHandle))
  {
    if (Stage::Initializing != m_stage)
      return false;
    using namespace std::placeholders;
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_onDisconnect = static_cast<OnClientDisconnectFnType>(
      std::bind(onClientDisconnect, object, _1, _2));
    return true;
  }

  template<class T>
  bool SetOnReceiveDataCallback(
    T* const object,
    void (T::*const onReceiveData)(Server&, ClientHandle, const void*, size_t))
  {
    if (Stage::Initializing != m_stage)
      return false;
    using namespace std::placeholders;
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_onReceiveData = static_cast<OnClientReceiveDataFnType>(
      std::bind(onReceiveData, object, _1, _2, _3, _4));
    return true;
  }

  template<class T>
  bool SetOnServerUpdate(T* const object,
                         void (T::*const onUpdate)(Server&, ServerReason,
                                                   PlatformError))
  {
    if (Stage::Initializing != m_stage)
      return false;
    using namespace std::placeholders;
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_onUpdate =
      static_cast<OnUpdateFnType>(std::bind(onUpdate, object, _1, _2, _3));
    return true;
  }

  template<class T>
  bool SetOnPasswordEntered(T* const object,
                            bool (T::*const onPasswordEntered)(
                              Server&, ClientHandle, const void*, size_t))
  {
    if (Stage::Initializing != m_stage)
      return false;
    using namespace std::placeholders;
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    m_onPasswordEntered = static_cast<OnPasswordEntered>(
      std::bind(onPasswordEntered, object, _1, _2, _3, _4));
    return true;
  }
};

} // namespace FtTCP