/**
 * @file        UDS Server Implement function.
 *
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "ite/itp.h"
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "uds_tp.h"
#include "server_implement.h"
#include "uds_server.h"
#include "errornos.h"
#include "test_can_upgrade.h"

uint8_t ECU_STATE = DEFAULT_DIAGNOSTIC_SESSION;
volatile int routine_status = 0;
int arr[MAX_DATA_IDS] = {0};
int dynamic_id_count;

/*diagnostic sesson control session structures*/
default_session dflt_sesn;
ext_diagn_session ext_diag_sessn;
progra_session program_sessn;
saftey_diag_session saftey_diag_sessn;

typedef struct dynamic_id
{
    uint16_t def_data_id;
    uint16_t src_id[MAX_SOURCE_ID];
    uint8_t pos[MAX_SOURCE_ID];
    uint8_t mem_size[MAX_SOURCE_ID];
    uint8_t data_record[MAX_DATA_BYTES];
} dynamic_data;
dynamic_data dyn_def_data[MAX_DYNAMIC_DATA_ID];

typedef struct data_id
{
    uint16_t read_data_id[MAX_READ_DATA_ID];
    uint16_t dynamic_id[MAX_DYNAMIC_DATA_ID];
} read_dataid;
read_dataid read_data_id;

uint8_t default_seed[NUMBER_OF_SEEDS] = {0x36, 0x57};

/**
 * @brief [SID 0x10]This API sets voltage and temperature level requirement for the Hardware device.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int system_requirement_conditions_proper()
{
    int ret = SUCCESS;
    /*voltage level*/

    /*current level*/

    /*temperature level*/
    return ret;
}

/**
 * @brief [SID 0x10]This API get ECU seesion mode
 *
 * @param None
 * @return ECU STATE
 */
uint8_t get_ECU_session()
{
    return ECU_STATE;
}

/**
 * @brief [SID 0x10]This API sets ECU to default session mode.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int set_ECU_to_default_session()
{
    int ret = SUCCESS;
    ECU_STATE = DEFAULT_DIAGNOSTIC_SESSION;
    dflt_sesn.TESTER_PRESENT = 1;
    dflt_sesn.ECU_RESET = 1;
    return ret;
}

/**
 * @brief [SID 0x10]This API sets ECU to programming session mode.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int set_ECU_to_programming_session()
{
    int ret = SUCCESS;
    program_sessn.RESPONSE_ON_EVENT = 1;
    program_sessn.CONTROL_DTC_SETTINGS = 1;
    ECU_STATE = PROGRAMMING_SESSION;
    return ret;
}

/**
 * @brief [SID 0x10]This API sets ECU to extended diagnostic session mode.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int set_ECU_to_extended_diagnostic_session()
{
    int ret = SUCCESS;
    ext_diag_sessn.CONTROL_DTC_SETTINGS = 1;
    ECU_STATE = EXTENDED_DIAGNOSTIC_SESSION;
    return ret;
}

/**
 * @brief [SID 0x10]This API sets ECU to extended diagnostic session mode.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int set_ECU_to_system_saftey_diagnostic_session()
{
    int ret = SUCCESS;
    saftey_diag_sessn.CONTROL_DTC_SETTINGS = 1;
    saftey_diag_sessn.SECURE_DATA_TRANSMISSION = 1;
    ECU_STATE = SYSTEM_SAFTEY_DIAGNOSTIC_SESSION;
    return ret;
}

/**
 * @brief [SID 0x10]This API sets ECU to system safety diagnostic session mode.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int set_ECU_to_vehicle_specific_session()
{
    int ret = SUCCESS;
    ECU_STATE = VEHICLE_MANUFACTURER_SPECIFIC_SESSION;
    return ret;
}

/**
 * @brief [SID 0x11]This API is used to do hard reset in ECU.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int start_ECU_hard_reset()
{
    int ret = SUCCESS;
    /* call the API to do hard reset the ECU*/
    return ret;
}

/**
 * @brief [SID 0x11]This API is used to do Key OFF and Key ON in ECU.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int start_ECU_key_off_on_reset()
{
    int ret = SUCCESS;
    /* call the API to do key on off reset the ECU*/
    return ret;
}

/**
 * @brief [SID 0x11]This API is used to do soft reset in ECU.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int start_ECU_soft_reset()
{
    int ret = SUCCESS;
    /* call the API to do soft reset the ECU*/
    return ret;
}

/**
 * @brief [SID 0x11]This API is used to do enable rapid power shutdown in ECU.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int start_ECU_enable_rapid_power_shutdown()
{
    int ret = SUCCESS;
    /* call the API to do enable rapid power shutdown the ECU*/
    return ret;
}

/**
 * @brief [SID 0x11]This API is used to do disable rapid power shutdown in ECU.
 *
 * @param None
 * @return ret Success 0, Failure -1
 */
int start_ECU_disable_rapid_power_shutdown()
{
    int ret = SUCCESS;
    /* call the API to do disable rapid power shutdown the ECU*/
    return ret;
}

/**
 * @brief This API is local function. API to collect the data for the read data by identifier.
 *
 * @param data_id
 * @param data_record
 * @return ret Success 0, Failure -1
 */
int collect_and_update_the_data_record_for_data_id(uint16_t data_id, uint8_t *data_record)
{
    int *ret_data = NULL;
    int ret = 0;
    int id_len = 0;
    int src_len = 0;
    int i = 0;
    int j = 0;
    int n = 0;
    int temp = 0;
    uint8_t mem_size_temp = 0;
    uint8_t *data_bytes[MAX_SOURCE_ID] = {0x00};
    while (id_len < MAX_DYNAMIC_DATA_ID)
    {
        if (dyn_def_data[id_len].def_data_id == data_id)
        {
            while ((src_len < MAX_SOURCE_ID) && (dyn_def_data[id_len].src_id[src_len] != (uint8_t)0x00))
            {
                for (i = 0; i < MAX_SOURCE_ID; i++)
                {
                    data_bytes[i] = (uint8_t *)pvPortMalloc((uint8_t)MAX_DYNAMIC_DATA_BYTES * sizeof(uint8_t));
                }
                ret_data = read_the_data_by_identifier(&dyn_def_data[id_len].src_id[src_len], 1, data_bytes); /*MISRA C violated */
                if (*ret_data != 0)
                {

                    n = 1;

                    mem_size_temp = dyn_def_data[id_len].mem_size[src_len];
                    for (i = 0; i <= (int)mem_size_temp; i++)
                    {

                        temp = dyn_def_data[id_len].pos[src_len] + (uint64_t)n;
                        data_record[j] = data_bytes[0][temp];
                        j++;
                        n++;
                    }
                    ret = ret + j;
                }
                src_len++;
            }
            break;
        }
        else
        {
            id_len++;
        }
    }

    return ret;
}

/**
 * @brief [SID 0x22, SID 0x2A]This API is used to request data record values from the server identified by one or more data Identifiers.
 *
 * @param data_id Holds the data identifier.
 * @param data_id_len Length of the data identifier.
 * @param resp_data Store the response.
 * @return Return integer pointer array which holds response data length of each data identifier.
 */
