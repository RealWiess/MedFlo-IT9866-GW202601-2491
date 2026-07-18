#ifndef UDS_SERVER_H
#define UDS_SERVER_H /*MISRA C violated */
#include <stdio.h>/*MISRA C violated */
#include <stdint.h>



#define OBD2_LIB_FAILURE1     (-2)
#define OBD2_LIB_FAILURE2     (-3)
#define OBD2_LIB_FAILURE3     (-1)
#define FILE_OPEN_ERROR      (-10)


#define DIAGNOSTIC_TESTER	                 1
#define DIAGNOSTIC_CONTROL	                 2
#define DIAGNOSTIC_SECURITY	                 3
#define DIAGNOSTIC_READ_DTC_INFORMATION          4
#define DIAGNOSTICE_CLEAR_DTC_INFORMATION        5
#define DIAGNOSTIC_ECU_RESET                     6
#define DIAGNOSTIC_POSITIVE_RESPONSE             0x50
#define NEGATIVE_RESPONSE             		 0x7F
#define POSITIVE_RESPONSE_DTC_INFORMATION        0x59
#define POSITIVE_RESPONSE_FOR_ECU_RESET          0x51
#define HARD_RESET				 0x01
#define KEY_OFF_ON_RESET			 0x02
#define SOFT_RESET				 0x03

#define SUCCESS                           0

#define INTERPRETTER_CAN_EN               74
#define CPU_CAN_EN                        81

/* UDS SIDs */
#define UDS_SID_DIAGNOSTIC_CONTROL                                   0x10 // GMLAN = Initiate Diagnostics
#define UDS_SID_ECU_RESET                                            0x11
#define UDS_SID_CLEAR_DTC                                            0x14
#define UDS_SID_READ_DTC_INFORMATION                                 0x19
#define UDS_SID_RESTART_COMMUNICATIONS                               0x20 // GMLAN - Restart a stopped com
#define UDS_SID_READ_DATA_BY_IDENTIFIER                              0x22
#define UDS_SID_READ_MEMORY_BY_ADDRESS                               0x23
#define UDS_SID_READ_SCALING_BY_ID                                   0x24
#define UDS_SID_SECURITY_ACCESS                                      0x27
#define UDS_SID_COMMUNICATION_CONTROL                                0x28 // GMLAN Stop Communications
#define UDS_SID_AUTHENTICATION                                       0x29
#define UDS_SID_READ_DATA_BY_ID_PERIODIC                             0x2A
#define UDS_SID_DYNAMICALLY_DEFINED_DATA_ID                          0x2C
#define UDS_SID_WRITE_DATA_BY_IDENTIFIER                             0x2E
#define UDS_SID_IO_CONTROL_BY_ID                                     0x2F
#define UDS_SID_ROUTINE_CONTROL                                      0x31
#define UDS_SID_REQUEST_DOWNLOAD                                     0x34
#define UDS_SID_REQUEST_UPLOAD                                       0x35
#define UDS_SID_TRANSFER_DATA                                        0x36
#define UDS_SID_REQUEST_TRANSFER_EXIT                                0x37
#define UDS_SID_REQUEST_XFER_FILE                                    0x38
#define UDS_SID_WRITE_MEMORY_BY_ADDRESS                              0x3D
#define UDS_SID_TESTER_PRESENT                                       0x3E
#define UDS_SID_ACCESS_TIMING_PARAMETER                              0x83
#define UDS_SID_SECURED_DATA_TRANS                                   0x84
#define UDS_SID_CONTROL_DTC_SETTINGS                                 0x85
#define UDS_SID_RESPONSE_ON_EVENT                                    0x86
#define UDS_SID_LINK_CONTROL                                         0x87
#define UDS_SID_GM_PROGRAMMED_STATE                                  0xA2
#define UDS_SID_GM_PROGRAMMING_MODE                                  0xA5
#define UDS_SID_GM_READ_DIAG_INFO                                    0xA9
#define UDS_SID_GM_READ_DATA_BY_ID                                   0xAA
#define UDS_SID_GM_DEVICE_CONTROL                                    0xAE

