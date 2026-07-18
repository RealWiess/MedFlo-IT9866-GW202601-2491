#ifndef MTK_BT_LIST_H
#define MTK_BT_LIST_H

/**
 List entry structure, could be header or node.

                  Prev<-----Header---->Next

Every List has an additional header, and list tail will be list header's previous node.
You can use list to form a queue or a stack data structure
queue:
    ListAddToTail----->LIST_FOR_EACH iterate--->manipulate on the list entry
Stack:
    ListAddToHead--- >LIST_FOR_EACH iterate--->manipulate on the list entry
*/

///RT list structure definition
typedef struct _RT_LIST_ENTRY {
    struct _RT_LIST_ENTRY *Next;   ///< Entry's next element
    struct _RT_LIST_ENTRY *Prev;   ///< Entry's previous element
} RT_LIST_ENTRY, *PRT_LIST_ENTRY;

///List head would be another name of list entry, and it points to the list header
typedef RT_LIST_ENTRY       RT_LIST_HEAD, *PRT_LIST_HEAD;

/*----------------------------------------------------------------------------------
    AL FUNCTION
----------------------------------------------------------------------------------*/

///Initialize a list with its header
void ListInitializeHeader(PRT_LIST_HEAD ListHead);

/**
    Add a new entry to the list.
    Insert a new entry after the specified head. This is good for implementing stacks.
    \param         ListNew     <RT_LIST_ENTRY>                 : new entry to be added
    \param  ListHead    <RT_LIST_ENTRY>                 : List header after which to add new entry
*/
void ListAddToHead(PRT_LIST_ENTRY ListNew, PRT_LIST_HEAD ListHead);

/**
    Add a new entry to the list.
    Insert a new entry before the specified head. This is good for implementing queues.
    \param         ListNew     <RT_LIST_ENTRY>                 : new entry to be added
    \param  ListHead    <RT_LIST_ENTRY>                 : List header before which to add new entry
*/
void ListAddToTail(PRT_LIST_ENTRY ListNew, PRT_LIST_HEAD ListHead);

/**
    Get entry in the head of the list
     \param [] ListHead    <RT_LIST_ENTRY>                 : List header
     \return entry in the head , otherwise NULL
*/
RT_LIST_ENTRY* ListGetTop(PRT_LIST_HEAD ListHead);

/**
    Get entry in the tail of the list
     \param [] ListHead    <RT_LIST_ENTRY>                 : List header
     \return entry in the tail , otherwise NULL
*/
RT_LIST_ENTRY*
ListGetTail(
    PRT_LIST_HEAD ListHead
);

/**
    Delete entry from the list
    Note: ListIsEmpty() on this list entry would not return true, since its state is undefined
    \param  ListToDelete     <RT_LIST_ENTRY>                 : list entry to be deleted
*/
void ListDeleteNode(PRT_LIST_ENTRY ListToDelete);

/**
    Tell whether the list is empty
    \param  ListHead          <RT_LIST_ENTRY>                 : List header of which to be test
*/
unsigned char ListIsEmpty(PRT_LIST_HEAD ListHead);

// void ListEmpty(PRT_LIST_HEAD ListHead);

void
    ListAdd(
    PRT_LIST_ENTRY New,
    PRT_LIST_ENTRY Prev,
    PRT_LIST_ENTRY Next
    );

/*----------------------------------------------------------------------------------
    MACRO
----------------------------------------------------------------------------------*/

/**
    Macros to iterate over the list.
    \param _Iter          : struct PRT_LIST_ENTRY type iterator to use as a loop cursor
    \param _ListHead   : List head of which to be iterated
*/
#define LIST_FOR_EACH(_Iter, _ListHead) \
        for ((_Iter) = (_ListHead)->Next; (_Iter) != (_ListHead); (_Iter) = (_Iter)->Next)

/**
    Macros to iterate over the list safely against removal of list entry.
    If you would delete any list entry from the list while iterating the list, should use this macro
    \param _Iter          : Struct PRT_LIST_ENTRY type iterator to use as a loop cursor
    \param _Temp       : Another Struct PRT_LIST_ENTRY type to use as a temporary storage
    \param _ListHead   : List head of which to be iterated
*/
#define LIST_FOR_EACH_SAFELY(_Iter, _Temp, _ListHead) \
        for ((_Iter) = (_ListHead)->Next, (_Temp) = (_Iter)->Next; (_Iter) != (_ListHead);  \
               (_Iter) = (_Temp), (_Temp) = (_Iter)->Next)

