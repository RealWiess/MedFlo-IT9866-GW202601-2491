#ifndef MYLZ_COMP_H_INCLUDED
#define MYLZ_COMP_H_INCLUDED

#ifdef BLZ_DLL
#  if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef BLZ_DLL_EXPORTS
#      define BLZ_API __declspec(dllexport)
#    else
#      define BLZ_API __declspec(dllimport)
#    endif
#    define BLZ_LOCAL
#  else
#    if __GNUC__ >= 4
#      define BLZ_API __attribute__ ((visibility ("default")))
#      define BLZ_LOCAL __attribute__ ((visibility ("hidden")))
#    else
#      define BLZ_API
#      define BLZ_LOCAL
#    endif
#  endif
#else
#  define BLZ_API
#  define BLZ_LOCAL
#endif

typedef struct {
  int bytePerUnit;
  int hashTableSizeBits;
  int blockSize;
  int codingMode;   // (0 = GG8 (defualt), 1 = EE8, 2 = G1G7, 3 = E1E7, 4 = G2G6, 5 = E2E6)
                    // G is Exp-Golomb k=1, E is Exp-Golomb k=0
                    // GG8:  length = EGk=1,      offset = EGk=1 + 8b
                    // EE8:  length = EGk=0,      offset = EGk=0 + 8b
                    // G1G7: length = EGk=1 + 1b, offset = EGk=1 + 7b
                    // E1E7: length = EGk=0 + 1b, offset = EGk=0 + 7b
                    // G2G6: length = EGk=1 + 2b, offset = EGk=1 + 6b
                    // E2E6: length = EGk=0 + 2b, offset = EGk=0 + 6b
} myLZParaType;

/* Internal data structure */
struct myLZ_state {
	const unsigned char *src;
	unsigned char *dst;
	unsigned char *tagpos;
	unsigned int tag;
	unsigned int bits_left;
};

/**
 * Get required size of `workmem` buffer.
 *
 * @see blz_pack
 *
 * @param src_size number of bytes to compress
 * @return required size in bytes of `workmem` buffer
 */
BLZ_API unsigned long myLZ_workmem_size(unsigned long src_size);

/**
 * Get bound on compressed data size.
 *
 * @see blz_pack
 *
 * @param src_size number of bytes to compress
 * @return maximum size of compressed data
 */
BLZ_API unsigned long myLZ_max_packed_size(unsigned long src_size);

/**
 * Compress `src_size` bytes of data from `src` to `dst`.
 *
 * @param src pointer to data
 * @param dst pointer to where to place compressed data
 * @param src_size number of bytes to compress
 * @param workmem pointer to memory for temporary use
 * @return size of compressed data
 */
BLZ_API unsigned long myLZ_pack(const void *src, void *dst, unsigned long src_size, void *workmem, myLZParaType *para);

/**
 * Decompress `depacked_size` bytes of data from `src` to `dst`.
 *
 * @param src pointer to compressed data
 * @param dst pointer to where to place decompressed data
 * @param depacked_size size of decompressed data
 * @return size of decompressed data
 */
unsigned long myLZ_depack(const void *src, void *dst, unsigned long depacked_size, unsigned int packblocksize , myLZParaType *para);

#endif /* MYLZ_COMP_H_INCLUDED */
