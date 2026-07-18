/**
 * @file        test_can_upgrade.c
 *
 * @brief
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ite/itp.h"
#include "user_config.h"
#include "test_can_upgrade.h"

pthread_mutex_t can_upgrade_mutex = PTHREAD_MUTEX_INITIALIZER;

CAN_UPGRADE_STATE ctrl_state = CAN_UPGRADE_UNINIT_STATE;
int pkg_total_size = 0;
int pkg_remaining_size = 0;
uint8_t *pkgaddr = NULL;
uint8_t *pkgptr = NULL;

int UDS_CAN_Upgrade_Init(int memory_size)
{
    int ret = 0;
    pthread_mutex_lock(&can_upgrade_mutex);
    if (ctrl_state == CAN_UPGRADE_UNINIT_STATE)
    {
        pkgaddr = (uint8_t *)malloc(memory_size);
        if (pkgaddr == NULL)
        {
            //printf("memory for upgrade cannot be allocated\n");
            ret = -1;
            pthread_mutex_unlock(&can_upgrade_mutex);
            exit(ret);
        }
        pkg_total_size = memory_size;
        pkg_remaining_size = memory_size;
        pkgptr = pkgaddr;
        //printf("memory pkgptr = %p \n", pkgptr);
        ctrl_state = CAN_UPGRADE_NORMAL_STATE;
    }
    pthread_mutex_unlock(&can_upgrade_mutex);

    return ret;
}

int UDS_CAN_Upgrade_StoreData(uint8_t *data, int len, int remaining_size)
{
    pthread_mutex_lock(&can_upgrade_mutex);
    if (pkg_remaining_size != 0)
    {
        memcpy((void *)pkgptr, (void *)data, (size_t)len);
        pkgptr += len;
        pkg_remaining_size -= len;
    }
    pthread_mutex_unlock(&can_upgrade_mutex);
    return 0;
}

int UDS_CAN_Get_Upgrade_State()
{
    return ctrl_state;
}

void UDS_CAN_Set_Upgrade_State(CAN_UPGRADE_STATE state)
{
    pthread_mutex_lock(&can_upgrade_mutex);
    ctrl_state = state;
    pthread_mutex_unlock(&can_upgrade_mutex);
}

uint8_t* UDS_CAN_Upgrade_GetPkgAddr()
{
    return pkgaddr;
}

int UDS_CAN_Upgrade_GetPkgSize()
{
    return pkg_total_size;
}

int UDS_CAN_Upgrade_Exit()
{
    int ret = 0;
    pthread_mutex_lock(&can_upgrade_mutex);
    if (pkgaddr)
    {
        free(pkgaddr);
        pkgaddr = NULL;
        pkgptr = NULL;
    }

    ctrl_state = CAN_UPGRADE_UNINIT_STATE;
    pkg_total_size = 0;
    pthread_mutex_unlock(&can_upgrade_mutex);

    return ret;
}
