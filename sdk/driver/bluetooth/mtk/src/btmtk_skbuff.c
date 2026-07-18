/******************************************************************************
 *
 *  Copyright (C) 2016 Realtek Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except compliance with the License.
 *  You may obtaa copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHWARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/******************************************************************************
*
*	Module Name:
*	    bt_list.c
*
*	Abstract:
*	    To implement list data structure
*
*	Major Change History:
*	      When             Who         What
*    	--------------------------------------------------------------
*	    2010-06-04         W.Bi       Created
*
*	Notes:
*
******************************************************************************/

#include "btmtk_skbuff.h"

#include <stdbool.h>



//****************************************************************************
// Structure
//****************************************************************************


//****************************************************************************
// FUNCTION
//****************************************************************************
//Initialize a list with its header
void ListInitializeHeader(PRT_LIST_HEAD ListHead)
{
    ListHead->Next = ListHead;
    ListHead->Prev = ListHead;
}

/**
    Tell whether the list is empty
    \param  ListHead          <RT_LIST_ENTRY>                 : List header of which to be test
*/
unsigned char ListIsEmpty(PRT_LIST_HEAD ListHead)
{
    return ListHead->Next == ListHead;
}

/*
    Insert a new entry between two known consecutive entries.
    This is only for internal list manipulation where we know the prev&next entries already
    @New : New element to be added
    @Prev: previous element the list
    @Next: Next element the list
*/
void
    ListAdd(
    PRT_LIST_ENTRY New,
    PRT_LIST_ENTRY Prev,
    PRT_LIST_ENTRY Next
    )
{
    Next->Prev = New;
    New->Next = Next;
    New->Prev = Prev;
    Prev->Next = New;
}
/**
    Add a new entry to the list.
    Insert a new entry after the specified head. This is good for implementing stacks.
    \param  ListNew            <RT_LIST_ENTRY>                 : new entry to be added
    \param  ListHead    <RT_LIST_ENTRY>                 : List header after which to add new entry
*/
void
ListAddToHead(
    PRT_LIST_ENTRY ListNew,
    PRT_LIST_HEAD ListHead
    )
{
    ListAdd(ListNew, ListHead, ListHead->Next);
}

/**
    Add a new entry to the list.
    Insert a new entry before the specified head. This is good for implementing queues.
    \param  ListNew            <RT_LIST_ENTRY>                 : new entry to be added
    \param  ListHead    <RT_LIST_ENTRY>                 : List header before which to add new entry
*/
void
ListAddToTail(
    PRT_LIST_ENTRY ListNew,
    PRT_LIST_HEAD ListHead
    )
{
    ListAdd(ListNew, ListHead->Prev, ListHead);
}

RT_LIST_ENTRY*
ListGetTop(
    PRT_LIST_HEAD ListHead
)
{

    if (ListIsEmpty(ListHead))
        return 0;

    return ListHead->Next;
}

RT_LIST_ENTRY*
ListGetTail(
    PRT_LIST_HEAD ListHead
)
{
    if (ListIsEmpty(ListHead))
        return 0;

    return ListHead->Prev;
}
/**
    Delete entry from the list
    Note: ListIsEmpty() on this list entry would not return true, since its state is undefined
    \param  ListToDelete     <RT_LIST_ENTRY>                 : list entry to be deleted
*/
void ListDeleteNode(PRT_LIST_ENTRY ListToDelete)
{
//    if (ListToDelete->Next != NULL && ListToDelete->Prev != NULL)
    {
        ListToDelete->Next->Prev = ListToDelete->Prev;
        ListToDelete->Prev->Next = ListToDelete->Next;
        ListToDelete->Next = ListToDelete->Prev = ListToDelete;
    }
}

#include <stdlib.h>
#include <errno.h>

#include "string.h"

//****************************************************************************
// CONSTANT DEFINITION
//****************************************************************************
#define DEFAULT_HEADER_SIZE    (8+4)

