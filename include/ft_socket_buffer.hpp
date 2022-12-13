#pragma once

#include <memory>
#include <cstring>

#if 0
namespace FtTCP
{
    class Buffer;

    using BufferPtr = std::shared_ptr<Buffer>;

    using BufferElement = std::byte;

    class Buffer
    {
    private:
        BufferElement *m_buffer;
        size_t m_maxSize;
        size_t m_count;
    public: 
        Buffer(std::size_t size)
        {
            m_maxSize = size;
            m_count = 0;
            m_buffer = static_cast<BufferElement*>(malloc(size));
        }
        ~Buffer()
        {
            free(m_buffer);
        }
        bool Push(BufferElement* source, size_t size)
        {
            if (size > m_maxSize)
                return false;

            memcpy(m_buffer, source, size);
            m_count = size;
            return true;
        }

        BufferElement* Get()
        {
            return m_buffer;
        }

        std::size_t Count()
        {
            return m_count;
        }
        
        static BufferPtr CreateBuffer(size_t size)
        {
            return std::allocate_shared<Buffer>(std::allocator<Buffer>(), size);
        }
    };
}

#endif