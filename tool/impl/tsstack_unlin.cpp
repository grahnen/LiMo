#include "impl.h"
#include <cassert>
#include <atomic>


//std::atomic<timestamp_t> gts = 1;
timestamp_t gts = 1;

timestamp_t get_ts() {
    return gts++;
}

struct Node {
    val_t data;
    timestamp_t ts;
    Node *next;
    std::atomic<bool> mark;

    Node(val_t d, timestamp_t ts, Node *next, bool m) : data(d), ts(ts), next(next), mark(m) {}
};


Node *mkNode(val_t v, bool mark) {
    return new Node(v, 0, nullptr, mark);
}

struct SPPool {
    std::atomic<Node *> top;
    tid_t id;

    SPPool() {
        Node *n = new Node(val_t(0,0), 0, nullptr, true);
        n->next = n;
        top.store(n);
    }

    Node *insert(val_t v) {
        Node *n = mkNode(v, false);
        n->ts = get_ts();
        n->next = top;
        top = n;

        Node *next = n->next;
        while(next->next != next && next->mark) {
            next = next->next;
        }
        n->next = next;
        return n;
    }


    std::pair<bool, res_t> remove(Node *oldTop, Node *n) {
        bool f = false;
        if(n->mark.compare_exchange_strong(f, true)) {
            Node *tmp = oldTop;
            top.compare_exchange_strong(tmp, n);

            if(oldTop != n) {
                oldTop->next = n;
            }

            Node *next = n->next;
            while(next->next != next && next->mark)
                next = next->next;
            n->next = next;
            return std::pair(true, n->data);
        }
        return std::pair<bool, res_t>(false, {});
    }


    std::pair<Node *, Node *> get_youngest() {
        Node *oldTop = top;
        Node *res = oldTop;
        while(true) {
            if(!res->mark)
                return std::pair(res, oldTop);
            else if (res->next == res)
                return std::pair(nullptr, oldTop);
            res = res->next;
        }
    }

};

class TSStack {
public:
        tid_t threads;
        SPPool *pools;
        TSStack(tid_t threads) : threads(threads) {
            pools = new SPPool[threads];
            for(int i = 0; i < threads; i++) {
                pools[i].id = i;
            }
        }

        void Push(val_t v, tid_t t) {
            SPPool &pool = pools[t];
            Node *n = pool.insert(v);
        }

        res_t Pop() {
            timestamp_t ts = get_ts();
            bool success = false;
            while(true) {
                std::pair<bool, res_t> r = tryRem(ts);
                if(r.first)
                    return r.second;
            }
        }

        std::pair<bool, res_t> tryRem(timestamp_t ts) {
            Node *yng = nullptr;
            timestamp_t t = 0;
            SPPool *pool;
            Node *top;
            Node *empty[threads];

            for(tid_t i = 0; i < threads; i++) {
                empty[i] = nullptr;
            }

            for(tid_t i = 0; i < threads; i++) {
                SPPool &c = pools[i];
                std::pair<Node *, Node *> youngest = c.get_youngest();
                if(youngest.first == nullptr) {
                    empty[i] = youngest.second;
                    continue;
                }
                timestamp_t nts = youngest.first->ts;
                if(ts < nts)
                    return c.remove(youngest.second, youngest.first);
                if(t < nts) {
                    yng = youngest.first;
                    t = nts;
                    pool = &c;
                    top = youngest.second;
                }
            }

            if(yng == nullptr) {
                for(tid_t i = 0; i < threads; i++) {
                    SPPool &c = pools[i];
                    if(c.top != empty[i])
                        return std::pair<bool, res_t>(false, {});
                }

                return std::pair<bool, res_t>(true, {});
            }

            return pool->remove(top, yng);
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
