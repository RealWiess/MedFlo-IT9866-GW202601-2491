/********************************************************************************
**                                                                              *
**  Name        gki_buffer.c                                                    *
**                                                                              *
**  Function    this file contains GKI buffer handling functions                *
**                                                                              *
**                                                                              *
**  Copyright (c) 1999-2009, Broadcom Corp., All Rights Reserved.               *
**  Proprietary and confidential.                                               *
**                                                                              *
*********************************************************************************/
#include "gki_int.h"
#include <stdio.h>
#if (GKI_NUM_TOTAL_BUF_POOLS > 16)
#error Number of pools out of range (16 Max)!
#endif

#if (!defined(BTU_STACK_LITE_ENABLED) || BTU_STACK_LITE_ENABLED == FALSE)
static void gki_add_to_pool_list(UINT8 pool_id);
static void gki_remove_from_pool_list(UINT8 pool_id);
#endif /*  BTU_STACK_LITE_ENABLED == FALSE */

/*******************************************************************************
**
** Function         gki_init_free_queue
**
** Description      Internal function called at startup to initialize a free
**                  queue. It is called once for each free queue.
**
** Returns          void
**
*******************************************************************************/
static void gki_init_free_queue (UINT8 id, UINT16 size, UINT16 total, void *p_mem)
{
    UINT16           i;
    UINT16           act_size;
    BUFFER_HDR_T    *hdr;
    BUFFER_HDR_T    *hdr1 = NULL;
    UINT32          *magic;
    INT32            tempsize = size;
    tGKI_COM_CB     *p_cb = &gki_cb.com;

    /* Ensure an even number of longwords */
    tempsize = (INT32)ALIGN_POOL(size);
    act_size = (UINT16)(tempsize + BUFFER_PADDING_SIZE);

    /* Remember pool start and end addresses */
    p_cb->pool_start[id] = (UINT8 *)p_mem;
    p_cb->pool_end[id]   = (UINT8 *)p_mem + (act_size * total);
    p_cb->pool_size[id]  = act_size;

    p_cb->freeq[id].size      = (UINT16) tempsize;
    p_cb->freeq[id].total     = total;
    p_cb->freeq[id].cur_cnt   = 0;
    p_cb->freeq[id].max_cnt   = 0;

    /* Initialize  index table */
    hdr = (BUFFER_HDR_T *)p_mem;
    p_cb->freeq[id].p_first = hdr;

    for (i = 0; i < total; i++)
    {
        hdr->task_id = GKI_INVALID_TASK;
        hdr->q_id    = id;
        hdr->status  = BUF_STATUS_FREE;
        magic        = (UINT32 *)((UINT8 *)hdr + BUFFER_HDR_SIZE + tempsize);
        *magic       = MAGIC_NO;
        hdr1         = hdr;
        hdr          = (BUFFER_HDR_T *)((UINT8 *)hdr + act_size);
        hdr1->p_next = hdr;
    }

    hdr1->p_next = NULL;
    p_cb->freeq[id].p_last = hdr1;

    return;
}


/*******************************************************************************
**
** Function         gki_buffer_init
**
** Description      Called once internally by GKI at startup to initialize all
**                  buffers and free buffer pools.
**
** Returns          void
**
*******************************************************************************/
void gki_buffer_init(void)
{
    UINT8   i, tt, mb;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    /* Initialize mailboxes */
    for (tt = 0; tt < GKI_MAX_TASKS; tt++)
    {
        for (mb = 0; mb < NUM_TASK_MBOX; mb++)
        {
            p_cb->OSTaskQFirst[tt][mb] = NULL;
            p_cb->OSTaskQLast [tt][mb] = NULL;
        }
    }

    for (tt = 0; tt < GKI_NUM_TOTAL_BUF_POOLS; tt++)
    {
        p_cb->pool_start[tt] = NULL;
        p_cb->pool_end[tt]   = NULL;
        p_cb->pool_size[tt]  = 0;

        p_cb->freeq[tt].p_first = 0;
        p_cb->freeq[tt].p_last  = 0;
        p_cb->freeq[tt].size    = 0;
        p_cb->freeq[tt].total   = 0;
        p_cb->freeq[tt].cur_cnt = 0;
        p_cb->freeq[tt].max_cnt = 0;
    }

    /* Use default from target.h */
    p_cb->pool_access_mask = GKI_DEF_BUFPOOL_PERM_MASK;

#if (GKI_USE_DYNAMIC_BUFFERS == TRUE)

#if (GKI_NUM_FIXED_BUF_POOLS > 0)
    p_cb->bufpool0 = (UINT8 *)GKI_os_malloc ((GKI_BUF0_SIZE + BUFFER_PADDING_SIZE) * GKI_BUF0_MAX);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 1)
    p_cb->bufpool1 = (UINT8 *)GKI_os_malloc ((GKI_BUF1_SIZE + BUFFER_PADDING_SIZE) * GKI_BUF1_MAX);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 2)
    p_cb->bufpool2 = (UINT8 *)GKI_os_malloc ((GKI_BUF2_SIZE + BUFFER_PADDING_SIZE) * GKI_BUF2_MAX);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 3)
    p_cb->bufpool3 = (UINT8 *)GKI_os_malloc ((GKI_BUF3_SIZE + BUFFER_PADDING_SIZE) * GKI_BUF3_MAX);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 4)
    p_cb->bufpool4 = (UINT8 *)GKI_os_malloc ((GKI_BUF4_SIZE + BUFFER_PADDING_SIZE) * GKI_BUF4_MAX);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 5)
    p_cb->bufpool5 = (UINT8 *)GKI_os_malloc ((GKI_BUF5_SIZE + BUFFER_PADDING_SIZE) * GKI_BUF5_MAX);
#endif

#endif


#if (GKI_NUM_FIXED_BUF_POOLS > 0)
    gki_init_free_queue(0, GKI_BUF0_SIZE, GKI_BUF0_MAX, p_cb->bufpool0);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 1)
    gki_init_free_queue(1, GKI_BUF1_SIZE, GKI_BUF1_MAX, p_cb->bufpool1);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 2)
    gki_init_free_queue(2, GKI_BUF2_SIZE, GKI_BUF2_MAX, p_cb->bufpool2);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 3)
    gki_init_free_queue(3, GKI_BUF3_SIZE, GKI_BUF3_MAX, p_cb->bufpool3);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 4)
    gki_init_free_queue(4, GKI_BUF4_SIZE, GKI_BUF4_MAX, p_cb->bufpool4);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 5)
        gki_init_free_queue(5, GKI_BUF5_SIZE, GKI_BUF5_MAX, p_cb->bufpool5);
