/**
 * @file         test_can_upgrade.h
 *
 * @brief
 */

#ifndef TEST_CAN_UPGRADE_H
#define TEST_CAN_UPGRADE_H

typedef enum{
    CAN_UPGRADE_UNINIT_STATE = 0,
    CAN_UPGRADE_NORMAL_STATE,
    CAN_UPGRADE_DONE_STATE,
}CAN_UPGRADE_STATE;

int UDS_CAN_Upgrade_Init(int memory_size);
int UDS_CAN_Upgrade_StoreData(uint8_t* data, int len, int remaining_size);
int UDS_CAN_Get_Upgrade_State();
void UDS_CAN_Set_Upgrade_State(CAN_UPGRADE_STATE state);
uint8_t* UDS_CAN_Upgrade_GetPkgAddr();
int UDS_CAN_Upgrade_GetPkgSize();
int UDS_CAN_Upgrade_Exit();

#endif /* TEST_CAN_UPGRADE_H */
