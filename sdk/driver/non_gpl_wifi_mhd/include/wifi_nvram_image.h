#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6256
/*
 *  #AP6256_NVRAM_V1.1_08252017
 *  # Cloned from bcm94345wlpagb_p2xx.txt  
 */
 
const char wifi_nvram_image[] = {
		//#WL72456_NVRAM_V1.1_07182019.txt FCC pre_test
		"NVRAMRev=$Rev: 498373 $"											"\x00"
		"sromrev=11"														"\x00"
		"vendid=0x14e4"														"\x00"
		"devid=0x43ab"														"\x00"
		"manfid=0x2d0"														"\x00"
		"prodid=0x06e4"														"\x00"
		"macaddr=00:90:4c:c5:12:38"                                         "\x00"
		"nocrc=1"															"\x00"
		"boardtype=0x6e4"													"\x00"
		"boardrev=0x1304"													"\x00"
		//#XTAL 37.4MHz
		"xtalfreq=37400"													"\x00"
		"btc_mode=1"														"\x00"
		//#-----------------------------------------
		//#boardflags: 5GHz eTR switch by default
		//#            2.4GHz eTR switch by default
		//#            bit1 for btcoex
		"boardflags=0x00480201"												"\x00"
		"boardflags2=0x40800000"											"\x00"
		"boardflags3=0x48200100"											"\x00"
		"phycal_tempdelta=15"												"\x00"
		"rxchain=1"															"\x00"
		"txchain=1"															"\x00"
		"aa2g=1"															"\x00"
		"aa5g=1"															"\x00"
		"tssipos5g=1"														"\x00"
		"tssipos2g=1"														"\x00"
		"femctrl=0"															"\x00"
		"AvVmid_c0=1,165,2,100,2,100,2,100,2,100"							"\x00"
		"pa2ga0=-109,6909,-723"												"\x00"
		//#1pa2ga0=-195,5558,-672
		"pa2ga1=-118,4850,-635"												"\x00"
		"pa5ga0=-205,5588,-688,-206,5724,-708,-203,5872,-721,-199,5898,-719"	"\x00"
		//#3pa5ga0=-214,6002,-737,-209,6102,-746,-209,6024,-738,-200,6149,-747
		//#2pa5ga0=-203,5495,-683,-205,5489,-682,-207,5494,-678,-208,5549,-683
		//#1pa5ga0=-203,5495,-683,-205,5489,-682,-207,5494,-678,-185,6130,-740
		"pa5ga1=-142,4930,-655,-150,4859,-646,-156,4887,-644,-158,4864,-641"	"\x00"
		"itrsw=1"															"\x00"
		"pdoffset2g40ma0=10"												"\x00"
		"pdoffset40ma0=0xaaaa"												"\x00"
		"pdoffset80ma0=0xaaaa"												"\x00"
		"extpagain5g=2"														"\x00"
		"extpagain2g=2"														"\x00"
		"tworangetssi2g=0"													"\x00"
		"tworangetssi5g=0"													"\x00"
		//# LTECX flags
		//# WCI2
		"ltecxmux=0"														"\x00"
		"ltecxpadnum=0x0504"												"\x00"
		"ltecxfnsel=0x22"													"\x00"
		"ltecxgcigpio=0x32"													"\x00"
		"maxp2ga0=78"														"\x00"
		"cckbw202gpo=0x3333"												"\x00"
		"cckbw20ul2gpo=0x5555"												"\x00"
		"ofdmlrbw202gpo=0x2245"												"\x00"
		"dot11agofdmhrbw202gpo=0x7744"										"\x00"
		"ccode=DE"															"\x00"
		"regrev=0"															"\x00"
		"mcsbw202gpo=0x98886422"											"\x00"
		"mcsbw402gpo=0xecaaaa88"											"\x00"
		"maxp5ga0=73,73,74,74"												"\x00"
		//#5GHz_20MHz Offset
		"mcsbw205glpo=0xba755544"											"\x00"
		"mcsbw205gmpo=0xba755544"											"\x00"
		"mcsbw205ghpo=0xba755544"											"\x00"
		//#5GHz_40MHz Offset
		"mcsbw405glpo=0xda777777"											"\x00"
		"mcsbw405gmpo=0xda777777"											"\x00"
		"mcsbw405ghpo=0xea777776"											"\x00"
		//#5GHz_80MHz Offset
		"mcsbw805glpo=0xda777777"											"\x00"
		"mcsbw805gmpo=0xda777777"											"\x00"
		"mcsbw805ghpo=0xda777777"											"\x00"
		"swctrlmap_2g=0x00000000,0x00000000,0x00000000,0x010000,0x3ff"		"\x00"
		"swctrlmap_5g=0x00100010,0x00200020,0x00200020,0x010000,0x3fe"		"\x00"
		"swctrlmapext_5g=0x00000000,0x00000000,0x00000000,0x000000,0x3"		"\x00"
		"swctrlmapext_2g=0x00000000,0x00000000,0x00000000,0x000000,0x3"		"\x00"
		"vcodivmode=1"														"\x00"
		"deadman_to=481500000"												"\x00"
		"ed_thresh2g=-54"													"\x00"
		"ed_thresh5g=-54"													"\x00"
		"eu_edthresh2g=-54"													"\x00"
		"eu_edthresh5g=-54"													"\x00"
		//#ed_thresh2g=-69
		//#ed_thresh5g=-69
		//#eu_edthresh2g=-69
		//#eu_edthresh5g=-69
		"ldo1=4"															"\x00"
		"rawtempsense=0x1ff"												"\x00"
		"cckPwrIdxCorr=3"													"\x00"
		"cckTssiDelay=150"													"\x00"
		"ofdmTssiDelay=150"													"\x00"
		"txpwr2gAdcScale=1"													"\x00"
		"txpwr5gAdcScale=1"													"\x00"
		"dot11b_opts=0x3aa85"												"\x00"
		"cbfilttype=0"														"\x00"
		//#fdsslevel_ch1=6
		//#fdsslevel_ch11=4
		//#btc_mode=1  
		"muxenab=0x110"														"\x00"
		"wowl_gpio=2"                                                       "\x00"
		"wowl_extgpio=2"                                                    "\x00"
		"wowl_gpiopol=0"                                                    "\x00"
		"cckdigfilttype=5"													"\x00"
		"fdss_level_2g=6"													"\x00"
		//#fdss_level_5g=6
		//#pacalshift5g=0,0,1
        "\x00\x00"
};
#endif	//#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6256