//MTK_BUFFER data buffer alignment
#define RTB_ALIGN   4

//do alignment with RTB_ALIGN
#define RTB_DATA_ALIGN(_Length)     ((_Length + (RTB_ALIGN - 1)) & (~(RTB_ALIGN - 1)))

//****************************************************************************
// STRUCTURE DEFINITION
//****************************************************************************
typedef struct _RTB_QUEUE_HEAD{
    RT_LIST_HEAD List;
    uint32_t  QueueLen;
    // osi_mutex_t Lock;
    uint8_t   Id[RTB_QUEUE_ID_LENGTH];
}RTB_QUEUE_HEAD, *PRTB_QUEUE_HEAD;

//****************************************************************************
// FUNCTION
//****************************************************************************
unsigned char
RtbQueueIsEmpty(
   RTB_QUEUE_HEAD* RtkQueueHead
)
{
    //return ListIsEmpty(&RtkQueueHead->List);
    return  RtkQueueHead->QueueLen > 0 ? false : true;
}

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
    )
{
    MTK_BUFFER* Rtb = NULL;
    ///Rtb buffer length:
    ///     MTK_BUFFER   48
    ///     HeadRoom      HeadRomm or 12
    ///     Length
    ///memory size: 48 + Length + 12(default) + 8*2(header for each memory) ---> a multiple of 8
    ///example:       (48 + 8)+ (300 + 12 + 8) = 372
    Rtb = malloc( sizeof(MTK_BUFFER) );
    if(Rtb)
    {
        uint32_t BufferLen = HeadRoom ? (Length + HeadRoom) : (Length + DEFAULT_HEADER_SIZE);
        BufferLen = RTB_DATA_ALIGN(BufferLen);
        Rtb->Head = malloc(BufferLen);
        if(Rtb->Head)
        {
            Rtb->HeadRoom = HeadRoom ? HeadRoom : DEFAULT_HEADER_SIZE;
            Rtb->data = Rtb->Head + Rtb->HeadRoom;
            Rtb->End = Rtb->data;
            Rtb->Tail = Rtb->End + Length;
            Rtb->len = 0;
            ListInitializeHeader(&Rtb->List);
            Rtb->RefCount = 1;
            return Rtb;
        }
    }

    if (Rtb)
    {
        if (Rtb->Head)
        {
            free(Rtb->Head);
        }

        free(Rtb);
    }
    return NULL;
}


/**
    Free specified MTK_BUFFER
    \param      RtkBuffer            <MTK_BUFFER*>        : buffer to free
*/
void
RtbFree(
    MTK_BUFFER* RtkBuffer
)
{
    if(RtkBuffer)
    {
        free(RtkBuffer->Head);
        free(RtkBuffer);
    }
    return;
}

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
    )
{

    if ((uint32_t)(RtkBuffer->data - RtkBuffer->Head) >= Length)
    {
        RtkBuffer->data -= Length;
        RtkBuffer->len += Length;
        RtkBuffer->HeadRoom -= Length;
        return RtkBuffer->data;
    }

    return NULL;
}
/**
    Remove a specified length data from the start of data buffer hold by specified MTK_BUFFER.
    This function returns the memory to the headroom.
    \param      RtkBuffer            <MTK_BUFFER*>        : data buffer to remove
    \param             Length                <uint32_t>                 : header length
    \return  Pointer to the next data the buffer is returned, usually useless
*/
unsigned char
RtbRemoveHead(
    MTK_BUFFER* RtkBuffer,
    uint32_t                 Length
    )
{

    if (RtkBuffer->len >= Length)
    {
        RtkBuffer->data += Length;
        RtkBuffer->len -= Length;
        RtkBuffer->HeadRoom += Length;
        return  true;
    }

    return false;
}

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
    )
{

    if ((uint32_t)(RtkBuffer->Tail - RtkBuffer->End) >= Length)
    {
        uint8_t* Tmp = RtkBuffer->End;
        RtkBuffer->End += Length;
        RtkBuffer->len += Length;
        return Tmp;
    }

    return NULL;
}

