#ifndef ITH_MEMBIST_H
#define ITH_MEMBIST_H

/** @addtogroup ith ITE Hardware Library
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ITH_MEMBIST_FD    = 0x0,  ///< Fixed data write/read test
    ITH_MEMBIST_SD    = 0x1,  ///< Sequence data write/read test
    ITH_MEMBIST_FDFB  = 0x2,  ///< Fixed data write/read test with fixed byte enable
    ITH_MEMBIST_SDFB  = 0x3,  ///< Sequence data write/read test with fixed byte enable
    ITH_MEMBIST_RD    = 0x4,  ///< Random data write/read test
    ITH_MEMBIST_RDA   = 0x5,  ///< Random data/address write/read test
    ITH_MEMBIST_RDRB  = 0x6,  ///< Random data write/read test with random byte enable     
    ITH_MEMBIST_RDARB = 0x7,  ///< Random data/address write/read test with random byte enable       
} ITHMemBistMode;

void ithMEMBISTEnable(unsigned int bist_id);
void ithMEMBISTDisable(unsigned int bist_id);
void ithMEMBIST_SET_MODE(unsigned int bist_id, ITHMemBistMode mode);
void ithMEMBIST_SET_STARTADD(unsigned int bist_id, unsigned int addr);
void ithMEMBIST_SET_ENDADD(unsigned int bist_id, unsigned int addr);
void ithMEMBIST_WRITE_LOWBIT(unsigned int bist_id, unsigned int data);
void ithMEMBIST_WRITE_HIGHBIT(unsigned int bist_id, unsigned int data);
bool ithMEMBIST_ISDone(unsigned int bist_id);
bool ithMEMBIST_ISFault(unsigned int bist_id);

#ifdef __cplusplus
}
#endif

#endif // ITH_MEMBIST_H
/** @} */ // end of ith