#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6236
/*
 *  #AP6256_NVRAM
 */

const char wifi_nvram_image[] = {
    "manfid=0x2d0"                                                                              "\x00"
    "prodid=0x0727"                                                                             "\x00"
    "vendid=0x14e4"                                                                             "\x00"
    "devid=0x43e2"                                                                              "\x00"
    "boardtype=0x0727"                                                                          "\x00"
    "boardrev=0x1331"                                                                           "\x00"
    "boardnum=22"                                                                               "\x00"
    "macaddr=00:90:4c:c5:12:38"                                                                 "\x00"
    "sromrev=11"                                                                                "\x00"
    "boardflags=0x00404201"                                                                     "\x00"
    "boardflags3=0x08000000"                                                                    "\x00"
    "xtalfreq=26000"                                                                            "\x00"
    "nocrc=1"                                                                                   "\x00"
    "ag0=255"                                                                                   "\x00"
    "aa2g=1"                                                                                    "\x00"
    "ccode=ALL"                                                                                 "\x00"
    "pa0itssit=0x20"                                                                            "\x00"
    "extpagain2g=0"                                                                             "\x00"
    "pa2ga0=-202,5582,-671"                                                                     "\x00"
    "AvVmid_c0=0x0,0xc8"                                                                        "\x00"
    "cckpwroffset0=5"                                                                           "\x00"
    "maxp2ga0=74"                                                                               "\x00"
    "txpwrbckof=6"                                                                              "\x00"
    "cckbw202gpo=0x2222"                                                                        "\x00"
    "legofdmbw202gpo=0x55555555"                                                                "\x00"
    "mcsbw202gpo=0x76666666"                                                                    "\x00"
    "propbw202gpo=0xcc"                                                                         "\x00"
    "ofdmdigfilttype=18"                                                                        "\x00"
    "ofdmdigfilttypebe=18"                                                                      "\x00"
    "papdmode=1"                                                                                "\x00"
    "pacalidx2g=42"                                                                             "\x00"
    "papdepsoffset=-22"                                                                         "\x00"
    "papdendidx=58"                                                                             "\x00"
    "ltecxmux=0"                                                                                "\x00"
    "ltecxpadnum=0x0102"                                                                        "\x00"
    "ltecxfnsel=0x44"                                                                           "\x00"
    "ltecxgcigpio=0x01"                                                                         "\x00"
    "il0macaddr=00:90:4c:c5:12:38"                                                              "\x00"
    "wl0id=0x431b"                                                                              "\x00"
    "deadman_to=0xffffffff"                                                                     "\x00"
    "muxenab=0x10"                                                                              "\x00"
    "spurconfig=0x3 "                                                                           "\x00"
    "AvVmidIQcal=0x2,0xa8"                                                                      "\x00"
    "\x00\x00"
};
#endif	//#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6236

