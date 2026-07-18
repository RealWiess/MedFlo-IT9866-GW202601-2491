/*********************************************************************

*********************************************************************/
#ifdef	CFG_NAND_POWER_DOWN_PROTECT
#define	GPIO_POWER_DETECT_PIN	(CFG_SN1_GPIO_PWN)	//(CFG_GPIO_SN1_PWN)
#else
#define	GPIO_POWER_DETECT_PIN	(13)
#endif


#define	PD_INT_LEVLE_TRIGGER    (1)
#define	PD_INT_EDGE_TRIGGER     (0)

#define	PD_ACTIVE_LOW           (0)
#define	PD_ACTIVE_HIGH          (1)
/*********************************************************************

*********************************************************************/
#define	ENABLE_POWER_DOWN_PROTECT
#define	ENABLE_POWER_HANDLE_PIN

#ifdef ENABLE_POWER_HANDLE_PIN
	#define	GPIO_POWER_HANDLE_PIN	(14)
#else
	#define	GPIO_POWER_HANDLE_PIN	(255)
#endif
/*********************************************************************

*********************************************************************/
extern int init_power_detect(void);
extern void doPowerDownProtectNand(int tag);
/*********************************************************************

*********************************************************************/

