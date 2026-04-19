#pragma once

#include <array>
#include <atomic>
#include <optional>
#include <type_traits>

template <typename T, size_t MAX_SIZE>
class LockFreeArray
{
  private:
    struct alignas(std::max(alignof(T), alignof(std::atomic<bool>))) Node
    {
        std::atomic<bool> occupied;
        alignas(T) unsigned char storage[sizeof(T)];

        Node() : occupied(false) {}

        ~Node()
        {
            if (occupied.load(std::memory_order_acquire))
            {
                get_ptr()->~T();
            }
        }

        T*       get_ptr() { return reinterpret_cast<T*>(storage); }
        const T* get_ptr() const { return reinterpret_cast<const T*>(storage); }
    };

    std::array<Node, MAX_SIZE> slots;
    std::atomic<size_t>        count;

  public:
    LockFreeArray() : count(0) {}

    ~LockFreeArray()
    {
        // Nodes handle their own destruction
    }

    bool push(const T& value)
    {
        if (count.load(std::memory_order_acquire) >= MAX_SIZE)
        {
            return false;
        }

        for (size_t i = 0; i < MAX_SIZE; ++i)
        {
            bool expected = false;

            if (slots[i].occupied.compare_exchange_strong(
                    expected, true, std::memory_order_acquire, std::memory_order_acquire))
            {
                // We've claimed this slot, now construct the object in place
                new (slots[i].get_ptr()) T(value);

                // Ensure construction is visible before marking as truly occupied
                std::atomic_thread_fence(std::memory_order_release);

                count.fetch_add(1, std::memory_order_release);
                return true;
            }
        }

        return false;
    }

    std::optional<T> get(size_t index) const
    {
        if (index >= MAX_SIZE)
        {
            return std::nullopt;
        }

        if (!slots[index].occupied.load(std::memory_order_acquire))
        {
            return std::nullopt;
        }

        return *slots[index].get_ptr();
    }

    bool remove(size_t index)
    {
        if (index >= MAX_SIZE)
        {
            return false;
        }

        bool expected = true;

        if (slots[index].occupied.compare_exchange_strong(
                expected, false, std::memory_order_acquire, std::memory_order_acquire))
        {
            // We've claimed the slot for removal
            slots[index].get_ptr()->~T();

            // Ensure destruction is complete before updating count
            std::atomic_thread_fence(std::memory_order_release);

            count.fetch_sub(1, std::memory_order_release);
            return true;
        }

        return false;
    }

    size_t size() const { return count.load(std::memory_order_acquire); }

    bool is_full() const { return count.load(std::memory_order_acquire) >= MAX_SIZE; }

    bool is_empty() const { return count.load(std::memory_order_acquire) == 0; }
};