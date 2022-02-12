#include "stack.h"

void init(struct stack *st)
{
    st->topInd = 0;
}

int empty(struct stack *st)
{
    return (st->topInd == 0) ? 1 : 0;
}

void push(struct stack *st, struct stackItem *item)
{
    if (st->topInd >= STACK_SIZE)
    {
        printf("Stack overflow");
        return;
    }

    strcpy(st->items[st->topInd].fileName, item->fileName);
    st->items[st->topInd].depth = item->depth;
    st->topInd++;
}

struct stackItem pop(struct stack *st)
{
    struct stackItem item = {.fileName = "", .depth = 0};

    if ((st->topInd) > 0)
    {
        st->topInd--;
        strcpy(item.fileName, st->items[st->topInd].fileName);
        item.depth = st->items[st->topInd].depth;
    }
    else
        printf("Stack is empty\n");

    return item;
}
