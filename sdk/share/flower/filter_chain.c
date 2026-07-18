#include <malloc.h>
#include <string.h>
#include "flower/flower.h"

#define MAX_FLOW_LIST 16
//=============================================================================
//                              Constant Definition
//=============================================================================

static IteFlowerList   gFList[MAX_FLOW_LIST];
static pthread_mutex_t gFList_mutex;

//=============================================================================
//                              Function Declaration
//=============================================================================

IteFilter *            ite_filter_new (IteFilterId id)
{
    // malloc a new filter struct
    IteFilter *    newPtr = NULL;
    IteFilterDes * tmpDes = NULL;
    int            i      = 0;

    DEBUG_PRINT("[%s], filter id=%d\n", __FUNCTION__, id);

    if (id <= 0)
    {
        printf("[%s] ERROR: id(%d) should be larger 0\n", __FUNCTION__, id);
        return NULL;
    }

    newPtr = (IteFilter *)malloc(sizeof(IteFilter));
    if (newPtr == NULL)
    {
        printf("[%s] ERROR: malloc fail.\n", __FUNCTION__);
        return NULL;
    }

    memset(newPtr, 0, sizeof(IteFilter));

    // lookup id to find filter
    while (gFilterDesSet[i] != NULL)
    {
        tmpDes = gFilterDesSet[i];
        i++;
        if (tmpDes->id == id)
        {
            newPtr->filterDes.id      = tmpDes->id;
            newPtr->filterDes.init    = tmpDes->init;
            newPtr->filterDes.uninit  = tmpDes->uninit;
            newPtr->filterDes.process = tmpDes->process;
            newPtr->filterDes.method  = tmpDes->method;

            DEBUG_PRINT("[%s] Found filter(%d)\n", __FUNCTION__, id);

            break;
        }
    }
    // newPtr->pthread_stacksize = configMINIMAL_STACK_SIZE *
    // sizeof(portSTACK_TYPE);//1024*4
    //  call filter init function
    if (newPtr->filterDes.init != NULL)
    {
        newPtr->filterDes.init(newPtr);
    }

    return newPtr;
}

void ite_filter_delete (IteFilter * f)
{
    // free a filter struct
    IteFilter * tmpPtr = f;

    DEBUG_PRINT("[%s] filter id=%d\n", __FUNCTION__, tmpPtr->filterDes.id);

    // call filter unint function
    tmpPtr->filterDes.uninit(tmpPtr);

    tmpPtr->filterDes.init    = NULL;
    tmpPtr->filterDes.method  = NULL;
    tmpPtr->filterDes.process = NULL;
    tmpPtr->filterDes.uninit  = NULL;

    free(tmpPtr);
    f = NULL;
}

void ite_filter_call_method (IteFilter * f, uint32_t MId, void * arg)
{
    IteFilter *    Filter = f;
    int            i      = 0;
    IteMethodDes * Method = NULL;

    Method                = Filter->filterDes.method;

    while (Method[i].id != 0)
    {
        if (Method[i].id == MId)
        {
            Method[i].method_func(Filter, arg);
            break;
        }
        i++;
    }

    if (Method[i].id == 0)
    {
        printf("[%s] ERROR: No such method.\n", __FUNCTION__);
    }
}

void ite_filter_set_semBind (IteFilter * f)
{
    IteFilter * tmpPtr = f;

    if (f == NULL)
    {
        printf("[%s] No such Filter\n", __FUNCTION__);
        return;
    }

    tmpPtr->inputSemBind = true;

    DEBUG_PRINT("[%s] Filter(%d) use the same semaphore for all input.\n",
                __FUNCTION__, tmpPtr->filterDes.id);
}

