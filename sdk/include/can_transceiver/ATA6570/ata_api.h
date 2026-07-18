/*
 * ATA_API.h
 *
 */

#ifndef ATA_API_H_
#define ATA_API_H_

//=============================================================================
//                Structure Definition
//=============================================================================
typedef enum {
	ATA6570_CAN_50K,
	ATA6570_CAN_100K,
	ATA6570_CAN_125K,
	ATA6570_CAN_250K,
	ATA6570_CAN_Reserved01,
	ATA6570_CAN_500K,
	ATA6570_CAN_Reserved02,
	ATA6570_CAN_1000K,
} ATA6570_CAN_BAUDRATE;

typedef enum {
	ATA6570_OPMODE_Reserved0,
	ATA6570_OPMODE_SLEEP,
	ATA6570_OPMODE_Reserved2,
	ATA6570_OPMODE_Reserved3,
	ATA6570_OPMODE_STBY,
	ATA6570_OPMODE_Reserved5,
	ATA6570_OPMODE_Reserved6,
	ATA6570_OPMODE_NORMAL,
} ATA6570_CAN_OPMODE;

typedef enum {
	ATA6570_COMODE_STBY,
	ATA6570_COMODE_NORMAL01,
	ATA6570_COMODE_NORMAL10,
	ATA6570_COMODE_SILENT,
} ATA6570_CAN_COMODE;

//=============================================================================
//                Public Function Definition
//=============================================================================
/**
 * @brief Initialize canbus transceiver controller(ATA6570).
 * 
 * @param port SPI interface 0 or 1.
 * @param datarate Config data rate.
 */
void ithATA6570Init(uint8_t port, ATA6570_CAN_BAUDRATE datarate);

/**
 * @brief Get Devices operation mode.
 * 
 * @param port SPI interface 0 or 1.
 * @return ATA6570_CAN_OPMODE return mode id.
 *         0x001 Sleep mode
 *         0x100 Standby mode
 *         0x111 Normal mode     
 */
ATA6570_CAN_OPMODE ithATA6570GetOPMode(uint8_t port);

/**
 * @brief Get device operation mode transtion related information.
 * 
 * @param port SPI interface 0 or 1.
 * @return uint8_t bit5:Normal mode transition status
 *                 bit6:Overtemperature prewarning status
 *                 bit7:Sleep mode transition status
 */
uint8_t ithATA6570GetModeStatus(uint8_t port);

/**
 * @brief Get CAN TRX operation mode.
 * 
 * @param port SPI interface 0 or 1.
 * @return ATA6570_CAN_COMODE 
 *         0x00 TRX stanby mode
 *         0x01 TRX Normal mode (VCC undervoltage detection active)
 *         0x10 TRX Normal mode (VCC undervoltage detection inactive)
 *         0x11 TRX Silent mode
 */
ATA6570_CAN_COMODE ithATA6570GetCOMode(uint8_t port);

/**
 * @brief Set Devices operation mode.
 * 
 * @param port SPI interface 0 or 1.
 * @param mode Set mdoe id.
 *         0x001 Sleep mode
 *         0x100 Standby mode
 *         0x111 Normal mode 
 */
void ithATA6570SetOPMode(uint8_t port, ATA6570_CAN_OPMODE mode);

#endif /* SW_SPI_H_ */