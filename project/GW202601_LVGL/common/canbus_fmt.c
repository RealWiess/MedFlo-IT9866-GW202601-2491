#include <stdio.h>
#include <string.h>
#include "iniparser/iniparser.h"
#include "dbArray.h"
#include "canbus_fmt.h"

#define CANFMT_FILENAME "canbusfmt.ini"
#define LINE_OF_ITEM    4
#define LEN_MAX         16
#define CUSTOM_STRING "custom"

//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) /* do nothing */
#endif

#define MAKE_MASK(bits)         ((1<<(bits))-1)
#define SHIFT_MASK(shift,bits)  (MAKE_MASK(bits) << shift)
#define GET_DATA_BIT(x,l,r)     ((x & SHIFT_MASK(r, l-r+1)) >> r)

typedef struct _CustomItemTable {
    FixedItem_e item;
    char *item_string;
} CustomItemTable_t;

typedef struct _CustomKeyTable {
    ItemKey_e key;
    char *key_string;
} CustomKeyTable_t;

CustomItemTable_t customItem[] =
{
    {FI_SPEED,  "speed"},
    {FI_ODO,    "odo"},
    {FI_GEAR,   "gear"},
};

CustomKeyTable_t itemKey[] =
{
    {IK_CANID,  "canid"},
    {IK_BYTE_H, "byteH"},
    {IK_BYTE_L, "byteL"},
    {IK_BIT_H,  "bitH"},
    {IK_BIT_L,  "bitL"},
};

CanbusFmt_t* createVertex(uint32_t canId);
void insertListWidgetNode(WidgetNode_t** sPtr, FixedItem_e item, uint8_t byteH, uint8_t byteL, uint8_t bitH, uint8_t bitL);
void printListWidgetNode(WidgetNode_t* sPtr);

static dictionary* canFmtIni;
DBARRAY canCfgArray;
uint32_t* canDataArray;
size_t gCount;

static void make_string(char* name, int len, char* key)
{
    int i = len;
    int size = LEN_MAX;

    while (i < size) {
        name[i] = '\0';
        i++;
    }
    //printf("%s, %d, %s\n", name, len, key);
    strcat(name, key);
    //printf("%s, %d, %s\n", name, len, key);
    return;
}

static int canid_cmp(void* ctx, void* data)
{
    CanbusFmt_t* canData = (CanbusFmt_t*)data;
    return ((uint32_t)canData->canId - (int)ctx);
}

static bool is_Canid_exist(const DBARRAY pArray, const uint32_t canid)
{
     CanbusFmt_t* pVertex = NULL;

    getByIdDbArray(pArray, (void**)&pVertex, canid_cmp, (void*)canid);
    if (pVertex == NULL)
        return false;
    return true;
}

static void find_canfmt(uint32_t canId, FixedItem_e item, char* name, size_t len)
{
    CanbusFmt_t* pVertex = NULL;
    uint8_t byteH, byteL, bitH, bitL;

    //printf("canId = 0x%X\n", canId);
    if (!is_Canid_exist(canCfgArray, canId)) {
        //printf("\t===>create\n");
        appendDbArray(canCfgArray, (void*)createVertex(canId));
    }

    make_string(name, len, itemKey[IK_BYTE_H].key_string);
    byteH = iniparser_getint(canFmtIni, name, 0);
    make_string(name, len, itemKey[IK_BYTE_L].key_string);
    byteL = iniparser_getint(canFmtIni, name, 0);
    make_string(name, len, itemKey[IK_BIT_H].key_string);
    bitH = iniparser_getint(canFmtIni, name, 0);
    make_string(name, len, itemKey[IK_BIT_L].key_string);
    bitL = iniparser_getint(canFmtIni, name, 0);
    getByIdDbArray(canCfgArray, (void**)&pVertex, canid_cmp, (void*)canId);
    //printf(">>%d, 0x%x\n", __LINE__, pVertex);
    insertListWidgetNode(&pVertex->start, item, byteH, byteL, bitH, bitL);
    //printListWidgetNode(pVertex->start);
}

static uint32_t getDataByte(uint8_t* data, uint8_t bH, uint8_t bL)
{
    int i;
    uint32_t value = 0;
    uint8_t size;

    size = bH - bL + 1;
    if (bH < bL || size > 4)
        return 0;

    for (i = 0; i < size; i++)
    {
        value <<= 8;
        value |= data[bH - i];
    }

    return value;
}

void canFmtPrint(void* data)
{
    CanbusFmt_t* canData = (CanbusFmt_t*)data;
    DEBUG_PRINT(" canId = 0x%X\n", canData->canId);
    printListWidgetNode(canData->start);
}

static void canFmtGrabber(WidgetNode_t* node, uint8_t* buf)
{
    DEBUG_PRINT("%d \t%d %d", node->no, node->byteL, node->byteH);
    if (node->byteH < node->byteL)
        canDataArray[node->no] = 0;
    if (node->bitH < node->bitL)
        canDataArray[node->no] = 0;

    if (node->byteH - node->byteL) {
        canDataArray[node->no] = getDataByte(buf, node->byteH, node->byteL);
    } else {
        if (node->bitH == 0 && node->bitL == 0)
            canDataArray[node->no] = getDataByte(buf, node->byteH, node->byteL);
        else
            canDataArray[node->no] = GET_DATA_BIT(buf[node->byteL], node->bitH, node->bitL);
    }
    DEBUG_PRINT(">> 0x%X\n", canDataArray[node->no]);
}