void ite_filter_set_status (IteFilter * f, IteFilterStatus status)
{
    IteFilter * tmpPtr = f;

    if (f == NULL)
    {
        printf("[%s] No such Filter\n", __FUNCTION__);
        return;
    }

    if (status == FILTER_PAUSE)
    {
        // init condition variable
        pthread_cond_init(&f->tCond, NULL);
        pthread_mutex_init(&f->tMutex, NULL);
    }
    else if (status == FILTER_RESUME)
    {
        if (f->tCond != (pthread_cond_t)NULL)
        {
            // signal, wakeup filter
            pthread_mutex_lock(&f->tMutex);
            pthread_cond_signal(&f->tCond);
            pthread_mutex_unlock(&f->tMutex);

            // destroy condition variable
            pthread_cond_destroy(&f->tCond);
            pthread_mutex_destroy(&f->tMutex);

            f->tCond = (pthread_cond_t)NULL;
        }
    }

    tmpPtr->status = status;

    DEBUG_PRINT("[%s] Fitler(%d) status = %d\n", tmpPtr->status);
}

static void ite_filter_link (IteFilter * Outft, int outPin, IteFilter * Inft,
                             int inPin, IteFcConf * fc_config)
{
    // link filters, create queue
    QueueHandle_t tmpQ_handler = NULL;
    sem_t         tmpSem;

    if ((Outft == NULL) || (Inft == NULL))
    {
        return;
    }

    DEBUG_PRINT("[%s] Out_filter(%d) In_filter(%d) is linking\n", __FUNCTION__,
                Outft->filterDes.id, Inft->filterDes.id);

    if ((outPin == -1) || (inPin == -1))
    {
        return;
    }

    // Create Queue
    tmpQ_handler = ite_queue_new(fc_config->QSize, sizeof(IteQueueblk));
    if (tmpQ_handler == NULL)
    {
        printf("[%s] ERROR: Queue create fail\n", __FUNCTION__);
        return;
    }

    Outft->output[outPin].Qhandle = tmpQ_handler;
    Inft->input[inPin].Qhandle    = tmpQ_handler;

    // Create Semaphore
    if (fc_config->Semaphore)
    {
        if (Inft->inputSemBind)
        {
            if (Inft->input[0].semHandle.__sem_lock)
            {
                Outft->output[outPin].semHandle = Inft->input[0].semHandle;
                Inft->input[inPin].semHandle    = Inft->input[0].semHandle;
            }
            else if (Inft->input[1].semHandle.__sem_lock)
            {
                Outft->output[outPin].semHandle = Inft->input[1].semHandle;
                Inft->input[inPin].semHandle    = Inft->input[1].semHandle;
            }
            else
            {
                sem_init(&tmpSem, 0, 0);

                Outft->output[outPin].semHandle = tmpSem;
                Inft->input[inPin].semHandle    = tmpSem;
            }
        }
        else
        {
            sem_init(&tmpSem, 0, 0);

            Outft->output[outPin].semHandle = tmpSem;
            Inft->input[inPin].semHandle    = tmpSem;
        }
    }
}

static void ite_filter_unlink (IteFilter * Outft, int outPin, IteFilter * Inft,
                               int inPin, IteFcConf * fc_config)
{
    // unlink filters
    QueueHandle_t tmpQ_handler = NULL;
    sem_t         tmpSem;

    if ((Outft == NULL) || (Inft == NULL))
    {
        return;
    }

    DEBUG_PRINT("[%s] Unlink Out_filter(%d) In_filter(%d)\n", __FUNCTION__,
                Outft->filterDes.id, Inft->filterDes.id);

    if ((outPin == -1) || (inPin == -1))
    {
        return;
    }

    // Free Queue
    tmpQ_handler = Outft->output[outPin].Qhandle;
    ite_queue_free(tmpQ_handler);

    Outft->output[outPin].Qhandle = NULL;
    Inft->input[inPin].Qhandle    = NULL;

    // Free Semaphore
    if (fc_config->Semaphore)
    {
        if (Inft->inputSemBind)
        {
            if (Inft->input[0].semHandle.__sem_lock &&
                Inft->input[1].semHandle.__sem_lock)
            {
                tmpSem = Outft->output[outPin].semHandle;
                sem_destroy(&tmpSem);
            }
        }
        else
        {
            tmpSem = Outft->output[outPin].semHandle;
            sem_destroy(&tmpSem);
        }

        Outft->output[outPin].semHandle.__sem_lock = NULL;
        Inft->input[inPin].semHandle.__sem_lock    = NULL;
    }
}