#endif

    /* add pools to the pool_list which is arranged in the order of size */
    for(i=0; i < GKI_NUM_FIXED_BUF_POOLS ; i++)
    {
        p_cb->pool_list[i] = i;
    }

    p_cb->curr_total_no_of_pools = GKI_NUM_FIXED_BUF_POOLS;

    return;
}

void gki_buffer_deinit(void)
{
    tGKI_COM_CB *p_cb = &gki_cb.com;

    /* Free all malloced buffers*/
#if (GKI_USE_DYNAMIC_BUFFERS == TRUE)

#if (GKI_NUM_FIXED_BUF_POOLS > 0)
     GKI_os_free (p_cb->bufpool0);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 1)
    GKI_os_free (p_cb->bufpool1);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 2)
    GKI_os_free (p_cb->bufpool2);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 3)
    GKI_os_free (p_cb->bufpool3);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 4)
    GKI_os_free (p_cb->bufpool4);
#endif

#if (GKI_NUM_FIXED_BUF_POOLS > 5)
    GKI_os_free (p_cb->bufpool5);
#endif

#endif
}

/*******************************************************************************
**
** Function         GKI_init_q
**
** Description      Called by an application to initialize a buffer queue.
**
** Returns          void
**
*******************************************************************************/
void GKI_init_q (BUFFER_Q *p_q)
{
    p_q->p_first = p_q->p_last = NULL;
    p_q->count = 0;

    return;
}

#if ((defined GKI_BUFFER_DEBUG_EXTEND) && (GKI_BUFFER_DEBUG_EXTEND == TRUE))
/*******************************************************************************
**
** Function         GKI_getbuf_T
**
** Description      Called by an application to get a free buffer which
**                  is of size greater or equal to the requested size.
**
**                  Note: This routine only takes buffers from public pools.
**                        It will not use any buffers from pools
**                        marked GKI_RESTRICTED_POOL.
**
** Parameters       size - (input) number of bytes needed.
**
** Returns          A pointer to the buffer, or NULL if none available
**
*******************************************************************************/
void *GKI_getbuf_T (UINT16 size, char const * function, int line)
{
    UINT8         i;
    FREE_QUEUE_T  *Q;
    BUFFER_HDR_T  *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    if (size == 0)
    {
        GKI_exception (GKI_ERROR_BUF_SIZE_ZERO, "getbuf: Size is zero");
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return (NULL);
    }

    /* Find the first buffer pool that is public that can hold the desired size */
    for (i=0; i < p_cb->curr_total_no_of_pools; i++)
    {
        if ( size <= p_cb->freeq[p_cb->pool_list[i]].size )
            break;
    }

    if(i == p_cb->curr_total_no_of_pools)
    {
        char buf[56];
        sprintf(buf, "%s to getbuf: Size is too big, buffer need size:%d ", function, size);
        GKI_exception (GKI_ERROR_BUF_SIZE_TOOBIG, buf);
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return (NULL);
    }

    //GKI_TRACE_3("%s %d try to alloc buffer: %d", function, line, size);

    /* Make sure the buffers aren't disturbed til finished with allocation */
    GKI_disable();

    /* search the public buffer pools that are big enough to hold the size
     * until a free buffer is found */
    for ( ; i < p_cb->curr_total_no_of_pools; i++)
    {
        /* Only look at PUBLIC buffer pools (bypass RESTRICTED pools) */
        if (((UINT16)1 << p_cb->pool_list[i]) & p_cb->pool_access_mask)
            continue;

        Q = &p_cb->freeq[p_cb->pool_list[i]];
        if ((Q->cur_cnt < Q->total) && (Q->p_first))
        {
            p_hdr = Q->p_first;
            Q->p_first = p_hdr->p_next;

            if (!Q->p_first)
                Q->p_last = NULL;

            if(++Q->cur_cnt > Q->max_cnt)
                Q->max_cnt = Q->cur_cnt;

            GKI_enable();

            p_hdr->task_id = GKI_get_taskid();

            p_hdr->status  = BUF_STATUS_UNLINKED;
            p_hdr->p_next  = NULL;
            p_hdr->Type    = 0;
            p_hdr->pTaskName = (UINT8*)gki_cb.com.OSTName[GKI_get_taskid()];
            strcpy((char*)&p_hdr->FuncName[0], function);
            p_hdr->line = line;
            return ((void *) ((UINT8 *)p_hdr + BUFFER_HDR_SIZE));
        }
    }

    GKI_enable();

    GKI_TRACE_2("%s line:%d can't alloc buffer", function, line);

    return (NULL);
}


/*******************************************************************************
**
** Function         GKI_getpoolbuf_T
**
** Description      Called by an application to get a free buffer from
**                  a specific buffer pool.
**
**                  Note: If there are no more buffers available from the pool,
**                        the public buffers are searched for an available buffer.
**
** Parameters       pool_id - (input) pool ID to get a buffer out of.
**
** Returns          A pointer to the buffer, or NULL if none available
**
*******************************************************************************/
void *GKI_getpoolbuf_T (UINT8 pool_id, char const * function, int line)
{
    FREE_QUEUE_T  *Q;
    BUFFER_HDR_T  *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
    {
        GKI_exception(GKI_ERROR_GETPOOLBUF_BAD_QID, "getpoolbuf bad pool");
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return (NULL);
    }

//    GKI_TRACE_3("%s %d try to alloc pool_id: %d", function, line, pool_id);

    /* Make sure the buffers aren't disturbed til finished with allocation */
    GKI_disable();

    Q = &p_cb->freeq[pool_id];
    if ((Q->cur_cnt < Q->total) && (Q->p_first))
    {
        p_hdr = Q->p_first;
        Q->p_first = p_hdr->p_next;

        if (!Q->p_first)
            Q->p_last = NULL;

        if(++Q->cur_cnt > Q->max_cnt)
            Q->max_cnt = Q->cur_cnt;

        GKI_enable();


        p_hdr->task_id = GKI_get_taskid();

        p_hdr->status  = BUF_STATUS_UNLINKED;
        p_hdr->p_next  = NULL;
        p_hdr->Type    = 0;
        strcpy((char*)&p_hdr->FuncName[0], function);
        p_hdr->line = line;

        return ((void *) ((UINT8 *)p_hdr + BUFFER_HDR_SIZE));
    }

    /* If here, no buffers in the specified pool */
    GKI_enable();

    /*when we can't get buffer pool, we don't need to indicate, because we will try to get buffer from public pools,
      if we can't get the buffer, we need post logs in the GKI_getbuf.*/
    //GKI_TRACE_6("%s line:%d can't alloc buffer pool,size:%d, cct:%d, tct:%d, pfirst:0x%02x", function, line, p_cb->freeq[pool_id].size, Q->cur_cnt, Q->total, Q->p_first);

    /* try for allocate free buffers in public pools */
    return (GKI_getbuf(p_cb->freeq[pool_id].size));

}