unsigned char
RtbRemoveTail(
    MTK_BUFFER * RtkBuffer,
        uint32_t       Length
)
{

    if ((uint32_t)(RtkBuffer->End - RtkBuffer->data) >= Length)
    {
        RtkBuffer->End -= Length;
        RtkBuffer->len -= Length;
        return true;
    }

    return false;
}
//****************************************************************************
// RTB list manipulation
//****************************************************************************
/**
    Initialize a rtb queue.
    \return  Initilized rtb queue if succeed, otherwise NULL
*/
RTB_QUEUE_HEAD*
RtbQueueInit(
)
{
    RTB_QUEUE_HEAD* RtbQueue = NULL;

    RtbQueue = malloc(sizeof(RTB_QUEUE_HEAD));
    if(RtbQueue)
    {
        // osi_mutex_new(&RtbQueue->Lock);
        ListInitializeHeader(&RtbQueue->List);
        RtbQueue->QueueLen = 0;
        return RtbQueue;
    }

    //error code comes here
    if (RtbQueue)
    {
        free(RtbQueue);
    }
    return NULL;

}

/**
    Free a rtb queue.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
*/
void
RtbQueueFree(
    RTB_QUEUE_HEAD* RtkQueueHead
    )
{
    if (RtkQueueHead)
    {
        RtbEmptyQueue(RtkQueueHead);
        // osi_mutex_free(&RtkQueueHead->Lock);
        free(RtkQueueHead);
    }
}

/**
    Queue specified RtkBuffer into a RtkQueue at list tail.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \param             RtkBuffer                <MTK_BUFFER*>                 : Rtk buffer to add
*/
void
RtbQueueTail(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 RtkBuffer
    )
{
    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);
    ListAddToTail(&RtkBuffer->List, &RtkQueueHead->List);
    RtkQueueHead->QueueLen++;
    // osi_mutex_unlock(&RtkQueueHead->Lock);
}

/**
    Queue specified RtkBuffer into a RtkQueue at list Head.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \param             RtkBuffer                <MTK_BUFFER*>                 : Rtk buffer to add
*/
void
RtbQueueHead(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 RtkBuffer
    )
{
    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);
    ListAddToHead(&RtkBuffer->List, &RtkQueueHead->List);
    RtkQueueHead->QueueLen++;
    // osi_mutex_unlock(&RtkQueueHead->Lock);
}


/**
    Insert new Rtkbuffer the old buffer
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \param             OldRtkBuffer                <MTK_BUFFER*>                 : old rtk buffer
    \param             NewRtkBuffer                <MTK_BUFFER*>                 : Rtk buffer to add
*/
void
RtbInsertBefore(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*  pOldRtkBuffer,
    MTK_BUFFER*  pNewRtkBuffer
)
{
    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);
    ListAdd(&pNewRtkBuffer->List, pOldRtkBuffer->List.Prev, &pOldRtkBuffer->List);
    RtkQueueHead->QueueLen++;
    // osi_mutex_unlock(&RtkQueueHead->Lock);
}

/**
    check whether the buffer is the last node the queue
*/
unsigned char
RtbNodeIsLast(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 pRtkBuffer
)
{
    MTK_BUFFER* pBuf;
    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);

    pBuf = (MTK_BUFFER*)RtkQueueHead->List.Prev;
    if(pBuf == pRtkBuffer)
    {
        // osi_mutex_unlock(&RtkQueueHead->Lock);
        return true;
    }
    // osi_mutex_unlock(&RtkQueueHead->Lock);
    return false;
}