void ite_filterChain_build (IteFChain * helper, char * helper_name)
{
    IteFChain * currFcPtr = helper;

    if (helper == NULL)
    {
        printf("[%s] ERROR: no such filter chain\n", __FUNCTION__);
        return;
    }

    if (helper_name == NULL)
    {
        helper_name = "Chain";
    }

    // init filterChain helper
    currFcPtr->filter  = NULL;
    currFcPtr->nextPtr = NULL;

    // add to Flower List
    ite_flower_add(currFcPtr, helper_name);
}

void ite_filterChain_setConfig (IteFChain * fc, int param_size, char ** param)
{
    IteFlowerList * currPtr = NULL;
    int             i       = 0;
    bool            found   = false;
    char            tmp_param[4][16];

    if (fc == NULL)
    {
        printf("[%s] ERROR: filter chain is NULL\n", __FUNCTION__);
        return;
    }

    while (i < MAX_FLOW_LIST)
    {
        if (gFList[i].fc == fc)
        {
            currPtr = &gFList[i];
            found   = true;
            break;
        }
        i++;
    }

    for (i = 0; i < param_size; i++)
    {
        strncpy(tmp_param[i], param[i], sizeof(tmp_param[i]) - 1);
        tmp_param[i][sizeof(tmp_param[i]) - 1] = '\0';
    }

    if (!found)
    {
        printf("[%s] ERROR: No such filter chain\n", __FUNCTION__);
        return;
    }

    // parse config
    i = 0;
    while (i < param_size)
    {
        if (strncmp("-S", tmp_param[i], 2) == 0)
        {
            // Use Semaphore
            currPtr->config.Semaphore = true;

            DEBUG_PRINT("[%s] Use Semaphore\n", __FUNCTION__);
        }
        else if (strncmp("-Q", tmp_param[i], 2) == 0)
        {
            char * tag = "=";
            char * tok = NULL;
            int    idx = 0;
            // Set Queue Size

            tok        = strtok(tmp_param[i], tag);
            while (tok != NULL)
            {

                if (idx == 1)
                {
                    currPtr->config.QSize = atoi(tok);
                }
                tok = strtok(NULL, tag);
                idx++;
            }

            // Set default size
            if (currPtr->config.QSize == 0)
            {
                currPtr->config.QSize = STREAM_QUEUE_SIZE;
            }

            DEBUG_PRINT("[%s] Set QSize=%d\n", __FUNCTION__,
                        currPtr->config.QSize);
        }
        else
        {
            printf("[%s] ERROR: Bad arguments\n", __FUNCTION__);
        }

        i++;
    }
}

void ite_filterChain_print (IteFChain * helper)
{
    IteFChain * currPtr   = helper;
    IteFilter * tmpFilter = NULL;
    IteFcConf * fc_Conf   = NULL;

    do
    {
        // find filter chain conf
        fc_Conf = ite_flower_findFChainConfig(helper);
        if (fc_Conf == NULL)
        {
            break;
        }

        printf("-------FilterChain: %s-------\n", fc_Conf->name);

        while (currPtr != NULL)
        {
            tmpFilter = currPtr->filter;
            printf("Filter(%d)-", tmpFilter->filterDes.id);

            currPtr = currPtr->nextPtr;
        }

        printf("\n-------FilterChain End-------\n");
    } while (false);
}