/*negative respons data*/
#define SUB_FUN_NOT_SUPPORTED                                  0x12
#define INCORRECT_MESSAGE_LENGTH                               0x13
#define RESPONSE_TOO_LONG                                      0x14
#define BUSY_REPEAT_REQUEST                                    0x21
#define CONDITION_NOT_CORRECT                                  0x22
#define REQUEST_SEQUENCE_ERROR                                 0x24
#define NO_RESPONSE_FROM_SUBNET_COMPONENET                     0x25
#define FAILURE_PREVENTS_EXECUTION_OF_REQUESTED_ACTION         0x26
#define REQUEST_OUT_OF_RANGE                                   0x31
#define SECURITY_ACCESS_DENIED                                 0x33
#define AUTHENTICATION_REQUIRED                                0x34
#define INVALID_KEY                                            0x35
#define EXCEDED_NUMBER_OF_ATTEMPTS                             0x36
#define REQUIRED_TIME_DELAY_NOT_EXPIRED                        0x37
#define SECURED_DATA_TRANSMISSION_REQUIRED                     0x38
#define SECURED_DATA_TRANSMISSION_NOT_ALLOWED                  0x39
#define SECURE_DATA_VERIFICATION_FAILED                        0x3A
#define CERTIFICATE_VERIFICATION_FAILED_INVALID_TIME_PERIOD    0x50
#define CERTIFICATE_VERIFICATION_FAILED_INVALID_SIGNATURE      0x51
#define CERTIFICATE_VERIFICATION_FAILED_INVALID_CHAIN_OF_TRUST 0x52
#define CERTIFICATE_VERIFICATION_FAILED_INVALID_TYPE           0x53
#define CERTIFICATE_VERIFICATION_FAILED_INVALID_FORMAT         0x54
#define CERTIFICATE_VERIFICATION_FAILED_INVALID_CONTENT        0x55
#define CERTIFICATE_VERIFICATION_FAILED_INVALID_SCOPE          0x56
#define CERTIFICATE_VERIFICATION_FAILED_INVALID_CERTIFICATE    0x57
#define OWNERSHIP_VERIFIACTION_FAILED                          0x58
#define CALLENGE_CALULATION_FAILED                             0x59
#define SETTING_ACCESS_RIGHTS_FAILED                           0x5A
#define SESSION_KEY_CREATION_FAILED                            0x5B
#define CONFIGURATION_DATA_USAGE_FAILED                        0x5C
#define DE_AUTHENTICATION_FAILED                               0x5D

#define UPLOAD_DOWNLOAD_NOT_ACCEPTED                     0x70
#define TRANSFER_DATA_SUSPENDED                          0x71
#define GENERAL_PROGRAMMING_ERROR                        0x72
#define WRONG_BLOCK_SEQUENCE_COUNTER                     0x73
#define REQUEST_RECEIVED_CORRECTLY_RESPONSE_PENDING      0x78
#define SUB_FUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION     0x7E
#define SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION          0x7F
 
/*parameter range negative response code*/
#define RPM_TOO_HIGH                                     0x81
#define RPM_TOO_LOW                                      0x82
#define ENGINE_IS_RUNNING                                0x83
#define ENGINE_IS_NOT_RUNNING                            0x84
#define ENGINE_RUN_TIME_TOO_LOW                          0x85
#define TEMPERATURE_TOO_HIGH                             0x86
#define TEMPERATURE_TOO_LOW                              0x87
#define VEHICLE_SPEED_TOO_HIGH                           0x88
#define VEHICLE_SPEED_TOO_LOW                            0x89
#define THROTTLE_TOO_HIGH                                0x8A
#define THROTTLE_TOO_LOW                                 0x8B
#define TRASMISSION_RANGE_NOT_IN_NEUTRAL                 0x8C
#define TRASMISSION_RANGE_NOT_IN_GEAR                    0x8D
#define BREAK_SWITCH_NOT_CLOSED                          0x8F
#define SHIFT_LEVEL_NOT_IN_PARK                          0x90
#define TORQUE_CONVERTER_CLUTCH_LOCKED                   0x91
#define VOLTAGE_TOO_HIGH                                 0x92
#define VOLTAGE_TOO_LOW                                  0x93
#define RESOURCE_TEMPORARILY_NOT_AVAILABLE               0x94
/*if no sunfunction*/
#define NO_SUB_FUNCTION                                  0x00
#define NOT_VALID_SUBFUNCTION                           (-1)

