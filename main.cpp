#include <memory>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <list>
#include <atomic>
#include "ft-socket/ft_socket_address.hpp"
#include "ft-socket/ft_socket.hpp"
#include "ft-socket/ft_socket_server.hpp"
#include "telnet_callbacks.hpp"

using namespace FtTCP;

static char datatosend[] = "\n Ok!\n\0";

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

    TelnetCallbacks callbacks;
    ServerParameters params{10303, 2, std::chrono::seconds(60)};
    Server server(params);
    server.SetOnStartListeningCallback(&callbacks, &TelnetCallbacks::OnStartListening);
    server.SetOnClientConnectCallback(&callbacks, &TelnetCallbacks::OnClientConnect);
    server.SetOnClientDisconnectCallback(&callbacks, &TelnetCallbacks::OnClientDisconnect);
    server.SetOnReceiveDataCallback(&callbacks, &TelnetCallbacks::OnClientReceiveData);
    server.SetOnServerUpdate(&callbacks, &TelnetCallbacks::OnUpdate);
    server.SetOnPasswordEntered(&callbacks, &TelnetCallbacks::OnClientPasswordEntered);

    while (!callbacks.m_shutingdown)
    {
        server.Start();
        while (!callbacks.m_stopping)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        server.Stop();
        callbacks.m_stopping = false;
    }
    return 0;
}