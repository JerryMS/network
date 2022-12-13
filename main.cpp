#include <memory>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <list>
#include <atomic>
#include "include/ft_socket_address.hpp"
#include "include/ft_socket.hpp"
#include "include/ft_socket_server.hpp"

using namespace FtTCP;

static char datatosend[] = "\n Ok!\n\0";
static char responce[] = "Ok\n";
static char prompt[] = "user@system:$ ";

static char bye[] = "shutting down\n";

static char close_cmd[] = "close";
static char close_msg[] = "Connection closed.\n";

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

static std::atomic_bool stopping{false};
static std::atomic_bool shutingdown{false};

static std::string reason_names[] = {
    "InitiallBindFail",
    "InitiallListenFail",
    "ConnectionAccepted",
    "ConnectionRejected",
    "ConnectionDeleted",
    "ServerStarted",
    "ServerStopSignal",
    "ServerStopped"};

void OnStartListening(Server &server)
{
    std::cout << "Listener started" << std::endl;
}

void OnClientConnect(Server &server, ClientHandle clientHandle)
{
    std::cout << "Client connected: " << clientHandle << std::endl;
    server.SendToClient(clientHandle, prompt, strlen(prompt));
}

void OnClientDisconnect(Server &server, ClientHandle clientHandle)
{
    std::cout << "Client diconnected: " << clientHandle << std::endl;
}

void OnClientReceiveData(Server &server, ClientHandle clientHandle, const void *data, size_t size)
{
    char *strdata = (char *)data;
    strdata[size] = 0;
    std::cout << "Client: " << clientHandle << " received: " << strdata << std::endl;
    server.SendToClient(clientHandle, responce, strlen(responce));
    if (0 == memcmp(data, close_cmd, std::min(size, strlen(close_cmd))))
    {
        server.SendToClient(clientHandle, close_msg, strlen(close_msg));
        server.CloseClient(clientHandle);
    }
    else if (0 == memcmp(data, stop_cmd, std::min(size, strlen(stop_cmd))))
    {
        server.SendToClient(clientHandle, stop_msg, strlen(stop_msg));
        stopping = true;
    }
    else if (0 == memcmp(data, shutdown_cmd, std::min(size, strlen(shutdown_cmd))))
    {   
        server.SendToClient(clientHandle, shutdown_msg, strlen(shutdown_msg));
        stopping = true;
        shutingdown = true;
    }
    else if (0 == memcmp(data, testout_cmd, std::min(size, strlen(shutdown_cmd))))
    {
        server.SendToClient(clientHandle, testout, strlen(testout));
    }
    else
    {
        server.SendToClient(clientHandle, prompt, strlen(prompt));
    }
}

static std::string m_passwd = "123";

bool PasswordEntered(Server &server, ClientHandle clientHandle, const void *data, size_t size)
{
    auto mm = std::minmax(m_passwd.length(), size);
    return (0 == std::memcmp(data, m_passwd.c_str(), mm.first));
}

void OnUpdate(Server &server, ServerReason reason, PlatformError err)
{
    std::cout << "Server updated: " << reason_names[reason] << " error: " << strerror(err) << std::endl;
    if (98 == err) // connection already used
        stopping = true;
}

void TestClientSocket()
{
    AddressPtr address = Address::CreateClientAddress("jerry-linux.local", 3051);
    SocketPtr m_socket = Socket::CreateSocket(address);
    std::cout << "Connecting to " << address->toString() << std::endl;
    if (!m_socket->Connect())
    {
        std::cout << "Error connection:" << std::endl
                  << m_socket->ErrorsToStr() << std::endl;
    }
    size_t sended = 0;
    std::cout << "Send data" << std::endl;
    if (!m_socket->Send(datatosend, strlen(datatosend), 0, &sended))
    {
        std::cout << "Error send" << std::endl
                  << m_socket->ErrorsToStr() << std::endl;
    }
    std::cout << "Sent " << sended << " bytes" << std::endl;
}

int main(int argc, char *argv[])
{
    std::list<AddressPtr> addresses{
        Address::CreateClientAddress("jerry-pi.local", 143),
        Address::CreateListenerAddress(3051, false),
        Address::CreateClientAddress("google.com", 5987),
        Address::CreateListenerAddress(0, false),
        Address::CreateClientAddress("bbbdsd.net", 33),
        Address::CreateClientAddress("jerry-ws.local", 0)};
    for (auto a : addresses)
    {
        std::cout << "address == " << a->toString() << " valid=" << a->IsValid() << std::endl;
    }

    ServerParameters params{10303, 2, std::chrono::seconds(60)};
    Server server(params);
    server.SetOnStartListeningCallback(&OnStartListening);
    server.SetOnClientConnectCallback(&OnClientConnect);
    server.SetOnClientDisconnectCallback(&OnClientDisconnect);
    server.SetOnReceiveDataCallback(&OnClientReceiveData);
    server.SetOnServerUpdate(&OnUpdate);
    server.SetOnPasswordEntered(&PasswordEntered);

    while (!shutingdown)
    {
        server.Start();
        while (!stopping)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        server.Stop();
        stopping = false;
    }
    return 0;
}