#ifndef ITH_UTILITY_H
#define ITH_UTILITY_H

#ifdef _MSC_VER
    #include <sys/types.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** @addtogroup ith ITE Hardware Library
 *  @{
 */
/** @addtogroup ith_utility Utility
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

// Macros
/**
 * Assertion at compile time.
 *
 * @param e Specifies any logical expression.
 */
#define ITH_STATIC_ASSERT(e) \
    typedef int ITH_STATIC_ASSERT_DUMMY_ ## __LINE__[(e) * 2 - 1]

/**
 * Counts the element number of an array.
 *
 * @param[in] array The array.
 *
 * @return The element number of array.
 *
 * @code
 *     int array[] = {1, 2, 3, 4, 5};
 *     int count = ITH_COUNT_OF(array);
 *     assert(count == 5);
 * @endcode
 */
#define ITH_COUNT_OF(array)            (sizeof(array) / sizeof(array[0]))

/**
 * @brief Aligns a value to its ceil.
 *
 * This macro aligns @a value to its ceil, which means the return value
 * will be the smallest number that is greater than or equal to @a value
 * and is divisible by @a align.
 *
 * @param value The value will be aligned.
 * @param align The alignment. It must be a power of 2.
 *
 * @return The aligned value.
 *
 * Example:
 * @code
 *     uint32_t val = 7;
 *     val = ITH_ALIGN_UP(val, 8);
 *     val = ITH_ALIGN_UP(val, 4);
 *     val = ITH_ALIGN_UP(val, 16);
 * @endcode
 */
#define ITH_ALIGN_UP(value, align)     (((value) + ((align) - 1U)) & ~((align) - 1U))

/**
 * @brief Aligns a value to its floor.
 *
 * This macro aligns @a value to its floor, which means the return value
 * will be the greatest number that is less than or equal to @a value
 * and is divisible by @a align.
 *
 * @param value The value will be aligned.
 * @param align The alignment. It must be a power of 2.
 *
 * @return The aligned value.
 *
 * Example:
 * @code
 *     uint32_t val = 7;
 *     val = ITH_ALIGN_DOWN(val, 8);
 *     val = ITH_ALIGN_DOWN(val, 4);
 *     val = ITH_ALIGN_DOWN(val, 16);
 * @endcode
 */
#define ITH_ALIGN_DOWN(value, align)   ((value) & ~((align) - 1U))

/**
 * Determines whether the value is aligned.
 *
 * @param value The value.
 * @param align The alignment. It must be a power of 2.
 *
 * @return Whether the value is aligned.
 *
 * Example:
 * @code
 *     uint32_t val = 7;
 *     if (ITH_IS_ALIGNED(val, 8))
 *         printf("aligned\n");
 *     else
 *         printf("unaligned\n");
 *     val = 16;
 *     if (ITH_IS_ALIGNED(val, 8))
 *         printf("aligned\n");
 *     else
 *         printf("unaligned\n");
 * @endcode
 */
#define ITH_IS_ALIGNED(value, align)   (((value) & ((align) - 1)) == 0)

/**
 * Determines whether the value is unaligned.
 *
 * @param value The value.
 * @param align The alignment. It must be a power of 2.
 *
 * @return Whether the value is unaligned.
 *
 * Example:
 * @code
 *     uint32_t val = 7;
 *     if (ITH_IS_UNALIGNED(val, 8))
 *         printf("unaligned\n");
 *     else
 *         printf("aligned\n");
 *     val = 16;
 *     if (ITH_IS_UNALIGNED(val, 8))
 *         printf("unaligned\n");
 *     else
 *         printf("aligned\n");
 * @endcode
 */
#define ITH_IS_UNALIGNED(value, align) (((value) & ((align) - 1)) != 0)


/**
 * Determines whether the value is power of two.
 *
 * @param x The value.
 *
 * @return Whether the value is power of two.
 *
 * Example:
 * @code
 *     int val = 1;
 *     if (ITH_IS_POWER_OF_TWO(val))
 *         printf("power of two\n");
 *     val = 2;
 *     if (ITH_IS_POWER_OF_TWO(val))
 *         printf("power of two\n");
 *     val = 3;
 *     if (ITH_IS_POWER_OF_TWO(val))
 *         printf("power of two\n");
 *     else
 *         printf("not power of two\n");
 * @endcode
 */
#define ITH_IS_POWER_OF_TWO(x)         ((((x) - 1) & (x)) == 0)

/**
 * Calculates the absolute value.
 *
 * @param x The value.
 * @return The absolute value.
 */
#define ITH_ABS(x)                     (((x) >= 0) ? (x) : -(x))

/**
 * Returns the larger of two values.
 *
 * @param a Values of any numeric type to be compared.
 * @param b Values of any numeric type to be compared.
 * @return The larger of its arguments.
 */
#define ITH_MAX(a, b)                  (((a) > (b)) ? (a) : (b))

/**
 * Returns the smaller of two values.
 *
 * @param a Values of any numeric type to be compared.
 * @param b Values of any numeric type to be compared.
 * @return The smaller of its arguments.
 */
#define ITH_MIN(a, b)                  (((a) < (b)) ? (a) : (b))

/**
 * Swaps two values.
 *
 * @param a The value.
 * @param b The another value.
 * @param type The value type.
 */
#define ITH_SWAP(a, b, type) \
    do { type tmp = (a); a = (b); b = tmp; } while (false)

/**
 * @brief Converts 16-bit value to another endian integer.
 *
 * This function converts a 16-bit value to another endian integer.<br />
 * The function is useful when the target platform is little endian and the
 * value needs to be converted to big endian or vice versa.
 *
 * @param value The value to convert.
 *
 * @return The converted value.
 */
static inline uint16_t ithBswap16(uint16_t value)
{
    return ((value & 0x00FFU) << 8U) |
           ((value & 0xFF00U) >> 8U);
}

/**
 * @brief Converts 32-bit value to another endian integer.
 *
 * This function converts a 32-bit value to another endian integer.<br />
 * The function is useful when the target platform is little endian and the
 * value needs to be converted to big endian or vice versa.
 *
 * @param value The value to convert.
 *
 * @return The converted value.
 */
static inline uint32_t ithBswap32(uint32_t value)
{
    return ((value & 0x000000FFUL) << 24UL) |
           ((value & 0x0000FF00UL) << 8UL)  |
           ((value & 0x00FF0000UL) >> 8UL)  |
           ((value & 0xFF000000UL) >> 24UL);
}

/**
 * @brief Converts 64-bit value to another endian integer.
 *
 * This function converts a 64-bit value to another endian integer.<br />
 * The function is useful when the target platform is little endian and the
 * value needs to be converted to big endian or vice versa.
 *
 * @param value The value to convert.
 *
 * @return The converted value.
 */
static inline uint64_t ithBswap64(uint64_t value)
{
    return ((value & 0XFF00000000000000ULL) >> 56ULL) |
           ((value & 0X00FF000000000000ULL) >> 40ULL) |
           ((value & 0X0000FF0000000000ULL) >> 24ULL) |
           ((value & 0X000000FF00000000ULL) >> 8ULL ) |
           ((value & 0X00000000FF000000ULL) << 8ULL ) |
           ((value & 0X0000000000FF0000ULL) << 24ULL) |
           ((value & 0X000000000000FF00ULL) << 40ULL) |
           ((value & 0X00000000000000FFULL) << 56ULL);
}

/**
 * Packs colors to a RGB565 format value.
 *
 * @param r red value.
 * @param g green value.
 * @param b blue value.
 * @return The packed value.
 */
#define ITH_RGB565(r, g, b) \
    ((((uint16_t)(r) >> 3U) << 11U) | (((uint16_t)(g) >> 2U) << 5U) | ((uint16_t)(b) >> 3U))

/**
 * Packs colors to a ARGB1555 format value.
 *
 * @param a alpha value.
 * @param r red value.
 * @param g green value.
 * @param b blue value.
 * @return The packed value.
 */
#define ITH_ARGB1555(a, r, g, b) \
    ((((uint16_t)(a) >> 7U) << 15U) | (((uint16_t)(r) >> 3U) << 10U) | (((uint16_t)(g) >> 3U) << 5U) | ((uint16_t)(b) >> 3U))

/**
 * Packs colors to a ARGB4444 format value.
 *
 * @param a alpha value.
 * @param r red value.
 * @param g green value.
 * @param b blue value.
 * @return The packed value.
 */
#define ITH_ARGB4444(a, r, g, b) \
    ((((uint16_t)(a) >> 4U) << 12U) | (((uint16_t)(r) >> 4U) << 8U) | (((uint16_t)(g) >> 4U) << 4U) | ((uint16_t)(b) >> 4U))

/**
 * Packs colors to a ARGB8888 format value.
 *
 * @param a alpha value.
 * @param r red value.
 * @param g green value.
 * @param b blue value.
 * @return The packed value.
 */
#define ITH_ARGB8888(a, r, g, b) \
    (((uint32_t)(a) << 24UL) | ((uint32_t)(r) << 16UL) | ((uint32_t)(g) << 8UL) | (uint32_t)(b))

/**
 * Sets the first len words (16-bits) of the block of memory pointed by ptr to the specified value.
 *
 * @param dst Pointer to the block of memory to fill.
 * @param val Value to be set. The value is passed as an int, but the function fills the block of memory using the uint16_t conversion of this value.
 * @param len Number of words (16-bits) to be set to the value.
 */
static inline void ithMemset16(void *dst, int val, size_t len)
{
    uint16_t * p = (uint16_t *)dst;

    while (len-- > 0U)
    {
        *p = (uint16_t)val;
        p++;
    }
}

/**
 * Sets the first len words (32-bits) of the block of memory pointed by ptr to the specified value.
 *
 * @param dst Pointer to the block of memory to fill.
 * @param val Value to be set. The value is passed as an int, but the function fills the block of memory using the uint32_t conversion of this value.
 * @param len Number of words (32-bits) to be set to the value.
 */
static inline void ithMemset32(void *dst, int val, size_t len)
{
    uint32_t * p = (uint32_t *)dst;

    while (len-- > 0U)
    {
        *p = (uint32_t)val;
        p++;
    }
}

/** List node */
typedef struct ITHListTag
{
    struct ITHListTag * next; /**< Next node */
    struct ITHListTag * prev; /**< Previous node */
} ITHList;

/**
 * Pushes a node to the front of list.
 *
 * @param list The list.
 * @param node The node wiil be pushed.
 */
void ithListPushFront(ITHList *list, void *node);

/**
 * Pushes a node to the back of list.
 *
 * @param list The list.
 * @param node The node wiil be pushed.
 */
void ithListPushBack(ITHList *list, void *node);

/**
 * Inserts a node before another node.
 *
 * @param list The list.
 * @param listNode The node will be after the inserted node.
 * @param node The node wiil be inserted.
 */
void ithListInsertBefore(ITHList *list, void *listNode, void *node);

/**
 * Inserts a node after another node.
 *
 * @param list The list.
 * @param listNode The node will be before the inserted node.
 * @param node The node wiil be inserted.
 */
void ithListInsertAfter(ITHList *list, void *listNode, void *node);

/**
 * Removes a node from a list.
 *
 * @param list The list.
 * @param node The node wiil be removed.
 */
void ithListRemove(ITHList *list, void *node);

/**
 * Clears every nodes in a list.
 *
 * @param list The list.
 * @param dtor The destructor to destroy every node. Can be MMP_NULL.
 */
void ithListClear(ITHList *list, void (*dtor)(void * node));

/**
 * Converts 32-bit unsigned value to binary format string.
 *
 * @param s The binary format string.
 * @param i The 32-bit unsigned value.
 */
char *ithU32tob(char *s, uint32_t i);

/**
 * Prints host register values.
 *
 * @param addr the start register address to dump
 * @param size the size of register block to dump
 */
void ithPrintRegH(uint16_t addr, unsigned int size);

/**
 * Prints AMBA register values.
 *
 * @param addr the start register address to dump
 * @param size the size of register block to dump
 */
void ithPrintRegA(uint32_t addr, unsigned int size);

/**
 * Prints VRAM values on 8-bit format.
 *
 * @param addr the start address to dump
 * @param size the size of block to dump
 */
void ithPrintVram8(uint32_t addr, unsigned int size);

/**
 * Prints VRAM values on 16-bit format.
 *
 * @param addr the start address to dump
 * @param size the size of block to dump
 */
void ithPrintVram16(uint32_t addr, unsigned int size);

/**
 * Prints VRAM values on 32-bit format.
 *
 * @param addr the start address to dump
 * @param size the size of block to dump
 */
void ithPrintVram32(uint32_t addr, unsigned int size);

// Print functions

/**
 * Putchar callback function for ithPrintf().
 *
 * @param c the character to output
 * @return same as c
 */
extern int (*ithPutcharFunc)(int c);

/**
 * General printf() function.
 *
 * @param fmt Format control.
 * @param ... Optional arguments.
 * @return the number of characters printed, or a negative value if an error occurs.
 */
int ithPrintf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // ITH_UTILITY_H
/** @} */ // end of ith_utility
/** @} */ // end of ith