/*uds diagnostic session control sub functions*/
#define DEFAULT_DIAGNOSTIC_SESSION           0x01
#define PROGRAMMING_SESSION                  0x02
#define EXTENDED_DIAGNOSTIC_SESSION          0x03
#define SYSTEM_SAFTEY_DIAGNOSTIC_SESSION     0x04
#define VEHICLE_MANUFACTURER_SPECIFIC_SESSION 0x55
/*ECU reset subfunctions*/
#define HARD_RESET                           0x01
#define KEY_OFF_ON_RESET                     0x02
#define SOFT_RESET                           0x03
#define ENABLE_RAPID_POWER_SHUT_DOWN         0x04
#define DISABLE_RAPID_POWER_SHUT_DOWN        0x05

/*Routine control service sub functions*/
#define START_ROUTINE                        0x01
#define STOP_ROUTINE                         0x02
#define REQUEST_ROUTINE_RESULTS              0x03


/*Read DTC information subfunctions*/
#define REPORT_NUMBER_OF_DTC_BY_STATUS_MASK                               0x01
#define REPORT_DTC_BY_STATUS_MASK                                         0x02
#define REPORT_DTC_SNAPSHOT_IDENTIFICATION                                0x03
#define REPORT_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER                          0x04
#define REPORT_DTC_STORED_DATA_BY_RECORD_NUMBER                           0x05
#define REPORT_EXTENDED_DTC_STORED_DATA_BY_RECORD_NUMBER                  0x06
#define REPORT_NUMBER_OF_DTC_BY_SEVERITY_MASK_RECORD                      0x07
#define REPORT_DTC_BY_SEVERITY_MASK_RECORD                                0x08
#define REPORT_SEVERITY_INFORMATION_OF_DTC                                0x09
#define REPORT_SUPPORTED_DTC                                              0X0A
#define REPORT_FIRST_TEST_FAILED_DTC                                      0x0B
#define REPORT_FIRST_CONFIRMED_DTC                                        0X0C
#define REPORT_MOST_RECENT_FAILED_DTC                                     0X0D
#define REPORT_MOST_RECENT_CONFIRMED_DTC                                  0x0E
#define REPORT_DTC_FAULT_DETECTION_COUNTER                                0x14
#define REPORT_DTC_WITH_PERMANENT_STATUS                                  0x15
#define REPORT_DTC_EXTENDED_DATA_RECORD_BY_RECORD_NUMBER                  0x16
#define REPORT_USER_DEFINED_MEMORY_DTC_BY_STATUS_MASK                     0x17
#define REPORT_USER_DEFINED_MEMORY_DTC_SNAPSHOT_RECORD_BY_DTC_NUMBER      0x18
#define REPORT_USER_DEFINED_MEMORY_DTC_EXTENDED_DATA_RECORD_BY_DTC_NUMBER 0x19
#define REPORT_SUPPORTED_DTC_EXTENDED_DATA_RECORD                         0x1A
#define REPORT_WWH_OBD_DTC_BY_MASK_RECORD                                 0x42
#define REPORT_WWH_OBD_DTC_WITH_PERMANENT_STATUS                          0x55
#define REPORT_DTC_INFORMATION_BY_DTC_READINESS_GROUP_IDENTIFIER          0x56

/*control dtc sub functions*/
#define DTC_ON                     0x01
#define DTC_OFF                    0x02

/*Link control sub functions*/
#define VERIFY_MODE_TRANSISION_WITH_FIXED_PARAMETER            0x01
#define VERIFY_MODE_TRANSISION_WITH_SPECIFIC_PARAMETER         0x02
#define TRANSITION_MODE                                        0x03

/*Authentiication service sub functions*/
#define DE_AUTHENTICATE						0x00
#define VERIFY_CERTIFICATE_UNIDIRECTIONAL                       0x01 
#define VERIFY_CERTIFICATE_BIDIRECTIONAL			0x02
#define PROOF_OF_OWNERSHIP					0x03
#define TRANSFER_CERTIFICATE             			0x04
#define REQUEST_CHALLENGE_FOR_AUTHENTICATION			0x05
#define VERIFY_PROOF_OF_OWNERSHIP_UNIDIRECTIONAL		0x06
#define VERIFY_PROOF_OF_OWNERSHIP_BIDIRECTIONAL			0x07
#define AUTHENTICATION_CONFIGURATION                            0x08
/*response on event*/
#define STOP_RESPONSE_ON_EVENT                                  0x00
#define ON_DTC_STATUS_CHANGE                                    0x01
#define ON_CHANGE_OF_DATA_IDENTIFIER                            0x03
#define REPORT_ACTIVATED_EVENTS                                 0x04
#define START_RESPONSE_ON_EVENT                                 0x05
#define CLEAR_RESPONSE_ON_EVENT                                 0x06
#define ON_COMPARISION_OF_VALUES                                0x07
#define REPORT_MOST_RECENT_DTC_ON_STATUS_CHANGE                 0x08
#define REPORT_DTC_RECORD_INFORMATION_ON_DTC_STATUS_CHANGE      0x09
/*Security services subfunctions*/