int *read_the_data_by_identifier(uint16_t *dataid, int data_id_len, uint8_t **resp_data)
{
    int ret = SUCCESS;
    uint16_t ffzero = 0xFF00;
    uint16_t zeroff = 0x00FF;
    int i = 0;
    int j = 0;
    int k = 0;
    int id = 0;
    int n = 0;
    int tempdata_id_len = 0;
    uint8_t *data_record = NULL;
    tempdata_id_len = data_id_len;

    (void)memset(arr, 0x0, MAX_DATA_IDS);
    /*call API to collect the parameter using data_ids and store the response in resp_data and return the success value*/
    while (tempdata_id_len != 0)
    {
        j = 0;
        switch (dataid[i])
        {
        case 0x010A:
            resp_data[i][j] = (uint8_t)((dataid[k] & ffzero) >> 8);
            j++;
            resp_data[i][j] = (uint8_t)(dataid[k] & zeroff);
            j++;
            resp_data[i][j] = 0x31;
            j++;
            resp_data[i][j] = 0x32;
            j++;
            resp_data[i][j] = 0x33;
            j++;
            arr[i] = j;
            break;
        case 0xF190:

            resp_data[i][j] = (uint8_t)((dataid[k] & ffzero) >> 8);

            j++;
            resp_data[i][j] = (uint8_t)(dataid[k] & zeroff);

            j++;
            resp_data[i][j] = 0x31;

            j++;
            resp_data[i][j] = 0x32;

            j++;
            resp_data[i][j] = 0x41;

            j++;
            resp_data[i][j] = 0x42;

            j++;
            resp_data[i][j] = 0x43;

            j++;
            resp_data[i][j] = 0x44;

            j++;
            resp_data[i][j] = 0x41;

            j++;
            resp_data[i][j] = 0x42;

            j++;
            resp_data[i][j] = 0x43;

            j++;
            resp_data[i][j] = 0x44;

            j++;
            resp_data[i][j] = 0x45;

            j++;
            arr[i] = j;
            break;
        case 0xFD00:
            j = 0x0;
            resp_data[i][j] = (uint8_t)((dataid[k] & ffzero) >> 8);

            j++;
            resp_data[i][j] = (uint8_t)(dataid[k] & zeroff);

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            resp_data[i][j] = (uint8_t)0x01;

            j++;
            resp_data[i][j] = (uint8_t)0x02;

            j++;
            resp_data[i][j] = (uint8_t)0x03;

            j++;
            resp_data[i][j] = (uint8_t)0x04;

            j++;
            resp_data[i][j] = (uint8_t)0x05;

            j++;
            resp_data[i][j] = (uint8_t)0x06;

            j++;
            resp_data[i][j] = (uint8_t)0x07;

            j++;
            resp_data[i][j] = (uint8_t)0x08;

            j++;
            resp_data[i][j] = (uint8_t)0x09;

            j++;
            resp_data[i][j] = (uint8_t)0x10;

            j++;
            resp_data[i][j] = (uint8_t)0x11;

            j++;
            resp_data[i][j] = (uint8_t)0x12;

            j++;
            resp_data[i][j] = (uint8_t)0x13;

            j++;
            resp_data[i][j] = (uint8_t)0x14;

            j++;
            resp_data[i][j] = (uint8_t)0x15;

            j++;
            arr[i] = j;
            break;

        case 0xF2E3:
            resp_data[i][j] = (uint8_t)((dataid[k] & ffzero) >> 8);

            j++;
            resp_data[i][j] = (uint8_t)(dataid[k] & zeroff);

            j++;
            resp_data[i][j] = (uint8_t)0x31;

            j++;
            resp_data[i][j] = (uint8_t)0x32;

            j++;
            resp_data[i][j] = (uint8_t)0x41;

            j++;
            resp_data[i][j] = (uint8_t)0x42;

            j++;
            resp_data[i][j] = (uint8_t)0x43;

            j++;
            resp_data[i][j] = (uint8_t)0x44;

            j++;
            resp_data[i][j] = (uint8_t)0x41;

            j++;
            resp_data[i][j] = (uint8_t)0x42;

            j++;
            resp_data[i][j] = (uint8_t)0x43;

            j++;
            resp_data[i][j] = (uint8_t)0x44;

            j++;
            resp_data[i][j] = (uint8_t)0x45;

            j++;
            arr[i] = j;
            break;
        default:
            id = 0;

            while (id != MAX_DYNAMIC_DATA_ID)
            {
                if (dataid[k] == read_data_id.dynamic_id[id])
                {

                    data_record = (uint8_t *)pvPortMalloc((uint8_t)MAX_DYNAMIC_DATA_BYTES * sizeof(uint8_t));
                    ret = collect_and_update_the_data_record_for_data_id(read_data_id.dynamic_id[id], data_record); /*MISRA C violated */
                    if (ret != E_SUCCESS)
                    {

                        resp_data[i][j] = (read_data_id.dynamic_id[id] & ffzero) >> 8;
                        j++;
                        resp_data[i][j] = (read_data_id.dynamic_id[id] & zeroff);
                        j++;

                        while (ret > 0)
                        {

                            resp_data[i][j] = data_record[n];
                            j++;
                            n++;
                            ret--;
                        }

                        arr[i] = j;
                    }
                    break;
                }
                else
                {
                    id++;
                }
                if (data_record != NULL)
                {
                    vPortFree(data_record);
                    data_record = NULL;
                }
            }
            break;
        }
        tempdata_id_len--;
        k++;
        i++;
    }

    arr[i] = ret;
    for (i = 0; i < 1; i++)
    {
        for (j = 0; j < arr[i]; j++)
        {
            printf("%x, ", resp_data[i][j]);
        }
    }
    return arr;
}

/**
 * @brief [SID 0x22 ,0x31 ,0x2E ,0x2F]This API is used to configure the data ID into active session
 *
 * @param data_id Holds the data identifier
 * @return ret Success 0, Failure -1
 */
int check_the_data_id_supported_in_active_session(uint16_t data_id)
{
    int ret = SUCCESS;
    if (data_id == (uint8_t)0)
    {
    }

    /* API is used to configure the data ids into different id's and return success or failure from API*/
    return ret;
}

/**
 * @brief [SID 0x31]This API is used to start the routine. If routine is already started then restart routine or send negative response.
 *
 * @param routine_id Give the routine identifier send by client request.
 * @param routine_record Give the routine record for routine control request send by client.
 * @param routine_len Give the length of routine record send by the client.
 * @param routine_status_rec Store the routine status record.
 * @param routine_status_len Store the total length of routine_status_rec.
 * @return ret Success 0 Request Sequence Fail -3 Request Fail -2
 */
int process_start_routine_sub_function(uint16_t routine_id, uint8_t *routine_record, int routine_len, uint8_t *routine_status_rec, int *routine_status_len)
{
    int ret = SUCCESS;

    if (routine_len == 0)
    {
    }
    if (routine_id == (uint8_t)0)
    {
    }
    if (routine_record[0] == (uint8_t)0)
    {
    }
    if (routine_status_len[0] == (int)0)
    {
    }
    if (routine_status_rec[0] == (uint8_t)0)
    {
    }

    /* API is used to start the routine. if routine is already started then restart routine or send negative response,  return success or failure from API*/
    switch (routine_id)
    {
    case 0x0201:
        if (routine_status != 1)
        {
            routine_status = 1;
        }
        else
        {
            ret = E_REQ_SEQ_FAIL;
        }
        break;
    default:
        ret = E_REQ_FAIL;
        break;
    }
    return ret;
}

/**
 * @brief [SID 0x31]This API is used to stop the routine. If routine is not started send negative response.
 *
 * @param routine_id Give the routine identifier send by client request.
 * @param routine_record Give the routine record for routine control request send by client.
 * @param routine_len Give the length of routine record send by the client.
 * @param routine_status_rec Store the routine status record.
 * @param routine_status_len Store the total length of routine_status_rec.
 * @return ret Success 0 Request Sequence Fail -3 Request Fail -2
 */
int process_stop_routine_sub_function(uint16_t routine_id, uint8_t *routine_record, int routine_len, uint8_t *routine_status_rec, int *routine_status_len)
{
    int ret = SUCCESS;
    if (routine_len == 0)
    {
    }
    if (routine_status_rec[0] == (uint8_t)0)
    {
    }
    if (routine_status_len[0] == (int)0)
    {
    }
    if (routine_record[0] == (uint8_t)0)
    {
    }

    /* API is used to stop the routine. if routine is not started then send negative response,  return success or failure from API*/
    switch (routine_id)
    {
    case 0x0201:
        if (routine_status == 1)
        {
            routine_status = 0;
        }
        else
        {
            ret = E_REQ_SEQ_FAIL;
        }
        break;
    default:
        ret = E_REQ_FAIL;
        break;
    }
    return ret;
}

/**
 * @brief [SID 0x31]This API is used by the client to request results referenced by a routine identifier
 *
 * @param routine_id Give the routine identifier send by client request.
 * @param routine_record Give the routine record for routine control request send by client.
 * @param routine_len Give the length of routine record send by the client.
 * @param routine_status_rec Store the routine status record.
 * @param routine_status_len Store the total length of routine_status_rec.
 * @return ret Success 0 Request Sequence Fail -3 Request Fail -2
 */
int process_request_routine_results_sub_function(uint16_t routine_id, uint8_t *routine_record, int routine_len, uint8_t *routine_status_rec, int *routine_status_len)
{
    int ret = SUCCESS;

    if (routine_len == 0)
    {
    }
    if (routine_status_rec[0] == (uint8_t)0)
    {
    }
    if (routine_status_len[0] == (int)0)
    {
    }
    if (routine_record[0] == (uint8_t)0)
    {
    }

    /* API is used to stop the routine. if routine is not started then send negative response,  return success or failure from API*/
    switch (routine_id)
    {
    case 0x0201:
        if (routine_status == 1)
        {
            routine_status = 0;
        }
        else
        {
            ret = E_REQ_SEQ_FAIL;
        }
        break;
    default:
        ret = E_REQ_FAIL;
        break;
    }
    return ret;
}

/**
 * @brief [SID 0x27]This API is used to provide the seed values to the client application to unlock the ECU.
 *
 * @param seed_data Used to store the seed bytes and return to the application.
 * @param no_of_seed  Provide the maximum number of seed bytes.
 * @return ret Success 0 Failure -1
 */
int send_seed_data_bytes_to_client(uint8_t *seed_data, int no_of_seed)
{
    int ret = SUCCESS;
    long currenttime = 0;
    if (no_of_seed == 0)
    {
    }
    if (seed_data[0] == (uint8_t)0)
    {
    }

    /* API is used to generate or assign the seed value to unlock value,  return success or failure from API*/
    seed_data[0] = 0x36;
    seed_data[1] = 0x57;


    return ret;
}