/**
    get the next buffer node after the specified buffer the queue
    if the specified buffer is the last node the queue , return NULL
    \param      RtkBuffer        <MTK_BUFFER*>        : Rtk Queue
    \param      RtkBuffer        <MTK_BUFFER*>        : Rtk buffer
    \return node after the specified buffer
*/
MTK_BUFFER*
RtbQueueNextNode(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 pRtkBuffer
)
{
    MTK_BUFFER* pBuf;
    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);
    pBuf = (MTK_BUFFER*)RtkQueueHead->List.Prev;
    if(pBuf == pRtkBuffer)
    {
        // osi_mutex_unlock(&RtkQueueHead->Lock);
        return NULL;    ///< if it is already the last node the queue , return NULL
    }
    pBuf = (MTK_BUFFER*)pRtkBuffer->List.Next;
    // osi_mutex_unlock(&RtkQueueHead->Lock);
    return pBuf;    ///< return next node after this node
}

/**
    Delete specified RtkBuffer from a RtkQueue.
    It don't hold spinlock itself, so caller must hold it at someplace.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \param             RtkBuffer                <MTK_BUFFER*>                 : Rtk buffer to Remove
*/
void
RtbRemoveNode(
    RTB_QUEUE_HEAD* RtkQueueHead,
    MTK_BUFFER*                 RtkBuffer
)
{
    RtkQueueHead->QueueLen--;
    ListDeleteNode(&RtkBuffer->List);
}


/**
    Get the RtkBuffer which is the head of a RtkQueue
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \return head of the RtkQueue , otherwise NULL
*/
MTK_BUFFER*
RtbTopQueue(
    RTB_QUEUE_HEAD* RtkQueueHead
)
{
    MTK_BUFFER* Rtb = NULL;
    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);

    if (RtbQueueIsEmpty(RtkQueueHead))
    {
        // osi_mutex_unlock(&RtkQueueHead->Lock);
        return NULL;
    }

    Rtb = (MTK_BUFFER*)RtkQueueHead->List.Next;
    // osi_mutex_unlock(&RtkQueueHead->Lock);

    return Rtb;
}

/**
    Remove a RtkBuffer from specified rtkqueue at list tail.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \return    removed rtkbuffer if succeed, otherwise NULL
*/
MTK_BUFFER*
RtbDequeueTail(
    RTB_QUEUE_HEAD* RtkQueueHead
)
{
    MTK_BUFFER* Rtb = NULL;

    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);
    if (RtbQueueIsEmpty(RtkQueueHead))
    {
         // osi_mutex_unlock(&RtkQueueHead->Lock);
         return NULL;
    }
    Rtb = (MTK_BUFFER*)RtkQueueHead->List.Prev;
    RtbRemoveNode(RtkQueueHead, Rtb);
    // osi_mutex_unlock(&RtkQueueHead->Lock);

    return Rtb;
}

/**
    Remove a RtkBuffer from specified rtkqueue at list head.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \return    removed rtkbuffer if succeed, otherwise NULL
*/
MTK_BUFFER*
RtbDequeueHead(
    RTB_QUEUE_HEAD* RtkQueueHead
    )
{
    MTK_BUFFER* Rtb = NULL;
    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);

     if (RtbQueueIsEmpty(RtkQueueHead))
     {
         // osi_mutex_unlock(&RtkQueueHead->Lock);
         return NULL;
     }
    Rtb = (MTK_BUFFER*)RtkQueueHead->List.Next;
    RtbRemoveNode(RtkQueueHead, Rtb);
    // osi_mutex_unlock(&RtkQueueHead->Lock);
    return Rtb;
}

/**
    Get current rtb queue's length.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
    \return    current queue's length
*/
signed long RtbGetQueueLen(
    RTB_QUEUE_HEAD* RtkQueueHead
    )
{
    return RtkQueueHead->QueueLen;
}