/*******************************************************************************
**
** Function         GKI_freebuf_T
**
** Description      Called by an application to return a buffer to the free pool.
**
** Parameters       p_buf - (input) address of the beginning of a buffer.
**
** Returns          void
**
*******************************************************************************/
void GKI_freebuf_T (void *p_buf, char const * function, int line)
{
    FREE_QUEUE_T    *Q;
    BUFFER_HDR_T    *p_hdr;

#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
//    GKI_TRACE_2("%s %d try to free buffer", function, line);

    if(!p_buf)
    {

        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Free - Buf pointer NULL");
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return;
    }

    if (gki_chk_buf_damage(p_buf))
    {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Free - Buf Corrupted");
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return;
    }
#endif

    p_hdr = (BUFFER_HDR_T *) ((UINT8 *)p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED)
    {
        GKI_exception(GKI_ERROR_FREEBUF_BUF_LINKED, "Freeing Linked Buf");
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return;
    }

    if (p_hdr->q_id >= GKI_NUM_TOTAL_BUF_POOLS)
    {
        GKI_exception(GKI_ERROR_FREEBUF_BAD_QID, "Bad Buf QId");
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return;
    }

    GKI_disable();

//    GKI_TRACE_2("Freed the %s %d alloc buffer", p_hdr->FuncName, p_hdr->line);

    /*
    ** Release the buffer
    */
    Q  = &gki_cb.com.freeq[p_hdr->q_id];
    if (Q->p_last)
        Q->p_last->p_next = p_hdr;
    else
        Q->p_first = p_hdr;

    Q->p_last      = p_hdr;
    p_hdr->p_next  = NULL;
    p_hdr->status  = BUF_STATUS_FREE;
    p_hdr->task_id = GKI_INVALID_TASK;
    if (Q->cur_cnt > 0)
        Q->cur_cnt--;

    GKI_enable();

    return;
}

#else
/*******************************************************************************
**
** Function         GKI_getbuf
**
** Description      Called by an application to get a free buffer which
**                  is of size greater or equal to the requested size.
**
**                  Note: This routine only takes buffers from public pools.
**                        It will not use any buffers from pools
**                        marked GKI_RESTRICTED_POOL.
**
** Parameters       size - (input) number of bytes needed.
**
** Returns          A pointer to the buffer, or NULL if none available
**
*******************************************************************************/
void *GKI_getbuf (UINT16 size)
{
    UINT8         i;
    FREE_QUEUE_T  *Q;
    BUFFER_HDR_T  *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    if (size == 0)
    {
        GKI_exception (GKI_ERROR_BUF_SIZE_ZERO, "getbuf: Size is zero");
        return (NULL);
    }

    /* Find the first buffer pool that is public that can hold the desired size */
    for (i=0; i < p_cb->curr_total_no_of_pools; i++)
    {
        if ( size <= p_cb->freeq[p_cb->pool_list[i]].size )
            break;
    }

    if(i == p_cb->curr_total_no_of_pools)
    {
        char buf[56];
        sprintf(buf, "getbuf: Size is too big, buffer need size:%d ", size);
        GKI_exception (GKI_ERROR_BUF_SIZE_TOOBIG, buf);
        return (NULL);
    }

    /* Make sure the buffers aren't disturbed til finished with allocation */
    GKI_disable();

    /* search the public buffer pools that are big enough to hold the size
     * until a free buffer is found */
    for ( ; i < p_cb->curr_total_no_of_pools; i++)
    {
        /* Only look at PUBLIC buffer pools (bypass RESTRICTED pools) */
        if (((UINT16)1 << p_cb->pool_list[i]) & p_cb->pool_access_mask)
            continue;

        Q = &p_cb->freeq[p_cb->pool_list[i]];
        if ((Q->cur_cnt < Q->total) && (Q->p_first))
        {
            p_hdr = Q->p_first;
            Q->p_first = p_hdr->p_next;

            if (!Q->p_first)
                Q->p_last = NULL;

            if(++Q->cur_cnt > Q->max_cnt)
                Q->max_cnt = Q->cur_cnt;

            GKI_enable();

            p_hdr->task_id = GKI_get_taskid();

            p_hdr->status  = BUF_STATUS_UNLINKED;
            p_hdr->p_next  = NULL;
            p_hdr->Type    = 0;

            return ((void *) ((UINT8 *)p_hdr + BUFFER_HDR_SIZE));
        }
    }

    GKI_enable();

//    GKI_exception (GKI_ERROR_OUT_OF_BUFFERS, "getbuf: out of buffers");
    return (NULL);
}


/*******************************************************************************
**
** Function         GKI_getpoolbuf
**
** Description      Called by an application to get a free buffer from
**                  a specific buffer pool.
**
**                  Note: If there are no more buffers available from the pool,
**                        the public buffers are searched for an available buffer.
**
** Parameters       pool_id - (input) pool ID to get a buffer out of.
**
** Returns          A pointer to the buffer, or NULL if none available
**
*******************************************************************************/
void *GKI_getpoolbuf (UINT8 pool_id)
{
    FREE_QUEUE_T  *Q;
    BUFFER_HDR_T  *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
    {
        GKI_exception(GKI_ERROR_GETPOOLBUF_BAD_QID, "getpoolbuf bad pool");
        return (NULL);
    }

    /* Make sure the buffers aren't disturbed til finished with allocation */
    GKI_disable();

    Q = &p_cb->freeq[pool_id];
    if ((Q->cur_cnt < Q->total) && (Q->p_first))
    {
        p_hdr = Q->p_first;
        Q->p_first = p_hdr->p_next;

        if (!Q->p_first)
            Q->p_last = NULL;

        if(++Q->cur_cnt > Q->max_cnt)
            Q->max_cnt = Q->cur_cnt;

        GKI_enable();


        p_hdr->task_id = GKI_get_taskid();

        p_hdr->status  = BUF_STATUS_UNLINKED;
        p_hdr->p_next  = NULL;
        p_hdr->Type    = 0;

        return ((void *) ((UINT8 *)p_hdr + BUFFER_HDR_SIZE));
    }

    /* If here, no buffers in the specified pool */
    GKI_enable();

    /* try for free buffers in public pools */
    return (GKI_getbuf(p_cb->freeq[pool_id].size));

}

