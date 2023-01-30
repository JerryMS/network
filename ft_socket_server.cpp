#include "ft-socket/ft_socket_server.hpp"

#include "esp_log.h"

namespace FtTCP {
Server::Server(const ServerParameters& params)
{
  m_parameters = params;
  m_stage = Stage::Initializing;
}

Server::~Server()
{
  Stop();
}

void Server::Start()
{
  m_stage = Server::Stage::Initializing;
  m_listenerThread = std::thread([this]() { this->Run(); });
}

void Server::Stop()
{
  {
    // mutex prevent change event function on calling
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    if (m_onUpdate) {
      m_onUpdate(*this, ServerReason::ServerStopSignal, 0);
    }
  }

  m_stage.store(Stage::Shutingdown);

  if (m_listenerThread.joinable())
    m_listenerThread.join();
}

void Server::SetPrompt(const char* prompt)
{
  m_commandPrompt = "\033[0;32m";
  m_commandPrompt += prompt;
  m_commandPrompt += "\033[0m ";
}

bool Server::DoInitializing()
{
  AddressPtr address = Address::CreateListenerAddress(m_parameters.port, true);
  m_listenerSocket = Socket::CreateSocket(address);
  m_listenerSocket->SetNonBlocking(true);
  if (m_listenerSocket->Bind() == false) {
    m_stage = Stage::Shutingdown;
    // mutex prevent change event function on calling
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    if (m_onUpdate) {
      m_onUpdate(*this, ServerReason::InitiallBindFail, errno);
    }
    return false;
  }
  if (m_listenerSocket->Listen() == false) {
    m_stage = Stage::Shutingdown;
    // mutex prevent change event function on calling
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    if (m_onUpdate) {
      m_onUpdate(*this, ServerReason::InitiallListenFail, errno);
    }
    return false;
  }

  m_stage = Stage::Listening;
  return true;
}

bool Server::DoListening()
{
  SocketPtr connectionSocket = m_listenerSocket->Accept(ACCEPT_TIMEOUT);
  if (connectionSocket && connectionSocket->IsInvalid() == false) {
    auto client = std::make_shared<Client>(
      *this, ++m_clientHandlesCounter, true, connectionSocket);
    {
      std::lock_guard<std::mutex> lock_listener(m_listenerMutex);
      if (m_clients.size() >= m_parameters.maxConnections) {
        // mutex prevent change event function on calling
        std::lock_guard<std::mutex> lock(m_notifierMutex);
        if (m_onUpdate) {
          m_onUpdate(*this, ServerReason::ConnectionRefused, 0);
        }
        return false;
      }
    }

    client->thread = std::thread([this, client]() { this->RunClient(client); });
    {
      std::lock_guard<std::mutex> lock(m_listenerMutex);
      m_clients[client->clientHandle] = client;
    }

    // mutex prevent change event function on calling
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    if (m_onUpdate) {
      m_onUpdate(*this, ServerReason::ConnectionAccepted, 0);
    }

    return true;
  }
  return false;
}

void Server::CleanupClients()
{
  bool ClientsDeleted = false;
  {
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    while (!m_clientsForDelete.empty()) {
      ClientHandle cl = m_clientsForDelete.front();
      auto clIter = m_clients.find(cl);
      if (clIter != m_clients.end()) {
        if (clIter->second->thread.joinable())
          clIter->second->thread.join();
        m_clients.erase(clIter);
        ClientsDeleted = true;
      }
      m_clientsForDelete.pop();
    }
  }
  {
    // mutex prevent change event function on calling
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    if (ClientsDeleted && m_onUpdate) {
      m_onUpdate(*this, ServerReason::ConnectionDeleted, 0);
    }
  }
}

void Server::Run()
{
  {
    // mutex prevent change event function on calling
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    if (m_onUpdate) {
      m_onUpdate(*this, ServerReason::ServerStarted, 0);
    }
  }
  ESP_LOGI(TAG, "Started");

  Stage stage;
  while (Stage::Shutingdown != (stage = m_stage.load())) {
    switch (stage) {
    case Stage::Initializing:
      ESP_LOGI(TAG, "Initializing");
      if (!DoInitializing()) {
        ESP_LOGW(TAG, "Initializing failed");
        continue;
      }
      break;
    case Stage::Listening:
      if (!DoListening()) {
        CleanupClients();
        std::this_thread::sleep_for(LISTENER_THROTTLE_TIME);
        continue;
      }
      break;
    default: std::this_thread::sleep_for(LISTENER_THROTTLE_TIME); break;
    }
  }

  ESP_LOGI(TAG, "Stopping");

  {
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    for (auto& client : m_clients)
      client.second->thread.join();

    m_clients.clear();
  }

  {
    // mutex prevent change event function on calling
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    if (m_onUpdate) {
      m_onUpdate(*this, ServerReason::ServerStopped, 0);
    }
  }
  m_listenerSocket = nullptr;
  ESP_LOGI(TAG, "Stopped");
}

bool Server::ProcessClientPassword(const ClientHandle clientHandle,
                                   const void* data, const size_t size)
{
  // mutex prevent change event function on calling
  std::lock_guard<std::mutex> lock(m_notifierMutex);
  if (nullptr == m_onPasswordEntered) {
    SendToClient(clientHandle, WRONG_PASSWORD_MESSAGE);
    SendToClient(clientHandle, PASSWORD_PROMPT);
    return false;
  }

  if (!m_onPasswordEntered(*this, clientHandle, data, size)) {
    SendToClient(clientHandle, WRONG_PASSWORD_MESSAGE);
    SendToClient(clientHandle, PASSWORD_PROMPT);
    return false;
  }

  return true;
}

void Server::RunClient(ClientPtr client)
{
  bool awaitPassword = true;

  std::chrono::time_point timeoutTime =
    std::chrono::system_clock::now() +
    client->server.m_parameters.clientTimeOut;

  Buffer receiveBuffer(RECEIVE_BUFFER_SIZE);

  std::this_thread::sleep_for(CLIENT_THROTTLE_TIME);

  SendToClient(client->clientHandle, PASSWORD_PROMPT);

  while (client->connected && Stage::Shutingdown != m_stage.load()) {
    std::this_thread::sleep_for(CLIENT_THROTTLE_TIME);

    if (std::chrono::system_clock::now() >= timeoutTime) {
      client->connected = false;
      break;
    }

    if (client->socket->IsReadyForWrite(NOWAIT)) {
      while (!client->forSend.IsEmpty()) {
        if (!client->forSend.Send(client->socket)) {
          client->connected = false;
          break;
        }
        if (!client->connected)
          break;

        timeoutTime = std::chrono::system_clock::now() +
                      client->server.m_parameters.clientTimeOut;
      }
    }

    if (client->socket->IsInvalid()) {
      break;
    }

    if (client->socket->IsReadyForRead(CLIENT_THROTTLE_TIME)) {
      size_t bytesReceived =
        client->socket->Receive(receiveBuffer.data(), RECEIVE_BUFFER_SIZE, 0);
      if (bytesReceived) {
        if (awaitPassword) {
          if (ProcessClientPassword(
                client->clientHandle, receiveBuffer.data(), bytesReceived)) {
            awaitPassword = false;
            // mutex prevent change event function on calling
            std::lock_guard<std::mutex> lock(m_notifierMutex);
            if (m_onConnect) {
              m_onConnect(*this, client->clientHandle);
            }
          }
          continue;
        }
        {
          // mutex prevent change event function on calling
          std::lock_guard<std::mutex> lock(m_notifierMutex);
          if (m_onReceiveData) {
            m_onReceiveData(*this, client->clientHandle, receiveBuffer.data(),
                            bytesReceived);
          }
        }
        timeoutTime = std::chrono::system_clock::now() +
                      client->server.m_parameters.clientTimeOut;
      }
      else {
        break;
      }
    }
  }
  {
    // mutex prevent change event function on calling
    std::lock_guard<std::mutex> lock(m_notifierMutex);
    if (m_onDisconnect) {
      m_onDisconnect(*this, client->clientHandle);
    }
  }
  std::lock_guard<std::mutex> lock(m_listenerMutex);
  client->socket = nullptr;
  client->server.m_clientsForDelete.push(client->clientHandle);
}

void Server::SendToClient(ClientHandle clientHandle, const std::string_view& msg)
{
  std::lock_guard<std::mutex> lock(m_listenerMutex);
  if (m_clients.find(clientHandle) == m_clients.end())
    return;
  m_clients[clientHandle]->forSend.Push(msg.data(), msg.length());
}

void Server::ShowPrompt(ClientHandle clientHandle)
{
  std::lock_guard<std::mutex> lock(m_listenerMutex);
  if (m_clients.find(clientHandle) == m_clients.end())
    return;
  m_clients[clientHandle]->forSend.Push(
    m_commandPrompt.c_str(), m_commandPrompt.length());
}

void Server::CloseClient(ClientHandle clientHandle)
{
  std::lock_guard<std::mutex> lock(m_listenerMutex);
  if (m_clients.find(clientHandle) == m_clients.end())
    return;
  m_clients[clientHandle]->connected = false;
}

ServerPtr Server::CreateServer(unsigned short int port,
                               unsigned short int maxConnection)
{
  ServerParameters parameters{port, maxConnection, std::chrono::seconds(600)};
  return std::make_shared<Server>(parameters);
}
} // namespace FtTCP