/**
 * @brief [SID 0x27]Generate Key
 *
 * @param key_bytes store the key bytes
 * @param seed_bytes store the seed bytes
 * @return None
 */
void generate_security_key(uint8_t *key_bytes, uint8_t *seed_bytes)
{
    int i = 0;
    while (i < NUMBER_OF_SEEDS)
    {
        key_bytes[i] = ~(seed_bytes[i]);
        i++;
    }
    key_bytes[i - 1] = key_bytes[i - 1] + (uint8_t)1;

    // printf("key = %x,%x\n", key_bytes[0], key_bytes[1]);
}

/**
 * @brief [SID 0x34]This API is used to request upload/download service configuration and the memory details to read/write data into particular memory address.
 *
 * @param len_formt_id  Holds the values of length format identifier. Update the values for "len_formt_id" send back the API.
 * @param max_number_of_blk_len Holds the maximum block length of data to share in the service transfer data. Update the values of the maximum "max_number_of_blk_len" send back the API
 * @return ret Success 0 Failure -1
 */
int process_request_upload_download_service(uint8_t *len_formt_id, uint8_t *max_number_of_blk_len)
{
    int ret = SUCCESS;
    uint8_t length = 0x0;
    uint64_t block_length = 0x0005;
    uint64_t ff = 0xFF;
    /* API is used to configure the length format identifier and maximum number of block length for response of request upload/download service
       this parameter is a one Byte value with each nibble encoded separately:
       — bit 7 to 4: Length (number of bytes) of the maxNumberOfBlockLength parameter;
       — bit 3 to 0: reserved by document, to be set to 0 16 ;
       */
    *len_formt_id = LENGTH_FORMAT_IDENTIFIER;
    length = ((*len_formt_id & (uint8_t)0xF0) >> 4);
    max_number_of_blk_len[0] = ((block_length >> 8) & ff);
    max_number_of_blk_len[1] = (block_length & ff);

    return ret;
}

/**
 * @brief [SID 0x36]This API is used to download and update the data into the memory location send by client.
 *
 * @param start_memory_addr  Holds the start of the memory address to rewrite the data. This memory address is updated using transfer download request sent by client.
 * @param memory_size_length Holding the value of total size of memory to rewrite by data.
 * @param transfer_rec Holding the data transferred by the client to rewrite the memory.
 * @param rec_len Maximum data length send by the client.
 * @return ret Success 0 Failure -1
 */
int transfer_data_from_client_to_server(uint64_t start_memory_addr, int memory_size_length, uint8_t *transfer_rec, int rec_len)
{
    int ret = SUCCESS;

    if (rec_len == 0)
    {
        return -1;
    }

    if (memory_size_length == 0)
    {
        return -1;
    }

    if (start_memory_addr == (uint64_t)0)
    {
    }

#if 0
    for(int i=0; i < rec_len; i++)
    {
        printf("data 0x%d ", transfer_rec[i]);
    }
#endif
#ifdef CFG_UPGRADE_FROM_CAN
    if (UDS_CAN_Get_Upgrade_State() == CAN_UPGRADE_UNINIT_STATE)
    {
        UDS_CAN_Upgrade_Init(memory_size_length);
        UDS_CAN_Upgrade_StoreData(transfer_rec, rec_len, memory_size_length);
    }
    else if (UDS_CAN_Get_Upgrade_State() == CAN_UPGRADE_NORMAL_STATE)
    {
        UDS_CAN_Upgrade_StoreData(transfer_rec, rec_len, memory_size_length);
    }
#endif
    printf("memory_size_length = 0x%x\n", memory_size_length);

    /*use the memory address and memory size write the trasfer record data into memory location*/
    return ret;
}

/**
 * @brief [SID 0x36] This API is used to read the data from the memory location and send to client.
 *
 * @param start_memory_addr Holding the start of the memory address to rewrite the data. This memory address is updated using transfer download request sent by client.
 * @param memory_size_length Holding the value of total size of memory to rewrite by data.
 * @param transfer_rec Used to store the data read by the memory location.
 * @param rec_len Maximum data length send by the server to client.
 * @return ret Success 0 Failure -1
 */
int transfer_data_from_server_to_client(uint64_t start_memory_addr, int memory_size_length, uint8_t *transfer_rec, int rec_len)
{
    int ret = SUCCESS;
    if (rec_len == 0)
    {
    }
    if (memory_size_length == 0)
    {
    }

    /*use the memory address and memory size
     *  ---read the data from memory location and store it in the trasfer_rec array
     *  ---tarsfer data record length is decide bt rec_len. so trasfer the data size of rec_len
     *  */
    switch (start_memory_addr)
    {
    case 0x2010:
        transfer_rec[0] = 0x31;
        transfer_rec[1] = 0x32;
        transfer_rec[2] = 0x33;
        transfer_rec[3] = 0x34;
        break;
    case 0x201000:
        transfer_rec[0] = 0x31;
        transfer_rec[1] = 0x32;
        transfer_rec[2] = 0x33;
        transfer_rec[3] = 0x34;
        transfer_rec[4] = 0x31;
        transfer_rec[5] = 0x32;
        transfer_rec[6] = 0x33;
        transfer_rec[7] = 0x34;
        transfer_rec[8] = 0x31;
        transfer_rec[9] = 0x32;
        transfer_rec[10] = 0x33;
        transfer_rec[11] = 0x34;
        break;
    default:
        break;
    }
    return ret;
}
/**
 * @brief [SID 0x37] This API is used to notifies the application when the transfer data exit event occurs.
 *
 * @param load_type 0: upload event 1:download event
 * @return ret Success 0 Failure -1
 */
int transfer_data_exit(int exit_type)
{
    int ret = SUCCESS;
#ifdef CFG_UPGRADE_FROM_CAN    
    if(exit_type == 1)
    {
        UDS_CAN_Set_Upgrade_State(CAN_UPGRADE_DONE_STATE);
    }
#endif
    return ret;
}

/*
 * dynamically defined data id: data id to be used to collect the data from the diffrent source id's, also can combine different position data into one data identifier
 * store the values in the structure defined below
 *	typedef struct dynamic_id
 {
 uint16_t def_data_id;
 uint16_t src_id[MAX_SOURCE_ID];
 uint8_t pos[MAX_SOURCE_ID];
 uint8_t mem_size[MAX_SOURCE_ID];
 uint8_t data_record[MAX_DATA_BYTES];
 }dynamic_data;
 */

/**
 * @brief [SID 0x2C Subfunction 0x01]This API is used to when the client needs to read the response a list of pre-defined data identifiers values with another specific identifier
 *
 * @param data_id  Holding the data identifier
 * @param source_id Hold the source ID
 * @param position Holds the positions of first data byte to be read
 * @param memory_size Holds the memory size of how much data bytes to be read for each source ID in array
 * @param data_len Holds the length of "memory_size" and "position "array.
 * @return ret Success 0 Failure -1
 */
int update_the_data_record_for_defined_identifier(uint16_t data_id, uint16_t *source_id, uint8_t *position, uint8_t *memory_size, int data_len)
{
    int ret = SUCCESS;
    int i = 0;
    int temp_datalen = 0;

    temp_datalen = data_len;

    /* data id to be defined for the read data by identifier*/
    dyn_def_data[dynamic_id_count].def_data_id = data_id;
    read_data_id.dynamic_id[dynamic_id_count] = data_id;

    for (i = 0; i < MAX_SOURCE_ID; i++)
    {
        if (i < temp_datalen)
        {
            dyn_def_data[dynamic_id_count].src_id[i] = source_id[i];
        }
        else
        {
            break;
        }
    }
    for (i = 0; i < MAX_SOURCE_ID; i++)
    {
        if (i < temp_datalen)
        {
            dyn_def_data[dynamic_id_count].pos[i] = position[i];
        }
        else
        {
            break;
        }
    }
    for (i = 0; i < MAX_SOURCE_ID; i++)
    {
        if (i < temp_datalen)
        {
            dyn_def_data[dynamic_id_count].mem_size[i] = memory_size[i];
        }
        else
        {
            break;
        }
    }
    dynamic_id_count++;
    return ret;
}

/**
 * @brief [SID 0x2E]This API is used to set the data ID's data record to buffer.
 *
 * @param data_id  Holding the data identifier
 * @param data_len Holds the length of "data_record" array
 * @param data_record Holds the values of data bytes to be written for the data identifier
 * @return ret Success 0 Failure -1
 */
