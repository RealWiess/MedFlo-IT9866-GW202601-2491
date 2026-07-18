#include "ssp/armNorWriterUtility.h"

void ArmNorWriterCleanState(void)
{
	ithWriteRegH(ARM_NOR_WRITER_STATUS,         (uint16_t)ANW_STATUS_NOT_READY);
	ithWriteRegH(ARM_NOR_WRITER_ROM_BUF_ADDR_L, 0);
	ithWriteRegH(ARM_NOR_WRITER_ROM_BUF_ADDR_H, 0);
	ithWriteRegH(ARM_NOR_WRITER_RB_BUF_ADDR_L,  0);
	ithWriteRegH(ARM_NOR_WRITER_RB_BUF_ADDR_H,  0);
	ithWriteRegH(ARM_NOR_WRITER_ROM_LENGTH_L,   0);
	ithWriteRegH(ARM_NOR_WRITER_ROM_LENGTH_H,   0);
	ithWriteRegH(ARM_NOR_WRITER_APP_STATUS,     (uint16_t)ANW_APP_STATUS_OK);
}

void SetArmNorWriterStatus(
	ANW_STATUS writerStatus)
{
	ithWriteRegH(ARM_NOR_WRITER_STATUS, (uint16_t)writerStatus);
}

ANW_STATUS GetArmNorWriterStatus(void)
{
    uint16_t value = 0;
    ANW_STATUS temp = ANW_STATUS_UNKNOWN;

    value = ithReadRegH(ARM_NOR_WRITER_STATUS);

    if (value == 0U) {temp = ANW_STATUS_NOT_READY;}
    else if (value == 1U) {temp = ANW_STATUS_WAIT_TO_WRITE;}
    else if (value == 2U) {temp = ANW_STATUS_PC_SEND_DATA_FINISH;}
    else {temp = ANW_STATUS_UNKNOWN;}

    return temp;
}

void SetArmNorWriterAppStatus(
	ANW_APP_STATUS appStatus)
{
	ithWriteRegH(ARM_NOR_WRITER_APP_STATUS, (uint16_t)appStatus);
}

ANW_APP_STATUS GetArmNorWriterAppStatus(void)
{
    uint16_t value = 0;
    ANW_APP_STATUS temp = ANW_APP_STATUS_UNKNOWN;

    value = ithReadRegH(ARM_NOR_WRITER_APP_STATUS);

    if (value == 0U) {temp = ANW_APP_STATUS_OK;}
    else if (value == 1U) {temp = ANW_APP_STATUS_NOR_INIT_FAIL;}
    else if (value == 2U) {temp = ANW_APP_STATUS_OUT_OF_MEMORY;}
    else if (value == 3U) {temp = ANW_APP_STATUS_ROM_LENGTH_ZERO;}
    else if (value == 4U) {temp = ANW_APP_STATUS_ROM_LENGTH_OVER_MAX_BUF_LENGTH;}
    else if (value == 5U) {temp = ANW_APP_STATUS_WRITING_ERASE_NOR;}
    else if (value == 6U) {temp = ANW_APP_STATUS_WRITING_PAGES;}
    else if (value == 7U) {temp = ANW_APP_STATUS_READING_PAGES;}
    else if (value == 8U) {temp = ANW_APP_STATUS_WRITE_FINISH_COMPARE_FAIL;}
    else if (value == 9U) {temp = ANW_APP_STATUS_WRITE_FINISH_COMPARE_OK;}
    else {temp = ANW_APP_STATUS_UNKNOWN;}

    return temp;
}

void SetArmNorWriterBufLength(uint32_t romBufLength)
{
	ithWriteRegH(ARM_NOR_WRITER_ROM_LENGTH_L, (uint16_t)(romBufLength & 0xFFFFU));
	ithWriteRegH(ARM_NOR_WRITER_ROM_LENGTH_H, (uint16_t)(romBufLength >> 16));
}