/**
    Empty the rtkqueue.
    \param      RtkQueueHead        <RTB_QUEUE_HEAD*>        : Rtk Queue
*/
void
RtbEmptyQueue(
    RTB_QUEUE_HEAD* RtkQueueHead
    )
{
    MTK_BUFFER* Rtb = NULL;
    // osi_mutex_lock(&RtkQueueHead->Lock, OSI_MUTEX_MAX_TIMEOUT);

    while( !RtbQueueIsEmpty(RtkQueueHead))
    {
        Rtb = (MTK_BUFFER*)RtkQueueHead->List.Next;
        RtbRemoveNode(RtkQueueHead, Rtb);
        RtbFree(Rtb);
    }

    // osi_mutex_unlock(&RtkQueueHead->Lock);
    return;
}


///Annie_tmp
unsigned char
RtbCheckQueueLen(RTB_QUEUE_HEAD* RtkQueueHead, uint8_t Len)
{
    return RtkQueueHead->QueueLen < Len ? true : false;
}

/**
    clone buffer for upper or lower layer, because original buffer should be stored l2cap
    \param <MTK_BUFFER* pDataBuffer: original buffer
    \return cloned buffer
*/
MTK_BUFFER*
RtbCloneBuffer(
    MTK_BUFFER* pDataBuffer
)
{
    MTK_BUFFER* pNewBuffer = NULL;
    if(pDataBuffer)
    {
        pNewBuffer = RtbAllocate(pDataBuffer->len,0);
        if(!pNewBuffer)
        {
            return NULL;
        }
        if(pDataBuffer && pDataBuffer->data)
            memcpy(pNewBuffer->data, pDataBuffer->data, pDataBuffer->len);
        else
        {
            RtbFree(pNewBuffer);
            return NULL;
        }

        pNewBuffer->len = pDataBuffer->len;
    }
    return pNewBuffer;
}

uint8_t *skb_get_data(sk_buff *skb)
{
    return skb->data;
}

uint32_t skb_get_data_length(sk_buff *skb)
{
    return skb->len;
}

sk_buff *skb_alloc(unsigned int len)
{
    sk_buff *skb = (sk_buff *)RtbAllocate(len, 0);
    return skb;
}

void skb_free(sk_buff **skb)
{
    RtbFree(*skb);
    *skb = NULL;
    return;
}

void skb_unlink(sk_buff *skb, struct _RTB_QUEUE_HEAD *list)
{
    RtbRemoveNode(list, skb);
}

// increase the date length sk_buffer by len,
// and return the increased header pointer
uint8_t *skb_put(sk_buff *skb, uint32_t len)
{
    MTK_BUFFER *rtb = (MTK_BUFFER *)skb;

    return RtbAddTail(rtb, len);
}

// change skb->len to len
// !!! len should less than skb->len
void skb_trim(sk_buff *skb, unsigned int len)
{
    MTK_BUFFER *rtb = (MTK_BUFFER *)skb;
    uint32_t skb_len = skb_get_data_length(skb);

    RtbRemoveTail(rtb, (skb_len - len));
    return;
}

uint8_t skb_get_pkt_type(sk_buff *skb)
{
    return BT_CONTEXT(skb)->PacketType;
}

void skb_set_pkt_type(sk_buff *skb, uint8_t pkt_type)
{
    BT_CONTEXT(skb)->PacketType = pkt_type;
}

// decrease the data length sk_buffer by len,
// and move the content forward to the header.
// the data header will be removed.
void skb_pull(sk_buff *skb, uint32_t len)
{
    MTK_BUFFER *rtb = (MTK_BUFFER *)skb;
    RtbRemoveHead(rtb, len);
    return;
}

sk_buff *skb_copy(sk_buff *skb)
{
    sk_buff *n = skb_alloc(skb->len);

    if (NULL == n) {
        return NULL;
    }

    memcpy(skb_put(n, skb->len), skb->data, skb->len);
    skb_set_pkt_type(n, skb_get_pkt_type(skb));

    return n;
}

sk_buff *skb_alloc_and_init(uint8_t PktType, uint8_t *Data, uint32_t  DataLen)
{
    sk_buff *skb = skb_alloc(DataLen);

    if (NULL == skb) {
        return NULL;
    }

    memcpy(skb_put(skb, DataLen), Data, DataLen);
    skb_set_pkt_type(skb, PktType);

    return skb;
}

