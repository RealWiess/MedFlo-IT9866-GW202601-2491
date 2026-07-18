
#ifndef __FSCBT_INTERFACE_H__
#define __FSCBT_INTERFACE_H__

#include <stdint.h>
#include "ite/ith.h"
#include "ite/itp.h"
#include "uart/uart.h"

#define AT_CMD_TX_QUEUE_LEN 20
#define AT_CMD_RX_QUEUE_LEN 100
#define AT_CMD_PAYLOAD_LEN 4096

#define DAC_BUFFER_SIZE (64*1024)
#define ADC_BUFFER_SIZE (64*1024)

typedef enum
{
    BT_A2DP_PLAY_START = 0,
    BT_A2DP_PLAY_STOP  = 1,
    BT_HFP_PLAY_START = 2,
    BT_HFP_PLAY_STOP = 3,
    BT_PLAY_STATE_END
} BT_PLAY_STATE_E;

typedef enum
{
    BLE_EASY_CONNECTION = 0,   /* for EASYCONNECTION */
    BLE_ERYA_CONNECTION = 1,   /* for ERYA CONNECTION */
    BLE_DEFAULT_CONNECTION = 2,   /* for DEFAULT CONNECTION */
    BLE_DEFAULT1_CONNECTION = 3,   /* for DEFAULT1 CONNECTION */
    BLE_CUSTOM_CONNECTION = 4,
    BLE_FACP2_CONNECTION = 5,
    BLE_CONNECTION_END
} BLE_CONNECTION_TYPE_E;

typedef enum
{
    AUD_PROCESS_SCO = 0,   		/* sco */
    AUD_PROCESS_PLAYBACK = 1,   /* playback  */
    AUD_PROCESS_PLAYBACKRES = 2,/* playbackres  */
    AUD_PROCESS_RINGTONE = 3,	/* ringtone */
    AUD_PROCESS_END			
}AUD_PROCESS_MODE_E;

typedef struct
{
	int uuid_len;
	unsigned char uuid[16];
}ble_custom_uuid_t;

typedef struct
{
	ITPDeviceType uart_type;
	const ITPDevice* uart_dev;	
	ITHUartPort uart_port;
	unsigned int uart_tx_pin;
	unsigned int uart_rx_pin;
	unsigned int bt_en_pin;
}bt_hw_cfg_t;

typedef int (* bt_play_state_callback)(BT_PLAY_STATE_E state, unsigned short samplerate, unsigned char channel);
typedef int (* bt_pcm_data_callback)(unsigned char* buffer, unsigned short length);

typedef struct
{
    void* tx_queue;                          /* AT-Command sent from upper layer application to blueware */
    void* rx_queue;                          /* AT-Response sent from blueware to upper layer application */
    int   a2dp_resampler;                    /* not support.default 0 */
    int   debug_mode;                        /* turn blueware debug log on/off */

    bt_play_state_callback play_state_cb;    /* play state callback function */
    bt_pcm_data_callback a2dp_cb;            /* a2dp pcm data callback function */
    bt_pcm_data_callback hfp_spk_cb;         /* hfp spk pcm data callback function */ 

    unsigned char   ble_connection_type;     /* BLE_CONNECTION_TYPE_E */
    unsigned char   ancs_enable;             /* ancs on/off */
    unsigned char   absvol_enable;           /* abs vol on/off */
   
	ble_custom_uuid_t service;				/*service uuid*/
    ble_custom_uuid_t notify;				/*notify uuid*/
    ble_custom_uuid_t write;				/*write uuid*/

}bt_sw_cfg_t;

/*!
    \brief:  initialise bluetooth stack (blueware)
    \param hw_cfg:  hardware configuration
     \param sw_cfg:  software configuration
*/
int fscbt_init(const bt_hw_cfg_t *hw_cfg,const bt_sw_cfg_t *sw_cfg);

/*!
    \brief:  blueware main loop , running in a standalone task
*/
void fscbt_run(void);

/*!
    \brief:  wake up blueooth task from sleeping
    \param from_isr:  set to 1 if call from isr, othewise set to 0
*/
void fscbt_wakeup(int from_isr);

/*!
    \brief:  blueware read database
*/
int bt_nvm_read(unsigned char* data, unsigned short len, unsigned char page);

/*!
    \brief:  blueware write database
*/
int bt_nvm_write(unsigned char* data,unsigned short len, unsigned char page);

/*!
    \brief:  get avrcp coverart image data..

    \param * pu32len: Image Data Length.
            0:No Data.
     \return: 
     JPG Image Data Address:
      start data: 0xff, 0xd8
      end data:0xff,0xd9
*/
void fscbt_get_coverart_data(char** pImage,unsigned int* pu32len);
/*!
    \brief:i2s DAC init
*/
void bt_aud_i2s_dac_init(int mode,int samplerate,unsigned char*dac_buf);
/*!
	\brief:i2s DAC deinit
*/
void bt_aud_i2s_dac_deinit(unsigned char*dac_buf);
/*!
	\brief:i2s ADC init
*/
void bt_aud_i2s_adc_init(int mode,int samplerate,unsigned char*adc_buf);
/*!
	\brief:i2s ADC deinit
*/
void bt_aud_i2s_adc_deinit(unsigned char*adc_buf);

#endif

