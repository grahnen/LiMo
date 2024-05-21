#!/bin/env python3
from collections import namedtuple
import sys

Operation = namedtuple('Operation', ['time', 'type', 'op', 'result'])

def read_history(filename):
    history = []
    with open(filename, 'r') as f:
        for line in f:
            try:
                thread, type_, op, val = line.strip().split()
                time = int(thread)
                type_ = 'call' if type_ == 'call' else 'return'
                op = 'push' if op == 'push' else 'pop'
                result = int(val) if op == 'pop' else None
                history.append(Operation(time, type_, op, result))
            except:
                pass
    return history

def is_linearizable(history):
    pending = []
    stack = []

    for event in history:
        thread, event_type, op, val = parse_event(event)

        if event_type == "call":
            pending.append((thread, op, val))
        else:
            pending.remove((thread, op, val))

            if op == "pop":
                if len(stack) == 0 or stack[-1] != val:
                    return False
                stack.pop()
            else:
                stack.append(val)

            for t, o, v in pending:
                if (o == "push" and t != thread) or (o == "pop" and v != stack[-1]):
                    return False

    return True

def verify_linearizability(history):
    stack = []
    push_seq = []
    last_push = None
    for op in history:
        if op.type == 'call' and op.op == 'push':
            stack.append(op)
            last_push = op
        elif op.type == 'call' and op.op == 'pop':
            if len(stack) == 0:
                return False
            stack[-1] = (stack[-1][0], stack[-1][1], stack[-1][2], op.result)
        elif op.type == 'return' and op.op == 'push':
            if last_push is None or last_push != op:
                return False
            last_push = None
            stack.pop()
        elif op.type == 'return' and op.op == 'pop':
            if len(stack) == 0 or stack[-1].op != 'pop' or stack[-1].result != op.result:
                return False
            stack.pop()
            if last_push is None and len(stack) > 0 and stack[-1].op == 'push':
                last_push = stack[-1]
                push_seq.append(last_push)
    return len(push_seq) == 0



def check_linearizability(filename):
    history = read_history(filename)
    is_lin = is_linearizable(history)
    if is_lin:
        print("The history is linearizable.")
    else:
        print("The history is not linearizable.")


if __name__ == "__main__":
    filename = sys.argv[1]
    check_linearizability(filename)
