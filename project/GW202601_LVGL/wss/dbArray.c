#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "ite/itu.h"
#include "dbArray.h"

#define MIN_PRE_ALLOCATE_NR 5

typedef struct _DashboardArray {
    char* name;
    size_t size;        // used size
    size_t capacity;    // allocate size
    void** data;
    dbArrayPrintFunc print;
    //void* destory_ctx;
    //dbArrayDestoryFunc destroy;
} DashboardArray_t;

static void destoryDataDbArray(const DBARRAY self, void* data)
{
#if 0
    DashboardArray_t* array = (DashboardArray_t*)self;
    if(array->destroy != NULL) {
        array->destroy(array->destory_ctx, data);
    }

    return;
#endif
}

DBARRAY createDbArray(const char* name, dbArrayPrintFunc print)
{
    DashboardArray_t* array = (DashboardArray_t*)malloc(sizeof(DashboardArray_t));
    array->name = (char*)malloc(strlen(name)+1);

    if (array != NULL)
    {
        array->size = 0;
        array->capacity = 0; //capacity
        array->data = NULL;
        array->print = print;
        // TODO:
        //array->destroy =
        //array->destroy_ctx =
    }

    return ((void*)array);
}


bool isEmptyDbArray(const DBARRAY self)
{
    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;
    return array->size == 0;
}

/* method of getting size/capacity  */
/*
static int getSizeDbArray(const DBARRAY self)
{
    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;
    return array->size;
}

static int getCapacityDbArray(const DBARRAY self)
{
    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;
    return array->capacity;
}
*/

void printDbArray(const DBARRAY self)
{
    int i;
    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;

    if (isEmptyDbArray(self))
        printf("Db Array is empty! capacity=%d\n", array->capacity);
    else
    {
        printf("capacity=%d, size=%d\n", array->capacity, array->size);
        for (i = 0; i < array->size; i++) {
            array->print(array->data[i]);
        }
    }
}

void swapDbArray(DBARRAY self, size_t indexA, size_t indexB)
{
    int i;
    void** temp;

    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;

    if (isEmptyDbArray(self))
        printf("Db Array is empty! capacity=%d\n", array->capacity);
    else
    {
        temp = array->data[indexA];
        array->data[indexA] = array->data[indexB];
        array->data[indexB] = temp;
    }
}

static bool expandDbArray(const DBARRAY self, size_t need)
{
    size_t capacity = 0;
    void** data = NULL;

    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;

    if (array->size + need > array->capacity)
    {
        capacity = array->capacity + (array->capacity >> 1) + MIN_PRE_ALLOCATE_NR;
        printf("expand: %d->%d\n", array->size, capacity);
        data = (void**)realloc(array->data, sizeof(void*) * capacity);
        if (data != NULL) {
            array->data = data;
            array->capacity = capacity;
        }
    }

    return ((array->size + need) <= array->capacity) ? true : false;
}

bool insertDbArray(const DBARRAY self, size_t index, void* data)
{
    bool ret = false;
    size_t cursor = index;
    size_t i;

    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;
    cursor = cursor < array->size ? cursor : array->size;
    if (expandDbArray(self, 1)) {
        for (i = array->size; i > cursor; i--) {
            array->data[i] = array->data[i-1];
        }
        array->data[cursor] = data;
        array->size++;
        ret = true;
    }

    return ret;
}

bool prependDbArray(const DBARRAY self, void* data)
{
    return insertDbArray(self, 0, data);
}

bool appendDbArray(const DBARRAY self, void* data)
{
    return insertDbArray(self, -1, data);
}

bool getByIdxDbArray(const DBARRAY self, size_t index, void** data)
{
    assert(self);

    DashboardArray_t* array = (DashboardArray_t*)self;
    *data = array->data[index];

    return true;
}

bool getByIdDbArray(const DBARRAY self, void** data, dbArrayCompareFunc cmp, void* ctx)
{
    void* ret = NULL;
    size_t i = 0;
    assert(self);

    DashboardArray_t* array = (DashboardArray_t*)self;
#if 1
    if (array->size == 0)
        goto end;

    for (i = 0; i < array->size; i++) {
        if (cmp(ctx, array->data[i]) == 0) {
            *data = array->data[i];
            return true;
        }
    }
    return false;
#else
    size_t left, right, mid;

    if (array->size == 0)
        goto end;

    left = 0;
    right = array->size - 1;
    while (left <= right) {
        mid = left + (right - left) / 2;
        if (cmp(ctx, array->data[mid]) == 0) {
            *data = array->data[mid];
            ret = array->data[mid];
            break;
        } else if (cmp(ctx, array->data[mid]) < 0) {
            left = mid + 1;
        } else if (cmp(ctx, array->data[mid]) > 0) {
            right = mid - 1;
        }
    }
#endif

end:
    return (ret != NULL) ? true : false;
}

size_t lengthDbArray(const DBARRAY self)
{
    size_t len = 0;

    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;
    return array->size;
}

bool foreachDbArray(const DBARRAY self, dbArrayVisitFunc visit, void* ctx)
{
    size_t i = 0;
    bool ret = true;

    DashboardArray_t* array = (DashboardArray_t*)self;
    for (i = 0; i < array->size; i++) {
        ret = visit(ctx, array->data[i]);
    }

    return ret;
}

bool deleteDbArray(const DBARRAY self, size_t index)
{
#if 0
    size_t i = 0;
    bool ret = true;

    assert(self);
    DashboardArray_t* array = (DashboardArray_t*)self;
    destroyDataDbArray(thiz, thiz->data[index]);
    /*
	for(i = index; (i+1) < thiz->size; i++) {
		thiz->data[i] = thiz->data[i+1];
	}
	thiz->size--;

	darray_shrink(thiz);
*/
#endif
    return true;
}


void destoryDbArray(const DBARRAY self)
{
    printf("%s\n", __func__);
#if 0
    if (self) {
        free(((DashboardArray_t*)self)->name);
        free(self);
    }
#endif
}