/*******************************************************************************
**
** Function         GKI_freebuf
**
** Description      Called by an application to return a buffer to the free pool.
**
** Parameters       p_buf - (input) address of the beginning of a buffer.
**
** Returns          void
**
*******************************************************************************/
void GKI_freebuf (void *p_buf)
{
    FREE_QUEUE_T    *Q;
    BUFFER_HDR_T    *p_hdr;

#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
    if(!p_buf)
    {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Free - Buf pointer NULL");
        return;
    }

    if (gki_chk_buf_damage(p_buf))
    {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Free - Buf Corrupted");
        return;
    }
#endif

    p_hdr = (BUFFER_HDR_T *) ((UINT8 *)p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED)
    {
        GKI_exception(GKI_ERROR_FREEBUF_BUF_LINKED, "Freeing Linked Buf");
        return;
    }

    if (p_hdr->q_id >= GKI_NUM_TOTAL_BUF_POOLS)
    {
        GKI_exception(GKI_ERROR_FREEBUF_BAD_QID, "Bad Buf QId");
        return;
    }

    GKI_disable();

    /*
    ** Release the buffer
    */
    Q  = &gki_cb.com.freeq[p_hdr->q_id];
    if (Q->p_last)
        Q->p_last->p_next = p_hdr;
    else
        Q->p_first = p_hdr;

    Q->p_last      = p_hdr;
    p_hdr->p_next  = NULL;
    p_hdr->status  = BUF_STATUS_FREE;
    p_hdr->task_id = GKI_INVALID_TASK;
    if (Q->cur_cnt > 0)
        Q->cur_cnt--;

    GKI_enable();

    return;
}

#endif

/*******************************************************************************
**
** Function         GKI_get_buf_size
**
** Description      Called by an application to get the size of a buffer.
**
** Parameters       p_buf - (input) address of the beginning of a buffer.
**
** Returns          the size of the buffer
**
*******************************************************************************/
UINT16 GKI_get_buf_size (void *p_buf)
{
    BUFFER_HDR_T    *p_hdr;

    p_hdr = (BUFFER_HDR_T *)((UINT8 *) p_buf - BUFFER_HDR_SIZE);

    if ((UINTPTR)p_hdr & 1)
        return (0);

    if (p_hdr->q_id < GKI_NUM_TOTAL_BUF_POOLS)
    {
        return (gki_cb.com.freeq[p_hdr->q_id].size);
    }

    return (0);
}

/*******************************************************************************
**
** Function         GKI_check_buf_from_gki
**
** Description      check for target buffer from gki or not
**
** Returns          TRUE: the buffer is alloced from gki;
**                  FALSE: the buffer is not alloced from gki
**
*******************************************************************************/
BOOLEAN GKI_check_buf_from_gki (void* p_buf)
{
    return (!gki_chk_buf_damage(p_buf));
}

/*******************************************************************************
**
** Function         gki_chk_buf_damage
**
** Description      Called internally by OSS to check for buffer corruption.
**
** Returns          TRUE if there is a problem, else FALSE
**
*******************************************************************************/
BOOLEAN gki_chk_buf_damage(void *p_buf)
{
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)

    UINT32 *magic;
    magic  = (UINT32 *)((UINT8 *) p_buf + GKI_get_buf_size(p_buf));

    if ((UINTPTR)magic & 1)
        return (TRUE);

    if (*magic == MAGIC_NO)
        return (FALSE);

    return (TRUE);

#else

    return (FALSE);

#endif
}

/*******************************************************************************
**
** Function         GKI_send_msg
**
** Description      Called by applications to send a buffer to a task
**
** Returns          Nothing
**
*******************************************************************************/
#if ((defined GKI_BUFFER_DEBUG_EXTEND) && (GKI_BUFFER_DEBUG_EXTEND == TRUE))
void GKI_send_msg_T (UINT8 task_id, UINT8 mbox, void *msg, char const * function, int line)
{
    BUFFER_HDR_T    *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    /* If task non-existant or not started, drop buffer */
    if ((task_id >= GKI_MAX_TASKS) || (mbox >= NUM_TASK_MBOX) || (p_cb->OSRdyTbl[task_id] == TASK_DEAD))
    {
        char buf[56];
        sprintf(buf, "%s line:%d Sending to unknown dest:%d, %d, %d ", function, line, task_id, mbox, p_cb->OSRdyTbl[task_id]);
        GKI_exception(GKI_ERROR_SEND_MSG_BAD_DEST, buf);
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        GKI_freebuf (msg);
        return;
    }

#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
    if (gki_chk_buf_damage(msg))
    {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Send - Buffer corrupted");
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return;
    }
#endif

    p_hdr = (BUFFER_HDR_T *) ((UINT8 *) msg - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED)
    {
        GKI_exception(GKI_ERROR_SEND_MSG_BUF_LINKED, "Send - buffer linked");
        GKI_TRACE_3("func:%s, call_func:%s, line:%d", __func__, function, line);
        return;
    }

    GKI_disable();

    if (p_cb->OSTaskQFirst[task_id][mbox])
        p_cb->OSTaskQLast[task_id][mbox]->p_next = p_hdr;
    else
        p_cb->OSTaskQFirst[task_id][mbox] = p_hdr;

    p_cb->OSTaskQLast[task_id][mbox] = p_hdr;

    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_QUEUED;
    p_hdr->task_id = task_id;


    GKI_enable();

    GKI_send_event(task_id, (UINT16)EVENT_MASK(mbox));

    return;
}
#else
void GKI_send_msg (UINT8 task_id, UINT8 mbox, void *msg)
{
    BUFFER_HDR_T    *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    /* If task non-existant or not started, drop buffer */
    if ((task_id >= GKI_MAX_TASKS) || (mbox >= NUM_TASK_MBOX) || (p_cb->OSRdyTbl[task_id] == TASK_DEAD))
    {
        char buf[56];
        sprintf(buf, "Sending to unknown dest:%d, %d, %d ", task_id, mbox, p_cb->OSRdyTbl[task_id]);
        GKI_exception(GKI_ERROR_SEND_MSG_BAD_DEST, buf);
        GKI_freebuf (msg);
        return;
    }

#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
    if (gki_chk_buf_damage(msg))
    {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Send - Buffer corrupted");
        return;
    }
#endif

    p_hdr = (BUFFER_HDR_T *) ((UINT8 *) msg - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED)
    {
        GKI_exception(GKI_ERROR_SEND_MSG_BUF_LINKED, "Send - buffer linked");
        return;
    }

    GKI_disable();

    if (p_cb->OSTaskQFirst[task_id][mbox])
        p_cb->OSTaskQLast[task_id][mbox]->p_next = p_hdr;
    else
        p_cb->OSTaskQFirst[task_id][mbox] = p_hdr;

    p_cb->OSTaskQLast[task_id][mbox] = p_hdr;

    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_QUEUED;
    p_hdr->task_id = task_id;


    GKI_enable();

    GKI_send_event(task_id, (UINT16)EVENT_MASK(mbox));

    return;
}
#endif

