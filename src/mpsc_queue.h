//
// Created by Jiamin Huang on 5/30/16.
//

#ifndef GLAKV_MPSC_QUEUE_H
#define GLAKV_MPSC_QUEUE_H

#include <atomic>
#include <assert.h>

template<typename T>
class mpsc_queue
{
public:
    mpsc_queue() :
            _head(reinterpret_cast<buffer_node_t*>(new buffer_node_aligned_t)),
            _tail(_head.load(std::memory_order_relaxed))
    {
        buffer_node_t* front = _head.load(std::memory_order_relaxed);
        front->next.store(NULL, std::memory_order_relaxed);
    }

    mpsc_queue(mpsc_queue &&rhs) :
            _head(rhs._head.load()),
            _tail(rhs._tail.load()) {
        rhs._head = nullptr;
        rhs._tail = nullptr;
    }

    ~mpsc_queue()
    {
        T output;
        while (this->try_dequeue(output)) {}
        buffer_node_t* front = _head.load(std::memory_order_relaxed);
        delete front;
    }

    void enqueue(const T& input)
    {
        buffer_node_t* node = reinterpret_cast<buffer_node_t*>(new buffer_node_aligned_t);
        node->data = input;
        node->next.store(NULL, std::memory_order_relaxed);

        buffer_node_t* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }

    bool try_dequeue(T& output)
    {
        buffer_node_t* tail = _tail.load(std::memory_order_relaxed);
        buffer_node_t* next = tail->next.load(std::memory_order_acquire);

        if (next == NULL) {
            return false;
        }

        output = next->data;
        _tail.store(next, std::memory_order_release);
        delete tail;
        return true;
    }


private:
    struct buffer_node_t
    {
        T                           data;
        std::atomic<buffer_node_t*> next;
    };

    typedef typename std::aligned_storage<sizeof(buffer_node_t), std::alignment_of<buffer_node_t>::value>::type buffer_node_aligned_t;

    std::atomic<buffer_node_t*> _head;
    std::atomic<buffer_node_t*> _tail;

    mpsc_queue(const mpsc_queue&) {}
    void operator=(const mpsc_queue&) {}
};

#endif //GLAKV_MPSC_QUEUE_H
