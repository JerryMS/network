#include <memory>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <list>
#include "include/ft_socket_address.hpp"
#include "include/ft_socket.hpp"
#include "include/ft_socket_server.hpp"

using namespace FtTCP;

static char datatosend[] = "\n Ok!\n\0";
static char responce[] = "Ok\n";
static char prompt[] = "user@system:$ ";

void OnStartListening(Server & server)
{
    std::cout << "Listener started" << std::endl;
}

void OnClientConnect(Server & server, ClientHandle clientHandle)
{
    std::cout << "Client connected: " << clientHandle << std::endl;
    server.SendToClient(clientHandle, prompt, strlen(prompt));
}
    
void OnClientDisconnect(Server & server, ClientHandle clientHandle)
{
    std::cout << "Client diconnected: " << clientHandle << std::endl;    
}
    
void OnClientReceiveData(Server & server, ClientHandle clientHandle, const void * data, size_t size)
{
    char* strdata = (char*) data;
    strdata[size] = 0;
    std::cout << "Client: " << clientHandle << " received: " << strdata << std::endl;    
    server.SendToClient(clientHandle, responce, strlen(responce));
    if ()
    server.SendToClient(clientHandle, prompt, strlen(prompt));
}

void OnUpdate(Server & server)
{
    std::cout << "Stage updated!" << std::endl;
}


void TestClientSocket()
{
    AddressPtr address = Address::CreateClientAddress("jerry-linux.local", 3051);
	SocketPtr m_socket = Socket::CreateSocket(address);
	std::cout << "Connecting to " << address->toString() << std::endl;
	if (!m_socket->Connect())
	{
        std::cout << "Error connection:" << std::endl << m_socket->ErrorsToStr() << std::endl;
    }
    size_t sended = 0;
    std::cout << "Send data" << std::endl;
    if (!m_socket->Send(datatosend, strlen(datatosend), 0, &sended))
    {
        std::cout << "Error send" << std::endl  << m_socket->ErrorsToStr() << std::endl;
    }
    std::cout << "Sent " << sended << " bytes" << std::endl;
}

int main (int argc, char *argv[]) 
{
    std::list<AddressPtr> addresses{ 
        Address::CreateClientAddress("jerry-pi.local", 143), 
        Address::CreateListenerAddress(3051, false), 
        Address::CreateClientAddress("google.com", 5987),
        Address::CreateListenerAddress(0, false), 
        Address::CreateClientAddress("bbbdsd.net", 33),
        Address::CreateClientAddress("jerry-ws.local", 0)
    };
    for (auto a: addresses)
    {
        std::cout << "address == " << a->toString() << " valid=" << a->IsValid() << std::endl;
    }

    ServerParameters params {10303, 2, std::chrono::seconds(60)};
    Server server(params);
    server.SetOnStartListeningCallback(&OnStartListening);
    server.SetOnClientConnectCallback(&OnClientConnect);
    server.SetOnClientDisconnectCallback(&OnClientDisconnect);
    server.SetOnReceiveDataCallback(&OnClientReceiveData);
    server.SetOnServerUpdate(&OnUpdate);
    server.Start();

    std::this_thread::sleep_for(std::chrono::seconds(60*10));

    return 0;
}