/*dynamically defined data identifier sub function  macros*/
#define DEFINE_BY_IDENTIFIER                    0x01
#define DEFINE_BY_MEMORY_ADDRESS                0x02
#define CLEAR_DYNAMICALLY_DEFINED_DATA_ID       0x03

/* Link Control Fixed Parameters*/
#define PC9600                                  0x01
#define PC19200                                 0x02
#define PC38400                                 0x03
#define PC57600                                 0x04
#define PC115200                                0x05
#define CAN125000                               0x10
#define CAN250000                               0x11
#define CAN500000                               0x12
#define CAN1000000                              0x13

/* Communication Control Sub Function*/
#define ENABLE_RX_AND_TX                                    0x00
#define ENABLE_RX_AND_DISABLE_TX                            0x01
#define DISABLE_RX_AND_ENABLE_TX                            0x02
#define DISABLE_RX_AND_TX                                   0x03
#define ENABLE_RX_AND_DISABLE_TX_WITH_ENHANCED_ADDR_INFO    0x04
#define ENABLE_RX_AND_TX_WITH_ENHANCED_ADDR_INFO            0x05

/* Access Timing Parameter Sub Function*/
#define READ_EXTENDED_PARAMETR                  0x01
#define SET_PARAMETR_TO_DEFAULT_VALUE           0x02
#define READ_ACTIVE_PARAMETR                    0x03
#define SET_PARAMETER                           0x04

#define  DIAGNOSTIC_SESSION_FAILURE       (-10)
#define  ECU_RESET_FAILURE                (-11)
#define  UDS_TESTER_FAILURE               (-23)

#define MAX_COUNT  100
#define MAX_NUM    100

#define E_REQ_FAIL                        (-2)
#define E_REQ_SEQ_FAIL                    (-3)

typedef struct addr_security_access
{
	uint32_t src_addr;
	uint32_t tgt_addr;
	uint16_t TAtype;
}security;
/*request confirmation*/
typedef struct server_session_confirm
{
        uint8_t S_Mtype;
        uint32_t S_SA;
        uint32_t S_TA;
	uint8_t S_TAtype;
        int  S_result;
}server_s_data_confirm;

typedef struct server_session_confm
{
	server_s_data_confirm S_confirm;
        int S_start_time;
        int S_req_count;
}server_s_data_confm;

/*repsonse indication*/
typedef struct server_session_indication
{
        uint8_t S_Mtype;
        uint32_t S_SA;
        uint32_t S_TA;
	uint8_t S_TAtype;
        uint8_t S_data[4080];
        int S_length;
        int  S_result;
}server_s_data_indication;
/*linked list*/

/*response type*/
typedef struct server_Appn_response
{
        uint8_t A_Mtype;
        uint32_t A_SA;
        uint32_t A_TA;
	uint8_t A_TAtype;
        uint8_t A_data[4080];
        int A_length;
        int  A_result;
}server_A_data_resp;
typedef struct client_data
{
	uint16_t data_iden[20];
        uint8_t event_window_time;
	uint8_t scheduled_events[10];
}cli_data;

/*Default session services*/
typedef struct default_sessn
{
   uint8_t TESTER_PRESENT;
   uint8_t ECU_RESET;

}default_session;
/*programming session services*/
typedef struct progra_sessn
{
	uint8_t CONTROL_DTC_SETTINGS;
	uint8_t RESPONSE_ON_EVENT;
}progra_session;
typedef struct ext_diagn_sessn
{
	uint8_t CONTROL_DTC_SETTINGS;
	uint8_t READ_DTC_INFORMATION;
}ext_diagn_session;
typedef struct saftey_diag_sessn
{
	uint8_t CONTROL_DTC_SETTINGS;
	uint8_t SECURE_DATA_TRANSMISSION;
}saftey_diag_session;

