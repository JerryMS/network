#pragma once

#include <mutex>
#include <queue>
#include "ft_socket.hpp"

namespace FtTCP
{
    using Buffer = std::vector<std::byte>;
    class SocketSendQueue
    {
    private:
        static constexpr size_t MAX_SEND_BUFFER{256};
        std::deque<Buffer> m_queue;
        mutable std::mutex m_mutex;
        std::size_t m_sent = 0;
    public:
        bool Send(SocketPtr socket);
        void Push(const void *source, size_t size);
        bool IsEmpty();
    };
}