uint32_t GetArmNorWriterBufLength(void)
{
	uint16_t value1 = 0;
	uint16_t value2 = 0;

	value1 = ithReadRegH(ARM_NOR_WRITER_ROM_LENGTH_L);
	value2 = ithReadRegH(ARM_NOR_WRITER_ROM_LENGTH_H);

	return ((((uint32_t)value2) << 16) | value1);
}

void SetArmNorWriterRomBufAddr(uint32_t romBufAddr)
{
	ithWriteRegH(ARM_NOR_WRITER_ROM_BUF_ADDR_L, (uint16_t)(romBufAddr & 0xFFFFU));
	ithWriteRegH(ARM_NOR_WRITER_ROM_BUF_ADDR_H, (uint16_t)(romBufAddr >> 16));
}

uint32_t GetArmNorWriterRomBufAddr(void)
{
	uint16_t value1 = 0;
	uint16_t value2 = 0;

	value1 = ithReadRegH(ARM_NOR_WRITER_ROM_BUF_ADDR_L);
	value2 = ithReadRegH(ARM_NOR_WRITER_ROM_BUF_ADDR_H);

	return ((((uint32_t)value2) << 16) | value1);
}

void SetArmNorWriterRbBufAddr(uint32_t rbBufAddr)
{
	ithWriteRegH(ARM_NOR_WRITER_RB_BUF_ADDR_L, (uint16_t)(rbBufAddr & 0xFFFFU));
	ithWriteRegH(ARM_NOR_WRITER_RB_BUF_ADDR_H, (uint16_t)(rbBufAddr >> 16));
}

void SetArmNorWriterAppProgressMin(uint32_t barMin)
{
	ithWriteRegH(ARM_NOR_WRITER_APP_BAR_MIN_L, (uint16_t)(barMin & 0xFFFFU));
	ithWriteRegH(ARM_NOR_WRITER_APP_BAR_MIN_H, (uint16_t)(barMin >> 16));
}

void SetArmNorWriterAppProgressMax(uint32_t barMax)
{
	ithWriteRegH(ARM_NOR_WRITER_APP_BAR_MAX_L, (uint16_t)(barMax & 0xFFFFU));
	ithWriteRegH(ARM_NOR_WRITER_APP_BAR_MAX_H, (uint16_t)(barMax >> 16));
}

void SetArmNorWriterAppProgressCurr(uint32_t barCurr)
{
	ithWriteRegH(ARM_NOR_WRITER_APP_BAR_CURR_L, (uint16_t)(barCurr & 0xFFFFU));
	ithWriteRegH(ARM_NOR_WRITER_APP_BAR_CURR_H, (uint16_t)(barCurr >> 16));
}

uint32_t GetArmNorWriterAppProgressMin(void)
{
	uint16_t value1 = 0;
	uint16_t value2 = 0;

	value1 = ithReadRegH(ARM_NOR_WRITER_APP_BAR_MIN_L);
	value2 = ithReadRegH(ARM_NOR_WRITER_APP_BAR_MIN_H);

	return ((((uint32_t)value2) << 16) | value1);
}

uint32_t GetArmNorWriterAppProgressMax(void)
{
	uint16_t value1 = 0;
	uint16_t value2 = 0;

	value1 = ithReadRegH(ARM_NOR_WRITER_APP_BAR_MAX_L);
	value2 = ithReadRegH(ARM_NOR_WRITER_APP_BAR_MAX_H);

	return ((((uint32_t)value2) << 16) | value1);
}

uint32_t GetArmNorWriterAppProgressCurr(void)
{
	uint16_t value1 = 0;
	uint16_t value2 = 0;

	value1 = ithReadRegH(ARM_NOR_WRITER_APP_BAR_CURR_L);
	value2 = ithReadRegH(ARM_NOR_WRITER_APP_BAR_CURR_H);

	return ((((uint32_t)value2) << 16) | value1);
}