/*******************************************************************************
**
** Function         GKI_read_mbox
**
** Description      Called by applications to read a buffer from one of
**                  the task mailboxes.  A task can only read its own mailbox.
**
** Parameters:      mbox  - (input) mailbox ID to read (0, 1, 2, or 3)
**
** Returns          NULL if the mailbox was empty, else the address of a buffer
**
*******************************************************************************/
void *GKI_read_mbox (UINT8 mbox)
{
    UINT8           task_id = GKI_get_taskid();
    void            *p_buf = NULL;
    BUFFER_HDR_T    *p_hdr;

    if ((task_id >= GKI_MAX_TASKS) || (mbox >= NUM_TASK_MBOX))
        return (NULL);

    GKI_disable();

    if (gki_cb.com.OSTaskQFirst[task_id][mbox])
    {
        p_hdr = gki_cb.com.OSTaskQFirst[task_id][mbox];
        gki_cb.com.OSTaskQFirst[task_id][mbox] = p_hdr->p_next;

        p_hdr->p_next = NULL;
        p_hdr->status = BUF_STATUS_UNLINKED;

        p_buf = (UINT8 *)p_hdr + BUFFER_HDR_SIZE;
    }

    GKI_enable();

    return (p_buf);
}



/*******************************************************************************
**
** Function         GKI_enqueue
**
** Description      Enqueue a buffer at the tail of the queue
**
** Parameters:      p_q  -  (input) pointer to a queue.
**                  p_buf - (input) address of the buffer to enqueue
**
** Returns          void
**
*******************************************************************************/
void GKI_enqueue (BUFFER_Q *p_q, void *p_buf)
{
    BUFFER_HDR_T    *p_hdr;

#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
    if (gki_chk_buf_damage(p_buf))
    {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Enqueue - Buffer corrupted");
        return;
    }
#endif

    p_hdr = (BUFFER_HDR_T *) ((UINT8 *) p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED)
    {
        GKI_exception(GKI_ERROR_ENQUEUE_BUF_LINKED, "Eneueue - buf already linked");
        return;
    }

    GKI_disable();

    /* Since the queue is exposed (C vs C++), keep the pointers in exposed format */
    if (p_q->p_last)
    {
        BUFFER_HDR_T *p_last_hdr = (BUFFER_HDR_T *)((UINT8 *)p_q->p_last - BUFFER_HDR_SIZE);
        p_last_hdr->p_next = p_hdr;
    }
    else
        p_q->p_first = p_buf;

    p_q->p_last = p_buf;
    p_q->count++;

    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_QUEUED;

    GKI_enable();

    return;
}


/*******************************************************************************
**
** Function         GKI_enqueue_head
**
** Description      Enqueue a buffer at the head of the queue
**
** Parameters:      p_q  -  (input) pointer to a queue.
**                  p_buf - (input) address of the buffer to enqueue
**
** Returns          void
**
*******************************************************************************/
void GKI_enqueue_head (BUFFER_Q *p_q, void *p_buf)
{
    BUFFER_HDR_T    *p_hdr;

#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
    if (gki_chk_buf_damage(p_buf))
    {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Enqueue - Buffer corrupted");
        return;
    }
#endif

    p_hdr = (BUFFER_HDR_T *) ((UINT8 *) p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED)
    {
        GKI_exception(GKI_ERROR_ENQUEUE_BUF_LINKED, "Eneueue head - buf already linked");
        return;
    }

    GKI_disable();

    if (p_q->p_first)
    {
        p_hdr->p_next = (BUFFER_HDR_T *)((UINT8 *)p_q->p_first - BUFFER_HDR_SIZE);
        p_q->p_first = p_buf;
    }
    else
    {
        p_q->p_first = p_buf;
        p_q->p_last  = p_buf;
        p_hdr->p_next = NULL;
    }
    p_q->count++;

    p_hdr->status = BUF_STATUS_QUEUED;

    GKI_enable();

    return;
}


/*******************************************************************************
**
** Function         GKI_dequeue
**
** Description      Dequeues a buffer from the head of a queue
**
** Parameters:      p_q  - (input) pointer to a queue.
**
** Returns          NULL if queue is empty, else buffer
**
*******************************************************************************/
void *GKI_dequeue (BUFFER_Q *p_q)
{
    BUFFER_HDR_T    *p_hdr;

    GKI_disable();

    if (!p_q || !p_q->count)
    {
        GKI_enable();
        return (NULL);
    }

    p_hdr = (BUFFER_HDR_T *)((UINT8 *)p_q->p_first - BUFFER_HDR_SIZE);

    /* Keep buffers such that GKI header is invisible
    */
    if (p_hdr->p_next)
        p_q->p_first = ((UINT8 *)p_hdr->p_next + BUFFER_HDR_SIZE);
    else
    {
        p_q->p_first = NULL;
        p_q->p_last  = NULL;
    }

    p_q->count--;

    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_UNLINKED;

    GKI_enable();

    return ((UINT8 *)p_hdr + BUFFER_HDR_SIZE);
}