#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6212
/*
 *  #AP6212A_NVRAM
 *  # 2.4 GHz, 20 MHz BW mode
 */

const char wifi_nvram_image[] = {
    //"#AP6212A_NVRAM_V1.0.2_20191121"					"\x00"
    //" 2.4 GHz, 20 MHz BW mode"					"\x00"

    //" The following parameter values are just placeholders, need to be updated."					"\x00"
    "manfid=0x2d0"					"\x00"
    "prodid=0x0726"					"\x00"
    "vendid=0x14e4"					"\x00"
    "devid=0x43e2"					"\x00"
    "boardtype=0x0726"					"\x00"
    "boardrev=0x1101"					"\x00"
    "boardnum=22"					"\x00"
    "macaddr=00:90:4c:c5:12:38"					"\x00"
    "sromrev=11"					"\x00"
    "boardflags=0x00404201"					"\x00"
    "xtalfreq=26000"					"\x00"
    "nocrc=1"					"\x00"
    "ag0=255"					"\x00"
    "aa2g=1"					"\x00"
    "ccode=ALL"					"\x00"

    "pa0itssit=0x20"					"\x00"
    "extpagain2g=0"					"\x00"

    //"PA parameters for 2.4GHz, measured at CHIP OUTPUT"					"\x00"
    "pa2ga0=-215,5267,-656"					"\x00"
    "AvVmid_c0=0x0,0xc8"					"\x00"
    "cckpwroffset0=5"					"\x00"

    //" PPR params"					"\x00"
    "maxp2ga0=80"					"\x00"
    "txpwrbckof=6"					"\x00"
    "cckbw202gpo=0x6666"					"\x00"
    "legofdmbw202gpo=0xaaaaaaaa"					"\x00"
    "mcsbw202gpo=0xbbbbbbbb"					"\x00"

    //" OFDM IIR :"					"\x00"
    "ofdmdigfilttype=18"					"\x00"
    "ofdmdigfilttypebe=18"					"\x00"
    //" PAPD mode:"					"\x00"
    "papdmode=2"					"\x00"

    "il0macaddr=00:90:4c:c5:12:38"					"\x00"
    "wl0id=0x431b"					"\x00"

    //"OOB parameters"					"\x00"
    "hostwake=0x40"					"\x00"
    "hostrdy=0x41"					"\x00"
    "usbrdy=0x03"					"\x00"
    "usbrdydelay=100"					"\x00"
    "deadman_to=0xffffffff"					"\x00"
    //" muxenab: 0x1 for UART enable, 0x10 for Host awake"					"\x00"
    "muxenab=0x10"					"\x00"    
//    "sd_gpout=0"                                                        "\x00"
//    "sd_gpval=1"                                                        "\x00"      
    //" CLDO PWM voltage settings - 0x4 - 1.1 volt"					"\x00"
    //"cldo_pwm=0x4"					"\x00"
	"glitch_based_crsmin=1"					"\x00"
	"noccpwrlmt=1"					"\x00"
    "\x00\x00"
};
#endif	//#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6212