#if 0
int system_requirement_conditions_proper(void);
int set_ECU_to_default_session(void);
int set_ECU_to_programming_session(void);
int set_ECU_to_extended_diagnostic_session(void);
int set_ECU_to_system_saftey_diagnostic_session(void);
int start_ECU_hard_reset(void);
int start_ECU_key_off_on_reset(void);
int start_ECU_soft_reset(void);
int start_ECU_enable_rapid_power_shutdown(void);
int start_ECU_disable_rapid_power_shutdown(void);
int *read_the_data_by_identifier(uint16_t * dataid ,int data_id_len,  uint8_t **resp_data);
int check_the_data_id_supported_in_active_session(uint16_t data_id);
int process_start_routine_sub_function(uint16_t routine_id,  uint8_t * routine_record, int routine_len, uint8_t *routine_status_rec, int *routine_status_len);
int process_stop_routine_sub_function(uint16_t routine_id, uint8_t * routine_record, int routine_len, uint8_t *routine_status_rec, int *routine_status_len);
int process_request_routine_results_sub_function(uint16_t routine_id, uint8_t * routine_record, int routine_len, uint8_t *routine_status_rec, int *routine_status_len);
int send_seed_data_bytes_to_client(uint8_t *seed_data, int no_of_seed);
int process_request_upload_download_service(uint8_t *len_formt_id, uint8_t * max_number_of_blk_len);
#endif
int check_the_security_access_of_data_id(uint16_t data_id);
int check_the_security_access(void);
int check_the_Authentication_of_data_id(uint16_t data_id);
int check_the_Authentication(void);
#if 0
int transfer_data_from_client_to_server(uint64_t start_memory_addr, int memory_size_length, uint8_t * transfer_rec, int rec_len);
int transfer_data_from_server_to_client(uint64_t start_memory_addr, int memory_size_length, uint8_t * transfer_rec, int rec_len);
int update_the_data_record_for_defined_identifier(uint16_t data_id, uint16_t * source_id, uint8_t *position, uint8_t *memory_size, int data_len);
int update_the_data_record_for_data_id(uint16_t data_id, int data_len, uint8_t *data_record);
int clear_diagnostic_information_for_DTC(uint32_t group_of_DTC, uint8_t mem_addr);
int process_report_number_of_dtc_by_status_mask_request(uint8_t DTC_status_mask , uint8_t * DTC_status_availability_mask, uint8_t * DTC_format_identfier, uint16_t * DTC_count);
int process_report_dtc_by_status_mask_request(uint8_t DTC_status_mask, uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_status_record, int * len);
int process_report_dtc_snapshot_identification_request(uint8_t * DTC_record, uint8_t * DTC_snapshot_rec_number, int * snapshot_len);
int process_report_dtc_snapshot_record_by_dtc_number_request(uint8_t * dtc_mask_record, uint8_t DTC_snapshot_num, uint8_t * DTC_and_status_record, uint8_t * DTC_snapshot_record_number, uint8_t * DTC_snapshot_record_number_of_identifiers, uint8_t **DTC_snapshot_record, int *DTC_snapshot_record_len );
int process_report_dtc_stored_data_by_record_number(uint8_t DTC_stored_rec_no, uint8_t * DTC_stored_record_number, uint8_t **dtc_and_status_record, uint8_t * DTC_stored_record_number_of_identifiers, uint8_t **DTC_stored_data_record, int * DTC_stored_data_record_len);
int process_report_extended_dtc_stored_data_by_record_number (uint8_t * dtc_mask_record, uint8_t ext_data_num, uint8_t * dtc_and_status_record, uint8_t * DTC_ext_data_record_number, uint8_t ** DTC_ext_data_record, int * DTC_ext_data_record_len);
int process_report_number_of_dtc_by_severity_mask_record(uint8_t *DTC_severity_mask_record, uint8_t *DTC_status_availability_mask, uint8_t * DTC_format_identfier, uint16_t *DTC_count);
int process_report_dtc_by_severity_mask_record(uint8_t * DTC_severity_mask_record, uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_severity_record, int *severity_record_len);
int process_report_severity_information_of_dtc(uint8_t * dtc_mask_record, uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_severity_record, int *severity_record_len);
int process_report_supported_dtc(uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_status_record, int * len);
int process_report_first_test_failed_dtc(uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_status_record, int * len);
int process_report_first_confirmed_dtc(uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_status_record, int * len);
int process_report_most_recent_failed_dtc(uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_status_record, int * len);
int process_report_most_recent_confirmed_dtc(uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_status_record, int * len);
int process_report_dtc_with_permanent_status(uint8_t * DTC_status_availability_mask, uint8_t * DTC_and_status_record, int * len);
int process_report_dtc_fault_detection_counter(uint8_t *DTC_fault_detection_counter_record, int * len);
int process_report_dtc_extended_data_record_by_record_number(uint8_t record_number, uint8_t **dtc_and_status_record, uint8_t **DTC_ext_data_record, int *DTC_ext_data_record_len);
int process_report_user_defined_memory_dtc_by_status_mask(uint8_t DTC_status_mask, uint8_t mem_selection, uint8_t * DTC_status_availability_mask, uint8_t *DTC_and_status_record, int * dtc_record_len);
int process_report_user_defined_memory_dtc_snapshot_record_by_dtc_number(uint8_t * dtc_mask_record, uint8_t user_defined_snapshot_no, uint8_t mem_selection, uint8_t * DTC_and_status_record, uint8_t * user_def_dtc_snapshot_record_number, uint8_t * DTC_snapshot_record_number_of_identifiers, uint8_t **DTC_snapshot_record, int * DTC_snapshot_record_len);
int process_report_user_def_memory_dtc_extended_data_record_by_dtc_number(uint8_t * dtc_mask_record, uint8_t dtc_ext_data_rec_no, uint8_t mem_selection, uint8_t * DTC_and_status_record, uint8_t *DTC_ext_data_record_number, uint8_t **DTC_ext_data_record, int *DTC_ext_data_record_len);
int process_report_supported_dtc_extended_data_record(uint8_t dtc_ext_data_rec_num, uint8_t * DTC_status_availability_mask, uint8_t * DTC_record_num, uint8_t * DTC_and_status_record, int * dtc_record_len);
int process_WWH_OBD_dtc_by_mask_record(uint8_t func_id , uint8_t * DTC_severity_mask_record, uint8_t * DTC_status_availability_mask, uint8_t * DTC_severity_availability_mask, uint8_t * DTC_format_identifier, uint8_t * DTC_and_severity_record, int * severity_record_len);
int process_WWH_OBD_dtc_with_permanent_status(uint8_t func_id, uint8_t *DTC_status_availability_mask, uint8_t * DTC_format_identifier, uint8_t * DTC_and_status_record, int * dtc_record_len);
int process_report_dtc_information_by_dtc_readiness_group_identifier(uint8_t func_id, uint8_t readiness_id, uint8_t * DTC_status_availability_mask, uint8_t * DTC_format_identifier, uint8_t * DTC_and_status_record, int * dtc_record_len);
int clear_the_data_id_from_the_list(uint16_t data_id);
int update_the_data_record_for_defined_identifier_by_memory_addr(uint16_t data_id,  uint64_t * DID_addr_byte, uint64_t * DID_size_byte, int no_of_data);
int update_the_dynamically_defined_data_identifier(uint16_t def_data_id);
int start_updating_of_diagnostic_trouble_code_status(uint8_t *);
int stop_updating_of_diagnostic_trouble_code_status(uint8_t *);
int set_ECU_to_vehicle_specific_session(void);
#endif

