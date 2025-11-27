#ifndef FINE_SET_H
#define FINE_SET_H

#include "node.h"

class FineSet {
private:
    Node* head;

public:
    FineSet() {
        head = new Node(-1); // sentinel
        head->next = new Node(1000000000); // tail sentinel
    }

    ~FineSet() {
        Node* cur = head;
        while(cur) {
            Node* tmp = cur;
            cur = cur->next;
            delete tmp;
        }
    }

    bool add(int val) {
        Node* pred = head;
        pthread_mutex_lock(&pred->lock);
        Node* curr = pred->next;
        pthread_mutex_lock(&curr->lock);

        while(curr->val < val) {
            pthread_mutex_unlock(&pred->lock);
            pred = curr;
            curr = curr->next;
            pthread_mutex_lock(&curr->lock);
        }
        if(curr->val == val) {
            pthread_mutex_unlock(&pred->lock);
            pthread_mutex_unlock(&curr->lock);
            return false;
        }
        Node* node = new Node(val);
        node->next = curr;
        pred->next = node;
        pthread_mutex_unlock(&pred->lock);
        pthread_mutex_unlock(&curr->lock);
        return true;
    }

    bool remove(int val) {
        Node* pred = head;
        pthread_mutex_lock(&pred->lock);
        Node* curr = pred->next;
        pthread_mutex_lock(&curr->lock);

        while(curr->val < val) {
            pthread_mutex_unlock(&pred->lock);
            pred = curr;
            curr = curr->next;
            pthread_mutex_lock(&curr->lock);
        }
        if(curr->val != val) {
            pthread_mutex_unlock(&pred->lock);
            pthread_mutex_unlock(&curr->lock);
            return false;
        }
        pred->next = curr->next;
        pthread_mutex_unlock(&pred->lock);
        pthread_mutex_unlock(&curr->lock);
        delete curr;
        return true;
    }

    bool contains(int val) {
        Node* pred = head;
        pthread_mutex_lock(&pred->lock);
        Node* curr = pred->next;
        pthread_mutex_lock(&curr->lock);

        while(curr->val < val) {
            pthread_mutex_unlock(&pred->lock);
            pred = curr;
            curr = curr->next;
            pthread_mutex_lock(&curr->lock);
        }
        bool found = (curr->val == val);
        pthread_mutex_unlock(&pred->lock);
        pthread_mutex_unlock(&curr->lock);
        return found;
    }
};

#endif

