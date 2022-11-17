#pragma once

#include <string>
#include <memory>
#include <netdb.h>
#include <arpa/inet.h>

namespace FtTCP
{
    class Address;

    using AddressPtr = std::shared_ptr<Address>;

    class Address
    {
    protected:
        static constexpr unsigned int MAX_BUFFER_LENGTH = 256;

        std::string m_host;
        int m_port;
        std::string m_presentation;
        sockaddr_in m_address;

        bool m_isValid;
        bool m_isListener;
        bool m_isAsync;

        void FillPresentation();

    public:
        Address(const char *host, const int port);
        Address(const int port, const bool async);
        ~Address();

        bool IsEmpty();
        bool IsValid();
        bool IsListener();
        std::string_view toString();
        const sockaddr_in* GetAddress(); 

        static AddressPtr CreateClientAddress(const char *host, const int port);
        static AddressPtr CreateListenerAddress(const int port, const bool isAsync);
        
    };
}
