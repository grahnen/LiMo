#include "impl.h"
#include <mutex>
#include <vector>
#include <cassert>
#include <atomic>
#include <chrono>
#include <thread>

struct Timestamp {
    timestamp_t first, last;
    bool operator<(Timestamp &o) const {
        return last < o.first;
    }

    static Timestamp nil() {
        return Timestamp { .first = 0, .last = 0 };
    }
};

struct TSStack {

    struct Node {
        val_t element;
        Timestamp tss;
        Node *next;
        std::atomic<bool> taken;
        Node(val_t e, Timestamp tss, Node *n, bool tk) : element(e), tss(tss), next(n), taken(tk) {}
        Node(val_t e, Timestamp tss) : Node(e, tss, nullptr, false) {}
        static Node *sentinel() {
            Node *s = new Node(0, Timestamp::nil(), nullptr, true);
            s->next = s;
            return s;
        }
    };

    struct SPPool {
        std::atomic<Node*> top;
        int ID;

        SPPool() {
            Node *sentinel = Node::sentinel();
            top = sentinel;
        }

        Node *insert(val_t e) {
            Node *n = new Node(e, Timestamp::nil(), top, false);
            top = n;
            Node *next = n->next;
            while(next->next != next && next->taken) {
                next = next->next;
            }
            n->next = next;
            return n;
        }

        std::pair<Node *, Node *> getYoungest() {
            Node *oldTop = top;
            Node *result = oldTop;
            while(true) {
                if(!result->taken)
                    return std::pair(result, oldTop);
                else if (result->next == result)
                    return std::pair(nullptr, oldTop);
                result = result->next;
            }
        }

        std::pair<bool, val_t> remove(Node *oldTop, Node *n) {
            bool tmp = false;
            if(n->taken.compare_exchange_strong(tmp, true)) {
                Node *tmp = oldTop;
                top.compare_exchange_strong(tmp, n);
                if(oldTop != n) {
                    oldTop->next = n;
                }
                Node *next = n->next;
                while(next->next != next && next->taken) {
                    next = next->next;
                }
                n->next = next;
                return std::pair(true, n->element);
            }
            return std::pair(false, 0);
        }
    };


    std::atomic<timestamp_t> counter = 1;

    SPPool *spPools;
    tid_t max_threads;

    TSStack(tid_t max_threads) : max_threads(max_threads) {
        spPools = new SPPool[max_threads];
        for(int i = 0; i < max_threads; i++) {
            spPools[i].ID = i;
        }
    }

    Timestamp getTimestamp() {
        timestamp_t tss = counter;
        std::this_thread::sleep_for(std::chrono::nanoseconds(8));
        timestamp_t ts2 = counter;
        if(tss != ts2) {
            return Timestamp(tss, ts2);
        }
        timestamp_t temp = tss;
        if(counter.compare_exchange_strong(temp, tss + 1)) {
            return Timestamp(tss, tss);
        }
        return Timestamp(tss, counter - 1);
    }

    void Push(val_t elem, tid_t thr) {
        SPPool &pool = spPools[thr];
        Node *n = pool.insert(elem);
        Timestamp tss = getTimestamp();
        n->tss = tss;
    }

    optval_t Pop() {
        Timestamp startTime = getTimestamp();
        bool success = false;
        optval_t elem = 0;
        do {
            std::pair<bool, optval_t> r = tryRem(startTime);
            success = r.first;
            elem = r.second;
        } while(!success);
        return elem;
    }

    std::pair<bool, optval_t> tryRem(Timestamp startTime) {
        Node *youngest = nullptr;
        Timestamp tss = Timestamp::nil();
        SPPool *pool;
        Node *top;
        Node **empty = new Node*[max_threads];
        for(tid_t t = 0; t < max_threads; t++)
            empty[t] = nullptr;
        for(tid_t t = 0; t < max_threads; t++) {
            auto p = spPools[t].getYoungest();
            Node *node = p.first;
            Node *poolTop = p.second;

            if(node == nullptr) {
                empty[t] = poolTop;
                continue;
            }
            Timestamp nodeTs = node->tss;
            if(startTime < nodeTs)
                return spPools[t].remove(poolTop, node);
            if(tss < nodeTs) {
                youngest = node;
                tss = nodeTs;
                pool = &spPools[t];
                top = poolTop;
            }
        }
        if(youngest == nullptr) {
            for(tid_t t = 0; t < max_threads; t++) {
                if(spPools[t].top != empty[t]){
                    //std::cout << "failure" << std::endl;
                    return std::pair<bool, optval_t>(false, {});
                }
            }
            return std::pair<bool, optval_t>(true, {});
        }
        return pool->remove(top, youngest);
    }
};

res_t ADTImpl::rmv(tid_t t) {
  TSStack *s = (TSStack *) this->state;

  res_t r =  s->Pop();

  return r;
}

void ADTImpl::add(val_t v, tid_t t) {
  TSStack *s = (TSStack *) this->state;

  s->Push(v, t);

}

ADTImpl::ADTImpl(tid_t threads) {
  state = new TSStack(threads);
}

ADTImpl::~ADTImpl() {}