/**
    Macros to get the struct pointer of this list entry
    You could make every RT_LIST_ENTRY at the first place of your structure to avoid the macro, which will be dangerouse.
    Copy from winnt.h.
    BUG:if offset of field in type larger than 32 bit interger, which is not likely to happen, it will error
    \param _Ptr               : Struct RT_LIST_ENTRY type pointer
    \param _Type            : The type of structure in which the RT_LIST_ENTRY embedded in
    \param _Field            : the name of the RT_LIST_ENTRY within the struct
*/
#define LIST_ENTRY(_Ptr, _Type, _Field) ((_Type *)((char *)(_Ptr)-(unsigned long)(&((_Type *)0)->_Field)))

#endif /*MTK_BT_LIST_H*/

/******************************************************************************
 *
 *  Copyright (C) 2016 Realtek Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHWARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/******************************************************************************
*
*	Module Name:
*	    bt_skbuff.h
*
*	Abstract:
*	    Data buffer managerment through whole bluetooth stack.
*
*	Major Change History:
*	      When             Who       What
*	    --------------------------------------------------------------
*	    2010-06-11       W.Bi     Created.
*
*	Notes:
*	      To reduce memory copy when pass data buffer to other layers,
*       MTK_BUFFER is designed referring to linux socket buffer.
*       But I still wonder its effect, since MTK_BUFFER is much bigger
*       than original data buffer.MTK_BUFFER will reduce its member if
*       it would not reach what i had expected.
*
******************************************************************************/


#ifndef MTK_BT_SKBUFF_H
#define MTK_BT_SKBUFF_H

#include <stdint.h>

/*----------------------------------------------------------------------------------
    CONSTANT DEFINITION
----------------------------------------------------------------------------------*/
#define RTK_CONTEXT_SIZE 12

#define RTB_QUEUE_ID_LENGTH          64

/*----------------------------------------------------------------------------------
    STRUCTURE DEFINITION
----------------------------------------------------------------------------------*/
/**
    Rtk buffer definition
      Head -->|<---Data--->|<-----Length------>| <---End
                     _________________________________
                    |_____________|___________________|
                    |<-headroom->|<--RealDataBuffer-->|

    Compared to socket buffer, there exists no tail and end pointer and tailroom as tail is rarely used in bluetooth stack
    \param List             : List structure used to list same type rtk buffer and manipulate rtk buffer like list.
    \param Head           : Pointer to truely allocated data buffer. It point to the headroom
    \param Data           : Pointer to real data buffer.
    \param Length        : currently data length
    \param HeadRoom  : Record initialize headroom size.
    \param RefCount    : Reference count. zero means able to be freed, otherwise somebody is handling it.
    \param Priv            : Reserved for multi-device support. Record Hci pointer which will handles this packet
    \param Contest      : Control buffer, put private variables here.
*/
typedef struct _MTK_BUFFER
{
    RT_LIST_ENTRY List;
    uint8_t *Head;
    uint8_t *data;
    uint8_t *Tail;
    uint8_t *End;
    uint32_t len;
    uint32_t HeadRoom;
//    RT_U16 TailRoom;
    signed char   RefCount;

    void* Priv;
    uint8_t Context[RTK_CONTEXT_SIZE];
}MTK_BUFFER, *PMTK_BUFFER;

typedef MTK_BUFFER sk_buff;

/**
    MTK_BUFFER Control Buffer Context
    \param  PacketType      : HCI data types, Command/Acl/...
    \param  LastFrag          : Is Current Acl buffer the last fragment.(0 for no, 1 for yes)
    \param  TxSeq             : Current packet tx sequence
    \param  Retries            : Current packet retransmission times
    \param  Sar                 : L2cap control field segmentation and reassembly bits
*/
struct BT_RTB_CONTEXT{
    uint8_t   PacketType;
    uint16_t Handle;
};

///definition to get MTK_BUFFER's control buffer context pointer
#define BT_CONTEXT(_Rtb) ((struct BT_RTB_CONTEXT *)((_Rtb)->Context))

