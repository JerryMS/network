#include "include/ft_socket_server.hpp"
#include "include/ft_buffer.hpp"

namespace FtTCP
{
    Server::Server(const ServerParameters &params)
    {
        m_parameters = params;
        m_stage = Stage::Initializing;
    }

    Server::~Server()
    {
        m_stage.store(Stage::Shutingdown);

        if (m_listenerThread.joinable())
            m_listenerThread.join();
    }

    void Server::Start()
    {
        m_listenerThread = std::thread([this]()
                                       { this->Run(); });

        std::unique_lock<std::mutex> lock(m_listenerMutex);
        m_listenerStageCond.wait_for(lock, std::chrono::milliseconds(1), [this]()
                                     { return m_stage != Stage::Listening; });
    }

    bool Server::DoInitializing()
    {
        AddressPtr address = Address::CreateListenerAddress(m_parameters.port, true);
        m_listenerSocket = Socket::CreateSocket(address);
        m_listenerSocket->SetNonBlocking(true);
        if (m_listenerSocket->Bind() == false)
        {
            m_stage = Stage::Shutingdown;
            if (m_onUpdate)
            {
                std::lock_guard<std::mutex> lock(m_notifierMutex);
                m_onUpdate(*this);
            }
            return false;
        }
        if (m_listenerSocket->Listen() == false)
        {
            m_stage = Stage::Shutingdown;
            if (m_onUpdate)
            {
                std::lock_guard<std::mutex> lock(m_notifierMutex);
                m_onUpdate(*this);
            }
            return false;
        }

        m_stage = Stage::Listening;
        return true;
    }

    bool Server::DoListening()
    {
        if (m_listenerSocket->IsReadyForRead())
        {
            if (m_clients.size() >= m_parameters.maxConnections)
            {
                return false;
            }

            SocketPtr connectionSocket = m_listenerSocket->Accept();
            if (connectionSocket && connectionSocket->IsInvalid() == false)
            {
                auto client = std::allocate_shared<Client>(std::allocator<Client>(), *this);
                client->clientHandle = ++m_clientHandlesCounter;
                client->connected = true;
                client->socket = connectionSocket;

                client->thread = std::thread([this, client]()
                                             { this->RunClient(client); });
                {
                    std::lock_guard<std::mutex> lock(m_listenerMutex);
                    m_clients[client->clientHandle] = client;
                }

                if (m_onConnect)
                {
                    std::lock_guard<std::mutex> lock(m_notifierMutex);
                    m_onConnect(*this, client->clientHandle);
                }

                if (m_onUpdate)
                {
                    std::lock_guard<std::mutex> lock(m_notifierMutex);
                    m_onUpdate(*this);
                }

                return true;
            }
        }
        return false;
    }

    void Server::Run()
    {
        m_listenerStageCond.notify_one();

        if (m_onUpdate)
        {
            std::lock_guard<std::mutex> lock(m_notifierMutex);
            m_onUpdate(*this);
        }

        Stage stage;
        while (Stage::Shutingdown != (stage = m_stage.load()))
        {
            switch (stage)
            {
            case Stage::Initializing:
                if (!DoInitializing())
                {
                    continue;
                }
                break;
            case Stage::Listening:
                if (!DoListening())
                {
                    std::this_thread::sleep_for(LISTENER_THROTTLE_TIME);
                    continue;
                }
                break;
            default:
                std::this_thread::sleep_for(LISTENER_THROTTLE_TIME);
                break;
            }
        }

        std::lock_guard<std::mutex> lock(m_listenerMutex);
        for (auto &client : m_clients)
            client.second->thread.join();

        m_clients.clear();
    }

    void Server::RunClient(ClientPtr client)
    {
        std::chrono::time_point timeoutTime = std::chrono::system_clock::now() + client->server.m_parameters.clientTimeOut;

        BufferPtr receiveBuffer = Buffer::CreateBuffer(RECEIVE_BUFFER_SIZE);

        while (client->connected && Stage::Shutingdown != m_stage.load())
        {
            std::this_thread::sleep_for(CLIENT_THROTTLE_TIME);

            if (std::chrono::system_clock::now() >= timeoutTime)
            {
                client->connected = false;
                break;
            }

            if (client->socket->IsInvalid())
            {
                break;
            }

            if (client->socket->IsReadyForWrite())
            {
                if (!client->forSend.IsEmpty())
                {
                    if (!client->forSend.Send(client->socket))
                    {
                        client->connected = false;
                        break;
                    }

                    std::this_thread::sleep_for(CLIENT_THROTTLE_TIME);

                    timeoutTime = std::chrono::system_clock::now() + client->server.m_parameters.clientTimeOut;
                }
            }

            if (client->socket->IsReadyForRead())
            {

                size_t bytesReceived = client->socket->Receive(receiveBuffer->Get(), RECEIVE_BUFFER_SIZE, 0);
                if (bytesReceived)
                {
                    if (m_onReceiveData)
                    {
                        std::lock_guard<std::mutex> lock(m_notifierMutex);
                        m_onReceiveData(*this, client->clientHandle, receiveBuffer->Get(), bytesReceived);
                    }

                    std::this_thread::sleep_for(CLIENT_THROTTLE_TIME);

                    timeoutTime = std::chrono::system_clock::now() + client->server.m_parameters.clientTimeOut;
                }
            }
        }

        if (m_onDisconnect)
        {
            std::lock_guard<std::mutex> lock(m_notifierMutex);
            m_onDisconnect(*this, client->clientHandle);
        }
        client->socket = nullptr;
    }

    void Server::SendToClient(ClientHandle clientHandle, const void *data, size_t size)
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        if (m_clients.find(clientHandle) == m_clients.end())
            return;
        m_clients[clientHandle]->forSend.Push(data, size);
    }

    bool Server::SetOnStartListeningCallback(OnStartListeningFnType onStartListening)
    {
        if (Stage::Initializing != m_stage)
            return false;
        m_onStartListening = onStartListening;
        return true;
    }

    bool Server::SetOnClientConnectCallback(OnClientConnectFnType onConnect)
    {
        if (Stage::Initializing != m_stage)
            return false;
        m_onConnect = onConnect;
        return true;
    }

    bool Server::SetOnClientDisconnectCallback(OnClientDisconnectFnType onDisconnect)
    {
        if (Stage::Initializing != m_stage)
            return false;
        m_onDisconnect = onDisconnect;
        return true;
    }

    bool Server::SetOnReceiveDataCallback(OnClientReceiveDataFnType onReceiveData)
    {
        if (Stage::Initializing != m_stage)
            return false;
        m_onReceiveData = onReceiveData;
        return true;
    }

    bool Server::SetOnServerUpdate(OnUpdateFnType onUpdate)
    {
        if (Stage::Initializing != m_stage)
            return false;
        m_onUpdate = onUpdate;
        return true;
    }
}
