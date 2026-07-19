#ifndef DB_ARRAY_H
#define DB_ARRAY_H

#include "ite/itp.h"

typedef void* DBARRAY;
typedef void (*dbArrayPrintFunc)(void* data);
typedef void (*dbArrayDestoryFunc)(void* ctx, void* data);
typedef int (*dbArrayCompareFunc)(void* ctx, void* data);
typedef bool (*dbArrayVisitFunc)(void* ctx, void* data);

DBARRAY createDbArray(const char* name, dbArrayPrintFunc print);
void printDbArray(const DBARRAY self);
void swapDbArray(DBARRAY self, size_t indexA, size_t indexB);
bool insertDbArray(const DBARRAY self, size_t index, void* data);
bool prependDbArray(const DBARRAY self, void* data);
bool appendDbArray(const DBARRAY self, void* data);
bool getByIdxDbArray(const DBARRAY self, size_t index, void** data);
bool getByIdDbArray(const DBARRAY self, void** data, dbArrayCompareFunc cmp, void* ctx);
size_t lengthDbArray(const DBARRAY self);
bool foreachDbArray(const DBARRAY self, dbArrayVisitFunc visit, void* ctx);


#endif
