
#include <atomic>
#include "impl.h"
#include <cassert>

#define EMPTY -1

#define PUSH 1
#define POP 2

struct Cell {
  Cell *pnext;
  void *pdata;

  Cell(void *pdata) : pnext(nullptr), pdata(pdata) {}
};

struct ThreadInfo {
  tid_t id;
  char op;
  Cell *cell;
};

struct Stack {
  tid_t threads;
  tid_t size;

  std::atomic<Cell *> ptop;
  std::atomic<ThreadInfo *> *location;
  std::atomic<tid_t> *collision;

  Stack(tid_t threads, tid_t size) : threads(threads), size(size) {
    ptop = nullptr;
    location = new std::atomic<ThreadInfo*>[threads];
    collision = new std::atomic<tid_t>[size];

    for(tid_t i = 0; i < threads; i++) {
      location[i] = nullptr;
    }

    for(tid_t i = 0; i < size; i++) {
      collision[i] = EMPTY;
    }
  }

  bool TryPerformStackOp(ThreadInfo *p) {
    Cell *mtop, *mnext;
    assert(p != nullptr);
    if(p->op == PUSH) {
      mtop = ptop.load();
      p->cell->pnext = mtop;

      return ptop.compare_exchange_weak(mtop, p->cell);
    }

    if(p->op == POP) {
      mtop = ptop;
      if(mtop == nullptr){
        p->cell = nullptr;
        return true;
      }
      mnext = mtop->pnext;
      if(ptop.compare_exchange_weak(mtop, mnext)) {
        p->cell = mtop;
        return true;
      }
    }
    return false;
  }

  tid_t GetPosition() {
    return rand() % size;
  }

  bool TryCollision(ThreadInfo *p, ThreadInfo *q) {
    if(p->op == PUSH) {
      ThreadInfo *qq = q;
      return location[q->id].compare_exchange_weak(qq, p);
    }

    if(p->op == POP) {
      ThreadInfo *qq = q;
      if(location[qq->id].compare_exchange_weak(qq, nullptr)) {
        p->cell = q->cell;
        location[p->id] = nullptr;
        return true;
      }
      return false;
    }
    // No op
    return false;
  }

  void FinishCollision(ThreadInfo *p) {
    if(p->op == POP) {
      ThreadInfo *m = location->load();
      p->cell = m->cell;
      location[p->id] = nullptr;
    }
  }

  void LesOP(ThreadInfo *p) {
    ThreadInfo *me = p;
    while(1) {
      location[p->id] = p;
      tid_t pos = GetPosition();
      tid_t him = collision[pos];

      while(!(collision[pos].compare_exchange_weak(him, p->id))) {}

      if(him != EMPTY) {
        ThreadInfo *q = location[him];
        if(q != nullptr && q->id==him && q->op != p->op ) {
          // Collision!
          if(location[p->id].compare_exchange_weak(p, nullptr)) {
            if(TryCollision(p, q))
              return;
            goto stack;
          } else {
            //Now p has been loaded in failed CAS so we use saved value
            FinishCollision(me);
            return;
          }
        }
      }

      p = me;
      if(!location[p->id].compare_exchange_weak(p, nullptr)) {
        FinishCollision(me);
        return;
      }

stack:
      if(TryPerformStackOp(p))
        return;
    }


  }

  void StackOp(ThreadInfo *p) {
    if(!TryPerformStackOp(p)) {
      LesOP(p);
    }
  }
};



ADTImpl::~ADTImpl(){
    Stack *S = (Stack *) state;
    delete S;
}

ADTImpl::ADTImpl(tid_t num_threads) {
    state = new Stack(num_threads, num_threads / 2);
}

void ADTImpl::add(val_t v, tid_t thread) {
    Stack *S = (Stack *)state;
    val_t *vl = new val_t(v);
    ThreadInfo *p = new ThreadInfo(thread, PUSH, new Cell(vl));
    S->StackOp(p);
}

res_t ADTImpl::rmv(tid_t thread) {
    Stack *S = (Stack *)state;
    val_t *vl = nullptr;
    ThreadInfo *p = new ThreadInfo(thread, POP, new Cell(nullptr));
    S->StackOp(p);

    assert(p->cell != nullptr); // There should always be a return cell

    val_t *v = (val_t *)p->cell->pdata;
    if(v == nullptr) {
      return {};
    }
    val_t vv = *v;
    delete p->cell;
    return vv;
}
