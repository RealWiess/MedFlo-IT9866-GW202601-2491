/**
 * @file         User Config.h
 *
 * @brief
 */

#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#define BIT_ADDRESS_FORMAT_11                1
#define BIT_ADDRESS_FORMAT_29                0

#if BIT_ADDRESS_FORMAT_11
#define PHYSICAL_ECU_ADDRESS                 0x7E0
#define FUNCTIONAL_ECU_ADDRESS               0x7DF
#define DESTINATION_ECU_ADDRESS              0x7E8
#define TARGET_TYPE                          0x01
#elif   BIT_ADDRESS_FORMAT_29
#define PHYSICAL_ECU_ADDRESS                 0x18DAF110
#define FUNCTIONAL_ECU_ADDRESS               0x18DBF133
#define DESTINATION_ECU_ADDRESS              0x18DA10F1
#define TARGET_TYPE                          0x05
#endif

unsigned int SID_Function_Disable(uint8_t sid);
unsigned int GET_PHYSICAL_ECU_ADDRESS();
unsigned int GET_FUNCTIONAL_ECU_ADDRESS();
unsigned int GET_DESTINATION_ECU_ADDRESS();
unsigned char GET_TARGET_TYPE();
unsigned char GET_ISOTP_PADDING_BYTE();

#endif /* USER_CONFIG_H */
