#ifndef __CEXKIT_RINGBUFFER_HPP__
#define __CEXKIT_RINGBUFFER_HPP__

#include <atomic>
#include <new>
#include <thread>

namespace cexkit {

/**
 * * A lock-free SPSC RingBuffer.
 */
template <typename T>
class RingBuffer {
 private:
  T* data_;
  std::atomic<uint64_t> head_, tail_;
  uint64_t capacity_;

 public:
  explicit RingBuffer(uint64_t capacity)
      : capacity_(capacity), head_(0), tail_(0) {
    data_ = reinterpret_cast<T*>(::operator new(sizeof(T) * capacity_));
  }
  ~RingBuffer() { ::operator delete(data_); }

  template <typename... Args>
  void pushBlock(Args&&... args) {
    uint64_t tail = tail_.load(std::memory_order_acquire);
    uint64_t nextTail = (tail + 1) % capacity_;

    // Yield if buffer is full.
    while (nextTail == head_.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }

    new (&data_[tail]) T(std::forward<Args>(args)...);

    tail_.store(nextTail, std::memory_order_release);
  }

  template <typename... Args>
  bool pushNonBlock(Args&&... args) {
    uint64_t tail = tail_.load(std::memory_order_acquire);
    uint64_t nextTail = (tail + 1) % capacity_;

    if (nextTail == head_.load(std::memory_order_acquire)) return false;

    new (&data_[tail]) T(std::forward<Args>(args)...);

    tail_.store(nextTail, std::memory_order_release);
  }

  // * The caller needs to guarantee that the returned object would not
  // * be destructed (call pop()) during usage.
  void frontBlock(T& entry) {
    uint64_t head = head_.load(std::memory_order_acquire);

    // Yield if buffer is empty.
    while (head == tail_.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }

    entry = data_[head];
  }

  bool frontNonBlock(T& entry) {
    uint64_t head = head_.load(std::memory_order_acquire);

    if (head == tail_.load(std::memory_order_acquire)) {
      return false;
    }

    entry = data_[head];
    return true;
  }

  void popBlock(bool nonEmtpy = false) {
    uint64_t head = head_.load(std::memory_order_acquire);

    // * If the caller makes sure the buffer is not empty, set nonEmtpy to true
    // * can skip the empty-buffer-check.
    if (!nonEmtpy) {
      // Yield if buffer is empty.
      while (head == tail_.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
    }

    data_[head].~T();

    head_.fetch_add(1, std::memory_order_release);
  }

  bool popNonBlock(bool nonEmpty = false) {
    uint64_t head = head_.load(std::memory_order_acquire);

    if (!nonEmpty && head == tail_.load(std::memory_order_acquire)) {
      return false;
    }

    data_[head].~T();

    head_.fetch_add(1, std::memory_order_release);
  }

  bool empty() const {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
  }

  bool full() const {
    return (tail_.load(std::memory_order_acquire) + 1) % capacity_ ==
           head_.load(std::memory_order_acquire);
  }
};  // RingBuffer

}  // namespace cexkit

#endif