#ifndef UDS_TP
#define UDS_TP
#include <stdint.h>
/*these macros are used to select the type of data identifier*/
#if 0
#define BIT_ADDRESS_FORMAT_11                1
#define BIT_ADDRESS_FORMAT_29                0

#if BIT_ADDRESS_FORMAT_11                 
#define PHYSICAL_ECU_ADDRESS                 0x7E0
#define FUNCTIONAL_ECU_ADDRESS               0x7DF
#define DESTINATION_ECU_ADDRESS              0x7E8
#define TARGET_TYPE                          0x01

#define MAX_DATA_BYTE_SUPPORTED              7

#elif   BIT_ADDRESS_FORMAT_29                
#define PHYSICAL_ECU_ADDRESS                 0x18DAF110
#define FUNCTIONAL_ECU_ADDRESS               0x18DBF133
#define DESTINATION_ECU_ADDRESS              0x18DA10F1
#define TARGET_TYPE                          0x05
#endif
#endif

#define MAX_DATA_BYTE_SUPPORTED              7

/*retry count is defined here to re request the same request if response is not received for the request*/
#define RETRY_COUNT                             3

/*This macro is used to limit the request count in Application layer*/
#define REQUEST_MESSAGE_COUNT                  20

#define LINUX_PLATFORM                          0
#define RTOS_PLATFORM                           1
#define BARE_METAL                              0

#define SERVICE                                 1
#define SUB_FUNCTION                            2
#define MULTI_FRAME_SERVICE                     6
#define MULTI_FRAME_SUB_FUNCTION                7

#define P2_SERVER_HIGH_BYTE                     0x00
#define P2_SERVER_LOW_BYTE                      0x32
#define P2_SERVER_MAX_HIGH_BYTE                 0x01
#define P2_SERVER_MAX_LOW_BYTE                  0xF4

/*configuration parameter for read data by identifier*/
#define MAX_DATA_IDS                            50
#define MAX_DATA_BYTES                          300
#define CHECK_PASSED                            0x01
#define MAX_ROUTINE_STATUS_BYTES                100
#define ROUTINE_MSB_BYTE                        0
#define ROUTINE_LSB_BYTE                        1

/*this macro is used to define length byte in message*/
#define LENGTH                                  0

#define ZERO_DATA_BYTE                          0x00

/*this macro is used to define the session time*/
#define SESSION_TIME                            5

/*this macro is used to set the number of seed bytes*/
#define NUMBER_OF_SEEDS                         2
#define MAX_KEY_VERIFICATION_REQ_COUNT          3
/*This macro is use to assign maximum length of read dtc information service*/
#define MAX_RESP_BUFFER                         300

/*This macro is used to define the position if dtc status mask in request = for classical CAN position = 0; for CANFD position = 0*/
#define DTC_STATUS_MASK                        0
#define MEM_SELECTION                          1

/*This macro is used to define the size of dtc status record*/
#define DTC_STATUS_RECORD                      100
/*for single DTC status record*/
#define MAX_DTC_STATUS_RECORD                  4

/*This macro is used to define maximum length of DTC record*/
#define DTC_RECORD                             100
#define DTC_SNAPSHOT_RECORD_NO                 5
#define DTC_AND_STATUS_RECORD                  4 
#define DTC_SNAPSHOT_RECORD_IDENTIFIERS        5
#define DTC_SNAPSHOT_RECORD                    50
#define DTC_SNAPSHOT_RECORD_LEN                5
#define NUMBER_OF_STORED_DTC_RECORD            5
#define MAX_DTC_EXTENDED_DATA_RECORD           50
#define USER_DEFINED_SNAPSHOT_RECORD_NUMBER    5

/*DTC exteded data record macros*/
#define DTC_EXTENDED_DATA_RECORD_NO            5

#if 0
/*These macros will use for DTC mask record position for CAN and CANFD*/
#define PRINT_ON_CONSOLE		       1
#endif

#define DTC_HIGH_BYTE                          0
#define DTC_MIDDLE_BYTE                        1
#define DTC_LOW_BYTE                           2
#define USER_DEFINED_SNAPSHOT_NUMBER           3 
#define MEM_SELECTION_POS_IN_18                4
#define SEVERITY_HIGH_BYTE                     0
#define SEVERITY_LOW_BYTE                      1
#define WWW_SEVERITY_HIGH_BYTE_42              1
#define WWW_SEVERITY_LOW_BYTE_42               2
#define DTC_SNAPSHOT_RECORD_NUMBER_04          3
#define DTC_STORED_RECORD_NUMBER_05            0
#define DTC_EXTENDED_DATA_RECORD_06            3
#define DTC_EXTENDED_DATA_RECORD_19            3
#define MEM_SELECTION_POS_IN_19                4
#define DTC_EXTENDED_DATA_RECORD_16            0
#define DTC_EXTENDED_DATA_RECORD_NUMBER_1A     0
#define FUNCTIONAL_GROUP_ID                    0
#define DTC_READINESS_GROUP_IDENTIFIER         1