#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6203
/*
 *  #AP6201BM_NVRAM_V1.0.1_20191213
 *  # Sample variables file for BCM943012 BU board
 */

const char wifi_nvram_image[] = {
    "NVRAMRev=$Rev: 351687 $"												"\x00"
    "sromrev=11"												"\x00"
    "etmode=0x11"												"\x00"
    "cckdigfilttype=4"												"\x00"
    "bphyscale=0x28"												"\x00"
    "boardflags3=0x40000101"												"\x00"
    "vendid=0x14e4"												"\x00"
    "devid=0xA804"												"\x00"
    "manfid=0x2d0"												"\x00"
    "prodid=0x052e"												"\x00"
    "macaddr=00:90:4c:c5:12:38"												"\x00"
    "nocrc=1"												"\x00"
    "boardtype=0x080e"												"\x00"
    "boardrev=0x1103"												"\x00"
    "lpflags=0x00000020"												"\x00"
    "xtalfreq=37400"												"\x00"
    "boardflags2=0xc0800000"												"\x00"
    "boardflags=0x00400001"												"\x00"
    "extpagain2g=2"												"\x00"
    "extpagain5g=2"												"\x00"
    "ccode=CN38"												"\x00"
    "regrev=0 "												"\x00"
    "antswitch = 0"												"\x00"
    "rxgains2gelnagaina0=0"												"\x00"
    "rxgains2gtrisoa0=15"												"\x00"
    "rxgains2gtrelnabypa0=0"												"\x00"
    "rxgains5gelnagaina0=0"												"\x00"
    "rxgains5gtrisoa0=9"												"\x00"
    "rxgains5gtrelnabypa0=0"												"\x00"
    "pdgain5g=0"												"\x00"
    "pdgain2g=0"												"\x00"
    "tworangetssi2g=0"												"\x00"
    "tworangetssi5g=0"												"\x00"
    "rxchain=1"												"\x00"
    "txchain=1"												"\x00"
    "aa2g=1"												"\x00"
    "aa5g=1"												"\x00"
    "tssipos5g=0"												"\x00"
    "tssipos2g=0"												"\x00"
    "tssisleep_en=0x5"												"\x00"
    "femctrl=17"												"\x00"
    "subband5gver=4"												"\x00"

    //"pa2ga0=-102,4805,-583"												"\x00"
    //"pa2ga0=-137,5218,-633"												"\x00"
    //"pa2ga0=-166,5339,-645"												"\x00"
    //"pa2ga0=-172,5143,-642"												"\x00"
    "pa2ga0=-152,5210,-638"												"\x00"
    //"pa2ga0=-135,5777,-677"												"\x00"

    //"pa5ga0=-194,5367,-658,-188,5286,-649,-166,5358,-653,-138,5140,-626"												"\x00"
    //"pa5ga0=-121,5285,-640,-113,5338,-646,-130,5574,-673,-160,5405,-665"												"\x00"
    //"pa5ga0=-175,5263,-641,-165,5229,-633,-140,5294,-630,-135,5255,-625"												"\x00"
    //"pa5ga0=-180,5167,-636,-170,5163,-634,-146,5281,-636,-143,5343,-638"												"\x00"
    //"pa5ga0=-152,5161,-631,-145,5181,-632,-128,5272,-640,-137,5228,-638"												"\x00"
    //"pa5ga0=-144,5209,-628,-135,5243,-631,-126,5280,-639,-124,5343,-644"												"\x00"
    "pa5ga0=-136,5180,-626,-130,5138,-623,-100,5343,-649,-99,5329,-650"												"\x00"
    //"pa5ga0=-164,5243,-632,-161,5228,-632,-139,5332,-637,-141,5307,-640"												"\x00"


    "cckpwroffset0=2"												"\x00"
    "pdoffset40ma0=0"												"\x00"
    "pdoffset80ma0=0"												"\x00"
    "lowpowerrange2g=0"												"\x00"
    "lowpowerrange5g=0"												"\x00"
    "ed_thresh2g=-63"												"\x00"
    "ed_thresh5g=-63"												"\x00"
    //"swctrlmap_2g=0x00000000,0x00400040, 0x00400040,0x000000,0x3e7"												"\x00"
    //"swctrlmapext_2g=0x00020002,0x00000000, 0x00000000,0x000000,0x003"												"\x00"
    //"swctrlmap_5g=0x00000000,0x00000000,0x00000000,0x000000,0x3a7"												"\x00"
    //"swctrlmapext_5g=0x00000000,0x00010001, 0x00010001,0x000000,0x001"												"\x00"
    "swctrlmap_2g=0x00000000,0x00400040, 0x00400040,0x204040,0x3e7"												"\x00"
    "swctrlmapext_2g=0x00020002,0x00000000, 0x00000000,0x000000,0x003"												"\x00"
    "swctrlmap_5g=0x00000000,0x00800080,0x00800080,0x000000,0x387"												"\x00"
    "swctrlmapext_5g=0x00010001,0x00000000, 0x00000000,0x000000,0x003"												"\x00"
    "ulpnap=0"												"\x00"
    "ulpadc=1"												"\x00"
    "ssagc_en=0"												"\x00"
    "ds1_nap=0"												"\x00"
    "epacal2g=0"												"\x00"
    "epacal5g=0"												"\x00"
    "epacal2g_mask=0x3fff"												"\x00"

    "maxp2ga0=81"												"\x00"
    "cckbw202gpo=0x1100"												"\x00"
    "ofdmlrbw202gpo=0x0011"												"\x00"
    "dot11agofdmhrbw202gpo=0x4211"												"\x00"
    "mcsbw202gpo=0x99642100"												"\x00"
    "mac_clkgating=1"												"\x00"
    //"mcsbw402gpo=0x99555533"												"\x00"

    "maxp5ga0=70,70,70,70"												"\x00"
    "mcsbw205glpo=0x99877666"												"\x00"
    "mcsbw205gmpo=0x99877666"												"\x00"
    "mcsbw205ghpo=0x99877666"												"\x00"

    //"mcsbw405glpo=0x99555000"												"\x00"
    //"mcsbw405gmpo=0x99555000"												"\x00"
    //"mcsbw405ghpo=0x99555000"												"\x00"
    //"mcsbw805glpo=0x99555000"												"\x00"
    //"mcsbw805gmpo=0x99555000"												"\x00"
    //"mcsbw805ghpo=0x99555000"												"\x00"

    "txwbpapden=1"												"\x00"
    "femctrlwar=0"												"\x00"
    "use5gpllfor2g=1"												"\x00"

    //"tx papd cal params"												"\x00"
    //"params are - 0x5g2g "												"\x00"
    "wb_rxattn=0x0303"												"\x00"
    "wb_txattn=0x0203"												"\x00"
    "wb_papdcalidx=0x1C05"												"\x00"
    "wb_eps_offset=0x01c101ad"												"\x00"
    "wb_bbmult=0x3C50"												"\x00"
    "wb_calref_db=0x1926"												"\x00"
    "wb_tia_gain_mode=0x0606"												"\x00"
    "wb_txbuf_offset=0x2020"												"\x00"
    "wb_frac_del=0x60b4"												"\x00"
    "wb_g_frac_bits=0xBA"												"\x00"

    "nb_rxattn=0x0403"												"\x00"
    "nb_txattn=0x0402"												"\x00"
    "nb_papdcalidx= 0x1405"												"\x00"
    "nb_eps_offset= 0x01d701ca"												"\x00"
    "nb_bbmult= 0x5A50"												"\x00"
    "nb_tia_gain_mode=0x0006"												"\x00"
    "AvVmid_c0=6,104,7,80,7,80,7,80,7,80"												"\x00"

    "lpo_select=4"												"\x00"

    "csml=0x10"												"\x00"
    "pt5db_gaintbl=0"												"\x00"

    "papdcck=0"												"\x00"
    "phycal_tempdelta=15"												"\x00"
    "ofdmfilttype_2gbe=1"												"\x00"

    "wowl_gpio=1"                                              	"\x00"
    "wowl_extgpio=2"                                          	"\x00"
    "wowl_gpiopol=0"                                          	"\x00"

    //"deadman_to=1"												"\x00"
    "btc_mode=0"												"\x00"
    "muxenab=0x1"												"\x00"
    "\x00\x00"
};	
//char *wifi_nvram_ptr = (char *)wifi_nvram_image;
//int wifi_nvram_size = sizeof(wifi_nvram_image);
#endif //#ifdef CFG_NET_WIFI_SDIO_NGPL_AP6203

