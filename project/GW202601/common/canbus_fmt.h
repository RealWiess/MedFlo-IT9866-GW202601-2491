#ifndef CANBUS_FMT_H
#define CANBUS_FMT_H

typedef enum _FixedItem {
    FI_SPEED    = 0,
    FI_ODO,
    FI_GEAR = 2,
    FI_MAX,
} FixedItem_e;

typedef enum _ItemKey {
    IK_CANID,
    IK_BYTE_H,
    IK_BYTE_L,
    IK_BIT_H,
    IK_BIT_L,
} ItemKey_e;

typedef struct _WidgetNode {
    FixedItem_e no;
    char byteH;
    char byteL;
    char bitH;
    char bitL;
    int len;
    struct WidgetNode_t* next;
} WidgetNode_t;

typedef struct _CanbusFmt {
    uint32_t canId;
    WidgetNode_t* start;
} CanbusFmt_t;

void CanFmtInit(void);
uint32_t** CanFmtParser(uint32_t canId, uint8_t* buf);
size_t CanFmtNodeCnt(void);

#endif