/*******************************************************************************
**
** Function         GKI_remove_from_queue
**
** Description      Dequeue a buffer from the middle of the queue
**
** Parameters:      p_q  - (input) pointer to a queue.
**                  p_buf - (input) address of the buffer to enqueue
**
** Returns          NULL if queue is empty, else buffer
**
*******************************************************************************/
void *GKI_remove_from_queue (BUFFER_Q *p_q, void *p_buf)
{
    BUFFER_HDR_T    *p_prev;
    BUFFER_HDR_T    *p_buf_hdr;

    GKI_disable();

    if (p_buf == p_q->p_first)
    {
        GKI_enable();
        return (GKI_dequeue (p_q));
    }

    p_buf_hdr = (BUFFER_HDR_T *)((UINT8 *)p_buf - BUFFER_HDR_SIZE);
    p_prev    = (BUFFER_HDR_T *)((UINT8 *)p_q->p_first - BUFFER_HDR_SIZE);

    for ( ; p_prev; p_prev = p_prev->p_next)
    {
        /* If the previous points to this one, move the pointers around */
        if (p_prev->p_next == p_buf_hdr)
        {
            p_prev->p_next = p_buf_hdr->p_next;

            /* If we are removing the last guy in the queue, update p_last */
            if (p_buf == p_q->p_last)
                p_q->p_last = p_prev + 1;

            /* One less in the queue */
            p_q->count--;

            /* The buffer is now unlinked */
            p_buf_hdr->p_next = NULL;
            p_buf_hdr->status = BUF_STATUS_UNLINKED;

            GKI_enable();
            return (p_buf);
        }
    }

    GKI_enable();
    return (NULL);
}

/*******************************************************************************
**
** Function         GKI_getfirst
**
** Description      Return a pointer to the first buffer in a queue
**
** Parameters:      p_q  - (input) pointer to a queue.
**
** Returns          NULL if queue is empty, else buffer address
**
*******************************************************************************/
void *GKI_getfirst (BUFFER_Q *p_q)
{
    return (p_q->p_first);
}


/*******************************************************************************
**
** Function         GKI_getlast
**
** Description      Return a pointer to the last buffer in a queue
**
** Parameters:      p_q  - (input) pointer to a queue.
**
** Returns          NULL if queue is empty, else buffer address
**
*******************************************************************************/
void *GKI_getlast (BUFFER_Q *p_q)
{
    return (p_q->p_last);
}

/*******************************************************************************
**
** Function         GKI_getnext
**
** Description      Return a pointer to the next buffer in a queue
**
** Parameters:      p_buf  - (input) pointer to the buffer to find the next one from.
**
** Returns          NULL if no more buffers in the queue, else next buffer address
**
*******************************************************************************/
void *GKI_getnext (void *p_buf)
{
    BUFFER_HDR_T    *p_hdr;

    p_hdr = (BUFFER_HDR_T *) ((UINT8 *) p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->p_next)
        return ((UINT8 *)p_hdr->p_next + BUFFER_HDR_SIZE);
    else
        return (NULL);
}

/*******************************************************************************
**
** Function         GKI_queue_is_empty
**
** Description      Check the status of a queue.
**
** Parameters:      p_q  - (input) pointer to a queue.
**
** Returns          TRUE if queue is empty, else FALSE
**
*******************************************************************************/
BOOLEAN GKI_queue_is_empty(BUFFER_Q *p_q)
{
    return ((BOOLEAN) (p_q->count == 0));
}

UINT16 GKI_queue_length(BUFFER_Q *p_q)
{
    return p_q->count;
}

/*******************************************************************************
**
** Function         GKI_find_buf_start
**
** Description      This function is called with an address inside a buffer,
**                  and returns the start address ofthe buffer.
**
**                  The buffer should be one allocated from one of GKI's pools.
**
** Parameters:      p_user_area - (input) address of anywhere in a GKI buffer.
**
** Returns          void * - Address of the beginning of the specified buffer if successful,
**                          otherwise NULL if unsuccessful
**
*******************************************************************************/
void *GKI_find_buf_start (void *p_user_area)
{
    UINT16       xx, size;
    UINT32       yy;
    tGKI_COM_CB *p_cb = &gki_cb.com;
    UINT8       *p_ua = (UINT8 *)p_user_area;

    for (xx = 0; xx < GKI_NUM_TOTAL_BUF_POOLS; xx++)
    {
        if ((p_ua > p_cb->pool_start[xx]) && (p_ua < p_cb->pool_end[xx]))
        {
            yy = (UINT32)(p_ua - p_cb->pool_start[xx]);

            size = p_cb->pool_size[xx];

            yy = (yy / size) * size;

            return ((void *) (p_cb->pool_start[xx] + yy + sizeof(BUFFER_HDR_T)) );
        }
    }

    /* If here, invalid address - not in one of our buffers */
    GKI_exception (GKI_ERROR_BUF_SIZE_ZERO, "GKI_get_buf_start:: bad addr");

    return (NULL);
}


/********************************************************
* The following functions are not needed for light stack
*********************************************************/
#if (!defined(BTU_STACK_LITE_ENABLED) || BTU_STACK_LITE_ENABLED == FALSE)

/*******************************************************************************
**
** Function         GKI_set_pool_permission
**
** Description      This function is called to set or change the permissions for
**                  the specified pool ID.
**
** Parameters       pool_id -       (input) pool ID to be set or changed
**                  permission -    (input) GKI_PUBLIC_POOL or GKI_RESTRICTED_POOL
**
** Returns          GKI_SUCCESS if successful
**                  GKI_INVALID_POOL if unsuccessful
**
*******************************************************************************/
UINT8 GKI_set_pool_permission(UINT8 pool_id, UINT8 permission)
{
    tGKI_COM_CB *p_cb = &gki_cb.com;

    if (pool_id < GKI_NUM_TOTAL_BUF_POOLS)
    {
        if (permission == GKI_RESTRICTED_POOL)
            p_cb->pool_access_mask = (UINT16)(p_cb->pool_access_mask | (1 << pool_id));

        else    /* mark the pool as public */
            p_cb->pool_access_mask = (UINT16)(p_cb->pool_access_mask & ~(1 << pool_id));

        return (GKI_SUCCESS);
    }
    else
        return (GKI_INVALID_POOL);
}

/*******************************************************************************
**
** Function         gki_add_to_pool_list
**
** Description      Adds pool to the pool list which is arranged in the
**                  order of size
**
** Returns          void
**
*******************************************************************************/
static void gki_add_to_pool_list(UINT8 pool_id)
{

    INT32 i, j;
    tGKI_COM_CB *p_cb = &gki_cb.com;

     /* Find the position where the specified pool should be inserted into the list */
    for(i=0; i < p_cb->curr_total_no_of_pools; i++)
    {

        if(p_cb->freeq[pool_id].size <= p_cb->freeq[ p_cb->pool_list[i] ].size)
            break;
    }

    /* Insert the new buffer pool ID into the list of pools */
    for(j = p_cb->curr_total_no_of_pools; j > i; j--)
    {
        p_cb->pool_list[j] = p_cb->pool_list[j-1];
    }

    p_cb->pool_list[i] = pool_id;

    return;
}