int set_baudrate_value(unsigned long int baudrate);
int enable_reception_and_transmission(uint8_t communication_type);
int enable_reception_and_disable_transmission(uint8_t communication_type);
int disable_reception_and_enable_transmission(uint8_t communication_type);
int disable_reception_and_transmission(uint8_t communication_type);
int enable_reception_and_disable_transmission_with_enhanced_addr_info(uint8_t communication_type, uint16_t node_id);
int enable_reception_and_transmission_with_enhanced_addr_info(uint8_t communication_type, uint16_t node_id);
int write_memory_by_address(uint8_t * mem_addr, uint8_t mem_addr_length,  int no_of_bytes, uint8_t * data_record);
int read_data_by_memory_address(uint8_t *mem_addr, uint8_t mem_addr_len, int no_of_bytes, uint8_t *mem_buf, int *data_len);
int read_scaling_data_by_identifier(uint16_t data_id, uint8_t * resp_buf, int *data_len);
#if 0
int input_output_control_data_by_identifier(uint16_t data_id, uint8_t io_ctrl_param, uint8_t * in_control_option_record, int in_data_len, uint8_t * output_control_status_record, int * resp_len);
#endif
int de_authenticate_request_process(uint8_t * ret_value);
int verify_certificate_unidirectional_authentication(uint8_t commn_config, uint8_t * cert_client, int cert_len, uint8_t * challenge_client, int challenge_len, uint8_t * return_value, uint8_t * challenge_server, int * len_of_challenge_server, uint8_t * server_ephemeral_key, int * len_of_serv_ephemeral_key);
int verify_certificate_bidirectional_authentication(uint8_t commn_config, uint8_t * cert_client, int cert_len, uint8_t * challenge_client, int challenge_len, uint8_t * return_value, uint8_t *challenge_server, int * len_of_challenge_server, uint8_t * cert_server, int * len_of_server_cert, uint8_t * server_proof_of_ownership, int * len_of_serv_proof_of_ownership, uint8_t * server_ephemeral_key, int *len_of_serv_ephemeral_key);
int verify_proof_of_ownership_authentication(uint8_t * client_proof_of_ownership, int proof_client_len, uint8_t * client_ephemeral_key, int ephemeral_client_len, uint8_t * return_value, uint8_t * session_key_info, int * len_of_session_key);
int transfer_certificate_for_authentication(uint16_t cert_evaluation_id, uint8_t * cert_client, int cert_len, uint8_t * return_value);
int request_challenge_for_authentication(uint8_t commn_config, uint8_t * algorithmIndicator, uint8_t * return_value, uint8_t * challenge_server, int * len_of_challenge_server, uint8_t * additional_parameter, int *len_of_addition_data);
int verify_proof_of_ownership_unidirectional_authentication(uint8_t * algorithmIndicator, uint8_t * client_proof_of_ownership, int proof_client_len, uint8_t * challenge_client, int challenge_len, uint8_t * additional_parameter, int len_of_addition_data, uint8_t * return_value, uint8_t * session_key_info, int * len_of_session_key);
int verify_proof_of_ownership_bidirectional_authentication(uint8_t * algorithmIndicator, uint8_t * client_proof_of_ownership, int proof_client_len, uint8_t * challenge_client, int challenge_len, uint8_t * additional_parameter, int len_of_addition_data, uint8_t * return_value, uint8_t * server_proof_of_ownership, int * len_of_serv_proof_of_ownership, uint8_t * session_key_info, int * len_of_session_key);
int authentication_configuration(uint8_t * return_value);
int stop_periodic_data_trasmission(uint8_t *data, int data_len);
int secure_data_transmission_for_internal_service(uint16_t Administrative_Param, uint8_t signature, uint16_t anti_replay_counter, uint8_t  internal_msg_service_id, uint8_t * service_specific_rec, int no_of_serv_rec, uint8_t * signature_byte, int sig_len, uint8_t * data_record, int * data_rec_len);
int read_data_by_periodic_identifier(uint8_t trans_mode, uint16_t *data_id, int data_len);
/*response on event*/
int stop_the_response_on_event(uint8_t event_window_time);
int on_change_of_dtc_status_trigger_the_event(uint8_t event_windowtime, uint8_t * event_type_record, int event_record_len, uint8_t * serviceToRespondToRecord, int service_record_len, uint8_t * numberOfIdentifiedEvents);
int on_change_of_data_identifier_trigger_the_event(uint8_t event_windowtime, uint8_t * event_type_record, int event_record_len, uint8_t * serviceToRespondToRecord, int service_record_len, uint8_t * numberOfIdentifiedEvents);
int report_activated_events(uint8_t * numberOfActivatedEvents, uint8_t * ActiveeventWindowTime, uint8_t * eventTypeOfActiveEvent, uint8_t ** ActiveeventTypeRecord, int * Active_event_len, uint8_t ** ActiveserviceToRespondToRecord, int * Active_service_rec_len);
int start_response_on_event(uint8_t eventWindowTime, uint8_t * numberOfActivatedEvents);
int clear_response_on_event(uint8_t eventWindowTime, uint8_t * numberOfActivatedEvents);
int on_comparision_of_values( uint8_t eventWindowTime, uint8_t * event_type_record, int event_record_len, uint8_t * serviceToRespondToRecord, int service_record_len, uint8_t * data_record, int * data_rec_len);
int report_most_recent_dtc_on_status_change_record(uint8_t  eventWindowTime, uint8_t * eventTypeParameter, int event_record_len, uint8_t * data_record, int * data_rec_len);
int report_dtc_record_information_on_dtc_status_change(uint8_t  eventWindowTime, uint8_t * eventTypeParameter, int event_record_len, uint8_t * data_record, int * data_rec_len);


#endif
