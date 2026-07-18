/**
 * @file         Server_Implement.h
 *
 * @brief
 */

#ifndef SERVER_IMPLEMENT_H
#define SERVER_IMPLEMENT_H

int system_requirement_conditions_proper(void);
uint8_t get_ECU_session(void);
int set_ECU_to_default_session(void);
int set_ECU_to_programming_session(void);
int set_ECU_to_extended_diagnostic_session(void);
int set_ECU_to_system_saftey_diagnostic_session(void);
int set_ECU_to_vehicle_specific_session(void);
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
void generate_security_key(uint8_t *key_bytes, uint8_t *seed_bytes);
int process_request_upload_download_service(uint8_t *len_formt_id, uint8_t * max_number_of_blk_len);
int transfer_data_from_client_to_server(uint64_t start_memory_addr, int memory_size_length, uint8_t * transfer_rec, int rec_len);
int transfer_data_from_server_to_client(uint64_t start_memory_addr, int memory_size_length, uint8_t * transfer_rec, int rec_len);
int transfer_data_exit(int exit_type);
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
int enable_rx_and_tx(uint8_t communication_type);
int enable_rx_and_disable_tx(uint8_t communication_type);
int disable_rx_and_enable_tx(uint8_t communication_type);
int disable_rx_and_tx(uint8_t communication_type);
int enable_rx_and_disable_tx_with_enhanced_addr_info(uint8_t communication_type, uint16_t node_id);
int enable_rx_and_tx_with_enhanced_addr_info(uint8_t communication_type, uint16_t node_id);
int input_output_control_data_by_identifier(uint16_t data_id, uint8_t io_ctrl_param, uint8_t * in_control_option_record, int in_data_len, uint8_t * output_control_status_record, int * resp_len);
int link_control_transition_baudrate(unsigned long int baudrate);

#endif /* SERVER_IMPLEMENT_H */
