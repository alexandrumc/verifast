#ifndef LISTEX_H
#define LISTEX_H

#include "list.h"

/*@

fixpoint t reverseHead<t>(t head, list<t> tail) {
    switch (tail) {
        case nil: return head;
        case cons(h, t): return reverseHead(h, t);
    }
}

fixpoint list<t> reverseTail<t>(t head, list<t> tail) {
    switch (tail) {
        case nil: return nil;
        case cons(h, t): return append(reverseTail(h, t), cons(head, nil));
    }
}

lemma void reverse_head_tail_lemma<t>(t h, list<t> t)
    requires true;
    ensures reverse(cons(h, t)) == cons(reverseHead(h, t), reverseTail(h, t));
{
    switch (t) {
        case nil:
        case cons(th, tt):
            reverse_head_tail_lemma(th, tt);
    }
}

@*/

#endif