void ite_filterChain_stop (IteFChain * helper)
{
    IteFChain * currPtr    = helper;
    IteFilter * currFilter = NULL;
    IteFcConf * fc_Conf    = NULL;
    sem_t       tmpSem;
    int         sem_count = 0;

    do
    {
        if (helper == NULL)
        {
            printf("[%s] ERROR: no such filter chain\n", __FUNCTION__);
            break;
        }

        // find helper in Flower List
        fc_Conf = ite_flower_findFChainConfig(helper);
        if (fc_Conf == NULL)
        {
            break;
        }

        while (currPtr != NULL)
        {
            currFilter = currPtr->filter;

            // stop thread
            if (currFilter->run == true)
            {

                // Set thread to stop
                currFilter->run = false;

                // Set thread status to NONE
                if (currFilter->status == FILTER_PAUSE)
                {
                    // signal, wakeup filter
                    pthread_mutex_lock(&currFilter->tMutex);
                    pthread_cond_signal(&currFilter->tCond);
                    pthread_mutex_unlock(&currFilter->tMutex);

                    // destroy condition variable
                    pthread_cond_destroy(&currFilter->tCond);
                    pthread_mutex_destroy(&currFilter->tMutex);

                    currFilter->tCond = (pthread_cond_t)NULL;
                }
                currFilter->status = FILTER_NONE;

                // NOTICE: post semaphore, avoid thread blocking.
                if (fc_Conf->Semaphore)
                {
                    if (currFilter->input[0].semHandle.__sem_lock)
                    {
                        tmpSem = currFilter->input[0].semHandle;
                        sem_getvalue(&tmpSem, &sem_count);
                        if (sem_count <= 0)
                        {
                            sem_post(&tmpSem);
                        }
                        DEBUG_PRINT("[%s] Filter(%d) has input[0].sem(%d)\n",
                                    __FUNCTION__, currFilter->filterDes.id,
                                    sem_count);
                    }

                    if (currFilter->input[1].semHandle.__sem_lock)
                    {
                        tmpSem = currFilter->input[1].semHandle;
                        sem_getvalue(&tmpSem, &sem_count);
                        if (sem_count <= 0)
                        {
                            sem_post(&tmpSem);
                        }
                        DEBUG_PRINT("[%s] Filter(%d) has input[1].sem(%d)\n",
                                    __FUNCTION__, currFilter->filterDes.id,
                                    sem_count);
                    }
                }

                // thread join
                pthread_join(currFilter->tID, NULL);
                DEBUG_PRINT("[%s]Filter(%d) stop therad\n", __FUNCTION__,
                            currFilter->filterDes.id);
            }

            currPtr = currPtr->nextPtr;
        }
    } while (false);
}

void ite_filterChain_delete (IteFChain * helper)
{
    IteFChain * currPtr = NULL;
    IteFChain * tmpPtr  = NULL;
    IteFChain * headPtr = helper;

    if (helper == NULL)
    {
        printf("[%s] ERROR: no such filter chain\n", __FUNCTION__);
        return;
    }

    // delete from Flow List
    ite_flower_delete(headPtr);

    // find the next, because the head is not malloc
    if (headPtr->nextPtr != NULL)
    {
        currPtr = headPtr->nextPtr;
    }

    // free filter chain
    while (currPtr != NULL)
    {
        tmpPtr         = currPtr;
        tmpPtr->filter = NULL;
        currPtr        = currPtr->nextPtr;

        free(tmpPtr);
    }

    memset(helper, 0, sizeof(IteFChain));
}

static IteFChain * ite_filterChain_add (IteFilter * f)
{
    // malloc a new filter chain helper
    IteFChain * newPtr = NULL;

    if (f == NULL)
    {
        printf("[%s] ERROR: no such Filter\n", __FUNCTION__);
        return NULL;
    }

    newPtr = (IteFChain *)malloc(sizeof(IteFChain));
    if (newPtr == NULL)
    {
        printf("[%s] ERROR: malloc fail\n", __FUNCTION__);
        return NULL;
    }

    memset(newPtr, 0, sizeof(IteFChain));
    newPtr->filter = f;

    return newPtr;
}