/*******************************************************************************
**
** Function         gki_remove_from_pool_list
**
** Description      Removes pool from the pool list. Called when a pool is deleted
**
** Returns          void
**
*******************************************************************************/
static void gki_remove_from_pool_list(UINT8 pool_id)
{
    tGKI_COM_CB *p_cb = &gki_cb.com;
    UINT8 i;

    for(i=0; i < p_cb->curr_total_no_of_pools; i++)
    {
        if(pool_id == p_cb->pool_list[i])
            break;
    }

    while (i < (p_cb->curr_total_no_of_pools - 1))
    {
        p_cb->pool_list[i] = p_cb->pool_list[i+1];
        i++;
    }

    return;
}

/*******************************************************************************
**
** Function         GKI_igetpoolbuf
**
** Description      Called by an interrupt service routine to get a free buffer from
**                  a specific buffer pool.
**
** Parameters       pool_id - (input) pool ID to get a buffer out of.
**
** Returns          A pointer to the buffer, or NULL if none available
**
*******************************************************************************/
void *GKI_igetpoolbuf (UINT8 pool_id)
{
    FREE_QUEUE_T  *Q;
    BUFFER_HDR_T  *p_hdr;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
        return (NULL);


    Q = &gki_cb.com.freeq[pool_id];
    if(Q->cur_cnt < Q->total)
    {
        p_hdr = Q->p_first;
        Q->p_first = p_hdr->p_next;

        if (!Q->p_first)
            Q->p_last = NULL;

        if(++Q->cur_cnt > Q->max_cnt)
            Q->max_cnt = Q->cur_cnt;

        p_hdr->task_id = GKI_get_taskid();

        p_hdr->status  = BUF_STATUS_UNLINKED;
        p_hdr->p_next  = NULL;
        p_hdr->Type    = 0;

        return ((void *) ((UINT8 *)p_hdr + BUFFER_HDR_SIZE));
    }

    return (NULL);
}

/*******************************************************************************
**
** Function         GKI_poolcount
**
** Description      Called by an application to get the total number of buffers
**                  in the specified buffer pool.
**
** Parameters       pool_id - (input) pool ID to get the free count of.
**
** Returns          the total number of buffers in the pool
**
*******************************************************************************/
UINT16 GKI_poolcount (UINT8 pool_id)
{
    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
        return (0);

    return (gki_cb.com.freeq[pool_id].total);
}

/*******************************************************************************
**
** Function         GKI_poolfreecount
**
** Description      Called by an application to get the number of free buffers
**                  in the specified buffer pool.
**
** Parameters       pool_id - (input) pool ID to get the free count of.
**
** Returns          the number of free buffers in the pool
**
*******************************************************************************/
UINT16 GKI_poolfreecount (UINT8 pool_id)
{
    FREE_QUEUE_T  *Q;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
        return (0);

    Q  = &gki_cb.com.freeq[pool_id];

    return ((UINT16)(Q->total - Q->cur_cnt));
}

/*******************************************************************************
**
** Function         GKI_change_buf_owner
**
** Description      Called to change the task ownership of a buffer.
**
** Parameters:      p_buf   - (input) pointer to the buffer
**                  task_id - (input) task id to change ownership to
**
** Returns          void
**
*******************************************************************************/
void GKI_change_buf_owner (void *p_buf, UINT8 task_id)
{
    BUFFER_HDR_T    *p_hdr = (BUFFER_HDR_T *) ((UINT8 *) p_buf - BUFFER_HDR_SIZE);

    p_hdr->task_id = task_id;

    return;
}

#if (defined(GKI_SEND_MSG_FROM_ISR) &&  GKI_SEND_MSG_FROM_ISR == TRUE)
/*******************************************************************************
**
** Function         GKI_isend_msg
**
** Description      Called from interrupt context to send a buffer to a task
**
** Returns          Nothing
**
*******************************************************************************/
void GKI_isend_msg (UINT8 task_id, UINT8 mbox, void *msg)
{
    BUFFER_HDR_T    *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    /* If task non-existant or not started, drop buffer */
    if ((task_id >= GKI_MAX_TASKS) || (mbox >= NUM_TASK_MBOX) || (p_cb->OSRdyTbl[task_id] == TASK_DEAD))
    {
        GKI_exception(GKI_ERROR_SEND_MSG_BAD_DEST, "Sending to unknown dest");
        GKI_freebuf (msg);
        return;
    }

#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
    if (gki_chk_buf_damage(msg))
    {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Send - Buffer corrupted");
        return;
    }
#endif

#if (GKI_ENABLE_OWNER_CHECK == TRUE)
    if (gki_chk_buf_owner(msg))
    {
        GKI_exception(GKI_ERROR_NOT_BUF_OWNER, "Send by non-owner");
        return;
    }
#endif

    p_hdr = (BUFFER_HDR_T *) ((UINT8 *) msg - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED)
    {
        GKI_exception(GKI_ERROR_SEND_MSG_BUF_LINKED, "Send - buffer linked");
        return;
    }

    if (p_cb->OSTaskQFirst[task_id][mbox])
        p_cb->OSTaskQLast[task_id][mbox]->p_next = p_hdr;
    else
        p_cb->OSTaskQFirst[task_id][mbox] = p_hdr;

    p_cb->OSTaskQLast[task_id][mbox] = p_hdr;

    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_QUEUED;
    p_hdr->task_id = task_id;

    GKI_isend_event(task_id, (UINT16)EVENT_MASK(mbox));

    return;
}
#endif

