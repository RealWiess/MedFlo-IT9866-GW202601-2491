#define CHIP_8821CS 0
#define CHIP_BR8051 1


#ifdef CFG_BARROT_MODULE_8821CS
    int barrot_chip_type = CHIP_8821CS;
#elif CFG_BARROT_MODULE_BR8051
    int barrot_chip_type = CHIP_BR8051;
#else
    int barrot_chip_type = CHIP_8821CS;
#endif