#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include <string.h>

#define STACK_SIZE 10000000
#define MAX_NAME_SIZE 150

struct stackItem
{
    char fileName[MAX_NAME_SIZE];
    int depth;
};

struct stack
{
    struct stackItem items[STACK_SIZE];
    int topInd;
};

void init(struct stack *st);
void push(struct stack *st, struct stackItem *item);
struct stackItem pop(struct stack *st);
int empty(struct stack *st);

#endif