uint32_t* CanFmtParser(uint32_t canId, uint8_t* buf)
{
    size_t i = 0;
    CanbusFmt_t* pVertex = NULL;
    WidgetNode_t* node = NULL;
    size_t cnt = 0;

    if (canCfgArray != NULL) {
        if (getByIdDbArray(canCfgArray, (void**)&pVertex, canid_cmp, (void*)canId)) {
            node = pVertex->start;
            while (node != NULL) {
                canFmtGrabber(node, buf);
                node = node->next;
                cnt++;
            }
        }
    } else {
        printf("Adjacency List of canbus format is not exist!\n");
        return NULL;
    }

    return canDataArray;
}

void canFmtCreate(void *data)
{
}

void canFmtDestroy(void *data)
{
}

CanbusFmt_t* createVertex(uint32_t canId)
{
    CanbusFmt_t* newVertex = (CanbusFmt_t*)malloc(sizeof(CanbusFmt_t));
    if (!newVertex) {
        printf("createVertex: malloc failed for canId=0x%X\n", canId);
        return NULL;
    }

    newVertex->canId = canId;
    newVertex->start = NULL;

    return newVertex;
}


void insertListWidgetNode(WidgetNode_t** sPtr, FixedItem_e item, uint8_t byteH, uint8_t byteL, uint8_t bitH, uint8_t bitL)
{
    WidgetNode_t* newNode = (WidgetNode_t*)malloc(sizeof(WidgetNode_t));
    WidgetNode_t* iter = *sPtr;

    if (newNode != NULL) {
        newNode->no = item;
        newNode->byteH = byteH;
        newNode->byteL = byteL;
        newNode->bitH = bitH;
        newNode->bitL = bitL;
        newNode->next = NULL;

        if (iter == NULL) {
            *sPtr = newNode;
        } else {
            while (iter->next != NULL)
                iter = iter->next;
            iter->next = newNode;
        }
    } else {
        printf("May not inserted. No memory available.\n");
    }
}

void printListWidgetNode(WidgetNode_t* sPtr)
{
    WidgetNode_t* current = sPtr;

    while (current != NULL) {
        DEBUG_PRINT("  %d %d %d %d\n", current->byteL, current->byteH, current->bitL, current->bitH);
        current = current->next;
    }
}

size_t CanFmtNodeCnt(void)
{
    return gCount;
}

static void canFmtSort(DBARRAY self)
{
    size_t a, b;
    size_t size;
    CanbusFmt_t* pVertexA = NULL;
    CanbusFmt_t* pVertexB = NULL;

    size = lengthDbArray(self);
    for (a = 0; a < size; a++) {
        for (b = 0; b < a; b++) {
            getByIdxDbArray(self, a, (void**)&pVertexA);
            getByIdxDbArray(self, b, (void**)&pVertexB);
            if (pVertexB->canId > pVertexA->canId)
                swapDbArray(self, a, b);
        }
    }
}

void CanFmtInit(void)
{
    uint8_t items = 0;
    size_t i, len;
    uint32_t canId;
    char name[LEN_MAX] = {0};

    canCfgArray = createDbArray("canbus", canFmtPrint);

    canFmtIni = iniparser_load(CFG_PUBLIC_DRIVE ":/" CANFMT_FILENAME);
    if (!canFmtIni) {
        printf("iniparser_load error: %s\n", CANFMT_FILENAME);
        return;
    }

    items = sizeof(customItem)/ sizeof(CustomItemTable_t);
    for (i = 0; i < items; i++) {
        strcpy(name, customItem[i].item_string);
        strcat(name, ":");
        len = strlen(name);
        make_string(name, len, itemKey[IK_CANID].key_string);
        canId = iniparser_getint(canFmtIni, name, 0);

        find_canfmt(canId, customItem[i].item, name, len);
    }

    items = (canFmtIni->n / LINE_OF_ITEM) - items;
    for (i = 0; i < items; i++) {
        strcpy(name, CUSTOM_STRING);
        sprintf(name, "%s%d:", name, i);
        len = strlen(name);
        make_string(name, len, itemKey[IK_CANID].key_string);
        canId = iniparser_getint(canFmtIni, name, 0);

        find_canfmt(canId, FI_MAX + i, name, len);
    }
    gCount = FI_MAX + i;
    canDataArray = (uint32_t*)malloc(gCount * sizeof(uint32_t));
    if (!canDataArray) {
        printf("CanFmtInit: malloc failed for canDataArray (gCount=%zu)\n", gCount);
        return;
    }
    memset(canDataArray, 0, gCount * sizeof(uint32_t));
    printDbArray(canCfgArray);
    //canFmtSort(canCfgArray);
    //printDbArray(canCfgArray);

#if 0 // sample code
    appendDbArray(canCfgArray, (void*)createVertex(0x100));
    appendDbArray(canCfgArray, (void*)createVertex(0x101));
    appendDbArray(canCfgArray, (void*)createVertex(0x102));
    appendDbArray(canCfgArray, (void*)createVertex(0x103));
    appendDbArray(canCfgArray, (void*)createVertex(0x104));
    appendDbArray(canCfgArray, (void*)createVertex(0x105));

    printDbArray(canCfgArray);
    printf("==\n");
    getByIdDbArray(canCfgArray, (void**)&pVertex, canid_cmp, (void*)0x103);
    canFmtPrint((void*)pVertex);
    printf("==\n");
    getByIdxDbArray(canCfgArray, 1, (void**)&pVertex);
    canFmtPrint((void*)pVertex);

    insertListWidgetNode(&pVertex->start, 3, 4, 5, 6);
    insertListWidgetNode(&pVertex->start, 7, 8, 9, 0);

    printListWidgetNode(pVertex->start);

    uint8_t buf[8] = {0x12, 0x34, 0x56, 0x78, 0x21, 0x43, 0x65, 0x87};
    CanFmtParser(0x219, buf);
#endif
}