/*******************************************************************************
**
** Function         GKI_create_pool
**
** Description      Called by applications to create a buffer pool.
**
** Parameters:      size        - (input) length (in bytes) of each buffer in the pool
**                  count       - (input) number of buffers to allocate for the pool
**                  permission  - (input) restricted or public access?
**                                        (GKI_PUBLIC_POOL or GKI_RESTRICTED_POOL)
**                  p_mem_pool  - (input) pointer to an OS memory pool, NULL if not provided
**
** Returns          the buffer pool ID, which should be used in calls to
**                  GKI_getpoolbuf(). If a pool could not be created, this
**                  function returns 0xff.
**
*******************************************************************************/
UINT8 GKI_create_pool (UINT16 size, UINT16 count, UINT8 permission, void *p_mem_pool)
{
    UINT8        xx;
    UINT32       mem_needed;
    INT32        tempsize = size;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    /* First make sure the size of each pool has a valid size with room for the header info */
    if (size > MAX_USER_BUF_SIZE)
        return (GKI_INVALID_POOL);

    /* First, look for an unused pool */
    for (xx = 0; xx < GKI_NUM_TOTAL_BUF_POOLS; xx++)
    {
        if (!p_cb->pool_start[xx])
            break;
    }

    if (xx == GKI_NUM_TOTAL_BUF_POOLS)
        return (GKI_INVALID_POOL);

    /* Ensure an even number of longwords */
    tempsize = (INT32)ALIGN_POOL(size);

    mem_needed = (tempsize + BUFFER_PADDING_SIZE) * count;

    if (!p_mem_pool)
        p_mem_pool = GKI_os_malloc(mem_needed);

    if (p_mem_pool)
    {
        /* Initialize the new pool */
        gki_init_free_queue (xx, size, count, p_mem_pool);
        gki_add_to_pool_list(xx);
        (void) GKI_set_pool_permission (xx, permission);
        p_cb->curr_total_no_of_pools++;

        return (xx);
    }
    else
        return (GKI_INVALID_POOL);
}

/*******************************************************************************
**
** Function         GKI_delete_pool
**
** Description      Called by applications to delete a buffer pool.  The function
**                  calls the operating specific function to free the actual memory.
**                  An exception is generated if an error is detected.
**
** Parameters:      pool_id - (input) Id of the poll being deleted.
**
** Returns          void
**
*******************************************************************************/
void GKI_delete_pool (UINT8 pool_id)
{
    FREE_QUEUE_T    *Q;
    tGKI_COM_CB     *p_cb = &gki_cb.com;

    if ((pool_id >= GKI_NUM_TOTAL_BUF_POOLS) || (!p_cb->pool_start[pool_id]))
        return;

    GKI_disable();
    Q  = &p_cb->freeq[pool_id];

    if (!Q->cur_cnt)
    {
        Q->size      = 0;
        Q->total     = 0;
        Q->cur_cnt   = 0;
        Q->max_cnt   = 0;
        Q->p_first   = NULL;
        Q->p_last    = NULL;

        GKI_os_free (p_cb->pool_start[pool_id]);

        p_cb->pool_start[pool_id] = NULL;
        p_cb->pool_end[pool_id]   = NULL;
        p_cb->pool_size[pool_id]  = 0;

        gki_remove_from_pool_list(pool_id);
        p_cb->curr_total_no_of_pools--;
    }
    else
        GKI_exception(GKI_ERROR_DELETE_POOL_BAD_QID, "Deleting bad pool");

    GKI_enable();

    return;
}

#endif /*  BTU_STACK_LITE_ENABLED == FALSE */

/*******************************************************************************
**
** Function         GKI_get_pool_bufsize
**
** Description      Called by an application to get the size of buffers in a pool
**
** Parameters       Pool ID.
**
** Returns          the size of buffers in the pool
**
*******************************************************************************/
UINT16 GKI_get_pool_bufsize (UINT8 pool_id)
{
    if (pool_id < GKI_NUM_TOTAL_BUF_POOLS)
        return (gki_cb.com.freeq[pool_id].size);

    return (0);
}

/*******************************************************************************
**
** Function         GKI_poolutilization
**
** Description      Called by an application to get the buffer utilization
**                  in the specified buffer pool.
**
** Parameters       pool_id - (input) pool ID to get the free count of.
**
** Returns          % of buffers used from 0 to 100
**
*******************************************************************************/
UINT16 GKI_poolutilization (UINT8 pool_id)
{
    FREE_QUEUE_T  *Q;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
        return (100);

    Q  = &gki_cb.com.freeq[pool_id];

    if (Q->total == 0)
        return (100);

    return ((Q->cur_cnt * 100) / Q->total);
}

/*******************************************************************************
**
** Function         GKI_print_free
**
** Description      GKI all gki pool free counts
**
** Parameters
**
** Returns
**
*******************************************************************************/
void GKI_print_free(void)
{
    UINT8 pool_id;

    for(pool_id = 0; pool_id < GKI_NUM_TOTAL_BUF_POOLS; pool_id ++)
    {
        GKI_TRACE_3("Pool-%d size:%d left free cnt: %d", pool_id , GKI_get_pool_bufsize(pool_id), GKI_poolfreecount(pool_id));
    }
}

#if ((defined GKI_BUFFER_DEBUG_EXTEND) && (GKI_BUFFER_DEBUG_EXTEND == TRUE))

void GKI_print_alloced(UINT8 pool_id)
{
    UINT8 idx = 0;
    tGKI_COM_CB *p_cb = &gki_cb.com;
    BUFFER_HDR_T  *p_hdr;
    UINT16        size,act_size,maxbuffs;
    UINT32        *magic;

    if(pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
    {
        return;
    }

    size = p_cb->freeq[pool_id].size;
    maxbuffs = p_cb->freeq[pool_id].total;
    act_size = size + BUFFER_PADDING_SIZE;
    GKI_TRACE_5("Buffer Pool[%u] size=%u cur_cnt=%u max_cnt=%u  total=%u ",
        pool_id, size,
        p_cb->freeq[pool_id].cur_cnt, p_cb->freeq[pool_id].max_cnt, maxbuffs);

    GKI_TRACE_0("      Owner  State    Sanity  Caller  Line");
    GKI_TRACE_0("------------------------------------------");
    p_hdr = (BUFFER_HDR_T *)(p_cb->pool_start[pool_id]);
    for(idx =0; idx < maxbuffs; idx ++)
    {
        magic = (UINT32 *)((UINT8 *)p_hdr + BUFFER_HDR_SIZE + size);
        if(p_hdr->status != 0)
            GKI_TRACE_6("%3d: 0x%02x %4d %10s  %s    %d", idx, p_hdr->task_id, p_hdr->status, (*magic == MAGIC_NO)?"OK":"CORRUPTED", p_hdr->FuncName, p_hdr->line);
        p_hdr          = (BUFFER_HDR_T *)((UINT8 *)p_hdr + act_size);
    }
    GKI_TRACE_0("\r\n");
}

void GKI_print_alloced_all(void)
{
    UINT8 pool_id;

    for(pool_id = 0; pool_id < GKI_NUM_TOTAL_BUF_POOLS; pool_id ++)
    {
        GKI_print_alloced(pool_id);
    }
}

#endif