/**
    Since RTBs are always used into/from list, so abstract this struct and provide APIs to easy process on RTBs
*/
typedef struct _RTB_QUEUE_HEAD  RTB_QUEUE_HEAD;
/*----------------------------------------------------------------------------------
    AL FUNCTION
----------------------------------------------------------------------------------*/
/**
    Allocate a MTK_BUFFER with specified data length and reserved headroom.
    If caller does not know actual headroom to reserve for further usage, specify it to zero to use default value.
    \param      Length            <uint32_t>        : current data buffer length to allcated
    \param      HeadRoom     <uint32_t>         : if caller knows reserved head space, set it; otherwise set 0 to use default value
    \return pointer to MTK_BUFFER if succeed, null otherwise
*/
MTK_BUFFER*
RtbAllocate(
    uint32_t Length,
    uint32_t HeadRoom
    );

/**
    Free specified MTK_BUFFER
    \param      RtkBuffer            <MTK_BUFFER*>        : buffer to free
*/
void
RtbFree(
    MTK_BUFFER* RtkBuffer
    );

/**
    increament reference count
*/
void
RtbIncreaseRefCount(
    MTK_BUFFER* RtkBuffer
);

/**
    Recycle a MTK_BUFFER after its usage if specified rtb could
    if rtb total length is not smaller than specified rtbsize to be recycled for, it will succeeded recycling
    \param      RtkBuffer            <MTK_BUFFER*>        : buffer to recycle
    \param              RtbSize              <uint32_t>                 : size of buffer to be recycled for
*/
/*
BOOLEAN
RtbCheckRecycle(
    MTK_BUFFER* RtkBuffer,
    uint32_t   RtbSize
    );
*/
/**
    Add a specified length protocal header to the start of data buffer hold by specified MTK_BUFFER.
    This function extends used data area of the buffer at the buffer start.
    \param      RtkBuffer            <MTK_BUFFER*>        : data buffer to add
    \param             Length                <uint32_t>                 : header length
    \return  Pointer to the first byte of the extra data is returned
*/
uint8_t*
RtbAddHead(
    MTK_BUFFER* RtkBuffer,
    uint32_t                 Length
    );

/**
    Remove a specified length data from the start of data buffer hold by specified MTK_BUFFER.
    This function returns the memory to the headroom.
    \param      RtkBuffer            <MTK_BUFFER*>        : data buffer to remove
    \param             Length                <uint32_t>                 : header length
    \return  Pointer to the next data in the buffer is returned, usually useless
*/
unsigned char
RtbRemoveHead(
    MTK_BUFFER* RtkBuffer,
    uint32_t                 Length
    );

/**
    Add a specified length protocal header to the end of data buffer hold by specified MTK_BUFFER.
    This function extends used data area of the buffer at the buffer end.
    \param      RtkBuffer            <MTK_BUFFER*>        : data buffer to add
    \param             Length                <uint32_t>                 : header length
    \return  Pointer to the first byte of the extra data is returned
*/
 uint8_t*
RtbAddTail(
    MTK_BUFFER* RtkBuffer,
    uint32_t                 Length
    );

/**
    Remove a specified length data from the end of data buffer hold by specified MTK_BUFFER.
*/
 unsigned char
RtbRemoveTail(
    MTK_BUFFER * RtkBuffer,
        uint32_t       Length
);

/**
    Initialize a rtb queue.
    \return  Initilized rtb queue if succeed, otherwise NULL
*/
 RTB_QUEUE_HEAD*
RtbQueueInit(
    );

/**
    Free a rtb queue.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
*/
 void
RtbQueueFree(
    RTB_QUEUE_HEAD* RtkQueueHead
    );
/**
    Queue specified RtkBuffer into a RtkQueue at list tail.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \param             RtkBuffer                <MTK_BUFFER*>                 : Rtk buffer to add
*/
 void
RtbQueueTail(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 RtkBuffer
    );

/**
    Queue specified RtkBuffer into a RtkQueue at list Head.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \param             RtkBuffer                <MTK_BUFFER*>                 : Rtk buffer to add
*/
 void
RtbQueueHead(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 RtkBuffer
    );

/**
    Remove a RtkBuffer from specified rtkqueue at list tail.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \return    removed rtkbuffer if succeed, otherwise NULL
*/
 MTK_BUFFER*
RtbDequeueTail(
    RTB_QUEUE_HEAD* RtkQueueHead
    );

/**
    Remove a RtkBuffer from specified rtkqueue at list head.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \return    removed rtkbuffer if succeed, otherwise NULL
*/
 MTK_BUFFER*