void ite_filterChain_A_link_B (bool bee, IteFChain * helper, IteFilter * outf,
                               int preFoutPin, IteFilter * inf, int inputPin)
{
    IteFChain * currPtr   = helper;
    IteFilter * preFilter = NULL;
    IteFilter * tmpFilter = NULL;
    IteFcConf * fc_Conf   = NULL;
    bool        out_found = false;

    do
    {
        if ((helper == NULL) || (inf == NULL) || (outf == NULL))
        {
            printf("[%s] ERROR: no such filter chain or filter\n",
                   __FUNCTION__);
            break;
        }

        // add the first filter
        if (helper->filter == NULL)
        {
            printf("[%s] ERROR: filter chain must have one existed filter\n",
                   __FUNCTION__);
            break;
        }

        // found input Filter
        while (currPtr)
        {
            tmpFilter = currPtr->filter;
            if (tmpFilter->filterDes.id == outf->filterDes.id)
            {
                preFilter = tmpFilter;
                out_found = true;
                break;
            }

            currPtr = currPtr->nextPtr;
        }

        if (out_found == false)
        {
            printf("[%s] ERROR: no such output filter\n", __FUNCTION__);
            break;
        }

        // reset currPtr to fc header
        currPtr = helper;

        // found the last instance
        while (currPtr->nextPtr)
        {
            currPtr = currPtr->nextPtr;
        }

        // malloc fliter chain
        currPtr->nextPtr = ite_filterChain_add(inf);

        // find helper in Flower List
        fc_Conf          = ite_flower_findFChainConfig(helper);
        if (fc_Conf == NULL)
        {
            break;
        }

        // link filters
        ite_filter_link(preFilter, preFoutPin, inf, inputPin, fc_Conf);

        // inf filter want to run thread right now.
        if (bee)
        {
            IteFilter * runFilter = inf;
            DEBUG_PRINT("[%s]Filter(%d) link and run.\n", __FUNCTION__,
                        runFilter->filterDes.id);

            // run filter thread
            if (runFilter->run == false)
            {
                runFilter->run    = true;
                runFilter->status = FILTER_NONE;

                if (pthread_create(&runFilter->tID, NULL,
                                   (void *)runFilter->filterDes.process,
                                   (void *)runFilter) != 0)
                {
                    printf("[%s] ERROR: Create thread(%d) fail.\n",
                           __FUNCTION__, runFilter->filterDes.id);
                }
            }
        }
    } while (false);
}

void ite_filterChain_link (IteFChain * helper, int preFoutPin, IteFilter * f,
                           int inputPin)
{
    IteFChain * currPtr   = helper;
    IteFilter * preFilter = NULL;
    IteFcConf * fc_Conf   = NULL;

    do
    {
        if ((helper == NULL) || (f == NULL))
        {
            printf("[%s] ERROR: no such filter chain or filter\n",
                   __FUNCTION__);
            break;
        }

        // add the first filter
        if (helper->filter == NULL)
        {
            helper->filter  = f;
            helper->nextPtr = NULL;
            break;
        }

        // found the last instance
        while (currPtr->nextPtr)
        {
            currPtr = currPtr->nextPtr;
        }

        // found the last filter
        preFilter        = currPtr->filter;

        // malloc fliter chain
        currPtr->nextPtr = ite_filterChain_add(f);

        // find helper in Flower List
        fc_Conf          = ite_flower_findFChainConfig(helper);
        if (fc_Conf == NULL)
        {
            break;
        }

        // link filters
        ite_filter_link(preFilter, preFoutPin, f, inputPin, fc_Conf);
    } while (false);
}

