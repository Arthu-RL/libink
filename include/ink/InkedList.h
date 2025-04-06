#ifndef INKEDLIST_H
#define INKEDLIST_H


#pragma once

#include <algorithm>

#include "ink/ink_base.hpp"

namespace ink {

template <typename T>
class InkedList {
public:
    struct Node {
        Node(const T& _data) : data(_data), prev(nullptr), next(nullptr) {}
        T data;
        Node* prev;
        Node* next;
    };

    InkedList() : root(nullptr), tail(nullptr), size(0)
    {
        // Empty
    }

    InkedList(const T& data) : size(1)
    {
        root = new Node(std::move(data));
        tail = root;
    }

    InkedList(const T& header_data, const T& data) : size(2)
    {
        root = new Node(std::move(header_data));
        root->next = new Node(std::move(data));
        root->next->prev = root;
        tail = root->next;
    }

    ~InkedList()
    {
        Node* current = root;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }

    // Methods
    Node* head() noexcept { return root; }

    ink_u32 length() const noexcept { return size; }

    void push_back(const T& data)
    {
        Node* newNode = new Node(std::move(data));
        if (root == nullptr) {
            root = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        size++;
    }

    void enqueue(const T& data)
    {
        Node* newNode = new Node(std::move(data));
        if (root == nullptr) {
            root = newNode;
            tail = newNode;
        } else {
            newNode->next = root;
            root->prev = newNode;
            root = newNode;
        }
        size++;
    }

    bool pop_front(T* data = nullptr)
    {
        if (root == nullptr) {
            return false;
        }
        if (data != nullptr) {
            *data = root->data;
        }
        Node* oldRoot = root;
        root = root->next;
        if (root == nullptr) {
            tail = nullptr;
        } else {
            root->prev = nullptr;
        }
        delete oldRoot;
        size--;
        return true;
    }

    bool pop_back(T* data = nullptr)
    {
        if (tail == nullptr) {
            return false;
        }
        if (data != nullptr) {
            *data = tail->data;
        }
        Node* oldTail = tail;
        tail = tail->prev;
        if (tail == nullptr) {
            root = nullptr;
        } else {
            tail->next = nullptr;
        }
        delete oldTail;
        size--;
        return true;
    }

    void insert(const T& data, ink_u32 index)
    {
        if (index == 0 || root == nullptr) {
            enqueue(data);
            return;
        }
        if (index >= size) {
            push_back(data);
            return;
        }
        Node* current = root;
        for (ink_u32 i = 0; i < index; i++) {
            current = current->next;
        }
        Node* newNode = new Node(std::move(data));
        newNode->prev = current->prev;
        newNode->next = current;
        current->prev->next = newNode;
        current->prev = newNode;
        size++;
    }

    bool remove(ink_u32 index)
    {
        if (index >= size)
            return false;

        if (index == 0)
            return pop_front();

        if (index == size - 1)
            return pop_back();

        if (index < 0)
            index = size - index;

        Node* current = root;
        for (ink_u32 i = 0; i != index; i++)
        {
            current = current->next;
        }
        current = current->next;

        current->prev->next = current->next;
        current->next->prev = current->prev;
        delete current;
        size--;
        return true;
    }

    bool remove(const T& data)
    {
        if (root == nullptr)
            return false;

        if (root->data == data)
            return pop_front();

        if (tail->data == data)
            return pop_back();

        Node* current = root->next;
        while (current != nullptr && current != tail) {
            if (current->data == data) {
                current->prev->next = current->next;
                current->next->prev = current->prev;
                delete current;
                size--;
                return true;
            }
            current = current->next;
        }
        return false;
    }

private:
    Node* root; // First element, that can have a header
    Node* tail; // Back
    ink_u32 size;
};

} // namespace ink

#endif // INKEDLIST_H
