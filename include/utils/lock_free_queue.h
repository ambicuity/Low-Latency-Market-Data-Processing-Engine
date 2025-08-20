#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <array>

namespace market_data {

// Lock-free SPSC (Single Producer Single Consumer) queue for high performance
template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::atomic<T*> data{nullptr};
        std::atomic<Node*> next{nullptr};
    };
    
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    
public:
    LockFreeQueue() {
        Node* dummy = new Node;
        head_.store(dummy);
        tail_.store(dummy);
    }
    
    ~LockFreeQueue() {
        while (Node* const old_head = head_.load()) {
            head_.store(old_head->next);
            delete old_head;
        }
    }
    
    // Non-copyable
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    
    void push(T item) {
        T* data = new T(std::move(item));
        Node* new_node = new Node;
        Node* prev_tail = tail_.exchange(new_node);
        prev_tail->data.store(data);
        prev_tail->next.store(new_node);
    }
    
    bool try_pop(T& result) {
        Node* head = head_.load();
        Node* next = head->next.load();
        
        if (next == nullptr) {
            return false;
        }
        
        T* data = next->data.load();
        if (data == nullptr) {
            return false;
        }
        
        result = *data;
        delete data;
        head_.store(next);
        delete head;
        
        return true;
    }
    
    bool empty() const {
        Node* head = head_.load();
        Node* next = head->next.load();
        return next == nullptr;
    }
};

// Ring buffer for fixed-size high-performance queue
template<typename T, size_t Size>
class RingBuffer {
private:
    static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");
    
    alignas(64) std::array<T, Size> buffer_;
    alignas(64) std::atomic<size_t> write_pos_{0};
    alignas(64) std::atomic<size_t> read_pos_{0};
    
    static constexpr size_t MASK = Size - 1;
    
public:
    bool try_push(const T& item) {
        const size_t current_write = write_pos_.load(std::memory_order_relaxed);
        const size_t next_write = (current_write + 1) & MASK;
        
        if (next_write == read_pos_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }
        
        buffer_[current_write] = item;
        write_pos_.store(next_write, std::memory_order_release);
        return true;
    }
    
    bool try_pop(T& item) {
        const size_t current_read = read_pos_.load(std::memory_order_relaxed);
        
        if (current_read == write_pos_.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }
        
        item = buffer_[current_read];
        read_pos_.store((current_read + 1) & MASK, std::memory_order_release);
        return true;
    }
    
    bool empty() const {
        return read_pos_.load(std::memory_order_acquire) == 
               write_pos_.load(std::memory_order_acquire);
    }
    
    bool full() const {
        const size_t next_write = (write_pos_.load(std::memory_order_acquire) + 1) & MASK;
        return next_write == read_pos_.load(std::memory_order_acquire);
    }
    
    size_t size() const {
        const size_t write = write_pos_.load(std::memory_order_acquire);
        const size_t read = read_pos_.load(std::memory_order_acquire);
        return (write - read) & MASK;
    }
};

} // namespace market_data