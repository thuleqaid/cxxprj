#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__
#include <vector>
#include <optional>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>

enum class RingBufferFullStrategy {
    Reject,             // 满时拒绝新元素
    OverwriteOldest,    // 覆盖最旧元素
    OverwriteNewest     // 覆盖最新元素
};

template<typename T>
class RingBuffer {
public:
    // 构造函数：指定容量和溢出策略
    explicit RingBuffer(size_t capacity = 1024, RingBufferFullStrategy policy = RingBufferFullStrategy::Reject)
        : m_capacity(capacity)
        , m_buffer(capacity)
        , m_head(0)
        , m_tail(0)
        , m_size(0)
        , m_policy(policy) {
        if (capacity == 0) {
            throw std::invalid_argument("RingBuffer capacity must be positive");
        }
    }
    
    // 禁用拷贝构造和拷贝赋值（可选，根据需求可以启用）
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    
    // 移动构造
    RingBuffer(RingBuffer&& other) noexcept
        : m_capacity(other.m_capacity)
        , m_buffer(std::move(other.m_buffer))
        , m_head(other.m_head)
        , m_tail(other.m_tail)
        , m_size(other.m_size)
        , m_policy(other.m_policy) {
        other.m_head = other.m_tail = other.m_size = 0;
    }
    
    // 添加元素
    bool push(const T& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        if (m_size < m_capacity) {
            // 未满，正常添加
            m_buffer[m_tail] = value;
            m_tail = (m_tail + 1) % m_capacity;
            ++m_size;
            return true;
        }
        
        // 已满，根据策略处理
        switch (m_policy) {
            case RingBufferFullStrategy::Reject:
                return false;
                
            case RingBufferFullStrategy::OverwriteOldest:
                // 覆盖最旧元素（head位置）
                m_buffer[m_head] = value;
                m_head = (m_head + 1) % m_capacity;
                m_tail = (m_tail + 1) % m_capacity;
                return true;
                
            case RingBufferFullStrategy::OverwriteNewest:
                // 覆盖最新元素（tail-1位置）
                if (m_size > 0) {
                    size_t newest_pos = (m_tail - 1 + m_capacity) % m_capacity;
                    m_buffer[newest_pos] = value;
                }
                return true;
                
            default:
                return false;
        }
    }
    
    // 移动版本
    bool push(T&& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        if (m_size < m_capacity) {
            m_buffer[m_tail] = std::move(value);
            m_tail = (m_tail + 1) % m_capacity;
            ++m_size;
            return true;
        }
        
        switch (m_policy) {
            case RingBufferFullStrategy::Reject:
                return false;
                
            case RingBufferFullStrategy::OverwriteOldest:
                m_buffer[m_head] = std::move(value);
                m_head = (m_head + 1) % m_capacity;
                m_tail = (m_tail + 1) % m_capacity;
                return true;
                
            case RingBufferFullStrategy::OverwriteNewest:
                if (m_size > 0) {
                    size_t newest_pos = (m_tail - 1 + m_capacity) % m_capacity;
                    m_buffer[newest_pos] = std::move(value);
                }
                return true;
                
            default:
                return false;
        }
    }
    
    // 弹出最旧元素
    std::optional<T> pop() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        if (m_size == 0) {
            return std::nullopt;
        }
        
        T value = std::move(m_buffer[m_head]);
        m_head = (m_head + 1) % m_capacity;
        --m_size;
        return value;
    }
    
    // 查看最旧元素（不弹出）
    std::optional<T> front() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        
        if (m_size == 0) {
            return std::nullopt;
        }
        return m_buffer[m_head];
    }
    
    // 查看最新元素
    std::optional<T> back() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        
        if (m_size == 0) {
            return std::nullopt;
        }
        size_t newest_pos = (m_tail - 1 + m_capacity) % m_capacity;
        return m_buffer[newest_pos];
    }
    
    // 检查是否为空
    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_size == 0;
    }
    
    // 检查是否已满
    bool full() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_size == m_capacity;
    }
    
    // 获取当前大小
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_size;
    }
    
    // 获取容量
    size_t capacity() const {
        return m_capacity;
    }
    
    // 清空缓冲区
    void clear() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_head = m_tail = m_size = 0;
        // 可选：重置buffer中的对象
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_t i = 0; i < m_capacity; ++i) {
                m_buffer[i].~T();
            }
            // 重新构造（使用placement new）
            for (size_t i = 0; i < m_capacity; ++i) {
                new (&m_buffer[i]) T();
            }
        }
    }
    
    // 修改溢出策略
    void set_policy(RingBufferFullStrategy policy) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_policy = policy;
    }
    
    // 获取当前策略
    RingBufferFullStrategy policy() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_policy;
    }
    
    // 迭代器支持（可选）
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        
        Iterator(RingBuffer* buffer, size_t pos, size_t remaining)
            : m_buffer(buffer), m_pos(pos), m_remaining(remaining) {}
        
        reference operator*() { return m_buffer->m_buffer[m_pos]; }
        pointer operator->() { return &m_buffer->m_buffer[m_pos]; }
        
        Iterator& operator++() {
            if (m_remaining > 0) {
                m_pos = (m_pos + 1) % m_buffer->m_capacity;
                --m_remaining;
            }
            return *this;
        }
        
        bool operator!=(const Iterator& other) const {
            return m_remaining != other.m_remaining || m_pos != other.m_pos;
        }
        
    private:
        RingBuffer* m_buffer;
        size_t m_pos;
        size_t m_remaining;
    };
    
    Iterator begin() {
        return Iterator(this, m_head, m_size);
    }
    
    Iterator end() {
        return Iterator(this, m_tail, 0);
    }
    
private:
    size_t m_capacity;
    std::vector<T> m_buffer;
    size_t m_head;      // 指向最旧元素
    size_t m_tail;      // 指向下一个写入位置
    size_t m_size;      // 当前元素数量
    RingBufferFullStrategy m_policy;
    mutable std::shared_mutex m_mutex;  // 读写锁，支持多读单写
};

#endif /* __RING_BUFFER_H__ */