#ifdef CFG_NET_WIFI_SDIO_NGPL_SYN4345x
/*
 *  # Cloned from bcm94345wlselg.txt
 *  # $ Copyright Broadcom $
 *
 * Nov 29, 2018: Updated by nikunjk@google.com to address b/118138937.
 */
static const char wifi_nvram_image[] = {
"NVRAMRev=$Rev: 498373 $"																			"\x00"
"sromrev=11"																			"\x00"
"vendid=0x14e4"																			"\x00"
"devid=0x43ab"																			"\x00"
"manfid=0x2d0"																			"\x00"
"prodid=0x06e4"																			"\x00"
"macaddr=00:90:4c:c5:10:38"																			"\x00"
"nocrc=1"																			"\x00"
"boardtype=0x6e4"																			"\x00"
"boardrev=0x1304"																			"\x00"
"xtalfreq=37400"																			"\x00"
"boardflags=0x00480201"																			"\x00"
"boardflags2=0x40800000"																			"\x00"
"boardflags3=0x48200100"																			"\x00"
"phycal_tempdelta=15"																			"\x00"
"rxchain=1"																			"\x00"
"txchain=1"																			"\x00"
"aa2g=1"																			"\x00"
"aa5g=1"																			"\x00"
"tssipos5g=1"																			"\x00"
"tssipos2g=1"																			"\x00"
"femctrl=0"																			"\x00"
"AvVmid_c0=1,165,2,100,2,100,2,100,2,100"																			"\x00"
"pa2ga0=-184,5960,-692"																			"\x00"
"pa5ga0=-198,6349,-768,-185,6602,-783,-195,6419,-770,-190,6552,-782"																			"\x00"
"pdoffsetcckma0=0x3"																			"\x00"
"itrsw=1"																			"\x00"
"pdoffset2g40ma0=10"																			"\x00"
"pdoffset40ma0=0xaaaa"																			"\x00"
"pdoffset80ma0=0xaaaa"																			"\x00"
"extpagain5g=2"																			"\x00"
"extpagain2g=2"																			"\x00"
"tworangetssi2g=0"																			"\x00"
"tworangetssi5g=0"																			"\x00"
"ltecxmux=0"																			"\x00"
"ltecxpadnum=0x0504"																			"\x00"
"ltecxfnsel=0x22"																			"\x00"
"ltecxgcigpio=0x32"																			"\x00"
"maxp2ga0=80"																			"\x00"
"ofdmlrbw202gpo=0x0022"																			"\x00"
"dot11agofdmhrbw202gpo=0x4442"																			"\x00"
"mcsbw202gpo=0x98444422"																			"\x00"
"mcsbw402gpo=0x98444422"																			"\x00"
"maxp5ga0=82,82,82,82"																			"\x00"
"mcsbw205glpo=0xb9555000"																			"\x00"
"mcsbw205gmpo=0xb9555000"																			"\x00"
"mcsbw205ghpo=0xb9555000"																			"\x00"
"mcsbw405glpo=0xb9555000"																			"\x00"
"mcsbw405gmpo=0xb9555000"																			"\x00"
"mcsbw405ghpo=0xb9555000"																			"\x00"
"mcsbw805glpo=0xb9555000"																			"\x00"
"mcsbw805gmpo=0xb9555000"																			"\x00"
"mcsbw805ghpo=0xb9555000"																			"\x00"

"swctrlmap_2g=0x00000000,0x00000000,0x00000000,0x010000,0x3ff"																			"\x00"
"swctrlmap_5g=0x00100010,0x00200020,0x00200020,0x010000,0x3fe"																			"\x00"
"swctrlmapext_5g=0x00000000,0x00000000,0x00000000,0x000000,0x3"																			"\x00"
"swctrlmapext_2g=0x00000000,0x00000000,0x00000000,0x000000,0x3"																			"\x00"

"vcodivmode=1"																			"\x00"
"deadman_to=481500000"																			"\x00"
"muxenab=0x10"																				"\x00"
"ed_thresh2g=-54"																			"\x00"
"ed_thresh5g=-54"																			"\x00"
"eu_edthresh2g=-54"																			"\x00"
"eu_edthresh5g=-54"																			"\x00"
"ldo1=4"																			"\x00"
"rawtempsense=0x1ff"																			"\x00"
"cckPwrIdxCorr=3"																			"\x00"
"cckTssiDelay=150"																			"\x00"
"ofdmTssiDelay=150"																			"\x00"
"txpwr2gAdcScale=1"																			"\x00"
"txpwr5gAdcScale=1"																			"\x00"
"dot11b_opts=0x3aa85"																			"\x00"
"cbfilttype=2"																			"\x00"
"fdsslevel_ch1=6"																			"\x00"
"fdsslevel_ch11=4 "																			"\x00"
	"\x00\x00"
};
#endif //#ifdef CFG_NET_WIFI_SDIO_NGPL_SYN4345x

