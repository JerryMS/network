#pragma once

#include <mutex>
#include <queue>
#include "ft_socket.hpp"

namespace FtTCP
{
    class SocketSendQueue
    {
    private:
        static constexpr size_t MAX_SEND_BUFFER{256};
        std::deque<BufferPtr> m_queue;
        mutable std::mutex m_mutex;
        std::size_t m_sent;
    public:
        bool Send(SocketPtr socket);
        void Push(const void *source, size_t size);
        bool IsEmpty();
    };
}
