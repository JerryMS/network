#include "include/ft_socket_queues.hpp"

namespace FtTCP
{

    bool SocketSendQueue::Send(SocketPtr socket)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return true;

        Buffer& buffer = m_queue.front();
        size_t bytesSent = 0;
        if (!socket->Send(&buffer[m_sent], buffer.size() - m_sent, 0, &bytesSent))
            return false;
        m_sent += bytesSent;
        if (m_sent == buffer.size())
        {
            m_queue.pop_front();
            m_sent = 0;
        }
        return true;
    }

    void SocketSendQueue::Push(const void *source, size_t size)
    {
        if (0 == size)
            return;

        std::lock_guard<std::mutex> lock(m_mutex);
        std::byte *pointer = (std::byte *)source;
        size_t remainder = size;
        while (remainder)
        {
            size_t bytesToWrite = std::min(remainder, MAX_SEND_BUFFER);
            Buffer buff(pointer, pointer + bytesToWrite);
            m_queue.push_back(std::move(buff));
            pointer += bytesToWrite;
            remainder -= bytesToWrite;
        }
    }

    bool SocketSendQueue::IsEmpty()
    {
	    std::lock_guard<std::mutex> lock(m_mutex);
	    return m_queue.empty();
    }
}
