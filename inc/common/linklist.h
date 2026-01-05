#ifndef LINKLIST_H
#define LINKLIST_H
#include "common/common.h"

template<typename T>
class LinkList {
public:
    struct LinkNode {
        T* data;
        LinkNode* next;
    };

    LinkList() {
        head = nullptr;
        tail = nullptr;
    }
    ~LinkList() {
        if (tail != nullptr) {
            LinkNode* tmp = tail;
            while (tmp->next != tail) {
                LinkNode* nxt = tmp->next;
                delete tmp;
                tmp = nxt;
            }
            delete tmp;
            tail = nullptr;
            head = nullptr;
        }
    }
    void push(T* data) {
        LinkNode* node = new LinkNode();
        node->data = data;
        if (head == nullptr) {
            head = node;
            sizeTail = node;
            tail = node;
            node->next = node;
        } else {
            sizeTail->next = node;
            sizeTail = node;
            sizeTail->next = head;
        }
        node_map[data] = node;
    }
    /**
     * @brief Get the head object
     * 
     * @return T* 
     */
    T* front() {
        return head->data;
    }
    /**
     * @brief Get the tail object
     * 
     * @return T* 
     */
    T* back() {
        return tail->data;
    }
    /**
     * @brief Get the tail object and move tail to the next node
     * 
     * @return T* 
     */
    T* next() {
        T* data = tail->data;
        tail = tail->next;
        return data;
    }
    /**
     * @brief Pop the head object and move head to the next node
     * 
     * @return T* 
     */
    T* pop() {
        T* data = head->data;
        head = head->next;
        return data;
    }
    bool empty() {
        return head == tail;
    }
    bool full() {
        return tail->next == head;
    }
    bool one() {
        return head->next == tail;
    }
    void setHead(T* head) {
        this->head = node_map[head];
    }
    void setTail(T* tail) {
        this->tail = node_map[tail];
    }
private:
    LinkNode* head;
    LinkNode* tail;
    LinkNode* sizeTail;
    std::unordered_map<T*, LinkNode*> node_map;
};

// type T must have a member next
template<typename T>
class LinkListN {
public:

    LinkListN() {
        head = nullptr;
        tail = nullptr;
    }
    ~LinkListN() {
    }
    void push(T* data) {
        if (head == nullptr) {
            head = data;
            sizeTail = data;
            tail = data;
            data->next = data;
        } else {
            sizeTail->next = data;
            sizeTail = data;
            sizeTail->next = head;
        }
    }
    /**
     * @brief Get the head object
     * 
     * @return T* 
     */
    T* front() {
        return head;
    }
    /**
     * @brief Get the tail object
     * 
     * @return T* 
     */
    T* back() {
        return tail;
    }
    /**
     * @brief Get the tail object and move tail to the next node
     * 
     * @return T* 
     */
    T* next() {
        T* data = tail;
        tail = tail->next;
        return data;
    }
    /**
     * @brief Pop the head object and move head to the next node
     * 
     * @return T* 
     */
    T* pop() {
        T* data = head;
        head = head->next;
        return data;
    }
    bool empty() {
        return head == tail;
    }
    bool full() {
        return tail->next == head;
    }
    bool one() {
        return head->next == tail;
    }
    void setHead(T* head) {
        this->head = head;
    }
    void setTail(T* tail) {
        this->tail = tail;
    }
private:
    T* head;
    T* tail;
    T* sizeTail;
};


#endif

