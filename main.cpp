#include <memory>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <list>
#include <atomic>
#include <unistd.h>
#include "ft-socket/ft_socket_address.hpp"
#include "ft-socket/ft_socket.hpp"
#include "ft-socket/ft_socket_server.hpp"
#include "ft-socket/ft_broadcast.hpp"
#include "telnet_callbacks.hpp"

using namespace FtTCP;

static char datatosend[] = "\n Ok!\n\0";

void TestClientSocket()
{
    AddressPtr address = Address::CreateClientAddress("127.0.0.1", 3051);
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

void StartTelnet()
{
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
}

void TestBroadcast()
{
#if 0
    const std::string packet = "Datagramm v.100.200.300\0";
    //FtTCP::AddressPtr addr = FtTCP::Address::CreateBroadcastAddress("192.168.100.242", 3055);
    FtTCP::AddressPtr addr = FtTCP::Address::CreateBroadcastAddress("127.0.0.1", 3055);
    FtTCP::Socket sock(addr);
    for(int i = 0; i < 20; i++)
    {
        std::cout << "sendto: " << addr->toString() << std::endl;
        if (sock.SendDatagram(static_cast<const void*>(packet.c_str()), packet.length()))
            std::cout << "UPD sended" << std::endl;
        else
        {
            std::cout << "Error on UPD send: " <<  sock.ErrorsToStr();
        }
        sleep(1);
    }
#endif
    FtTCP::BroadcastPtr br = FtTCP::Broadcast::CreateBroadcast(std::string("Application Version"));
    br->Start();
    int counter = 0;
    bool local_shutdown = false;
    while(!local_shutdown)
    {
        sleep(1);
        if (5 == counter)
        {
            br->Renew(true, std::string("127.0.0.1"));
            std::cout << "Broadcast: Reneved connected" << std::endl;
        }
        else if (70 == counter)
        {
            br->Renew(false);
            std::cout << "Broadcast: Reneved disconnected" << std::endl;
        }
        else if (counter > 80)
        {
            break;
        }
        counter++;
    }
    std::cout << "Broadcast: stopping" << std::endl;
    br->Stop();
    std::cout << "Broadcast: all done" << std::endl;
}

int main(int argc, char *argv[])
{
    std::list<AddressPtr> addresses{
        Address::CreateClientAddress("jerry-pi.local", 143),
        Address::CreateListenerAddress(3051, false),
        Address::CreateClientAddress("google.com", 5987),
        Address::CreateListenerAddress(0, false),
        Address::CreateClientAddress("bbbdsd.net", 33),
        Address::CreateClientAddress("jerry-ws.local", 0),
        Address::CreateBroadcastAddress("192.168.10.36", 3055),
        Address::CreateBroadcastAddress("127.0.0.1", 3055),        
        };
    for (auto a : addresses)
    {
        std::cout << "address == " << a->toString() << " valid=" << a->IsValid() << std::endl;
    }

    if (argc > 1)
    {
        std::string arg1 = argv[1];
        if (0 == arg1.compare("-telnet"))
        {
            StartTelnet();
            return 0;
        }
        else if (0 == arg1.compare("-broadcast"))
        {
            TestBroadcast();
            return 0;
        }
        else if (0 == arg1.compare("-client"))
        {
            TestClientSocket();
            return 0;
        }
    }
    TestBroadcast();
    return 0;
}