RtbDequeueHead(
    RTB_QUEUE_HEAD* RtkQueueHead
    );

/**
    Get current rtb queue's length.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \return    current queue's length
*/
 signed long
RtbGetQueueLen(
    RTB_QUEUE_HEAD* RtkQueueHead
    );

/**
    Empty the rtkqueue.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
*/
 void
RtbEmptyQueue(
    RTB_QUEUE_HEAD* RtkQueueHead
    );

/**
    Get the RtkBuffer which is the head of a RtkQueue
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \return head of the RtkQueue , otherwise NULL
*/
 MTK_BUFFER*
RtbTopQueue(
    RTB_QUEUE_HEAD* RtkQueueHead
);

/**
    Insert new Rtkbuffer in the old buffer
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \param             OldRtkBuffer                <MTK_BUFFER*>                 : old rtk buffer
    \param             NewRtkBuffer                <MTK_BUFFER*>                 : Rtk buffer to add
*/
 void
RtbInsertBefore(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER* pOldRtkBuffer,
    MTK_BUFFER* pNewRtkBuffer
);

/**
    check whether the buffer is the last node in the queue
*/
 unsigned char
RtbNodeIsLast(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 pRtkBuffer
);

/**
    get the next buffer node after the specified buffer in the queue
    if the specified buffer is the last node in the queue , return NULL
    \param      RtkBuffer        <MTK_BUFFER*>        : Rtk Queue
    \param      RtkBuffer        <MTK_BUFFER*>        : Rtk buffer
    \return node after the specified buffer
*/
 MTK_BUFFER*
RtbQueueNextNode(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 pRtkBuffer
);

/**
    check whether queue is empty
*/
/* BOOLEAN
RtbQueueIsEmpty(
   RTB_QUEUE_HEAD* RtkQueueHead
);
*/

//annie_tmp
 unsigned char
RtbCheckQueueLen(
   RTB_QUEUE_HEAD* RtkQueueHead,
   uint8_t Len
);

 void
RtbRemoveNode(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*         RtkBuffer
);

 MTK_BUFFER*
    RtbCloneBuffer(
    MTK_BUFFER* pDataBuffer
    );

#endif /*MTK_BT_SKBUFF_H*/

void skb_queue_head(RTB_QUEUE_HEAD *skb_head, MTK_BUFFER *skb);

void skb_queue_tail(RTB_QUEUE_HEAD *skb_head, MTK_BUFFER *skb);

MTK_BUFFER *skb_dequeue_head(RTB_QUEUE_HEAD *skb_head);

MTK_BUFFER *skb_dequeue_tail(RTB_QUEUE_HEAD *skb_head);

uint32_t skb_queue_get_length(RTB_QUEUE_HEAD *skb_head);

void skb_pull(sk_buff *skb, uint32_t len);

void skb_unlink(sk_buff *skb, struct _RTB_QUEUE_HEAD *list);

// increase the date length sk_buffer by len,
// and return the increased header pointer
uint8_t *skb_put(sk_buff *skb, uint32_t len);

sk_buff *skb_copy(sk_buff *skb);

void skb_set_pkt_type(sk_buff *skb, uint8_t pkt_type);
uint32_t skb_get_data_length(sk_buff *skb);
void skb_free(sk_buff **skb);

sk_buff *skb_alloc_and_init(uint8_t PktType, uint8_t *Data, uint32_t  DataLen);

uint8_t skb_get_pkt_type(sk_buff *skb);

#if 0

#ifndef _BTMTK_SKBUFF_H_
#define _BTMTK_SKBUFF_H_

typedef struct _RT_LIST_ENTRY {
    struct _RT_LIST_ENTRY *Next;   ///< Entry's next element
    struct _RT_LIST_ENTRY *Prev;   ///< Entry's previous element
} RT_LIST_ENTRY, *PRT_LIST_ENTRY;

struct btmtk_sk_buff {
	struct btmtk_sk_buff *next, *prev;
    u8 *data;
    unsigned int len;
	u8 pkt_type;
};

struct btmtk_sk_buff_head {
	struct btmtk_sk_buff *next, *prev;
	unsigned int qlen;
};

struct btmtk_sk_buff * btmtk_alloc_skb(unsigned int size);
void btmtk_free_skb(struct btmtk_sk_buff *skb);
struct btmtk_sk_buff * btmtk_skb_copy(struct btmtk_sk_buff *skb);

#endif

#endif