static const uint16_t I2S_DIGVOL_TABLE[] = {
    0x000U, /* idx[ 0]:-INFdB */
    0x001U, /* idx[ 1]:-48 dB */
    0x002U, /* idx[ 2]:-42 dB */
    0x003U, /* idx[ 3]:-39 dB */
    0x004U, /* idx[ 4]:-36 dB */
    0x005U, /* idx[ 5]:-34 dB */
    0x006U, /* idx[ 6]:-33 dB */
    0x007U, /* idx[ 7]:-31 dB */
    0x008U, /* idx[ 8]:-30 dB */
    0x009U, /* idx[ 9]:-29 dB */
    0x00AU, /* idx[10]:-28 dB */
    0x00BU, /* idx[11]:-27 dB */
    0x00DU, /* idx[12]:-26 dB */
    0x00EU, /* idx[13]:-25 dB */
    0x010U, /* idx[14]:-24 dB */
    0x012U, /* idx[15]:-23 dB */
    0x014U, /* idx[16]:-22 dB */
    0x017U, /* idx[17]:-21 dB */
    0x01AU, /* idx[18]:-20 dB */
    0x01DU, /* idx[19]:-19 dB */
    0x020U, /* idx[20]:-18 dB */
    0x024U, /* idx[21]:-17 dB */
    0x029U, /* idx[22]:-16 dB */
    0x02EU, /* idx[23]:-15 dB */
    0x033U, /* idx[24]:-14 dB */
    0x039U, /* idx[25]:-13 dB */
    0x040U, /* idx[26]:-12 dB */
    0x048U, /* idx[27]:-11 dB */
    0x051U, /* idx[28]:-10 dB */
    0x05BU, /* idx[29]:- 9 dB */
    0x066U, /* idx[30]:- 8 dB */
    0x072U, /* idx[31]:- 7 dB */
    0x080U, /* idx[32]:- 6 dB */
    0x090U, /* idx[33]:- 5 dB */
    0x0A1U, /* idx[34]:- 4 dB */
    0x0B5U, /* idx[35]:- 3 dB */
    0x0CAU, /* idx[36]:- 2 dB */
    0x0E4U, /* idx[37]:- 1 dB */
    0x100U, /* idx[38]:  0 dB */
};

#define I2S_DIGVOL_TABLE_Size    (sizeof(I2S_DIGVOL_TABLE) / sizeof(uint16_t))

#define I2S_DIGITAL_VOL_MAX_IDX  (I2S_DIGVOL_TABLE_Size - 1) /* 0x100: 0 dB */
#define I2S_DIGITAL_VOL_ZERO_IDX (I2S_DIGVOL_TABLE_Size - 1) /* 0x100: 0 dB */
#define I2S_DIGITAL_VOL_MIN_IDX  0                           /* 0x000: -INFdB */