void ite_filterChain_A_unlink_B (bool bee, IteFChain * helper, IteFilter * outf,
                                 int preFoutPin, IteFilter * inf, int inputPin)
{
    IteFChain * currFcPtr  = helper;
    IteFChain * nextFcPtr  = NULL;
    IteFilter * currFilter = NULL;
    IteFilter * preFilter  = NULL;
    IteFilter * tmpFilter  = NULL;
    IteFilter * inFilter   = NULL;
    bool        in_found   = false;
    bool        out_found  = false;
    IteFcConf * fc_Conf    = NULL;

    do
    {
        if ((helper == NULL) || (inf == NULL) || (outf == NULL))
        {
            printf("[%s] ERROR: no such filter chain or filter\n",
                   __FUNCTION__);
            break;
        }

        while (currFcPtr)
        {
            tmpFilter = currFcPtr->filter;
            if (outf->filterDes.id == tmpFilter->filterDes.id)
            {
                // find the output filter
                preFilter = tmpFilter;
                out_found = true;
            }

            nextFcPtr = currFcPtr->nextPtr;
            if (nextFcPtr)
            {
                currFilter = nextFcPtr->filter;
                if (currFilter->filterDes.id == inf->filterDes.id)
                {
                    inFilter = currFilter;
                    in_found = true;
                }
            }
            currFcPtr = currFcPtr->nextPtr;
        }

        if (in_found && out_found)
        {
            // find helper in Flower List
            fc_Conf = ite_flower_findFChainConfig(helper);
            if (fc_Conf == NULL)
            {
                break;
            }

            // unlink filters
            ite_filter_unlink(preFilter, preFoutPin, inFilter, inputPin,
                              fc_Conf);

            // inf filter want to unlink and stop thread right now
            if (bee)
            {
                IteFilter * stopFilter = inf;
                int         sem_count  = 0;
                sem_t       tmpSem;

                if (stopFilter->run == true)
                {
                    // Set thread to stop
                    stopFilter->run = false;

                    // Set thread status to NONE
                    if (stopFilter->status == FILTER_PAUSE)
                    {
                        // signal, wakeup filter
                        pthread_mutex_lock(&stopFilter->tMutex);
                        pthread_cond_signal(&stopFilter->tCond);
                        pthread_mutex_unlock(&stopFilter->tMutex);

                        // destroy condition variable
                        pthread_cond_destroy(&stopFilter->tCond);
                        pthread_mutex_destroy(&stopFilter->tMutex);

                        stopFilter->tCond = (pthread_cond_t)NULL;
                    }
                    stopFilter->status = FILTER_NONE;

                    // NOTICE: post semaphore, avoid thread blocking.
                    if (fc_Conf->Semaphore)
                    {
                        if (stopFilter->input[0].semHandle.__sem_lock)
                        {
                            tmpSem = stopFilter->input[0].semHandle;
                            sem_getvalue(&tmpSem, &sem_count);
                            if (sem_count <= 0)
                            {
                                sem_post(&tmpSem);
                            }
                            DEBUG_PRINT(
                                "[%s] Filter(%d) has input[0].sem(%d)\n",
                                __FUNCTION__, stopFilter->filterDes.id,
                                sem_count);
                        }

                        if (stopFilter->input[1].semHandle.__sem_lock)
                        {
                            tmpSem = stopFilter->input[1].semHandle;
                            sem_getvalue(&tmpSem, &sem_count);
                            if (sem_count <= 0)
                            {
                                sem_post(&tmpSem);
                            }
                            DEBUG_PRINT(
                                "[%s] Filter(%d) has input[1].sem(%d)\n",
                                __FUNCTION__, stopFilter->filterDes.id,
                                sem_count);
                        }
                    }

                    // stop fitler thread
                    pthread_join(stopFilter->tID, NULL);
                    DEBUG_PRINT("[%s]Filter(%d) stop therad\n", __FUNCTION__,
                                stopFilter->filterDes.id);
                }
            }
        }
        else
        {
            printf("[%s] ERROR: no such filter\n", __FUNCTION__);
        }
    } while (false);
}

void ite_filterChain_unlink (IteFChain * helper, int preFoutPin, IteFilter * f,
                             int inputPin)
{
    IteFChain * currFcPtr  = helper;
    IteFChain * nextFcPtr  = NULL;
    IteFilter * currFilter = NULL;
    IteFilter * preFilter  = NULL;
    bool        found      = false;
    IteFcConf * fc_Conf    = NULL;

    do
    {
        if ((helper == NULL) || (f == NULL))
        {
            printf("[%s] ERROR: no such filter chain or filter\n",
                   __FUNCTION__);
            break;
        }

        while (currFcPtr)
        {
            preFilter = currFcPtr->filter;
            nextFcPtr = currFcPtr->nextPtr;
            if (nextFcPtr)
            {
                currFilter = nextFcPtr->filter;
                if (currFilter->filterDes.id == f->filterDes.id)
                {
                    found = true;
                    break;
                }
            }
            currFcPtr = currFcPtr->nextPtr;
        }

        if (found)
        {
            // find helper in Flower List
            fc_Conf = ite_flower_findFChainConfig(helper);
            if (fc_Conf == NULL)
            {
                break;
            }

            // unlink filters
            ite_filter_unlink(preFilter, preFoutPin, currFilter, inputPin,
                              fc_Conf);
        }
    } while (false);
}

