#pragma once

#include <array>
#include <optional>

/**
 * @brief basic ring buffer implementation
 * 
 * This is a fixed-size FIFO container
 * 
 * The memory consumption is sizeof(T) * (Capacity + 1) + 2 * sizeof(void*) + alignment
 */
template <typename T, size_t Capacity>
class RingBuffer
{
public:
    bool empty() const
    {
        return read == write;
    }

    bool full() const
    {
        return size() == capacity();
    }

    constexpr size_t capacity() const
    {
        return storage.size() - 1;
    }

    size_t size() const
    {
        if (write < read)
        {
            return storage.size() - (read - write);
        }
        return write - read;
    }

    /**
     * Push an element into the ring buffer.
     * 
     * @return true if the push has succeeded
     */
    bool push(T t)
    {
        const bool result = !full();
        if (result)
        {
            *write = std::move(t);
            incr(write);
        }
        return result;
    }

    /**
     * Fetch an element from the ring buffer
     * 
     * @return a valid optional in case of success
     */
    std::optional<T> pop()
    {
        std::optional<T> result;
        if (empty() == false)
        {
            result = std::move(*read);
            incr(read);
        }
        return result;
    }

private:
    using Storage = std::array<T, Capacity + 1>;

    void incr(typename Storage::iterator &it)
    {
        ++it;
        if (it == storage.end())
        {
            it = storage.begin();
        }
    }

    Storage storage;
    typename Storage::iterator write = storage.begin();
    typename Storage::iterator read = storage.begin();
};
