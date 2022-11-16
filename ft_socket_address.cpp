#include "include/ft_socket_address.hpp"
#include <cstring>

namespace FtTCP
{

Address::Address(const char* host, const int port):
    m_host{host},
    m_port{port},
    m_isListener{false},
    m_isAsync{false}
{
    FillPresentation();
}

Address::Address(const int port, const bool async):
    m_host{""},
    m_port{port},
    m_isListener{true},
    m_isAsync(async)
{
    FillPresentation();
}

Address::~Address()
{

}

void Address::FillPresentation()
{
    m_isValid = false;
    if (IsEmpty())
    {
        m_presentation = "[empty]";
    }
    else
    {   
        addrinfo hints;
	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM;
	    hints.ai_protocol = IPPROTO_TCP;
	    // Resolve the address and port to be used by the socket
	    const char * addressStr = m_host.size() > 0 ? m_host.c_str() : nullptr;
        {
            addrinfo* address = nullptr;
            int res = getaddrinfo(addressStr, std::to_string(m_port).c_str(), &hints, &address); 
	        if (0 == res)
	        {
                m_address = *(reinterpret_cast<sockaddr_in*>(address->ai_addr));
                m_isValid = true;
            }
            freeaddrinfo(address);
        }
        char ipstr[INET6_ADDRSTRLEN];
        if (m_isValid)
        {
            inet_ntop(m_address.sin_family, &m_address.sin_addr, ipstr, INET_ADDRSTRLEN);
        }
        else
        {
            strncpy(ipstr, "[not resolved]", INET_ADDRSTRLEN);
        }

        char buf[MAX_BUFFER_LENGTH] = {0};
        if (m_isValid)
        {
            std::snprintf(buf, MAX_BUFFER_LENGTH - 1, "%s:%d ip=%s", m_host.c_str(), m_port, ipstr);            
        }
        else
        {
            std::snprintf(buf, MAX_BUFFER_LENGTH - 1, "%s:%d ip=not resolved", m_host.c_str(), m_port);
        }
        m_presentation = buf;
    }
}

bool Address::IsEmpty()
{
    if (m_isListener)
    {
        return (m_port < 1);    
    }
    else
    {
        return (m_host.empty()) || (m_port < 1);
    }
}

bool Address::IsValid()
{
    return (m_isValid);
}

std::string_view Address::toString()
{
    return std::string_view(m_presentation); 
}

const sockaddr_in* Address::GetAddress()
{
    return &m_address;
}

AddressPtr Address::CreateClientAddress(const char* host, const int port)
{
    return std::allocate_shared<Address>(std::allocator<Address>(), host, port);
}

AddressPtr Address::CreateListenerAddress(const int port, const bool isAsync)
{
    return std::allocate_shared<Address>(std::allocator<Address>(), port, isAsync);
}

} // namespace