void ite_filterChain_run (IteFChain * helper)
{
    IteFChain * currPtr   = helper;
    IteFilter * tmpFilter = NULL;

    if (helper == NULL)
    {
        printf("[%s] ERROR: no such filter chain\n", __FUNCTION__);
        return;
    }

    currPtr = helper;
    while (currPtr)
    {
        tmpFilter = currPtr->filter;

        // create thread
        if (tmpFilter->run == false)
        {
            pthread_attr_t attr;

            pthread_attr_init(&attr);

            tmpFilter->run    = true;
            tmpFilter->status = FILTER_NONE;

            if (tmpFilter->pthread_stacksize > 0)
            {
                pthread_attr_setstacksize(&attr, tmpFilter->pthread_stacksize);
            }

            if (pthread_create(&tmpFilter->tID, &attr,
                               (void *)tmpFilter->filterDes.process,
                               (void *)tmpFilter) != 0)
            {
                printf("[%s] ERROR: Create thread(%d) fail.\n", __FUNCTION__,
                       tmpFilter->filterDes.id);
            }
        }

        currPtr = currPtr->nextPtr;
    }
}

void ite_filterChain_setStatus (IteFChain * helper, IteFilterStatus status)
{
    IteFChain * currPtr   = helper;
    IteFilter * tmpFilter = NULL;

    if (helper == NULL)
    {
        printf("[%s] ERROR: no such filter chain\n", __FUNCTION__);
        return;
    }

    while (currPtr)
    {
        tmpFilter = currPtr->filter;

        // set filter status
        ite_filter_set_status(tmpFilter, status);

        currPtr = currPtr->nextPtr;
    }
}

IteFilterStatus ite_filterChain_getStatus (IteFChain * helper)
{
    IteFChain *     currPtr   = helper;
    IteFilter *     tmpFilter = NULL;
    IteFilterStatus status    = FILTER_NONE;

    if (helper == NULL)
    {
        printf("[%s] ERROR: no such filter chain\n", __FUNCTION__);
        return FILTER_ERR;
    }

    status = currPtr->filter->status;
    while (currPtr)
    {
        tmpFilter = currPtr->filter;
        if (status != tmpFilter->status)
        {
            printf("[%s] WARNING: the status of filters in chain are "
                   "inconsistent.\n",
                   __FUNCTION__);
            status = FILTER_ERR;
            break;
        }
        currPtr = currPtr->nextPtr;
    }

    return status;
}

void ite_flower_reset (void)
{
    int i = 0;

    DEBUG_PRINT("[%s]\n", __FUNCTION__);

    pthread_mutex_lock(&gFList_mutex);
    // Reset gFList
    for (i = 0; i < MAX_FLOW_LIST; i++)
    {
        gFList[i].fc               = NULL;
        gFList[i].config.Semaphore = false;
        gFList[i].config.QSize     = STREAM_QUEUE_SIZE;
        gFList[i].config.name      = NULL;
    }
    pthread_mutex_unlock(&gFList_mutex);
}

void ite_flower_init (void)
{
    int i = 0;

    DEBUG_PRINT("[%s]\n", __FUNCTION__);

    pthread_mutex_init(&gFList_mutex, NULL);

    pthread_mutex_lock(&gFList_mutex);
    // Reset gFList
    for (i = 0; i < MAX_FLOW_LIST; i++)
    {
        gFList[i].fc               = NULL;
        gFList[i].config.Semaphore = false;
        gFList[i].config.QSize     = STREAM_QUEUE_SIZE;
        gFList[i].config.name      = NULL;
    }
    pthread_mutex_unlock(&gFList_mutex);
}