int update_the_data_record_for_data_id(uint16_t data_id, int data_len, uint8_t *data_record)
{
    int ret = SUCCESS;
    int i = 0;
    if (data_id == (uint8_t)0)
    {
    }
    printf("data_id = 0x%x\n",data_id);

    printf("data_len = %d\n",data_len);
    if (data_len == 0)
    {
    }

    if (data_record[0] == (uint8_t)0)
    {
    }

    printf("data_record = [ ");
    for(i = 0; i < data_len ; i++)
    {
        printf("0x%x,",data_record[i]);
    }
    printf("]\n ");

    /* API is used to set the dataid's data record to buffer,  return success or failure from API*/
    return ret;
}

/**
 * @brief [SID 0x14]This API is used to clear the DTC with memory address
 *
 * @param group_of_DTC Holds the group of DTC values
 * @param mem_addr Holds the memory address
 * @return ret Success 0 Failure -1
 */
int clear_diagnostic_information_for_DTC(uint32_t group_of_DTC, uint8_t mem_addr)
{
    int ret = SUCCESS;
    if (mem_addr == (uint8_t)0)
    {
    }
    if (group_of_DTC == (uint32_t)0)
    {
    }
    else
    {
        printf("group_of_DTC = %x\n", group_of_DTC);
    }

    /*
     * call API to clear the the DTC with memory address
     */
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x01]This API is used to update the Read DTC information
 *
 * @param dtc_status_mask Holds the DTC status mask
 * @param dtc_status_availability_mask Holds the DTC status availability mask
 * @param dtc_format_identfier Holds the DTC format identifier
 * @param dtc_count Holds the count of the DTC
 * @return ret Success 0 Failure -1
 */