/*communication control service byte position macros*/
#define COMMUNICATION_TYPE                     0
#define NODE_ID_HIGH_BYTE                      1
#define NODE_ID_LOW_BYTE                       2

/*macros used for configuring the upload download bytes position*/
#define DATA_FOARMAT_ID				0
#define ADDRESS_LENGTH_FORMAT_ID		1
#define MEMORY_ADDRESS_BYTE                     2
#define LENGTH_FORMAT_IDENTIFIER                0x20

/*transfer data request*/
#define MAX_NUM_OF_TRANSFER_RECORD              20

/*macros used for configuring the dynamically defined data identifier*/
#define MAX_SOURCE_ID                           5
#define DYNAMIC_ID_MSB_BYTE                     0
#define DYNAMIC_ID_LSB_BYTE                     1
#define ADDR_LEN_FMT_ID                         2 
#define MAX_DEFINED_DATA_ID                     5
#define MAX_READ_DATA_ID                        10
#define MAX_DYNAMIC_DATA_ID                     5
#define MAX_DYNAMIC_DATA_BYTES                  20


#define MAX_DATA_LINK_LAYER_DATA                4070

/*0x83 Access timing parameter*/
#if 0
#define P2_SERVER_MIN                           50
#define P2_SERVER_MAX                           9000
#define P2_SERVER_MAX1                          255
#else
#define P2_SERVER_MIN                           50
#define P2_SERVER_MAX                           5000
#endif

/*io control*/
#define MSB_DATA_ID_BYTE                        0
#define LSB_DATA_ID_BYTE                        1


/* Authentication macros for position and data byte allocation */
#define COMMNUNICATION_CONFIG_BYTE                              0
#define MSB_LEN_OF_CERT                                         1
#define LSB_LEN_OF_CERT                                         2
#define MSB_BYTE                                                0
#define LSB_BYTE                                                1
#define CERTIFICATE_CONFIG_MSB_BYTE                             0
#define CERTIFICATE_CONFIG_LSB_BYTE                             1
#define TRANSFER_MSB_LEN_OF_CERT                                2
#define TRANSFER_LSB_LEN_OF_CERT                                3
#define ALGORITHM_LENGTH                                        4


#define MAX_CERTIFICATE_LENGTH                                  100
#define MAX_RESP_LENGTH_AUTH                                    500
#define MAX_CERT_LENGTH_BYTE                                    2

/*RESPONSE ON EVENT*/
#define MAX_NUMBER_OF_EVENT_RECORD                             10
#define EVENT_RECORD                                           50
#define RESPONSE_LENGTH                                        500

/*secure data transmission*/
#define ADMINISTRATIVE_PARAM_MSB                               0
#define ADMINISTRATIVE_PARAM_LSB                               1
#define SIGNATURE                                              2
#define SIGNATURE_MSB                                          3
#define SIGNATURE_LSB                                          4
#define ANTI_REPLAY_COUNTER_MSB                                5
#define ANTI_REPLAY_COUNTER_LSB                                6
#define INTERNAL_MSG_SERV_ID                                   7

/*periodic data identifiers*/
#define SLOW_RATE_TRASMISSION                                  300
#define MEDIUM_RATE_TRASMISSION                                200
#define FAST_RATE_TRASMISSION                                  100
#define SLOW_RATE                                              0x01
#define MEDIUM_RATE                                            0x02
#define FAST_RATE                                              0x03
#define RESPONSE_LEN_FOR_ID                                    20
#define RESPONSE_LEN                                           30
#define MAX_PERIODIC_ID                                        2

/*to clear the DTC bytes service 0x14*/

#define flexCAN2  0x0

typedef struct pre_req
{
	uint8_t security_access;
	uint8_t Authentication;
	uint8_t ECU_session;
}pre_req_check;
/*uds tp pulbic api */
int UDS_init(void);
long UDS_get_request_start_time();
long UDS_get_current_time();
int UDS_send_frame_negative_for_response_pending();
/*uds tp private api*/
void decToHexa(int n, uint32_t * hex_num);
#if 0
void session_server_layer_time_handling_thread(void);
#endif
void UDS_server_request_recv_thread(void);
void periodic_trasmission_of_id(void);
#if 0
void trasmission_of_can_wait(void);
#endif
int frame_negative_response_for_subfunction_not_supported(uint8_t service_id);
int frame_negative_response_for_incorrect_message_length(uint8_t service_id);
int frame_negative_response_for_service_not_supported(uint8_t service_id);
int frame_negative_response_for_conditions_not_correct(uint8_t service_id);
int frame_positive_response_for_periodic_data_all_identifiers(uint8_t * resp_buf, int len);
int frame_negative_response_for_negative_code_response(uint8_t service_id);
int frame_response_for_negative_code_wait_response(uint8_t service_id);
#endif