void ite_flower_add (IteFChain * fc, char * name)
{
    int  i     = 0;
    bool found = false;

    if (fc == NULL)
    {
        printf("[%s] ERROR: filter chain is empty\n", __FUNCTION__);
        return;
    }

    pthread_mutex_lock(&gFList_mutex);
    while (i < MAX_FLOW_LIST)
    {
        if (gFList[i].fc == NULL)
        {
            gFList[i].fc          = fc;
            gFList[i].config.name = strdup(name);
            DEBUG_PRINT("[%s] add filter chain(%s) in flower list.\n",
                        __FUNCTION__, name);

            found = true;
            break;
        }
        i++;
    }
    pthread_mutex_unlock(&gFList_mutex);

    if (!found)
    {
        printf("[%s] ERROR: Exceed MAX_Flow_List\n", __FUNCTION__);
    }
}

void ite_flower_delete (IteFChain * fc)
{
    int  i     = 0;
    bool found = false;

    if (fc == NULL)
    {
        printf("[%s] ERROR: filter chain is empty\n", __FUNCTION__);
        return;
    }

    pthread_mutex_lock(&gFList_mutex);
    while (i < MAX_FLOW_LIST)
    {
        if (gFList[i].fc == fc)
        {
            gFList[i].fc               = NULL;
            gFList[i].config.QSize     = STREAM_QUEUE_SIZE;
            gFList[i].config.Semaphore = false;
            DEBUG_PRINT("[%s] Remove filter chain(%s) in flower list.\n",
                        __FUNCTION__, gFList[i].config.name);

            if (gFList[i].config.name)
            {
                free(gFList[i].config.name);
                gFList[i].config.name = NULL;
            }

            found = true;
            break;
        }
        i++;
    }
    pthread_mutex_unlock(&gFList_mutex);

    if (!found)
    {
        printf("[%s] ERROR: No such filter chain\n", __FUNCTION__);
    }
}

int ite_flower_findFChainQSize (IteFChain * fc)
{
    int i     = 0;
    int QSize = 0;

    if (fc == NULL)
    {
        printf("[%s] filter chain is empty\n", __FUNCTION__);
        return 0;
    }

    while (i < MAX_FLOW_LIST)
    {
        if (gFList[i].fc == fc)
        {
            QSize = gFList[i].config.QSize;
            break;
        }
        i++;
    }

    return QSize;
}

bool ite_flower_findFChainUseSem (IteFChain * fc)
{
    int  i      = 0;
    bool UseSem = false;

    if (fc == NULL)
    {
        printf("[%s] filter chain is empty\n", __FUNCTION__);
        return false;
    }

    while (i < MAX_FLOW_LIST)
    {
        if (gFList[i].fc == fc)
        {
            UseSem = gFList[i].config.Semaphore;
            break;
        }
        i++;
    }

    return UseSem;
}

IteFcConf * ite_flower_findFChainConfig (IteFChain * fc)
{
    int i = 0;

    if (fc == NULL)
    {
        printf("[%s] filter chain is empty\n", __FUNCTION__);
        return NULL;
    }

    while (i < MAX_FLOW_LIST)
    {
        if (gFList[i].fc == fc)
        {
            return &gFList[i].config;
        }

        i++;
    }

    // Error: No found
    return NULL;
}

void ite_flower_print (void)
{
    int i = 0;

    printf("-------Flower List Start-------\n");

    for (i = 0; i < MAX_FLOW_LIST; i++)
    {
        if (gFList[i].fc)
        {
            printf("(%d)FilterChain(%s) QSize=%d Use_Sem=%d\n", i,
                   gFList[i].config.name, gFList[i].config.QSize,
                   gFList[i].config.Semaphore);
        }
    }

    printf("-------Flow List End-------\n");
}

void ite_flower_deinit (void)
{
    int i = 0;

    DEBUG_PRINT("[%s]\n", __FUNCTION__);

    pthread_mutex_lock(&gFList_mutex);
    // Reset gFList
    for (i = 0; i < MAX_FLOW_LIST; i++)
    {
        gFList[i].fc               = NULL;
        gFList[i].config.Semaphore = false;
        gFList[i].config.QSize     = STREAM_QUEUE_SIZE;
        gFList[i].config.name      = NULL;
    }
    pthread_mutex_unlock(&gFList_mutex);

    pthread_mutex_destroy(&gFList_mutex);
}