int process_report_number_of_dtc_by_status_mask_request(uint8_t dtc_status_mask, uint8_t *dtc_status_availability_mask, uint8_t *dtc_format_identfier, uint16_t *dtc_count)
{
    int ret = SUCCESS;
    /*
     * API to fill the DTC information to the empty buffer
     */
    switch (dtc_status_mask)
    {
    case 0x08:
        *dtc_status_availability_mask = 0x2F;
        *dtc_format_identfier = 0x01;
        *dtc_count = 0x0001;
        break;
    default:
        ret = -1;

        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x02]This API is used to update the Read DTC information
 *
 * @param dtc_status_mask Holds the DTC status mask
 * @param dtc_status_availability_mask Holds the DTC status availability mask
 * @param dtc_and_status_rec Holds the DTC status Record
 * @param len Holds the length of the "dtc_and_status_rec" array
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_by_status_mask_request(uint8_t dtc_status_mask, uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_status_rec, int *len)
{
    int ret = SUCCESS;
    int i = 0;
    /*
     * API to fill the DTC information to the empty buffer
     */
    switch (dtc_status_mask)
    {
    case 0x84:
        *dtc_status_availability_mask = 0x7F;
        dtc_and_status_rec[i] = 0x31;
        i++;
        dtc_and_status_rec[i] = 0x32;
        i++;
        dtc_and_status_rec[i] = 0x33;
        i++;
        dtc_and_status_rec[i] = 0x34;
        i++;
        dtc_and_status_rec[i] = 0x35;
        i++;
        dtc_and_status_rec[i] = 0x36;
        i++;
        dtc_and_status_rec[i] = 0x37;
        i++;
        dtc_and_status_rec[i] = 0x38;
        i++;
        *len = i;
        break;
    default:
        ret = -1;

        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x03]This API to update the Report DTC Snapshot identification
 *
 * @param dtc_record  Holds the DTC record data array
 * @param dtc_snapshot_rec_number Holds the DTC snapshot record number in array
 * @param snapshot_len Holds the length of "dtc_snapshot_rec_number" array
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_snapshot_identification_request(uint8_t *dtc_record, uint8_t *dtc_snapshot_rec_number, int *snapshot_len)
{
    int ret = SUCCESS;
    int i = 0;
    int j = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    dtc_record[i] = 0x31;
    i++;
    dtc_record[i] = 0x32;
    i++;
    dtc_record[i] = 0x33;
    i++;
    dtc_snapshot_rec_number[j] = 0x41;
    j++;
    dtc_record[i] = 0x34;
    i++;
    dtc_record[i] = 0x35;
    i++;
    dtc_record[i] = 0x36;
    i++;
    dtc_snapshot_rec_number[j] = 0x42;
    j++;
    dtc_record[i] = 0x37;
    i++;
    dtc_record[i] = 0x38;
    i++;
    dtc_record[i] = 0x39;
    i++;
    dtc_snapshot_rec_number[j] = 0x43;
    j++;
    *snapshot_len = j;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x04]This API to update the Report DTC Snapshot record by DTC number
 *
 * @param dtc_mask_record Holds the DTC mask record
 * @param dtc_snapshot_num Holds the DTC Snapshot number
 * @param dtc_and_status_rec Holds the DTC Status record
 * @param dtc_snapshot_record_number Holds the DTC snapshot record number
 * @param dtc_snapshot_record_number_of_identifiers Holds the snapshot number identifier
 * @param dtc_snapshot_record Holds the DTC snapshot record
 * @param dtc_snapshot_record_len Holds the DTC snapshot record length
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_snapshot_record_by_dtc_number_request(uint8_t *dtc_mask_record, uint8_t dtc_snapshot_num, uint8_t *dtc_and_status_rec, uint8_t *dtc_snapshot_record_number, uint8_t *dtc_snapshot_record_number_of_identifiers, uint8_t **dtc_snapshot_record, int *dtc_snapshot_record_len)
{
    int ret = SUCCESS;
    uint8_t two = 0x02;
    uint32_t dtcmaskrec = 0x00;
    int i = 0;
    int j = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    dtcmaskrec = dtc_mask_record[i];
    i++;
    dtcmaskrec = dtcmaskrec << 8;
    dtcmaskrec = dtcmaskrec | dtc_mask_record[i];
    i++;
    dtcmaskrec = dtcmaskrec << 8;
    dtcmaskrec = dtcmaskrec | dtc_mask_record[i];
    i = 0;
    switch (dtcmaskrec)
    {
    case 0x123456:
        if (dtc_snapshot_num == two)
        {
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = 0x24;
            i++;

            /*store snapchat number*/
            /*first set*/
            i = 0;
            dtc_snapshot_record_number[i] = 0x02;
            dtc_snapshot_record_number_of_identifiers[i] = 0x01;

            dtc_snapshot_record[i][j] = 0x89;
            j++;
            dtc_snapshot_record[i][j] = 0x28;
            j++;
            dtc_snapshot_record[i][j] = 0x11;
            j++;
            dtc_snapshot_record[i][j] = 0x16;
            j++;
            dtc_snapshot_record[i][j] = 0x11;
            j++;
            dtc_snapshot_record[i][j] = 0x11;
            j++;
            dtc_snapshot_record[i][j] = 0x11;
            j++;
            dtc_snapshot_record[i][j] = 0x16;
            j++;
            dtc_snapshot_record[i][j] = 0x11;
            j++;
            dtc_snapshot_record_len[i] = j;
            i++;
            dtc_snapshot_record_number[i] = 0x04;
            dtc_snapshot_record_number_of_identifiers[i] = 0x01;
            j = 0;
            dtc_snapshot_record[i][j] = 0x31;
            j++;
            dtc_snapshot_record[i][j] = 0x32;
            j++;
            dtc_snapshot_record[i][j] = 0x33;
            j++;
            dtc_snapshot_record[i][j] = 0x34;
            j++;
            dtc_snapshot_record[i][j] = 0x35;
            j++;
            dtc_snapshot_record[i][j] = 0x36;
            j++;
            dtc_snapshot_record[i][j] = 0x37;
            j++;
            dtc_snapshot_record_len[i] = j;
            i++;
        }
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x05]This API to update the Report DTC stored data by record number
 *
 * @param dtc_stored_rec_no Holds DTC stored record number
 * @param dtc_stored_record_number Holds DTC stored record number array
 * @param DTC_and_status_rec Holds DTC and Status record
 * @param dtc_stored_record_number_of_identifiers Holds DTC stored record number of identifiers
 * @param dtc_stored_data_record Holds DTC stored data record
 * @param dtc_stored_data_record_len Holds DTC stored data record length
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_stored_data_by_record_number(uint8_t dtc_stored_rec_no, uint8_t *dtc_stored_record_number, uint8_t **DTC_and_status_rec, uint8_t *dtc_stored_record_number_of_identifiers, uint8_t **dtc_stored_data_record, int *dtc_stored_data_record_len)
{
    int ret = SUCCESS;
    int i = 0;
    int j = 0;
    /*
     * API to fill the dtc information to the empty buffer*/

    switch (dtc_stored_rec_no)
    {
    case 0x02:
        /*store dtc number*/
        /*first set*/
        j = 0;
        dtc_stored_record_number[i] = 0x02;
        DTC_and_status_rec[i][j] = 0x12;
        j++;
        DTC_and_status_rec[i][j] = 0x34;
        j++;
        DTC_and_status_rec[i][j] = 0x56;
        j++;
        DTC_and_status_rec[i][j] = 0x24;
        j++;

        dtc_stored_record_number_of_identifiers[i] = 0x01;
        j = 0;
        dtc_stored_data_record[i][j] = 0x31;
        j++;
        dtc_stored_data_record[i][j] = 0x32;
        j++;
        dtc_stored_data_record[i][j] = 0x33;
        j++;
        dtc_stored_data_record[i][j] = 0x34;
        j++;
        dtc_stored_data_record[i][j] = 0x35;
        j++;
        dtc_stored_data_record[i][j] = 0x36;
        j++;
        dtc_stored_data_record[i][j] = 0x37;
        j++;
        dtc_stored_data_record_len[i] = j;
        i++;
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

/**
 * @brief [0x19 Subfunction 0x06]This API to update the Report DTC extended DTC stored data by record number
 *
 * @param dtc_mask_record Holds the DTC stored number
 * @param ext_data_num Holds the DTC stored record number array
 * @param dtc_and_status_rec Holds the 2D array of DTC and status record
 * @param dtc_ext_data_record_number Holds the dtc stored record number of data identifiers
 * @param dtc_ext_data_record Holds the 2D array of dtc stored data record
 * @param dtc_ext_data_record_len Holds the dtc stored data record length
 * @return ret Success 0 Failure -1
 */
int process_report_extended_dtc_stored_data_by_record_number(uint8_t *dtc_mask_record, uint8_t ext_data_num, uint8_t *dtc_and_status_rec, uint8_t *dtc_ext_data_record_number, uint8_t **dtc_ext_data_record, int *dtc_ext_data_record_len)
{
    int ret = SUCCESS;
    uint8_t ff = 0xFF;
    /*
     * API to fill the dtc information to the empty buffer
     */
    uint32_t dtcmaskrec = 0x00;
    int i = 0;
    int j = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    dtcmaskrec = dtc_mask_record[i];
    i++;
    dtcmaskrec = dtcmaskrec << 8;
    dtcmaskrec = dtcmaskrec | dtc_mask_record[i];
    i++;
    dtcmaskrec = dtcmaskrec << 8;
    dtcmaskrec = dtcmaskrec | dtc_mask_record[i];
    i = 0;
    switch (dtcmaskrec)
    {
    case 0x123456:
        if (ext_data_num == ff)
        {
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = 0x24;
            i++;
            i = 0;
            j = 0;
            /*first set*/
            dtc_ext_data_record_number[i] = 0x05;
            i++;
            dtc_ext_data_record[i][j] = 0x17;
            j++;
            dtc_ext_data_record_len[i] = j;
            /*second set*/
            i++;
            j = 0;
            dtc_ext_data_record_number[i] = 0x10;
            dtc_ext_data_record[i][j] = 0x79;
            j++;
            dtc_ext_data_record_len[i] = j;
        }
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x07]This API to update the Report number of DTC by severity mask record
 *
 * @param dtc_severity_mask_record Holds the DTC severity mask record
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_format_identfier  Holds the dtc format identifier
 * @param dtc_count Holds the dtc count
 * @return ret Success 0 Failure -1
 */
int process_report_number_of_dtc_by_severity_mask_record(uint8_t *dtc_severity_mask_record, uint8_t *dtc_status_availability_mask, uint8_t *dtc_format_identfier, uint16_t *dtc_count)
{
    int ret = SUCCESS;
    uint32_t dtcmask = 0x0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    dtcmask = dtc_severity_mask_record[0];
    dtcmask = dtcmask << 8;
    dtcmask = dtcmask | dtc_severity_mask_record[1];
    switch (dtcmask)
    {
    case 0xC001:
        *dtc_status_availability_mask = 0x2F;
        *dtc_format_identfier = 0x01;
        *dtc_count = 0x0001;
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x08]This API to update the Report DTC by severity mask record
 *
 * @param dtc_severity_mask_record Holds the DTC severity mask record
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_severity_record Holds the dtc and serverity record
 * @param severity_record_len Holds the length of "dtc_and_severity_record" array
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_by_severity_mask_record(uint8_t *dtc_severity_mask_record, uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_severity_record, int *severity_record_len)
{
    int ret = SUCCESS;
    uint32_t dtcmask = 0x0;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    dtcmask = dtc_severity_mask_record[0];
    dtcmask = dtcmask << 8;
    dtcmask = dtcmask | dtc_severity_mask_record[1];
    switch (dtcmask)
    {
    case 0xC001:
        *dtc_status_availability_mask = 0x2F;
        dtc_and_severity_record[i] = 0x31;
        i++;
        dtc_and_severity_record[i] = 0x32;
        i++;
        dtc_and_severity_record[i] = 0x33;
        i++;
        dtc_and_severity_record[i] = 0x34;
        i++;
        dtc_and_severity_record[i] = 0x35;
        i++;
        dtc_and_severity_record[i] = 0x36;
        i++;
        dtc_and_severity_record[i] = 0x41;
        i++;
        dtc_and_severity_record[i] = 0x42;
        i++;
        dtc_and_severity_record[i] = 0x43;
        i++;
        dtc_and_severity_record[i] = 0x44;
        i++;
        dtc_and_severity_record[i] = 0x45;
        i++;
        dtc_and_severity_record[i] = 0x46;
        i++;
        *severity_record_len = i;
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x09]This API to update the Report severity information mask record
 *
 * @param dtc_mask_record Holds the record of the DTC mask array
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_severity_record Holds the dtc and serverity record
 * @param severity_record_len Holds the length of "dtc_and_severity_record" array
 * @return ret Success 0 Failure -1
 */
int process_report_severity_information_of_dtc(uint8_t *dtc_mask_record, uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_severity_record, int *severity_record_len)
{
    int ret = SUCCESS;
    if (dtc_mask_record[0] == (uint8_t)0)
    {
    }
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */

    *dtc_status_availability_mask = 0x2F;
    dtc_and_severity_record[i] = 0x31;
    i++;
    dtc_and_severity_record[i] = 0x32;
    i++;
    dtc_and_severity_record[i] = 0x33;
    i++;
    dtc_and_severity_record[i] = 0x34;
    i++;
    dtc_and_severity_record[i] = 0x35;
    i++;
    dtc_and_severity_record[i] = 0x36;
    i++;
    dtc_and_severity_record[i] = 0x41;
    i++;
    dtc_and_severity_record[i] = 0x42;
    i++;
    dtc_and_severity_record[i] = 0x43;
    i++;
    dtc_and_severity_record[i] = 0x44;
    i++;
    dtc_and_severity_record[i] = 0x45;
    i++;
    dtc_and_severity_record[i] = 0x46;
    i++;
    *severity_record_len = i;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x0A]This API to update the Report supported DTC.
 *
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_status_rec Holds the array of dtc and status record
 * @param len Holds the length of dtc and status record array
 * @return ret Success 0 Failure -1
 */
int process_report_supported_dtc(uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_status_rec, int *len)
{
    int ret = SUCCESS;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    *dtc_status_availability_mask = 0x24;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    *len = i;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x0B]This API to update the Report first test failed DTC.
 *
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_status_rec Holds the array of dtc and status record
 * @param len Holds the length of dtc and status record array
 * @return ret Success 0 Failure -1
 */
int process_report_first_test_failed_dtc(uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_status_rec, int *len)
{
    int ret = SUCCESS;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    *dtc_status_availability_mask = 0x24;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    *len = i;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x0C]This API to update the Report first confirmed DTC.
 *
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_status_rec Holds the array of dtc and status record
 * @param len Holds the length of dtc and status record array
 * @return ret Success 0 Failure -1
 */
int process_report_first_confirmed_dtc(uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_status_rec, int *len)
{
    int ret = SUCCESS;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    *dtc_status_availability_mask = 0x24;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    *len = i;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x0D]This API to update the Report most recent failed DTC
 *
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_status_rec Holds the array of dtc and status record
 * @param len Holds the length of dtc and status record array
 * @return ret Success 0 Failure -1
 */
int process_report_most_recent_failed_dtc(uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_status_rec, int *len)
{
    int ret = SUCCESS;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    *dtc_status_availability_mask = 0x24;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    *len = i;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x0E]This API to update the Report most recent confirmed dtc
 *
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_status_rec Holds the array of dtc and status record
 * @param len Holds the length of dtc and status record array
 * @return ret Success 0 Failure -1
 */
int process_report_most_recent_confirmed_dtc(uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_status_rec, int *len)
{
    int ret = SUCCESS;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    *dtc_status_availability_mask = 0x24;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    *len = i;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x14]This API to update the Report DTC with permanent status.
 *
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_status_rec Holds the array of dtc and status record
 * @param len Holds the length of dtc and status record array
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_with_permanent_status(uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_status_rec, int *len)
{
    int ret = SUCCESS;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    *dtc_status_availability_mask = 0x24;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    dtc_and_status_rec[i] = 0x31;
    i++;
    dtc_and_status_rec[i] = 0x32;
    i++;
    dtc_and_status_rec[i] = 0x33;
    i++;
    dtc_and_status_rec[i] = 0x34;
    i++;
    *len = i;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x15]The API is used to process report dtc fault detection counter
 *
 * @param dtc_fault_detection_counter_record Holds the dtc fault detection counter array
 * @param len Holds the length of the "dtc_fault_detection_counter_record" array
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_fault_detection_counter(uint8_t *dtc_fault_detection_counter_record, int *len)
{
    int ret = SUCCESS;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    dtc_fault_detection_counter_record[i] = 0x31;
    i++;
    dtc_fault_detection_counter_record[i] = 0x32;
    i++;
    dtc_fault_detection_counter_record[i] = 0x33;
    i++;
    dtc_fault_detection_counter_record[i] = 0x34;
    i++;
    dtc_fault_detection_counter_record[i] = 0x31;
    i++;
    dtc_fault_detection_counter_record[i] = 0x32;
    i++;
    dtc_fault_detection_counter_record[i] = 0x33;
    i++;
    dtc_fault_detection_counter_record[i] = 0x34;
    i++;
    dtc_fault_detection_counter_record[i] = 0x31;
    i++;
    dtc_fault_detection_counter_record[i] = 0x32;
    i++;
    dtc_fault_detection_counter_record[i] = 0x33;
    i++;
    dtc_fault_detection_counter_record[i] = 0x34;
    i++;
    dtc_fault_detection_counter_record[i] = 0x31;
    i++;
    dtc_fault_detection_counter_record[i] = 0x32;
    i++;
    dtc_fault_detection_counter_record[i] = 0x33;
    i++;
    dtc_fault_detection_counter_record[i] = 0x34;
    i++;
    *len = i;
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x16]This API to update the Report DTC extended data record by record number
 *
 * @param record_number Holds the record number
 * @param DTC_and_status_rec  Holds the DTC status record array
 * @param dtc_ext_data_record Holds the extended data record array
 * @param dtc_ext_data_record_len Holds the length of "dtc_ext_data_record" array
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_extended_data_record_by_record_number(uint8_t record_number, uint8_t **DTC_and_status_rec, uint8_t **dtc_ext_data_record, int *dtc_ext_data_record_len)
{
    int ret = SUCCESS;
    int i = 0;
    int j = 0;
    uint8_t five = 0x05;

    printf("\n\rrecord number = %x\n\r", record_number);
    /*
     * API to fill the dtc information to the empty buffer
     */
    if (record_number == five)
    {
        DTC_and_status_rec[i][j] = 0x12;
        j++;
        DTC_and_status_rec[i][j] = 0x34;
        j++;
        DTC_and_status_rec[i][j] = 0x56;
        j++;
        DTC_and_status_rec[i][j] = 0x24;
        j++;

        j = 0;

        dtc_ext_data_record[i][j] = 0x31;
        j++;
        dtc_ext_data_record[i][j] = 0x32;
        j++;
        dtc_ext_data_record[i][j] = 0x33;
        j++;

        dtc_ext_data_record_len[i] = j;

        i++;
        j = 0;

        DTC_and_status_rec[i][j] = 0x12;
        j++;
        DTC_and_status_rec[i][j] = 0x34;
        j++;
        DTC_and_status_rec[i][j] = 0x56;
        j++;
        DTC_and_status_rec[i][j] = 0x24;
        j++;

        j = 0;

        dtc_ext_data_record[i][j] = 0x41;
        j++;
        dtc_ext_data_record[i][j] = 0x42;
        j++;
        dtc_ext_data_record[i][j] = 0x43;
        j++;

        dtc_ext_data_record_len[i] = j;
    }
    else
    {
        ret = -1;
    }

    return ret;
}

/**
 * @brief [0x19 Subfunction 0x17]This API to update the Report user defined memory DTC by status mask
 *
 * @param dtc_status_mask Holds the dtc status mask
 * @param mem_selection Holds the memory section by the dtc mask
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_and_status_rec Holds the dtc status record mask array
 * @param dtc_record_len Holds the length of the "dtc_and_status_rec" array
 * @return ret Success 0 Failure -1
 */
int process_report_user_defined_memory_dtc_by_status_mask(uint8_t dtc_status_mask, uint8_t mem_selection, uint8_t *dtc_status_availability_mask, uint8_t *dtc_and_status_rec, int *dtc_record_len)
{
    int ret = SUCCESS;
    int i = 0;
    uint8_t twentyfour = 0x24;
    uint8_t ten = 0x10;
    uint8_t tempmem_selection = 0;

    tempmem_selection = mem_selection;

    /*
     * API to fill the dtc information to the empty buffer
     */
    if ((dtc_status_mask == twentyfour) && (tempmem_selection == ten))
    {
        *dtc_status_availability_mask = 0x31;
        dtc_and_status_rec[i] = 0x41;
        i++;
        dtc_and_status_rec[i] = 0x41;
        i++;
        dtc_and_status_rec[i] = 0x42;
        i++;
        dtc_and_status_rec[i] = 0x43;
        i++;
        dtc_and_status_rec[i] = 0x44;
        i++;
        dtc_and_status_rec[i] = 0x45;
        i++;
        dtc_and_status_rec[i] = 0x46;
        i++;
        dtc_and_status_rec[i] = 0x47;
        i++;
        *dtc_record_len = i;
    }
    else
    {
        ret = -1;
    }

    return ret;
}

/**
 * @brief [0x19 Subfunction 0x18]This API to update the Report user defined memory DTC by snapshot record by DTC number
 *
 * @param dtc_mask_record Holds the dtc mask record
 * @param user_defined_snapshot_no Holds the user defined snapshot number
 * @param mem_selection Holds the memory selection
 * @param dtc_and_status_rec Holds the dtc status record
 * @param user_def_dtc_snapshot_record_number Holds the dtc snapshot record number
 * @param dtc_snapshot_record_number_of_identifiers Holds the dtc snapshot record number identifier
 * @param dtc_snapshot_record Holds the dtc snapshot record array
 * @param dtc_snapshot_record_len Holds the length of the "dtc_snapshot_record" array
 * @return ret Success 0 Failure -1
 */
int process_report_user_defined_memory_dtc_snapshot_record_by_dtc_number(uint8_t *dtc_mask_record, uint8_t user_defined_snapshot_no, uint8_t mem_selection, uint8_t *dtc_and_status_rec, uint8_t *user_def_dtc_snapshot_record_number, uint8_t *dtc_snapshot_record_number_of_identifiers, uint8_t **dtc_snapshot_record, int *dtc_snapshot_record_len)
{
    int ret = SUCCESS;

    if (mem_selection == (uint8_t)0)
    {
    }
    uint8_t two = 0x02;
    uint32_t dtcmaskrec = 0x00;
    int i = 0;
    int j = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    dtcmaskrec = dtc_mask_record[i];
    i++;
    dtcmaskrec = dtcmaskrec << 8;
    dtcmaskrec = dtcmaskrec | dtc_mask_record[i];
    i++;
    dtcmaskrec = dtcmaskrec << 8;
    dtcmaskrec = dtcmaskrec | dtc_mask_record[i];
    i = 0;
    switch (dtcmaskrec)
    {
    case 0x123456:
        if (user_defined_snapshot_no == two)
        {
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = 0x24;
            i++;

            /*store snapchat number*/
            /*first set*/
            i = 0;
            user_def_dtc_snapshot_record_number[i] = 0x02;
            dtc_snapshot_record_number_of_identifiers[i] = 0x01;
            dtc_snapshot_record[i][j] = 0x31;
            j++;
            dtc_snapshot_record[i][j] = 0x47;
            j++;
            dtc_snapshot_record[i][j] = 0x11;
            j++;
            dtc_snapshot_record[i][j] = 0xA6;
            j++;
            dtc_snapshot_record[i][j] = 0x66;
            j++;
            dtc_snapshot_record[i][j] = 0x07;
            j++;
            dtc_snapshot_record[i][j] = 0x50;
            j++;
            dtc_snapshot_record[i][j] = 0x20;
            j++;
            dtc_snapshot_record_len[i] = j;
            /*second set*/
            i++;
            user_def_dtc_snapshot_record_number[i] = 0x04;
            dtc_snapshot_record_number_of_identifiers[i] = 0x01;
            j = 0;
            dtc_snapshot_record[i][j] = 0x31;
            j++;
            dtc_snapshot_record[i][j] = 0x32;
            j++;
            dtc_snapshot_record[i][j] = 0x33;
            j++;
            dtc_snapshot_record[i][j] = 0x34;
            j++;
            dtc_snapshot_record[i][j] = 0x35;
            j++;
            dtc_snapshot_record[i][j] = 0x36;
            j++;
            dtc_snapshot_record[i][j] = 0x37;
            j++;
            dtc_snapshot_record_len[i] = j;
            i++;
        }
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x19]This API used to update the Report user defined memory DTC by extended record by DTC number
 *
 * @param dtc_mask_record Holds the dtc mask record
 * @param dtc_ext_data_rec_no Holds the extended data record number
 * @param mem_selection Holds the selection of memory address
 * @param dtc_and_status_rec Holds the dtc status record
 * @param dtc_ext_data_record_number Holds the dtc extended record number
 * @param dtc_ext_data_record Holds the extended data record array
 * @param dtc_ext_data_record_len Holds the length of the "dtc_ext_data_record" array
 * @return ret Success 0 Failure -1
 */
int process_report_user_def_memory_dtc_extended_data_record_by_dtc_number(uint8_t *dtc_mask_record, uint8_t dtc_ext_data_rec_no, uint8_t mem_selection, uint8_t *dtc_and_status_rec, uint8_t *dtc_ext_data_record_number, uint8_t **dtc_ext_data_record, int *dtc_ext_data_record_len)
{
    int ret = SUCCESS;

    if (mem_selection == (uint8_t)0)
    {
    }

    uint8_t ff = 0xFF;
    /*
     * API to fill the dtc information to the empty buffer
     */
    uint32_t dtcmaskrec = 0x00;
    int i = 0;
    int j = 0;

    dtcmaskrec = dtc_mask_record[i];
    i++;
    dtcmaskrec = dtcmaskrec << 8;
    dtcmaskrec = dtcmaskrec | dtc_mask_record[i];
    i++;
    dtcmaskrec = dtcmaskrec << 8;
    dtcmaskrec = dtcmaskrec | dtc_mask_record[i];
    i = 0;
    switch (dtcmaskrec)
    {
    case 0x123456:
        if (dtc_ext_data_rec_no == ff)
        {
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = dtc_mask_record[i];
            i++;
            dtc_and_status_rec[i] = 0x24;
            i++;
            i = 0;
            j = 0;
            /*first set*/
            dtc_ext_data_record_number[i] = 0x05;
            i++;
            dtc_ext_data_record[i][j] = 0x17;
            j++;
            dtc_ext_data_record_len[i] = j;
            /*second set*/
            i++;
            j = 0;
            dtc_ext_data_record_number[i] = 0x10;
            dtc_ext_data_record[i][j] = 0x79;
            j++;
            dtc_ext_data_record_len[i] = j;
        }
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x1A]This API used to update the Report supported DTC extended data record
 *
 * @param dtc_ext_data_rec_num Holds the extended data record number
 * @param dtc_status_availability_mask Holds the dtc status availability mask
 * @param dtc_record_num Holds the dtc record number
 * @param dtc_and_status_rec Holds the dtc status record arry
 * @param dtc_record_len Holds the length of the "dtc_and_status_rec" array
 * @return ret Success 0 Failure -1
 */
int process_report_supported_dtc_extended_data_record(uint8_t dtc_ext_data_rec_num, uint8_t *dtc_status_availability_mask, uint8_t *dtc_record_num, uint8_t *dtc_and_status_rec, int *dtc_record_len)
{
    int ret = SUCCESS;
    int i = 0;
    uint8_t twentyfour = 0x24;
    /*
     * API to fill the dtc information to the empty buffer
     */
    if (dtc_ext_data_rec_num == twentyfour)
    {
        *dtc_status_availability_mask = 0x31;
        *dtc_record_num = 0x32;
        dtc_and_status_rec[i] = 0x41;
        i++;
        dtc_and_status_rec[i] = 0x41;
        i++;
        dtc_and_status_rec[i] = 0x42;
        i++;
        dtc_and_status_rec[i] = 0x43;
        i++;
        dtc_and_status_rec[i] = 0x44;
        i++;
        dtc_and_status_rec[i] = 0x45;
        i++;
        dtc_and_status_rec[i] = 0x46;
        i++;
        dtc_and_status_rec[i] = 0x47;
        i++;
        *dtc_record_len = i;
    }
    else
    {
        ret = -1;
    }

    return ret;
}

/**
 * @brief [0x19 Subfunction 0x42]This API used to update the WWW OBD dtc mask record
 *
 * @param func_id Holds the function ID
 * @param dtc_severity_mask_record Holds the dtc severity mask record
 * @param dtc_status_availability_mask Holds the status availability mask
 * @param dtc_severity_availability_mask Holds the dtc severity availability mask
 * @param dtc_format_identifier Holds the dtc format identifier
 * @param dtc_and_severity_record Holds the dtc severity record array
 * @param severity_record_len Holds the length of the "dtc_and_severity_record" array
 * @return ret Success 0 Failure -1
 */
int process_WWH_OBD_dtc_by_mask_record(uint8_t func_id, uint8_t *dtc_severity_mask_record, uint8_t *dtc_status_availability_mask, uint8_t *dtc_severity_availability_mask, uint8_t *dtc_format_identifier, uint8_t *dtc_and_severity_record, int *severity_record_len)
{
    int ret = SUCCESS;

    if (func_id == (uint8_t)0)
    {
    }
    uint32_t dtcmask = 0x0;
    int i = 0;
    /*
     * API to fill the dtc information to the empty buffer
     */
    dtcmask = dtc_severity_mask_record[0];
    dtcmask = dtcmask << 8;
    dtcmask = dtcmask | dtc_severity_mask_record[1];

    switch (dtcmask)
    {
    case 0xC001:
        *dtc_status_availability_mask = 0x2F;
        *dtc_severity_availability_mask = 0x45;
        *dtc_format_identifier = 0x67;
        dtc_and_severity_record[i] = 0x31;
        i++;
        dtc_and_severity_record[i] = 0x32;
        i++;
        dtc_and_severity_record[i] = 0x33;
        i++;
        dtc_and_severity_record[i] = 0x34;
        i++;
        dtc_and_severity_record[i] = 0x35;
        i++;
        dtc_and_severity_record[i] = 0x36;
        i++;
        dtc_and_severity_record[i] = 0x41;
        i++;
        dtc_and_severity_record[i] = 0x42;
        i++;
        dtc_and_severity_record[i] = 0x43;
        i++;
        dtc_and_severity_record[i] = 0x44;
        i++;
        dtc_and_severity_record[i] = 0x45;
        i++;
        dtc_and_severity_record[i] = 0x46;
        i++;
        *severity_record_len = i;
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

/**
 * @brief [0x19 Subfunction 0x55]This API used to update the WWW OBD dtc with permanent status
 *
 * @param func_id Holds the function ID
 * @param dtc_severity_mask_record Holds the dtc severity mask record
 * @param dtc_format_identifier Holds the dtc format identifier
 * @param dtc_and_status_rec Holds the dtc status record array
 * @param dtc_record_len Holds the length of the "dtc_and_status_rec" array
 * @return ret Success 0 Failure -1
 */
int process_WWH_OBD_dtc_with_permanent_status(uint8_t func_id, uint8_t *dtc_status_availability_mask, uint8_t *dtc_format_identifier, uint8_t *dtc_and_status_rec, int *dtc_record_len)
{
    int ret = SUCCESS;
    int i = 0;
    uint8_t func_idval = 0x24;
    /*
     * API to fill the dtc information to the empty buffer
     */
    if (func_id == func_idval)
    {
        *dtc_status_availability_mask = 0x31;
        *dtc_format_identifier = 0x67;
        dtc_and_status_rec[i] = 0x41;
        i++;
        dtc_and_status_rec[i] = 0x41;
        i++;
        dtc_and_status_rec[i] = 0x42;
        i++;
        dtc_and_status_rec[i] = 0x43;
        i++;
        dtc_and_status_rec[i] = 0x44;
        i++;
        dtc_and_status_rec[i] = 0x45;
        i++;
        dtc_and_status_rec[i] = 0x46;
        i++;
        dtc_and_status_rec[i] = 0x47;
        i++;
        dtc_and_status_rec[i] = 0x48;
        i++;
        dtc_and_status_rec[i] = 0x49;
        i++;
        dtc_and_status_rec[i] = 0x50;
        i++;
        dtc_and_status_rec[i] = 0x51;
        i++;
        dtc_and_status_rec[i] = 0x52;
        i++;
        dtc_and_status_rec[i] = 0x53;
        i++;
        dtc_and_status_rec[i] = 0x54;
        i++;
        dtc_and_status_rec[i] = 0x55;
        i++;
        *dtc_record_len = i;
    }
    else
    {
        ret = -1;
    }

    return ret;
}

/**
 * @brief [0x19 Subfunction 0x56]This API used to update dtc information by dtc readiness group identifier
 *
 * @param func_id Holds the function ID
 * @param readiness_id
 * @param dtc_status_availability_mask
 * @param dtc_format_identifier
 * @param dtc_and_status_rec
 * @param dtc_record_len
 * @return ret Success 0 Failure -1
 */
int process_report_dtc_information_by_dtc_readiness_group_identifier(uint8_t func_id, uint8_t readiness_id, uint8_t *dtc_status_availability_mask, uint8_t *dtc_format_identifier, uint8_t *dtc_and_status_rec, int *dtc_record_len)
{
    int ret = SUCCESS;
    if (readiness_id == (uint8_t)0)
    {
    }

    int i = 0;
    uint8_t func_idval = 0x24;
    /*
     * API to fill the dtc information to the empty buffer
     */
    if (func_id == func_idval)
    {
        *dtc_status_availability_mask = 0x31;
        *dtc_format_identifier = 0x67;
        dtc_and_status_rec[i] = 0x41;
        i++;
        dtc_and_status_rec[i] = 0x41;
        i++;
        dtc_and_status_rec[i] = 0x42;
        i++;
        dtc_and_status_rec[i] = 0x43;
        i++;
        dtc_and_status_rec[i] = 0x44;
        i++;
        dtc_and_status_rec[i] = 0x45;
        i++;
        dtc_and_status_rec[i] = 0x46;
        i++;
        dtc_and_status_rec[i] = 0x47;
        i++;
        dtc_and_status_rec[i] = 0x48;
        i++;
        dtc_and_status_rec[i] = 0x49;
        i++;
        dtc_and_status_rec[i] = 0x50;
        i++;
        dtc_and_status_rec[i] = 0x51;
        i++;
        dtc_and_status_rec[i] = 0x52;
        i++;
        dtc_and_status_rec[i] = 0x53;
        i++;
        dtc_and_status_rec[i] = 0x54;
        i++;
        dtc_and_status_rec[i] = 0x55;
        i++;
        *dtc_record_len = i;
    }
    else
    {
        ret = -1;
    }

    return ret;
}

/**
 * @brief [0x2C Subfunction 0x03]This API used to clear the dynamic defined data identifier
 *
 * @param data_id Holds the data id
 * @return ret Success 0 Failure -1
 */
int clear_the_data_id_from_the_list(uint16_t data_id)
{
    int ret = SUCCESS;
    int id = 0;
    while (id < MAX_DYNAMIC_DATA_ID)
    {

        if (dyn_def_data[id].def_data_id == data_id)
        {
            read_data_id.dynamic_id[id] = 0x0000;
            dyn_def_data[id].def_data_id = 0x0000;
            break;
            dynamic_id_count--;
        }
        id++;
    }
    return ret;
}

/**
 * @brief [0x2C Subfunction 0x02]This API used to update the data id by using memory address and memory size bytes
 *
 * @param data_id Holds the data id
 * @param DID_addr_byte Holds the DID address array
 * @param DID_size_byte Holds the DID size byte array
 * @param no_of_data Holds the number of data
 * @return ret Success 0 Failure -1
 */
int update_the_data_record_for_defined_identifier_by_memory_addr(uint16_t data_id, uint64_t *DID_addr_byte, uint64_t *DID_size_byte, int no_of_data)
{
    int ret = SUCCESS;
    int temp_no_of_data = 0;

    if (temp_no_of_data == 0)
    {
    }

    uint16_t data_idcheck = 0xF190;
    temp_no_of_data = no_of_data;
    int i = 0;
    int j = 0;
    if (data_id == data_idcheck)
    {
        DID_addr_byte[i] = 0x41;
        i++;
        j++;
        DID_addr_byte[i] = 0x41;
        i++;
        j++;
        DID_addr_byte[i] = 0x42;
        i++;
        j++;
        DID_addr_byte[i] = 0x43;
        i++;
        j++;
        i = 0;
        DID_size_byte[i] = 0x41;
        i++;
        j++;
        DID_size_byte[i] = 0x41;
        i++;
        j++;
        DID_size_byte[i] = 0x42;
        i++;
        j++;
        DID_size_byte[i] = 0x43;
        i++;
        j++;
        temp_no_of_data = j;
    }
    return ret;
}

/**
 * @brief [SID 0x2C]This API used to update the dynamically defined data identifiers
 *
 * @param def_data_id  Holds the defined data identifier
 * @return ret Success 0 Failure -1
 */
int update_the_dynamically_defined_data_identifier(uint16_t def_data_id)
{

    if (def_data_id == (uint16_t)0)
    {
    }

    int ret = SUCCESS;
    return ret;
}

/**
 * @brief [SID 0x85]This API used to start the setting of diagnostic trouble codes in the ECU by client
 *
 * @param dtc_code Holds the array of DTC code
 * @return ret Success 0 Failure -1
 */
int start_updating_of_diagnostic_trouble_code_status(uint8_t *dtc_code)
{
    int ret = SUCCESS;
    if (dtc_code[0] == (uint8_t)0)
    {
    }

    /*
     * call API to start update dtC code status based on the dtc_code
     */

    return ret;
}

/**
 * @brief [SID 0x85]This API used to stop the setting of diagnostic trouble codes in the ECU by client
 *
 * @param dtc_code Holds the array of DTC code
 * @return ret Success 0 Failure -1
 */
int stop_updating_of_diagnostic_trouble_code_status(uint8_t *dtc_code)
{
    int ret = SUCCESS;
    if (dtc_code[0] == (uint8_t)0)
    {
    }

    /*
     * call API to stop update dtC code status based on the dtc_code
     */
    return ret;
}

/**
 * @brief [SID 0x28]API is to enable reception and trasmission the network messages
 *
 * @param communication_type
 * @return ret Success 0 Failure -1
 */
int enable_rx_and_tx(uint8_t communication_type)
{
    int ret = SUCCESS;

    return ret;
}

/**
 * @brief [SID 0x28]API is to enable reception and disable trasmission the network messages
 *
 * @param communication_type
 * @return ret Success 0 Failure -1
 */
int enable_rx_and_disable_tx(uint8_t communication_type)
{
    int ret = SUCCESS;

    return ret;
}

/**
 * @brief [SID 0x28]API is to disable reception and enable trasmission the network messages
 *
 * @param communication_type
 * @return ret Success 0 Failure -1
 */
int disable_rx_and_enable_tx(uint8_t communication_type)
{
    int ret = SUCCESS;

    return ret;
}

/**
 * @brief [SID 0x28]API is to disable reception and trasmission the network messages
 *
 * @param communication_type
 * @return ret Success 0 Failure -1
 */
int disable_rx_and_tx(uint8_t communication_type)
{
    int ret = SUCCESS;

    return ret;
}

/**
 * @brief [SID 0x28]API is to enable reception and disable trasmission with enhanced addr info of  the network messages
 *
 * @param communication_type
 * @param node_id
 * @return ret Success 0 Failure -1
 */
int enable_rx_and_disable_tx_with_enhanced_addr_info(uint8_t communication_type, uint16_t node_id)
{
    int ret = SUCCESS;

    return ret;
}

/**
 * @brief [SID 0x28]API is to enable reception and trasmission with enhanced addr info of  the network messages
 *
 * @param communication_type
 * @param node_id
 * @return ret Success 0 Failure -1
 */
int enable_rx_and_tx_with_enhanced_addr_info(uint8_t communication_type, uint16_t node_id)
{
    int ret = SUCCESS;

    return ret;
}

/**
 * @brief [SID 0x2F]This API used to control the input and output using data identifier
 * @param data_id Holds the data id
 * @param io_ctrl_param Holds input and output control parameters
 * @param in_control_option_record Holds the array of control option record
 * @param in_data_len Holds the length of input data
 * @param output_control_status_record Holds the arrays of output control status record
 * @param resp_len Holds the array for respone length
 * @return ret Success 0 Failure -1
 */
int input_output_control_data_by_identifier(uint16_t data_id, uint8_t io_ctrl_param, uint8_t *in_control_option_record, int in_data_len, uint8_t *output_control_status_record, int *resp_len)
{
	int ret = E_SUCCESS;
	uint16_t data_idcheck = 0xF190;

	if (in_data_len == 0)
	{
	}
	if (in_control_option_record[0] == (uint8_t)0)
	{
	}
	if (io_ctrl_param == (uint8_t)0)
	{
	}
	int i = 0;
	/* Using the input control option record for inputoutputcontrol parameter 0x03 (shortter adjstment). and update the ouput response in output_control_status_record array*/
	if (data_id == data_idcheck)
	{
		i = 0;
		output_control_status_record[i] = 0x44;
		i++;
		output_control_status_record[i] = 0x45;
		i++;
		output_control_status_record[i] = 0x46;
		i++;
		output_control_status_record[i] = 0x47;
		i++;
		*resp_len = i;
	}
	else
	{
		ret = -1;
	}
	return ret;
}

/**
 * @brief [SID 0x87]API is to setting the baudrate of communication channel
 *
 * @param baudrate The baud rate selected by the client. 
 * @return ret Success 0 Failure -1
 */
int link_control_transition_baudrate(unsigned long int baudrate)
{
    int ret = SUCCESS;
    printf("baudrate = %d\n", baudrate);
    return ret;
}