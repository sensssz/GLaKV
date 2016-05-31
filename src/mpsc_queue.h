//
// Created by Jiamin Huang on 5/30/16.
//

#ifndef GLAKV_MPSC_QUEUE_H
#define GLAKV_MPSC_QUEUE_H

template <typename T>
class mpsc_queue{
private:
    struct mpsc_node_t
    {
        T element;
        mpsc_node_t volatile *next;
        mpsc_node_t() : next(nullptr) {}
        mpsc_node_t(T element_in, mpsc_node_t volatile *next_in) : element(element_in), next(next_in) {}
    };
    mpsc_node_t volatile     *head;
    mpsc_node_t              *tail;
    mpsc_node_t               stub;
    void enqueue(mpsc_node_t *node);
public:
    mpsc_queue();
    mpsc_queue(const mpsc_queue &rhs);
    mpsc_queue &operator=(const mpsc_queue &rhs);
    void enqueue(T element);
    bool try_dequeue(T &element);
};

template <typename T>
mpsc_queue<T>::mpsc_queue() : head(&stub), tail(&stub) {}

template <typename T>
mpsc_queue<T>::mpsc_queue(const mpsc_queue &rhs) : head(&stub), tail(&stub), stub(rhs.stub) {}

template <typename T>
mpsc_queue &operator=(const mpsc_queue &rhs) {}

template <typename T>
void mpsc_queue<T>::enqueue(mpsc_node_t *node) {
    mpsc_node_t *prev = (mpsc_node_t *) __sync_lock_test_and_set(&head, node);
    prev->next = node;
}

template <typename T>
void mpsc_queue<T>::enqueue(T element) {
    enqueue(new mpsc_node_t {element, nullptr});
}

template <typename T>
bool mpsc_queue<T>::try_dequeue(T &element) {
    mpsc_node_t volatile *to_dequeue = tail;
    mpsc_node_t *next = (mpsc_node_t *) tail->next;
    if (to_dequeue == &stub)
    {
        if (0 == next)
            return false;
        tail = next;
        to_dequeue = next;
        next = (mpsc_node_t *) next->next;
    }
    if (next)
    {
        tail = next;
        element = to_dequeue->element;
        delete to_dequeue;
        return true;
    }
    if (to_dequeue != head)
        return false;
        enqueue(&stub);
    next = (mpsc_node_t *) to_dequeue->next;
    if (next)
    {
        tail = next;
        element = to_dequeue->element;
        delete to_dequeue;
        return true;
    }
    return false;
}

/*
struct mpscq_node_t
{
    mpscq_node_t* volatile  next;

};

struct mpscq_t
{
    mpscq_node_t* volatile  head;
    mpscq_node_t*           tail;
    mpscq_node_t            stub;

};

void mpscq_create(mpscq_t* self)
{
    self->head = &self->stub;
    self->tail = &self->stub;
    self->stub.next = 0;

}

void mpscq_push(mpscq_t* self, mpscq_node_t* n)
{
    n->next = 0;
    mpscq_node_t* prev = __sync_lock_test_and_set(&self->head, n);
    //(*)
    prev->next = n;

}

mpscq_node_t* mpscq_pop(mpscq_t* self)
{
    mpscq_node_t* tail = self->tail;
    mpscq_node_t* next = tail->next;
    if (tail == &self->stub)
    {
        if (0 == next)
            return 0;
        self->tail = next;
        tail = next;
        next = next->next;
    }
    if (next)
    {
        self->tail = next;
        return tail;
    }
    mpscq_node_t* head = self->head;
    if (tail != head)
        return 0;
    mpscq_push(self, &self->stub);
    next = tail->next;
    if (next)
    {
        self->tail = next;
        return tail;
    }
    return 0;
}
 */

#endif //GLAKV_MPSC_QUEUE_H
