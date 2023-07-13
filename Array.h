#ifndef ARRAY_H
#define ARRAY_H

#include "Lib.h"
typedef struct{
    int size;
    int count;
    void **item;
} Array;

typedef enum {KEEP_SPLITER, REMOVE_SPLITER} SplitMode;

extern Array* ArrayNew(int size);
extern void ArrayFree(Array *array, FuncPtr1 freeFuncPtr);
extern void ArrayAdd(Array *array, void *item);
extern void* ArrayGet(Array *array, int i);
extern void ArrayPush(Array *array,void *item);
extern void* ArrayPop(Array *array);
extern void* ArrayPeek(Array *array);
extern void* ArrayLast(Array *array);
extern void ArrayEach(Array *array, FuncPtr1 f);

extern Array* split(char *str, char *spliter, SplitMode mode);


#endif