sk_buff *skb_alloc_and_init_append_type(uint8_t PktType, uint8_t *Data, uint32_t  DataLen)
{
    sk_buff *skb = skb_alloc(DataLen+1);

    if (NULL == skb) {
        return NULL;
    }

    uint8_t * buf = skb_put(skb, DataLen+1);

    *buf = PktType;
    memcpy(buf+1, Data, DataLen);

    skb_set_pkt_type(skb, PktType);

    return skb;
}

void skb_queue_head(RTB_QUEUE_HEAD *skb_head, MTK_BUFFER *skb)
{
    RtbQueueHead(skb_head, skb);
}

void skb_queue_tail(RTB_QUEUE_HEAD *skb_head, MTK_BUFFER *skb)
{
    RtbQueueTail(skb_head, skb);
}

MTK_BUFFER *skb_dequeue_head(RTB_QUEUE_HEAD *skb_head)
{
    return RtbDequeueHead(skb_head);
}

MTK_BUFFER *skb_dequeue_tail(RTB_QUEUE_HEAD *skb_head)
{
    return RtbDequeueTail(skb_head);
}

uint32_t skb_queue_get_length(RTB_QUEUE_HEAD *skb_head)
{
    return RtbGetQueueLen(skb_head);
}

#if 0

#include <stdio.h>
#include <string.h>

#include "btmtk_skbuff.h"

struct btmtk_sk_buff * btmtk_alloc_skb(unsigned int size)
{
    struct btmtk_sk_buff *skb = NULL;
    u8 *data = NULL;

    skb = malloc(sizeof(struct btmtk_sk_buff));
    if (!skb) {
	    printf("btmtk_alloc_skb alloc skb fail!\n");
		return skb;
	}
    memset(skb, 0, sizeof(struct btmtk_sk_buff));
	
	data = malloc(size);
	if (!data) {
		printf("btmtk_alloc_skb alloc data fail!\n");
		free(skb);
        skb = NULL;
		return skb;
	}
	skb->data = data;
	return skb;
}

void btmtk_free_skb(struct btmtk_sk_buff *skb)
{
    struct btmtk_sk_buff *skb = NULL;
    u8 *data = NULL;
	
	if (skb == NULL) {
		printf("btmtk_free_skb skb is NULL!\n");
		return;
	}
	
	if (skb->data != NULL) {
		free(skb->data);
	}
	
	free(skb);
}

struct btmtk_sk_buff * btmtk_skb_copy(struct btmtk_sk_buff *skb)
{
    struct btmtk_sk_buff *n = NULL;
    u8 *data = NULL;

    n = malloc(sizeof(struct btmtk_sk_buff));
    if (!n) {
	    printf("btmtk_skb_copy alloc skb fail!\n");
		return n;
	}
    memcpy(n, skb, sizeof(struct btmtk_sk_buff));
	
	data = malloc(skb->len);
	if (!data) {
		printf("btmtk_skb_copy alloc data fail!\n");
		free(skb);
        skb = NULL;
		return skb;
	}
	memcpy(data, skb->data, skb->len);
	n->data = data;
	return skb;
}

void btmtk_skb_pull(struct btmtk_sk_buff *skb, unsigned int size)
{
    u8 *data = NULL;
	
	data = malloc(skb->len - size);
	if (!data) {
		printf("btmtk_skb_pull alloc data fail!\n");
		return;
	}
	memcpy(data, skb->data + size, skb->len - size);
	free(skb->data);
	skb->data = data;
	skb->len -= size;
}

void btmtk_skb_queue_tail(struct btmtk_sk_buff_head *list, struct btmtk_sk_buff *skb)
{
}

void btmtk_skb_queue_head_init(struct btmtk_sk_buff_head *list)
{
	list->prev = list->next = (struct btmtk_sk_buff *)list;
	list->qlen = 0;
}

#endif
