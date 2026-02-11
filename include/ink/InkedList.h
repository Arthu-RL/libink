#ifndef INKEDLIST_H
#define INKEDLIST_H


#pragma once

#include <algorithm>

#include "ink/ink_base.hpp"

namespace ink {

template <typename T>
class INK_API InkedList {
public:
    InkedList() : root(nullptr), tail(nullptr), size(0)
    {
        // Empty
    }


    InkedList(const T& data) : size(1)
    {
        root = new Node(data);
        tail = root;
    }

    InkedList(T&& data) : size(1)
    {
        root = new Node(std::move(data));
        tail = root;
    }

    InkedList(const T& header_data, const T& data) : size(2)
    {
        root = new Node(data);
        Node* header = new Node(header_data);
        root->prev = header;
        header->next = root;

        tail = root;
    }

    InkedList(T&& header_data, T&& data) : size(2)
    {
        root = new Node(std::move(data));
        Node* header = new Node(std::move(header_data));
        root->prev = header;
        header->next = root;

        tail = root;
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

    InkedList(InkedList&& inkedList) = delete;
    InkedList(const InkedList& inkedList) = delete;
    InkedList& operator=(const InkedList& inkedList) = delete;
    InkedList& operator=(InkedList&& inkedList) = delete;

    size_t length() const noexcept { return size; }

    void push_back(const T& data)
    {
        Node* newNode = new Node(data);
        if (root == nullptr)
        {
            root = newNode;
            tail = newNode;
        }
        else
        {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        size++;
    }

    void push_back(T&& data)
    {
        Node* newNode = new Node(std::move(data));
        if (root == nullptr)
        {
            root = newNode;
            tail = newNode;
        }
        else
        {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        size++;
    }

    void enqueue(const T& data)
    {
        Node* newNode = new Node(data);
        if (root == nullptr)
        {
            root = newNode;
            tail = newNode;
        }
        else
        {
            newNode->next = root;
            root->prev = newNode;
            root = newNode;
        }
        size++;
    }

    void enqueue(T&& data)
    {
        Node* newNode = new Node(std::move(data));
        if (root == nullptr)
        {
            root = newNode;
            tail = newNode;
        }
        else
        {
            newNode->next = root;
            root->prev = newNode;
            root = newNode;
        }
        size++;
    }

    bool pop_front(T* data = nullptr)
    {
        if (root == nullptr)
            return false;

        if (data != nullptr)
            *data = std::move(root->data);

        Node* oldRoot = root;
        root = root->next;
        if (root == nullptr)
            tail = nullptr;
        else
            root->prev = nullptr;

        delete oldRoot;
        size--;
        return true;
    }

    bool pop_back(T* data = nullptr)
    {
        if (tail == nullptr)
            return false;

        if (data != nullptr)
            *data = std::move(tail->data);

        Node* oldTail = tail;
        tail = tail->prev;

        if (tail == nullptr)
            root = nullptr;
        else
            tail->next = nullptr;

        delete oldTail;
        size--;
        return true;
    }

    void insert(const T& data, size_t index)
    {
        if (index == 0 || root == nullptr)
        {
            enqueue(data);
            return;
        }

        if (index >= size)
        {
            push_back(data);
            return;
        }

        Node* current = root;
        for (size_t i = 0; i < index; i++)
            current = current->next;

        Node* newNode = new Node(data);
        newNode->prev = current->prev;
        newNode->next = current;
        current->prev->next = newNode;
        current->prev = newNode;
        size++;
    }

    void insert(T&& data, size_t index)
    {
        if (index == 0 || root == nullptr)
        {
            enqueue(data);
            return;
        }

        if (index >= size)
        {
            push_back(data);
            return;
        }

        Node* current = root;
        for (size_t i = 0; i < index; i++)
            current = current->next;

        Node* newNode = new Node(std::move(data));
        newNode->prev = current->prev;
        newNode->next = current;
        current->prev->next = newNode;
        current->prev = newNode;
        size++;
    }

    bool remove_idx(size_t index)
    {
        if (index >= size)
            return false;

        if (index == 0)
            return pop_front();

        if (index == size - 1)
            return pop_back();

        Node* current = root;
        for (size_t i = 0; i < index; i++)
            current = current->next;

        current->prev->next = current->next;
        current->next->prev = current->prev;
        delete current;
        size--;
        return true;
    }

    bool remove_data(const T& data)
    {
        if (root == nullptr)
            return false;

        if (root->data == data)
            return pop_front();

        if (tail->data == data)
            return pop_back();

        Node* current = root->next;
        while (current != nullptr && current != tail)
        {
            if (current->data == data)
            {
                if (current == root)
                    return pop_front();

                if (current == tail)
                    return pop_back();

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
    struct Node {
        Node(const T& _data) : data(_data), prev(nullptr), next(nullptr) {}
        T data;
        Node* prev;
        Node* next;
    };

    Node* root; // First element, that can have a header
    Node* tail; // Back
    size_t size;
};

} // namespace ink

#endif // INKEDLIST_H