#ifdef CFG_NET_WIFI_SDIO_NGPL_SYN4343x
/*
 *  # Cloned from bcm943438wlpth_P400_26MHz.txt
 *  # $ Copyright Broadcom $
 *
 */
static const char wifi_nvram_image[] = {
"manfid=0x2d0"																			"\x00"
"prodid=0x0727"																			"\x00"
"vendid=0x14e4"																			"\x00"
"devid=0x43e2"																			"\x00"
"boardtype=0x0727"																			"\x00"
"boardrev=0x1400"																			"\x00"
"boardnum=22"																			"\x00"
"macaddr=00:90:4c:c5:12:38"																			"\x00"
"sromrev=11"																			"\x00"
"boardflags=0x00404201"																			"\x00"
"boardflags3=0x08000000"																			"\x00"
"xtalfreq=26000"																			"\x00"
"nocrc=1"																			"\x00"
"ag0=255"																			"\x00"
"aa2g=1"																			"\x00"
"ccode=ALL"																			"\x00"
"pa0itssit=0x20"																			"\x00"
"extpagain2g=0"																			"\x00"
"pa2ga0=-203,6150,-732"																			"\x00"
"AvVmid_c0=0x0,0xc8"																			"\x00"
"cckpwroffset0=5"																			"\x00"
"maxp2ga0=78"																			"\x00"
"txpwrbckof=6"																			"\x00"
"cckbw202gpo=0"																			"\x00"
"legofdmbw202gpo=0x66111111"																			"\x00"
"mcsbw202gpo=0x76611111"																			"\x00"
"propbw202gpo=0xcc"																			"\x00"
"ofdmdigfilttype=18"																			"\x00"
"ofdmdigfilttypebe=18"																			"\x00"
"papdmode=1"																			"\x00"
"il0macaddr=00:90:4c:c5:12:38"																			"\x00"
"wl0id=0x431b"																			"\x00"
"deadman_to=0xffffffff"																			"\x00"
"muxenab=0x10"																			"\x00"
"spurconfig=0x13"																			"\x00"
"AvVmidIQcal=0x2,0xa8"																			"\x00"
"glitch_based_crsmin=1"																			"\x00"
"edonthd20l=-62"																			"\x00"
"edoffthd20ul=-68"																			"\x00"
"edonthd20l_adj=2"																			"\x00"
"edoffthd20ul_adj=2"																			"\x00"
	"\x00\x00"
};
#endif //#ifdef CFG_NET_WIFI_SDIO_NGPL_SYN4343x