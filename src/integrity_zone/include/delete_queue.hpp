#ifndef DELETE_QUEUE_HPP
#define DELETE_QUEUE_HPP

#include "rid.h"
#include <iostream>

#define MAX_ELE_NUM (1024 * 1024)

class DeleteQueue {
public:
    DeleteQueue() : head(0), reuse(0), tail(0), size(0), dequeue_cnt(0) {}

    bool enqueue(RID rid) {
        if (size >= MAX_ELE_NUM) {
            std::cerr << "Queue is full!" << std::endl;
            return false;
        }
        queue[tail] = rid;
        tail = (tail + 1) % MAX_ELE_NUM;
        size++;
        return true;
    }

    RID dequeue() {
        if (head == reuse) {
            return INVALID_RID;
        }
        RID rid = queue[head];
        head = (head + 1) % MAX_ELE_NUM;
        size--;
        dequeue_cnt ++;
        return rid;
    }

    void set_reuse() {
        reuse = tail;
        size = count_between(head, tail);
    }

    void discard_pending() {
        tail = reuse;
        size = count_between(head, tail);
    }

    void print_statistics() {
        std::cout << "Queue size: " << size << std::endl;
        std::cout << "Dequeue count: " << dequeue_cnt << std::endl;
    }

private:
    int count_between(int begin, int end) const {
        return end >= begin ? end - begin : MAX_ELE_NUM - begin + end;
    }

    RID queue[MAX_ELE_NUM];
    int head;
    int reuse;
    int tail;
    int size;
    int dequeue_cnt;
};

#endif // DELETE_QUEUE_HPP
