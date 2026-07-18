/*
 * Copyright (c) 2022 ITE Tech. Inc.
 *
 */

#include "ite_it2d.h"

#include <assert.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include "ite/ith.h"
#include "ite/itp.h"

#define IT2D_DOUBLE_BUFFER_ENABLE

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define SWAP(a, b, type)                                                       \
    do                                                                         \
    {                                                                          \
        type tmp = (a);                                                        \
        a        = (b);                                                        \
        b        = tmp;                                                        \
    } while (false)

#define BIT(n)      (1UL << (n))
#define BIT_MASK(n) (BIT(n) - 1UL)

#if (CFG_CHIP_FAMILY == 9860)
    #define REG_HOST_BASE 0xD8000000
    #define REG_CMDQ_BASE 0xB0600000
    #define REG_2D_BASE   0xB0700000
#else
    #define REG_HOST_BASE 0xB8000000
    #define REG_CMDQ_BASE 0xE0600000
    #define REG_2D_BASE   0xE0700000
#endif

#define REG_CQ_CLK     0x0024

#define REG_CMDQ_SR1   0x0020

#define REG_2D_SRC     0x000
#define REG_2D_SRCXY   0x004
#define REG_2D_SRHWR   0x008
#define REG_2D_SPR     0x00C
#define REG_2D_MASK    0x010
#define REG_2D_MASKXY  0x014
#define REG_2D_MHWR    0x018
#define REG_2D_MPR     0x01C
#define REG_2D_DST     0x020
#define REG_2D_DSTXY   0x024
#define REG_2D_DHWR    0x028
#define REG_2D_DPR     0x02C
#define REG_2D_PXCR    0x030
#define REG_2D_PYCR    0x034
#define REG_2D_LNER    0x038
#define REG_2D_ITMR00  0x040
#define REG_2D_ITMR01  0x044
#define REG_2D_ITMR02  0x048
#define REG_2D_ITMR10  0x04C
#define REG_2D_ITMR11  0x050
#define REG_2D_ITMR12  0x054
#define REG_2D_ITMR20  0x058
#define REG_2D_ITMR21  0x05C
#define REG_2D_ITMR22  0x060
#define REG_2D_FGCOLOR 0x064
#define REG_2D_LINE_P1 0x068
#define REG_2D_CAR     0x074
#define REG_2D_SAFE    0x07C
#define REG_2D_CR1     0x080
#define REG_2D_CR2     0x084
#define REG_2D_CR3     0x088
#define REG_2D_CMD     0x08C
#define REG_2D_ICR     0x090
#define REG_2D_ISR     0x094
#define REG_2D_ID1     0x098
#define REG_2D_IDM1    0x0A4
#define REG_2D_ST1     0x0C4

#define WAIT_BUSY_TIMEOUT    (500000U)
#define RETRY_TIMEOUT        (5U)

static void it2d_reset (void);
static void itclock_enable_2d_clock (uint32_t base);

struct it2d_config
{
    uint32_t base;
};

struct it2d_data
{
    int32_t  last_id;
    uint32_t lcd_index;
};

static const uint8_t it2d_pixel_format_to_bits[] = {
    32, 32, 32, 32, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 4, 2, 1, 32, 32};

static const struct it2d_config it2d_config = {.base = REG_2D_BASE};

static struct it2d_data         it2d_data;

/**
 * @brief Reads a 32-bit unsigned integer from the given address.
 *
 * @param addr Address to read from.
 *
 * @return The 32-bit unsigned integer value read from the given address.
 */
static inline uint32_t          sys_read32 (uint32_t addr)
{
    uint32_t value;

    value = *(volatile uint32_t *)addr;

    return value;
}

/**
 * @brief Writes a 32-bit unsigned integer to the given address.
 *
 * @param data The 32-bit unsigned integer value to write.
 * @param addr Address to write to.
 */
static inline void sys_write32 (uint32_t data, uint32_t addr)
{
    *(volatile uint32_t *)addr = data;
}

/**
 * @brief Sets the bits in a register that are covered by a given mask to a
 *        given value.
 *
 * @param addr The address of the register to modify.
 * @param mask A mask that specifies which bits to modify.
 * @param value The value to set the specified bits to.
 */
static inline void sys_set_mask (uint32_t addr, uint32_t mask, uint32_t value)
{
    uint32_t temp = sys_read32(addr);

    temp &= ~(mask);
    temp |= value;

    sys_write32(temp, addr);
}

static inline void sys_set_bit(uint32_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp | (1 << bit);
}

static inline void sys_clear_bit(uint32_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp & ~(1 << bit);
}

/**
 * @brief Checks if two rectangles have an intersection.
 *
 * @param a First rectangle.
 * @param b Second rectangle.
 *
 * @return true if the rectangles have an intersection, false otherwise.
 *
 * @details The rectangles are considered to have an intersection if the
 *          intersection of the rectangles is not empty.
 *
 *          The rectangles are defined by their top-left corner and their
 *          width and height.
 */
bool ite_it2d_rect_has_intersect (const ite_it2d_rect_t * a,
                                  const ite_it2d_rect_t * b)
{
    assert(a);
    assert(b);

    if ((a->w <= 0) || (a->h <= 0) || (b->w <= 0) || (b->h <= 0))
    {
        return false;
    }

    int16_t a_min = a->x;
    int16_t a_max = a_min + a->w;
    int16_t b_min = b->x;
    int16_t b_max = b_min + b->w;
    if (b_min > a_min)
    {
        a_min = b_min;
    }
    if (b_max < a_max)
    {
        a_max = b_max;
    }
    if (a_max <= a_min)
    {
        return false;
    }

    a_min = a->y;
    a_max = a_min + a->h;
    b_min = b->y;
    b_max = b_min + b->h;
    if (b_min > a_min)
    {
        a_min = b_min;
    }
    if (b_max < a_max)
    {
        a_max = b_max;
    }
    if (a_max <= a_min)
    {
        return false;
    }
    return true;
}

/**
 * @brief Gets the number of bits per pixel for a given pixel format.
 *
 * @param format Pixel format to get the number of bits per pixel for.
 *
 * @return The number of bits per pixel for the given pixel format.
 */
int ite_it2d_bits_per_pixel (ite_it2d_pixel_format_t format)
{
    return it2d_pixel_format_to_bits[format];
}

/**
 * @brief Initializes an ite_it2d_transform_dsc_t structure to default values.
 *
 * @param dsc Structure to initialize.
 *
 * This function initializes the given ite_it2d_transform_dsc_t structure to
 * default values. The default values are:
 *
 * - scale_x: 1.0f
 * - scale_y: 1.0f
 * - pos: (0, 0)
 * - angle: 0.0f
 * - center: (0, 0)
 * - flags: 0
 *
 * This is useful for initializing a structure before passing it to functions
 * that expect a fully-initialized structure.
 */
void ite_it2d_init_transform_dsc (ite_it2d_transform_dsc_t * dsc)
{
    assert(dsc);
    memset(dsc, 0, sizeof(ite_it2d_transform_dsc_t));
    dsc->scale_x = 1.0f;
    dsc->scale_y = 1.0f;
}

/**
 * @brief Initializes a 3x3 matrix to the identity matrix.
 *
 * @param mat The matrix to initialize.
 *
 * This function sets the given 3x3 matrix to be the identity matrix,
 * where the diagonal elements are 1.0f and all other elements are 0.0f.
 */

void ite_it2d_mat_identity (ite_it2d_matrix_t mat)
{
    mat[0][0] = 1.0f;
    mat[0][1] = 0.0f;
    mat[0][2] = 0.0f;
    mat[1][0] = 0.0f;
    mat[1][1] = 1.0f;
    mat[1][2] = 0.0f;
    mat[2][0] = 0.0f;
    mat[2][1] = 0.0f;
    mat[2][2] = 1.0f;
}

/**
 * @brief Multiplies two 3x3 matrices.
 *
 * @param src_mat The left-hand operand matrix.
 * @param mat The right-hand operand matrix.
 * @param dst_mat The matrix to store the result in.
 *
 * This function multiplies two 3x3 matrices, and stores the result in the given
 * destination matrix. The order of multiplication is as follows: dst_mat =
 * src_mat * mat. The destination matrix is not allowed to be the same as either
 * of the source matrices.
 */
void ite_it2d_mat_multiply (const ite_it2d_matrix_t src_mat,
                            const ite_it2d_matrix_t mat,
                            ite_it2d_matrix_t       dst_mat)
{
    assert(src_mat);
    assert(mat);
    assert(dst_mat);
    int i, j;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            dst_mat[i][j] = src_mat[i][0] * mat[0][j] +
                            src_mat[i][1] * mat[1][j] +
                            src_mat[i][2] * mat[2][j];
        }
    }
}

/**
 * @brief Multiplies a 3x3 matrix by a translation matrix.
 *
 * @param src_mat The matrix to translate.
 * @param dst_mat The matrix to store the result in.
 * @param x The x coordinate of the translation.
 * @param y The y coordinate of the translation.
 *
 * This function multiplies the given 3x3 matrix by a translation matrix,
 * which is a matrix that translates points by the given x and y coordinates.
 * The order of multiplication is as follows: dst_mat = src_mat * translate_mat.
 * The destination matrix is not allowed to be the same as the source matrix.
 */
void ite_it2d_mat_translate (const ite_it2d_matrix_t src_mat,
                             ite_it2d_matrix_t dst_mat, float x, float y)
{
    assert(src_mat);
    assert(dst_mat);
    const ite_it2d_matrix_t mat = {
        {1.0f, 0.0f,    x},
        {0.0f, 1.0f,    y},
        {0.0f, 0.0f, 1.0f}
    };

    ite_it2d_mat_multiply(src_mat, mat, dst_mat);
}

/**
 * @brief Scales a 3x3 matrix.
 *
 * @param src_mat The matrix to scale.
 * @param dst_mat The matrix to store the result in.
 * @param x The x coordinate scale factor.
 * @param y The y coordinate scale factor.
 *
 * This function multiplies the given 3x3 matrix by a scaling matrix,
 * which is a matrix that scales points by the given x and y coordinates.
 * The order of multiplication is as follows: dst_mat = src_mat * scale_mat.
 * The destination matrix is not allowed to be the same as the source matrix.
 */
void ite_it2d_mat_scale (const ite_it2d_matrix_t src_mat,
                         ite_it2d_matrix_t dst_mat, float x, float y)
{
    assert(src_mat);
    assert(dst_mat);
    const ite_it2d_matrix_t mat = {
        {   x, 0.0f, 0.0f},
        {0.0f,    y, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };

    ite_it2d_mat_multiply(src_mat, mat, dst_mat);
}

/**
 * @brief Rotates a 3x3 matrix by the given angle.
 *
 * @param src_mat The matrix to rotate.
 * @param dst_mat The matrix to store the result in.
 * @param angle The angle to rotate by, in degrees.
 *
 * This function multiplies the given 3x3 matrix by a rotation matrix,
 * which is a matrix that rotates points by the given angle.
 * The order of multiplication is as follows: dst_mat = src_mat * rotate_mat.
 * The destination matrix is not allowed to be the same as the source matrix.
 */
void ite_it2d_mat_rotate (const ite_it2d_matrix_t src_mat,
                          ite_it2d_matrix_t dst_mat, float angle)
{
    assert(src_mat);
    assert(dst_mat);
    float                   rad     = (angle * M_PI / 180.0f);
    float                   sin_val = sinf(rad);
    float                   cos_val = cosf(rad);
    const ite_it2d_matrix_t mat     = {
        {cos_val, -sin_val, 0.0f},
        {sin_val,  cos_val, 0.0f},
        {   0.0f,     0.0f, 1.0f}
    };

    ite_it2d_mat_multiply(src_mat, mat, dst_mat);
}

/**
 * @brief Applies a transformation matrix to a point.
 *
 * @param mat The transformation matrix.
 * @param src_pt The point to transform.
 * @param dst_pt The transformed point.
 *
 * This function transforms the given point by the given matrix.
 * The order of multiplication is as follows: dst_pt = src_pt * mat.
 * The destination point is not allowed to be the same as the source point.
 */
void ite_it2d_mat_transform (const ite_it2d_matrix_t  mat,
                             const ite_it2d_point_t * src_pt,
                             ite_it2d_point_t *       dst_pt)
{
    assert(src_pt);
    assert(dst_pt);

    dst_pt->x = (src_pt->x * mat[0][0] + src_pt->y * mat[0][1] + mat[0][2]) /
                (src_pt->x * mat[2][0] + src_pt->y * mat[2][1] + mat[2][2]);
    dst_pt->y = (src_pt->x * mat[1][0] + src_pt->y * mat[1][1] + mat[1][2]) /
                (src_pt->x * mat[2][0] + src_pt->y * mat[2][1] + mat[2][2]);
}

/**
 * @brief Compares two floating-point numbers for approximate equality.
 *
 * @param x The first floating-point number.
 * @param y The second floating-point number.
 * @return A non-zero value if x and y are approximately equal, otherwise 0.
 *
 * This function considers two floating-point numbers to be equal if the
 * absolute difference between them is less than or equal to a small
 * tolerance value, scaled by the smaller of the magnitudes of x and y.
 */

static inline float float_equal (float x, float y)
{
    return fabsf(x - y) <= 0.00001f * MIN(fabsf(x), fabsf(y));
}

/**
 * @brief Tests whether a floating-point number is approximately zero.
 *
 * @param x The floating-point number to test.
 * @return A non-zero value if x is approximately zero, otherwise 0.
 *
 * This function considers a floating-point number to be zero if adding it
 * to 1.0f results in a value that is approximately equal to 1.0f.
 */
static inline float float_is_zero (float x)
{
    return float_equal((x) + 1.0f, 1.0f);
}

/**
 * @brief Checks if a 3x3 matrix is affine.
 *
 * @param mtx The matrix to check.
 * @return True if the matrix is affine, otherwise false.
 *
 * This function determines whether the given 3x3 matrix is an affine
 * transformation matrix. An affine matrix has the properties that the
 * elements (0,2) and (1,2) are approximately zero, and the element (2,2)
 * is approximately one.
 */

static inline bool ite_it2d_mat_is_affine (const ite_it2d_matrix_t mtx)
{
    return float_is_zero(mtx[0][2]) && float_is_zero(mtx[1][2]) &&
           float_equal(mtx[2][2], 1);
}

/**
 * @brief Computes the inverse of a 3x3 matrix.
 *
 * @param src_mat The source matrix to invert.
 * @param dst_mat The matrix to store the inverse.
 *
 * @return Returns 0 on success, or -EINVAL if the matrix is singular and
 *         cannot be inverted.
 *
 * This function calculates the inverse of the given 3x3 matrix and stores
 * the result in the provided destination matrix. It checks if the source
 * matrix is affine and adjusts the destination matrix accordingly. The
 * function returns an error if the determinant is zero, indicating that
 * the matrix is non-invertible.
 */

int ite_it2d_mat_inverse (const ite_it2d_matrix_t src_mat,
                          ite_it2d_matrix_t       dst_mat)
{
    assert(src_mat);
    assert(dst_mat);
    bool  affine = ite_it2d_mat_is_affine(src_mat);
    float d0 = src_mat[1][1] * src_mat[2][2] - src_mat[2][1] * src_mat[1][2];
    float d1 = src_mat[2][0] * src_mat[1][2] - src_mat[1][0] * src_mat[2][2];
    float d2 = src_mat[1][0] * src_mat[2][1] - src_mat[2][0] * src_mat[1][1];
    float d  = src_mat[0][0] * d0 + src_mat[0][1] * d1 + src_mat[0][2] * d2;

    if (d == 0.0f)
    {
        return -EINVAL;
    }
    d             = 1.0f / d;

    dst_mat[0][0] = d * d0;
    dst_mat[1][0] = d * d1;
    dst_mat[2][0] = d * d2;
    dst_mat[0][1] =
        d * (src_mat[2][1] * src_mat[0][2] - src_mat[0][1] * src_mat[2][2]);
    dst_mat[1][1] =
        d * (src_mat[0][0] * src_mat[2][2] - src_mat[2][0] * src_mat[0][2]);
    dst_mat[2][1] =
        d * (src_mat[2][0] * src_mat[0][1] - src_mat[0][0] * src_mat[2][1]);
    dst_mat[0][2] =
        d * (src_mat[0][1] * src_mat[1][2] - src_mat[1][1] * src_mat[0][2]);
    dst_mat[1][2] =
        d * (src_mat[1][0] * src_mat[0][2] - src_mat[0][0] * src_mat[1][2]);
    dst_mat[2][2] =
        d * (src_mat[0][0] * src_mat[1][1] - src_mat[1][0] * src_mat[0][1]);

    if (affine)
    {
        dst_mat[0][2] = 0;
        dst_mat[1][2] = 0;
        dst_mat[2][2] = 1;
    }
    return 0;
}

/**
 * @brief Initializes a display surface.
 *
 * @param disp_surf The display surface to initialize.
 *
 * @return Returns 0 on success.
 *
 * This function initializes a display surface with the size, pitch, format
 * and buffer of the active LCD. It also sets the static buffer flag to true
 * and assigns an ID from the current ID generator. The buffer address is
 * either the base address of LCD A or B, depending on the double buffering
 * configuration.
 */
int ite_it2d_create_display_surface (ite_it2d_surface_t * disp_surf)
{
    disp_surf->width            = ithLcdGetWidth();
    disp_surf->height           = ithLcdGetHeight();
    disp_surf->pitch            = ithLcdGetPitch();
    disp_surf->flags.format     = (ithLcdGetFormat() == ITH_LCD_ARGB8888)
                                      ? ITE_IT2D_PIXEL_FORMAT_ARGB_8888
                                      : ITE_IT2D_PIXEL_FORMAT_RGB_565;
    disp_surf->flags.static_buf = true;
#ifdef IT2D_DOUBLE_BUFFER_ENABLE
    disp_surf->buf = (void *)(it2d_data.lcd_index ? ithLcdGetBaseAddrB()
                                                  : ithLcdGetBaseAddrA());
#else
    disp_surf->buf = (void *)ithLcdGetBaseAddrA();
#endif
    disp_surf->id = ite_it2d_get_current_id();

    return 0;
}

/**
 * @brief Creates a 2D surface.
 *
 * @param surf The surface structure to initialize.
 *
 * @return Returns 0 on success, or -ENOMEM if memory allocation fails.
 *
 * This function initializes a 2D surface with the specified width, height,
 * and pixel format. If the pitch is not provided, it calculates the pitch
 * based on the pixel format. If the surface does not have a static buffer,
 * it allocates memory for the buffer and copies existing data if present.
 * The function also assigns a unique ID to the surface.
 */

int ite_it2d_create_surface (ite_it2d_surface_t * surf)
{
    assert(surf);

    uint16_t pitch = surf->pitch;
    if (pitch == 0)
    {
        uint8_t bits = it2d_pixel_format_to_bits[surf->flags.format];
        if (bits >= 8)
        {
            pitch = surf->width * bits / 8;
        }
        else
        {
            uint8_t div = 8 / bits;
            pitch       = (surf->width + (div - 1)) / div;
        }
    }

    if (!surf->flags.static_buf)
    {
        uint8_t * buf = (uint8_t *)malloc(pitch * surf->height);
        if (buf == NULL)
        {
            printf("failed to allocate surface buffer\n");
            return -ENOMEM;
        }
        if (surf->buf)
        {
            memcpy(buf, surf->buf, pitch * surf->height);
            ithFlushDCacheRange(buf, pitch * surf->height);
        }
        surf->buf = buf;
    }
    surf->pitch = pitch;
    surf->id    = ite_it2d_get_current_id();

    return 0;
}

/**
 * Destroys a 2D surface by freeing its associated resources.
 *
 * @param surf Pointer to the surface to be destroyed.
 *
 * This function waits for any pending operations on the surface to
 * complete before freeing the allocated memory for the surface buffer,
 * provided it was not statically allocated. It ensures that the buffer
 * pointer is set to NULL after being freed.
 */

int ite_it2d_destroy_surface (ite_it2d_surface_t * surf)
{
    assert(surf);

    ite_it2d_wait_surface_finish(surf);

    if (!surf->flags.static_buf)
    {
        free(surf->buf - surf->flags.buf_offset);
    }
    surf->buf = NULL;

    return 0;
}

/**
 * @brief Waits for all pending operations on a 2D surface to finish.
 *
 * @param surf Pointer to the surface to wait for.
 *
 * @return Returns the result of waiting for the surface's operations
 *         to complete by checking the associated ID.
 *
 * This function ensures that all operations on the given surface have
 * finished by waiting for the completion of the operations associated
 * with the surface's unique ID.
 */

int ite_it2d_wait_surface_finish (ite_it2d_surface_t * surf)
{
    assert(surf);
    return ite_it2d_wait_id_finish(surf->id);
}

/**
 * @brief Allocates a buffer of the given size.
 *
 * @param size The size of the buffer to allocate in bytes.
 *
 * @return Returns a pointer to the allocated buffer, or NULL if allocation
 *         fails.
 *
 * This function allocates a buffer of the specified size and returns a
 * pointer to the allocated buffer. The buffer is allocated on the heap
 * and must be freed manually by calling ite_it2d_free_buffer.
 */
void * ite_it2d_alloc_buffer (uint32_t size)
{
    assert(size > 0);
    return (void *)malloc(size);
}

/**
 * @brief Frees a buffer allocated with ite_it2d_alloc_buffer.
 *
 * @param buf The buffer to free.
 *
 * This function frees a buffer allocated with ite_it2d_alloc_buffer.
 * The buffer must be a valid pointer returned by that function.
 */
void ite_it2d_free_buffer (void * buf)
{
    assert(buf);
    free(buf);
}

/**
 * @brief Sets the destination surface.
 *
 * @param ctx The graphics context.
 * @param surf The destination surface.
 *
 * This function sets the destination surface of the graphics context. If
 * the destination surface is valid, it also sets the destination rectangle
 * to the entire surface.
 *
 * @note The destination surface must be a valid surface with a format
 *       between A_1 and A_8.
 */
void ite_it2d_set_dst_surface (ite_it2d_context_t * ctx,
                               ite_it2d_surface_t * surf)
{
    assert(ctx);

    ctx->dst_surf = surf;

    if (surf)
    {
        assert(surf->flags.format <= ITE_IT2D_PIXEL_FORMAT_A_8);

        ctx->dst_rect.x = 0;
        ctx->dst_rect.y = 0;
        ctx->dst_rect.w = surf->width;
        ctx->dst_rect.h = surf->height;
    }
}

/**
 * @brief Sets the source surface.
 *
 * @param ctx The graphics context.
 * @param surf The source surface.
 *
 * This function sets the source surface of the graphics context. If the
 * source surface is valid, it also sets the source rectangle to the entire
 * surface.
 *
 * @note The source surface must be a valid surface with a format between
 *       A_1 and A_8.
 */
void ite_it2d_set_src_surface (ite_it2d_context_t * ctx,
                               ite_it2d_surface_t * surf)
{
    assert(ctx);

    ctx->op.bitblt.src_surf = surf;

    if (surf)
    {
        ctx->op.bitblt.src_rect.x = 0;
        ctx->op.bitblt.src_rect.y = 0;
        ctx->op.bitblt.src_rect.w = surf->width;
        ctx->op.bitblt.src_rect.h = surf->height;
    }
}

/**
 * @brief Sets the destination rectangle in the graphics context.
 *
 * @param ctx The graphics context.
 * @param rect The destination rectangle to set. If NULL, the destination
 *             rectangle is set to the full size of the destination surface.
 *
 * This function sets the destination rectangle in the graphics context. If
 * the provided rectangle is valid, it is assigned to the context. If the
 * rectangle is NULL and a destination surface is present, the rectangle is
 * set to cover the entire surface.
 */

void ite_it2d_set_dst_rect (ite_it2d_context_t * ctx, ite_it2d_rect_t * rect)
{
    assert(ctx);
    if (rect)
    {
        ctx->dst_rect = *rect;
    }
    else if (ctx->dst_surf)
    {
        ctx->dst_rect.x = 0;
        ctx->dst_rect.y = 0;
        ctx->dst_rect.w = ctx->dst_surf->width;
        ctx->dst_rect.h = ctx->dst_surf->height;
    }
}

/**
 * @brief Sets the clipping area of the graphics context.
 *
 * @param ctx The graphics context.
 * @param area The clipping area to set. If NULL, the clipping area is
 *             reset to the full size of the destination surface.
 *
 * This function sets the clipping area in the graphics context. If the
 * provided area is valid, it is assigned to the context. If the area is
 * NULL and a destination surface is present, the clipping area is reset
 * to cover the entire surface.
 */
void ite_it2d_set_clip_area (ite_it2d_context_t * ctx, ite_it2d_area_t * area)
{
    assert(ctx);
    if (area)
    {
        ctx->clip_area = *area;
    }
}

/**
 * @brief Sets the source rectangle of the graphics context.
 *
 * @param ctx The graphics context.
 * @param rect The source rectangle to set. If NULL, the source rectangle
 *             is set to the full size of the source surface.
 *
 * This function sets the source rectangle of the graphics context. If the
 * provided rectangle is valid, it is assigned to the context. If the
 * rectangle is NULL and a source surface is present, the rectangle is
 * set to cover the entire surface.
 */
void ite_it2d_set_src_rect (ite_it2d_context_t * ctx, ite_it2d_rect_t * rect)
{
    assert(ctx);
    if (rect)
    {
        ctx->op.bitblt.src_rect = *rect;
    }
    else if (ctx->op.bitblt.src_surf)
    {
        ctx->op.bitblt.src_rect.x = 0;
        ctx->op.bitblt.src_rect.y = 0;
        ctx->op.bitblt.src_rect.w = ctx->op.bitblt.src_surf->width;
        ctx->op.bitblt.src_rect.h = ctx->op.bitblt.src_surf->height;
    }
}

/**
 * @brief Sets the transformation matrix for the graphics context.
 *
 * @param ctx The graphics context.
 * @param mat The transformation matrix to set. If NULL, the current
 *            transformation matrix remains unchanged.
 *
 * This function assigns the specified transformation matrix to the
 * graphics context, which will be used for subsequent operations
 * that support transformations.
 */

void ite_it2d_set_transform (ite_it2d_context_t * ctx, ite_it2d_matrix_t * mat)
{
    assert(ctx);
    if (mat)
    {
        memcpy(ctx->op.bitblt.transform_mat, mat, sizeof(ite_it2d_matrix_t));
    }
}

/**
 * @brief Sets the line area of the graphics context.
 *
 * @param ctx The graphics context.
 * @param area The line area to set. If NULL, the line area is
 *             reset to an empty area.
 *
 * This function assigns the specified line area to the graphics
 * context, which will be used for subsequent line drawing operations.
 */
void ite_it2d_set_line_area (ite_it2d_context_t * ctx, ite_it2d_area_t * area)
{
    assert(ctx);
    if (area)
    {
        ctx->op.line.line_area = *area;
    }
}

/**
 * @brief Initializes the graphics context to zero.
 *
 * @param ctx The graphics context to initialize.
 *
 * This function initializes the graphics context to zero. It is used to
 * reset the context to its default state.
 */
void ite_it2d_ctx_init (ite_it2d_context_t * ctx)
{
    assert(ctx);
    memset(ctx, 0, sizeof(ite_it2d_context_t));
}

/**
 * @brief Calculates the command size for a bitblt operation.
 *
 * @param ctx The graphics context.
 *
 * This function calculates the command size for a bitblt operation
 * according to the settings of the graphics context. The command size
 * is the memory size of the command queue needed to store the bitblt
 * commands.
 *
 * @return The command size in bytes.
 */
static uint32_t it2d_calc_bitblt_cmd_size (ite_it2d_context_t * ctx)
{
    uint32_t cmd_size = 0;

    if (ctx->op.bitblt.src_surf)
    {
        /* SRC, SRCXY, SRHWR, SPR */
        cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);
    }

    if (ctx->flags.masking)
    {
        /* MASK, MASKXY, MHWR, MPR */
        cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);
    }

    /* DST, DSTXY, DHWR, DPR */
    cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);

    if (ctx->flags.clipping)
    {
        /* PXCR, PYCR */
        cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 2 * sizeof(uint32_t);
    }

    if (ctx->op.bitblt.flags.transforming)
    {
        /* ITMR00 ~ ITMR22, NULL */
        cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 10 * sizeof(uint32_t);
    }

    if (ctx->op.bitblt.flags.fg_color)
    {
        /* FGCOLOR */
        cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;
    }

    /* CAR */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    /* SAFE */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    /* CR1, CR2, CR3, CMD */
    cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);

    /* ID1 */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    return cmd_size;
}

/**
 * @brief Calculate the size of command queue required for gradient fill operation
 *
 * @param[in] ctx    Pointer to the IT2D context
 *
 * @return The size of command queue required
 */
static uint32_t it2d_calc_gradfill_cmd_size (ite_it2d_context_t * ctx)
{
    uint32_t cmd_size = 0;

    if (ctx->flags.masking)
    {
        /* MASK, MASKXY, MHWR, MPR */
        cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);
    }

    /* DST, DSTXY, DHWR, DPR */
    cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);

    if (ctx->flags.clipping)
    {
        /* PXCR, PYCR */
        cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 2 * sizeof(uint32_t);
    }

    /* ITMR00 ~ ITMR22, NULL */
    cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 10 * sizeof(uint32_t);

    /* FGCOLOR */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    /* CAR */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

#if (CFG_CHIP_FAMILY == 9860)
    /* SAFE */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;
#endif

    /* CR1, CR2, CR3, CMD */
    cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);

    /* ID1 */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    return cmd_size;
}

/**
 * @brief Calculates the command size for a line drawing operation.
 *
 * @param ctx The graphics context.
 *
 * This function calculates the command size for a line drawing operation
 * based on the settings of the graphics context. It determines the memory
 * size of the command queue needed to store the commands for drawing a line,
 * including handling of source, destination, masking, clipping, and other
 * related attributes.
 *
 * @return The command size in bytes.
 */

static uint32_t it2d_calc_line_cmd_size (ite_it2d_context_t * ctx)
{
    uint32_t cmd_size = 0;

    /* SRCXY */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    if (ctx->flags.masking)
    {
        /* MASK, MASKXY, MHWR, MPR */
        cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);
    }

    /* DST, DSTXY, DHWR, DPR */
    cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);

    if (ctx->flags.clipping)
    {
        /* PXCR, PYCR */
        cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 2 * sizeof(uint32_t);
    }

    /* LNER */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    /* FGCOLOR */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    /* LINE_P1 */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    /* CAR */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

#if (CFG_CHIP_FAMILY == 9860)
    /* SAFE */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;
#endif

    /* CR1, CR2, CR3, CMD */
    cmd_size += ITH_CMDQ_BURST_CMD_SIZE + 4 * sizeof(uint32_t);

    /* ID1 */
    cmd_size += ITH_CMDQ_SINGLE_CMD_SIZE;

    return cmd_size;
}

/**
 * @brief Fills command queue with source surface commands
 *
 * @param[in] ctx    Pointer to the IT2D context
 * @param[in,out] ptr Pointer to the command queue
 *
 * @return The updated pointer to the command queue
 *
 * This function fills the command queue with the source surface commands
 * required for a bitblt operation. It fills the commands for source surface
 * address, source surface position, source surface width and height, and
 * source surface pitch.
 */
static uint32_t * it2d_fill_src_surf_cmds (const ite_it2d_context_t * ctx,
                                           uint32_t *                 ptr)
{
    assert(ctx->op.bitblt.src_surf);

    /* SRC */
    *ptr++ = (uint32_t)ctx->op.bitblt.src_surf->buf;

    /* SRCXY */
    *ptr++ = (ctx->op.bitblt.src_rect.x << 16) | ctx->op.bitblt.src_rect.y;

    /* SRHWR */
    *ptr++ = (ctx->op.bitblt.src_rect.w << 16) | ctx->op.bitblt.src_rect.h;

    /* SPR */
    *ptr++ = ctx->op.bitblt.src_surf->pitch;

    return ptr;
}

/**
 * @brief Fills command queue with mask surface commands
 *
 * @param[in] ctx    Pointer to the IT2D context
 * @param[in,out] ptr Pointer to the command queue
 *
 * @return The updated pointer to the command queue
 *
 * This function fills the command queue with the mask surface commands
 * required for a bitblt operation. It fills the commands for mask surface
 * address, mask surface position, mask surface width and height, and
 * mask surface pitch.
 */
static uint32_t * it2d_fill_mask_surf_cmds (const ite_it2d_context_t * ctx,
                                            uint32_t *                 ptr)
{
    assert(ctx->mask_surf);

    /* MASK */
    *ptr++ = (uint32_t)ctx->mask_surf->buf;

    /* MASKXY */
    *ptr++ = (ctx->mask_pos.x << 16) | ctx->mask_pos.y;

    /* MHWR */
    *ptr++ = (ctx->dst_rect.w << 16) | ctx->dst_rect.h;

    /* MPR */
    *ptr++ = ctx->mask_surf->pitch;

    return ptr;
}

/**
 * @brief Fills command queue with destination surface commands
 *
 * @param[in] ctx    Pointer to the IT2D context
 * @param[in,out] ptr Pointer to the command queue
 *
 * @return The updated pointer to the command queue
 *
 * This function fills the command queue with the destination surface
 * commands required for a bitblt operation. It fills the commands for
 * destination surface address, destination surface position, destination
 * surface width and height, and destination surface pitch.
 */
static uint32_t * it2d_fill_dst_surf_cmds (const ite_it2d_context_t * ctx,
                                           uint32_t *                 ptr)
{
    assert(ctx->dst_surf);

    /* DST */
    *ptr++ = (uint32_t)ctx->dst_surf->buf;

    /* DSTXY */
    *ptr++ = (ctx->dst_rect.x << 16) | ctx->dst_rect.y;

    /* DHWR */
    *ptr++ = (ctx->dst_rect.w << 16) | ctx->dst_rect.h;

    /* DPR */
    *ptr++ = ctx->dst_surf->pitch;

    return ptr;
}

/**
 * @brief Fills command queue with clip commands
 *
 * @param[in] ctx    Pointer to the IT2D context
 * @param[in,out] ptr Pointer to the command queue
 *
 * @return The updated pointer to the command queue
 *
 * This function fills the command queue with the clipping commands
 * required for a bitblt operation. It fills the commands for the
 * clip rectangle top-left and bottom-right coordinates.
 */
static uint32_t * it2d_fill_clip_cmds (const ite_it2d_context_t * ctx,
                                       uint32_t *                 ptr)
{
    /* PXCR */
    *ptr++ = (ctx->clip_area.x1 << 16) | ctx->clip_area.x2;

    /* PYCR */
    *ptr++ = (ctx->clip_area.y1 << 16) | ctx->clip_area.y2;

    return ptr;
}

/**
 * @brief Fills command queue with transformation commands
 *
 * @param[in] ctx    Pointer to the IT2D context
 * @param[in,out] ptr Pointer to the command queue
 *
 * @return The updated pointer to the command queue
 *
 * This function fills the command queue with the transformation commands
 * required for a bitblt operation. It fills the commands for the 3x3 matrix
 * elements, and a null command to end the list.
 */
static uint32_t * it2d_fill_transform_cmds (const ite_it2d_context_t * ctx,
                                            uint32_t *                 ptr)
{
    /* ITMR00 ~ ITMR22 */
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            *ptr++ =
                (int32_t)(ctx->op.bitblt.transform_mat[row][col] * (1 << 19));
        }
    }

    /* NULL CMD */
    *ptr++ = 0;

    return ptr;
}

/**
 * @brief Fills command queue with constant alpha commands
 *
 * @param[in] ctx    Pointer to the IT2D context
 * @param[in,out] ptr Pointer to the command queue
 *
 * @return The updated pointer to the command queue
 *
 * This function fills the command queue with the constant alpha commands
 * required for a bitblt operation. It fills the command for the constant
 * alpha value, and optionally the color key enable bit.
 */
static uint32_t * it2d_fill_const_alpha_cmds (const ite_it2d_context_t * ctx,
                                              uint32_t *                 ptr)
{
    uint32_t value = (ctx->const_alpha << 8) | ctx->op.bitblt.rop3;

    if ((ctx->flags.cmd == ITE_IT2D_CMD_BITBLT) &&
        ctx->op.bitblt.flags.color_key)
    {
        value |= 0x1 << 16;
    }

    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_CAR, value);

    return ptr;
}

/**
 * @brief Fills command queue with safe commands
 *
 * @param[in] ctx    Pointer to the IT2D context
 * @param[in,out] ptr Pointer to the command queue
 *
 * @return The updated pointer to the command queue
 *
 * This function fills the command queue with the safe commands required
 * for a bitblt operation. It fills the command for the safe mode, and
 * optionally the force transform enable bit.
 */
static uint32_t * it2d_fill_safe_cmds (const ite_it2d_context_t * ctx,
                                       uint32_t *                 ptr)
{
    uint32_t value = 0;

#if (CFG_CHIP_FAMILY == 9860)
	value |= 0x1 << 19; /* disable write reorder */
	value |= 0x1 << 18; /* disable read reorder */
#else
	value |= 0x1 << 13; /* disable boundary interpolation */
#endif

    if ((ctx->flags.cmd == ITE_IT2D_CMD_BITBLT) &&
        ctx->op.bitblt.flags.force_transform)
    {
        value |= 0x1 << 9;
    }

    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_SAFE, value);

    return ptr;
}

/**
 * @brief Fills command queue with control commands
 *
 * @param[in] ctx    Pointer to the IT2D context
 * @param[in,out] ptr Pointer to the command queue
 *
 * @return The updated pointer to the command queue
 *
 * This function fills the command queue with the control commands required
 * for a bitblt operation. It fills the commands for the control register 1,
 * control register 2, control register 3 and the command.
 */
static uint32_t * it2d_fill_ctrl_cmds (const ite_it2d_context_t * ctx,
                                       uint32_t *                 ptr)
{
    assert(ctx->dst_surf);
    uint32_t value;

    /* CR1 */
    value = 0;
    value |= ctx->flags.alpha_dst << 22;
    if (ctx->flags.dithering)
    {
        value |= 0x1 << 16;
    }
    if (ctx->op.bitblt.flags.transforming)
    {
        if (ctx->op.bitblt.flags.scan_column_major)
        {
            value |= 0x1 << 21;
        }
        if ((ctx->op.bitblt.transform_mat[2][0] != 0.0f) ||
            (ctx->op.bitblt.transform_mat[2][1] != 0.0f)) {
            value |= 0x1 << 10;
        }
        value |= 0x1 << 9;

        if (ctx->op.bitblt.flags.interpolation)
        {
            value |= 0x1 << 6;
        }
        value |= ctx->op.bitblt.flags.tile_mode << 4;
    }
    if (ctx->flags.clipping)
    {
        value |= 0x1 << 7;
    }
    if (ctx->flags.masking)
    {
        value |= 0x1 << 2;
    }
    if (ctx->flags.blending)
    {
        value |= 0x1 << 1;
    }
    if (ctx->flags.const_alpha)
    {
        value |= 0x1 << 0;
    }
    *ptr++ = value;

    /* CR2 */
    value  = 0;
    value |= ctx->op.gradfill.direction << 20;
    if (ctx->flags.masking)
    {
        assert(ctx->mask_surf);
        assert((ctx->mask_surf->flags.format >= ITE_IT2D_PIXEL_FORMAT_A_8) &&
               (ctx->mask_surf->flags.format <= ITE_IT2D_PIXEL_FORMAT_A_1));
        value |= (ctx->mask_surf->flags.format - ITE_IT2D_PIXEL_FORMAT_A_8)
                 << 16;
    }
    value |= ctx->dst_surf->flags.format << 8;

    if ((ctx->flags.cmd == ITE_IT2D_CMD_BITBLT) && ctx->op.bitblt.src_surf)
    {
        value |= ctx->op.bitblt.src_surf->flags.format;
    }
    else
    {
        value |= ctx->dst_surf->flags.format;
    }

    *ptr++ = value;

    /* CR3 */
  	value = 0x1;
  
  	if (ctx->flags.cmd == ITE_IT2D_CMD_LINE) {
    		value |= ctx->op.line.line_p2 << 16;
    		value |= ctx->op.line.line_width << 8;
    		if (ctx->op.line.flags.alter_algorithm) {
    			 value |= 0x1 << 2;
    		}
    		if (ctx->op.line.flags.last_pixel) {
    			 value |= 0x1 << 1;
    		}
  	}
    *ptr++ = value;

    /* CMD */
    *ptr++ = ctx->flags.cmd;

    return ptr;
}

/**
 * @brief Fill color delta commands for gradient fill.
 *
 * @param[in]  ctx  Pointer to the IT2D context.
 * @param[out] ptr   Pointer to the command buffer.
 *
 * @return Pointer to the end of the generated command buffer.
 *
 * This function generates color delta commands for a gradient fill. The
 * commands are used to update the color registers of the IT2D hardware. The
 * function takes the destination surface width and height into account to
 * calculate the color delta values. The function also takes the gradient
 * direction into account to generate the correct commands.
 *
 * @note The function does not check if the generated commands exceed the
 *       command buffer size. The caller is responsible to ensure that the
 *       command buffer is large enough to hold the generated commands.
 */
static uint32_t * it2d_fill_color_delta_cmds (const ite_it2d_context_t * ctx,
                                              uint32_t *                 ptr)
{
    assert(ctx);
    assert(ctx->dst_surf);
    int dst_width  = ctx->dst_rect.w;
    int dst_height = ctx->dst_rect.h;
    int delta_alpha =
        ctx->op.gradfill.end_color.ch.alpha - ctx->fg_color.ch.alpha;
    int delta_red = ctx->op.gradfill.end_color.ch.red - ctx->fg_color.ch.red;
    int delta_green =
        ctx->op.gradfill.end_color.ch.green - ctx->fg_color.ch.green;
    int delta_blue = ctx->op.gradfill.end_color.ch.blue - ctx->fg_color.ch.blue;

    switch (ctx->op.gradfill.direction)
    {
        case ITE_IT2D_GRAD_DIR_HOR:
            /* ITMR00 */
            *ptr++ = ((delta_alpha * (1 << 12)) / dst_width) * (1 << 4);
            /* ITMR01 */
            *ptr++ = ((delta_red * (1 << 12)) / dst_width) * (1 << 4);
            /* ITMR02 */
            *ptr++ = ((delta_green * (1 << 12)) / dst_width) * (1 << 4);
            /* ITMR10 */
            *ptr++ = ((delta_blue * (1 << 12)) / dst_width) * (1 << 4);
            /* ITMR11 */
            *ptr++ = 0x80000;
            /* ITMR12 */
            *ptr++ = 0x0;
            /* ITMR20 */
            *ptr++ = 0x0;
            /* ITMR21 */
            *ptr++ = 0x0;
            /* ITMR22 */
            *ptr++ = 0x80000;
            break;

        case ITE_IT2D_GRAD_DIR_VER:
            /* ITMR00 */
            *ptr++ = 0x80000;
            /* ITMR01 */
            *ptr++ = 0x0;
            /* ITMR02 */
            *ptr++ = 0x0;
            /* ITMR10 */
            *ptr++ = 0x0;
            /* ITMR11 */
            *ptr++ = ((delta_alpha * (1 << 12)) / dst_height) * (1 << 4);
            /* ITMR12 */
            *ptr++ = ((delta_red * (1 << 12)) / dst_height) * (1 << 4);
            /* ITMR20 */
            *ptr++ = ((delta_green * (1 << 12)) / dst_height) * (1 << 4);
            /* ITMR21 */
            *ptr++ = ((delta_blue * (1 << 12)) / dst_height) * (1 << 4);
            /* ITMR22 */
            *ptr++ = 0x80000;
            break;

        case ITE_IT2D_GRAD_DIR_DIAG:
            /* ITMR00 */
            *ptr++ = ((delta_alpha * (1 << 11)) / dst_width) * (1 << 4);
            /* ITMR01 */
            *ptr++ = ((delta_red * (1 << 11)) / dst_width) * (1 << 4);
            /* ITMR02 */
            *ptr++ = ((delta_green * (1 << 11)) / dst_width) * (1 << 4);
            /* ITMR10 */
            *ptr++ = ((delta_blue * (1 << 11)) / dst_width) * (1 << 4);
            /* ITMR11 */
            *ptr++ = ((delta_alpha * (1 << 11)) / dst_height) * (1 << 4);
            /* ITMR12 */
            *ptr++ = ((delta_red * (1 << 11)) / dst_height) * (1 << 4);
            /* ITMR20 */
            *ptr++ = ((delta_green * (1 << 11)) / dst_height) * (1 << 4);
            /* ITMR21 */
            *ptr++ = ((delta_blue * (1 << 11)) / dst_height) * (1 << 4);
            /* ITMR22 */
            *ptr++ = 0x80000;
            break;

        default:
            assert(0);
    }

    /* NULL CMD */
    *ptr++ = 0;

    return ptr;
}

/**
 * @brief Fills the command queue with the starting coordinates of a line.
 *
 * @param[in] ctx Pointer to the IT2D context containing line information.
 * @param[in,out] ptr Pointer to the command queue buffer.
 *
 * @return The updated pointer to the command queue buffer.
 *
 * This function encodes the starting x and y coordinates of a line into a
 * command and appends it to the command queue. The coordinates are extracted
 * from the line_area field of the context's line operation.
 */

static uint32_t * it2d_fill_line_start_cmds (const ite_it2d_context_t * ctx,
                                             uint32_t *                 ptr)
{
    uint32_t value =
        (ctx->op.line.line_area.x1 << 16) | ctx->op.line.line_area.y1;

    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_SRCXY, value);

    return ptr;
}

/**
 * @brief Fills the command queue with the ending coordinates of a line.
 *
 * @param[in] ctx Pointer to the IT2D context containing line information.
 * @param[in,out] ptr Pointer to the command queue buffer.
 *
 * @return The updated pointer to the command queue buffer.
 *
 * This function encodes the ending x and y coordinates of a line into a command
 * and appends it to the command queue. The coordinates are extracted from the
 * line_area field of the context's line operation.
 */
static uint32_t * it2d_fill_line_end_cmds (const ite_it2d_context_t * ctx,
                                           uint32_t *                 ptr)
{
    uint32_t value =
        (ctx->op.line.line_area.x2 << 16) | ctx->op.line.line_area.y2;

    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_LNER, value);

    return ptr;
}


/**
 * @brief Increments the last used ID, ensuring it is never zero.
 *
 * This function increments the `last_id` field in the `it2d_data` structure.
 * If the incremented ID value reaches zero, it wraps around to one to ensure
 * that the ID is never zero, as zero might be reserved or invalid.
 */

static void it2d_incr_last_id (void)
{
    if (++it2d_data.last_id == 0)
    {
        it2d_data.last_id = 1;
    }
}

/**
 * @brief Generates commands for a bitblt operation.
 *
 * @param ctx Pointer to the IT2D context containing bitblt information.
 *
 * This function generates the commands for a bitblt operation according to
 * the settings of the graphics context. The command generation includes
 * the source and destination surface commands, optional mask surface
 * commands, optional clipping commands, optional color key enable and
 * foreground color commands, and the control commands.
 *
 * @return 0 on success, negative value on failure.
 */
static int it2d_bitblt (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->flags.cmd == ITE_IT2D_CMD_BITBLT);
    uint32_t cmd_size = it2d_calc_bitblt_cmd_size(ctx);
    ithCmdQLock(ITH_CMDQ0_OFFSET);
    uint32_t * ptr = ithCmdQWaitSize(cmd_size, ITH_CMDQ0_OFFSET);
    assert(ptr);

    if (ctx->op.bitblt.src_surf)
    {
        /* SRC, SRCXY, SRHWR, SPR */
        ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_SRC, 4);
        ptr                         = it2d_fill_src_surf_cmds(ctx, ptr);
        ctx->op.bitblt.src_surf->id = it2d_data.last_id;
    }

    if (ctx->flags.masking)
    {
        assert(ctx->mask_surf);
        /* MASK, MASKXY, MHWR, MPR */
        ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_MASK, 4);
        ptr                = it2d_fill_mask_surf_cmds(ctx, ptr);
        ctx->mask_surf->id = it2d_data.last_id;
    }

    /* DST, DSTXY, DHWR, DPR */
    assert(ctx->dst_surf);
    ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_DST, 4);
    ptr               = it2d_fill_dst_surf_cmds(ctx, ptr);
    ctx->dst_surf->id = it2d_data.last_id;

    if (ctx->flags.clipping)
    {
        /* PXCR, PYCR */
        ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_PXCR, 2);
        ptr = it2d_fill_clip_cmds(ctx, ptr);
    }

    if (ctx->op.bitblt.flags.transforming)
    {
        /* ITMR00 ~ ITMR22, NULL */
        ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_ITMR00, 10);
        ptr = it2d_fill_transform_cmds(ctx, ptr);
    }

    if (ctx->op.bitblt.flags.fg_color)
    {
        /* FGCOLOR */
        ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_FGCOLOR,
                            ctx->fg_color.full);
    }

    /* CAR */
    ptr = it2d_fill_const_alpha_cmds(ctx, ptr);

    /* SAFE */
    ptr = it2d_fill_safe_cmds(ctx, ptr);

    /* CR1, CR2, CR3, CMD */
    ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_CR1, 4);
    ptr = it2d_fill_ctrl_cmds(ctx, ptr);

    /* ID1 */
    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_ID1, it2d_data.last_id);

    ithCmdQFlush(ptr, ITH_CMDQ0_OFFSET);
    ithCmdQUnlock(ITH_CMDQ0_OFFSET);

    it2d_incr_last_id();

    return 0;
}

/**
 * @brief Executes a gradient fill operation.
 *
 * This function generates and queues the necessary commands for performing
 * a gradient fill operation based on the provided IT2D context. It handles
 * various operations including masking, clipping, and color transformations
 * as needed, and ensures the command queue is properly locked and flushed.
 *
 * @param[in] ctx Pointer to the IT2D context containing gradient fill information.
 *
 * @return 0 on success, negative value on failure.
 */

static int it2d_gradfill (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->flags.cmd == ITE_IT2D_CMD_GRADFILL);
    uint32_t cmd_size = it2d_calc_gradfill_cmd_size(ctx);
    ithCmdQLock(ITH_CMDQ0_OFFSET);
    uint32_t * ptr = ithCmdQWaitSize(cmd_size, ITH_CMDQ0_OFFSET);
    assert(ptr);

    if (ctx->flags.masking)
    {
        assert(ctx->mask_surf);
        /* MASK, MASKXY, MHWR, MPR */
        ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_MASK, 4);
        ptr                = it2d_fill_mask_surf_cmds(ctx, ptr);
        ctx->mask_surf->id = it2d_data.last_id;
    }

    /* DST, DSTXY, DHWR, DPR */
    assert(ctx->dst_surf);
    ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_DST, 4);
    ptr               = it2d_fill_dst_surf_cmds(ctx, ptr);
    ctx->dst_surf->id = it2d_data.last_id;

    if (ctx->flags.clipping)
    {
        /* PXCR, PYCR */
        ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_PXCR, 2);
        ptr = it2d_fill_clip_cmds(ctx, ptr);
    }

    /* ITMR00 ~ ITMR22, NULL */
    ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_ITMR00, 10);
    ptr = it2d_fill_color_delta_cmds(ctx, ptr);

    /* FGCOLOR */
    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_FGCOLOR,
                        ctx->fg_color.full);

    /* CAR */
    ptr = it2d_fill_const_alpha_cmds(ctx, ptr);

    /* SAFE */
#if (CFG_CHIP_FAMILY == 9860)
    ptr = it2d_fill_safe_cmds(ctx, ptr);
#endif

    /* CR1, CR2, CR3, CMD */
    ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_CR1, 4);
    ptr = it2d_fill_ctrl_cmds(ctx, ptr);

    /* ID1 */
    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_ID1, it2d_data.last_id);

    ithCmdQFlush(ptr, ITH_CMDQ0_OFFSET);
    ithCmdQUnlock(ITH_CMDQ0_OFFSET);

    it2d_incr_last_id();

    return 0;
}

/**
 * @brief Executes a line drawing operation.
 *
 * This function generates and queues the necessary commands for performing
 * a line drawing operation based on the provided IT2D context. It handles
 * operations such as masking, clipping, and color transformations when required.
 * The function ensures the command queue is properly locked and flushed, and
 * the command ID is incremented after execution.
 *
 * @param[in] ctx Pointer to the IT2D context containing line drawing information.
 *
 * @return 0 on success, negative value on failure.
 */

static int it2d_line (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->flags.cmd == ITE_IT2D_CMD_LINE);
    uint32_t cmd_size = it2d_calc_line_cmd_size(ctx);
    ithCmdQLock(ITH_CMDQ0_OFFSET);
    uint32_t * ptr = ithCmdQWaitSize(cmd_size, ITH_CMDQ0_OFFSET);
    assert(ptr);

    /* SRCXY */
    ptr = it2d_fill_line_start_cmds(ctx, ptr);

    if (ctx->flags.masking)
    {
        assert(ctx->mask_surf);
        /* MASK, MASKXY, MHWR, MPR */
        ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_MASK, 4);
        ptr                = it2d_fill_mask_surf_cmds(ctx, ptr);
        ctx->mask_surf->id = it2d_data.last_id;
    }

    /* DST, DSTXY, DHWR, DPR */
    assert(ctx->dst_surf);
    ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_DST, 4);
    ptr               = it2d_fill_dst_surf_cmds(ctx, ptr);
    ctx->dst_surf->id = it2d_data.last_id;

    if (ctx->flags.clipping)
    {
        /* PXCR, PYCR */
        ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_PXCR, 2);
        ptr = it2d_fill_clip_cmds(ctx, ptr);
    }

    /* LNER */
    ptr = it2d_fill_line_end_cmds(ctx, ptr);

    /* FGCOLOR */
    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_FGCOLOR,
                        ctx->fg_color.full);

    /* LINE_P1 */
    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_LINE_P1,
                        ctx->op.line.line_p1);

    /* CAR */
    ptr = it2d_fill_const_alpha_cmds(ctx, ptr);

    /* SAFE */
#if (CFG_CHIP_FAMILY == 9860)
    ptr = it2d_fill_safe_cmds(ctx, ptr);
#endif

    /* CR1, CR2, CR3, CMD */
    ITH_CMDQ_BURST_CMD(ptr, it2d_config.base + REG_2D_CR1, 4);
    ptr = it2d_fill_ctrl_cmds(ctx, ptr);

    /* ID1 */
    ITH_CMDQ_SINGLE_CMD(ptr, it2d_config.base + REG_2D_ID1, it2d_data.last_id);

    ithCmdQFlush(ptr, ITH_CMDQ0_OFFSET);
    ithCmdQUnlock(ITH_CMDQ0_OFFSET);

    it2d_incr_last_id();

    return 0;
}

/**
 * @brief Calculates the destination rectangle for a graphics operation.
 *
 * @param ctx Pointer to the IT2D context
 *
 * @return true if the destination rectangle is valid, false otherwise.
 *
 * This function calculates the destination rectangle for a graphics
 * operation. It takes into account the destination surface's width and
 * height, and also considers the destination rectangle's position and
 * size. If the destination rectangle is outside the destination surface,
 * the function returns false. Otherwise, it updates the destination
 * rectangle and returns true.
 */
static bool ite_it2d_calc_dst_rect (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->dst_surf);
    ite_it2d_surface_t *  dst_surf      = ctx->dst_surf;
    ite_it2d_rect_t *     dst_rect      = &ctx->dst_rect;
	  ite_it2d_point_t *mask_pos = &ctx->mask_pos;
    const ite_it2d_rect_t dst_surf_rect = {0, 0, dst_surf->width,
                                           dst_surf->height};

    if (dst_rect->x < 0)
    {
		    mask_pos->x -= dst_rect->x;
        dst_rect->x = 0;
    }

    if (dst_rect->y < 0)
    {
		    mask_pos->y -= dst_rect->y;
        dst_rect->y = 0;
    }

    if (dst_rect->x + dst_rect->w > dst_surf->width)
    {
        dst_rect->w = dst_surf->width - dst_rect->x;
    }

    if (dst_rect->y + dst_rect->h > dst_surf->height)
    {
        dst_rect->h = dst_surf->height - dst_rect->y;
    }

    if (!ite_it2d_rect_has_intersect(&dst_surf_rect, dst_rect))
    {
        return false;
    }
    return true;
}

/**
 * @brief Calculates the destination rectangle with respect to the source surface.
 *
 * This function adjusts the destination and source rectangles within the given
 * graphics context to ensure they are within the bounds of their respective surfaces.
 * It checks and corrects the positions and dimensions of the rectangles if they
 * are outside of the surface boundaries. The function also ensures that the
 * destination and source rectangles intersect with their corresponding surfaces.
 *
 * @param ctx Pointer to the IT2D context containing the source and destination
 *            surface and rectangle information.
 *
 * @return true if both the destination and source rectangles are valid and intersect
 *         with their respective surfaces, false otherwise.
 */

static bool ite_it2d_calc_dst_rect_with_src (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->dst_surf);
    assert(ctx->op.bitblt.src_surf);
    ite_it2d_surface_t *  dst_surf      = ctx->dst_surf;
    ite_it2d_surface_t *  src_surf      = ctx->op.bitblt.src_surf;
    ite_it2d_rect_t *     dst_rect      = &ctx->dst_rect;
    ite_it2d_rect_t *     src_rect      = &ctx->op.bitblt.src_rect;
	  ite_it2d_point_t *mask_pos = &ctx->mask_pos;
    const ite_it2d_rect_t dst_surf_rect = {0, 0, dst_surf->width,
                                           dst_surf->height};
    const ite_it2d_rect_t src_surf_rect = {0, 0, src_surf->width,
                                           src_surf->height};

    if (src_rect->x < 0)
    {
        dst_rect->x -= src_rect->x;
        src_rect->w += src_rect->x;
		    mask_pos->x -= src_rect->x;
        src_rect->x = 0;
    }

    if (src_rect->y < 0)
    {
        dst_rect->y -= src_rect->y;
        src_rect->h += src_rect->y;
		    mask_pos->y -= src_rect->y;
        src_rect->y = 0;
    }

    if (dst_rect->x < 0)
    {
        src_rect->x -= dst_rect->x;
        src_rect->w += dst_rect->x;
		    mask_pos->x -= dst_rect->x;
        dst_rect->x = 0;
    }

    if (dst_rect->y < 0)
    {
        src_rect->y -= dst_rect->y;
        src_rect->h += dst_rect->y;
		    mask_pos->y -= dst_rect->y;
        dst_rect->y = 0;
    }

    if (src_rect->x + src_rect->w > src_surf->width)
    {
        src_rect->w = src_surf->width - src_rect->x;
    }

    if (src_rect->y + src_rect->h > src_surf->height)
    {
        src_rect->h = src_surf->height - src_rect->y;
    }

    dst_rect->w = src_rect->w;
    dst_rect->h = src_rect->h;

    if (dst_rect->x + dst_rect->w > dst_surf->width)
    {
        dst_rect->w = dst_surf->width - dst_rect->x;
    }

    if (dst_rect->y + dst_rect->h > dst_surf->height)
    {
        dst_rect->h = dst_surf->height - dst_rect->y;
    }

    if (!ite_it2d_rect_has_intersect(&dst_surf_rect, dst_rect))
    {
        return false;
    }
    if (!ite_it2d_rect_has_intersect(&src_surf_rect, src_rect))
    {
        return false;
    }
    return true;
}

/**
 * @brief Fills the destination rectangle with the foreground color.
 *
 * @param ctx Pointer to the IT2D context.
 *
 * @return The number of pixels that were drawn.
 *
 * This function fills the destination rectangle with the foreground color.
 * The foreground color is a 32-bit color value specified by the
 * ite_it2d_set_fg_color() function. If the destination rectangle is
 * outside the destination surface, the function returns false.
 * Otherwise, it updates the destination rectangle and returns true.
 */
int ite_it2d_fill (ite_it2d_context_t * ctx)
{
    assert(ctx);

    if (!ite_it2d_calc_dst_rect(ctx))
    {
        return 0;
    }

    ctx->flags.cmd          = ITE_IT2D_CMD_BITBLT;
    ctx->op.bitblt.src_surf = NULL;
    if (!ctx->op.bitblt.flags.rop3) {
        ctx->op.bitblt.rop3 = ITE_IT2D_ROP3_PATCOPY;
    }
    ite_it2d_enable_transforming(ctx, false);
    ctx->op.bitblt.flags.fg_color = true;

    return it2d_bitblt(ctx);
}

/**
 * @brief Copies the source surface to the destination surface.
 *
 * @param ctx Pointer to the IT2D context.
 *
 * @return The number of pixels that were drawn.
 *
 * This function copies the source surface to the destination surface.
 * The source surface must be a valid surface with a format between
 * A_1 and A_8. The destination surface must also be a valid surface
 * with a format between A_1 and A_8. If the source surface is not
 * valid, the function returns false. If the destination rectangle is
 * outside the destination surface, the function returns false.
 * Otherwise, it updates the destination rectangle and returns true.
 */
int ite_it2d_copy (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->dst_surf);
    assert(ctx->op.bitblt.src_surf);

    if (!ite_it2d_calc_dst_rect_with_src(ctx))
    {
        return 0;
    }

    ctx->flags.cmd = ITE_IT2D_CMD_BITBLT;
    ite_it2d_enable_transforming(ctx, false);
    if (ctx->op.bitblt.flags.color_key)
    {
        ctx->op.bitblt.rop3 = ITE_IT2D_ROP3_SRCAND;
    }
    else if (!ctx->op.bitblt.flags.rop3)
    {
        ctx->op.bitblt.rop3 = ITE_IT2D_ROP3_SRCCOPY;
    }

    return it2d_bitblt(ctx);
}

/**
 * @brief Generates commands for a bitblt operation.
 *
 * @param ctx Pointer to the IT2D context containing bitblt information.
 *
 * This function generates the commands for a bitblt operation according to
 * the settings of the graphics context. The command generation includes
 * the source and destination surface commands, optional mask surface
 * commands, optional clipping commands, optional color key enable and
 * foreground color commands, and the control commands.
 *
 * @return 0 on success, negative value on failure.
 */
int ite_it2d_bitblt (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->dst_surf);

    if (ctx->op.bitblt.src_surf)
    {
        if (!ite_it2d_calc_dst_rect_with_src(ctx))
        {
            return 0;
        }
    }
    else
    {
        if (!ite_it2d_calc_dst_rect(ctx))
        {
            return 0;
        }
    }

    ctx->flags.cmd = ITE_IT2D_CMD_BITBLT;
    ite_it2d_enable_transforming(ctx, false);

    return it2d_bitblt(ctx);
}

/**
 * @brief Generates commands for a bitblt operation with transform.
 *
 * @param ctx Pointer to the IT2D context containing bitblt information.
 * @param dsc Pointer to the IT2D transform descriptor.
 *
 * This function generates the commands for a bitblt operation according to
 * the settings of the graphics context. The command generation includes
 * the source and destination surface commands, optional mask surface
 * commands, optional clipping commands, optional color key enable and
 * foreground color commands, and the control commands.
 *
 * @return 0 on success, negative value on failure.
 */
int ite_it2d_transform_copy (ite_it2d_context_t *       ctx,
                             ite_it2d_transform_dsc_t * dsc)
{
    assert(ctx);
    assert(ctx->dst_surf);
    assert(ctx->op.bitblt.src_surf);
    assert(dsc);
    const ite_it2d_rect_t dst_surf_rect = {0, 0, ctx->dst_surf->width,
                                           ctx->dst_surf->height};
    const ite_it2d_rect_t src_surf_rect = {0, 0, ctx->op.bitblt.src_surf->width,
                                           ctx->op.bitblt.src_surf->height};
    float                 angle         = fmodf(dsc->angle, 360.0f);
    ite_it2d_matrix_t     mat1          = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };
    ite_it2d_matrix_t   mat2;
    ite_it2d_matrix_t * src_mat = &mat1;
    ite_it2d_matrix_t * dst_mat = &mat2;
    int                 ret;

    if (!dsc->flags.auto_dst_rect &&
        !ite_it2d_rect_has_intersect(&dst_surf_rect, &ctx->dst_rect))
    {
        return 0;
    }
    if (!ite_it2d_rect_has_intersect(&src_surf_rect, &ctx->op.bitblt.src_rect))
    {
        return 0;
    }

    if (dsc->pos.x != 0 || dsc->pos.y != 0)
    {
        ite_it2d_mat_translate(*src_mat, *dst_mat, dsc->pos.x, dsc->pos.y);
        SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);
    }

    if (angle != 0.0f)
    {
        if (dsc->center.x != 0.0f || dsc->center.y != 0.0f)
        {
            ite_it2d_mat_translate(*src_mat, *dst_mat, dsc->center.x,
                                   dsc->center.y);
            SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);

            ite_it2d_mat_rotate(*src_mat, *dst_mat, dsc->angle);
            SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);

            ite_it2d_mat_translate(*src_mat, *dst_mat, -dsc->center.x,
                                   -dsc->center.y);
            SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);
        }
        else
        {
            ite_it2d_mat_rotate(*src_mat, *dst_mat, dsc->angle);
            SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);
        }
    }

    if (dsc->scale_x != 1.0f || dsc->scale_y != 1.0f)
    {
        ite_it2d_mat_scale(*src_mat, *dst_mat, dsc->scale_x, dsc->scale_y);
        SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);
    }

    if (dsc->flags.auto_dst_rect)
    {
        ite_it2d_point_t pos1, pos2, p0, p1, p2, p3;

        /* left-top */
        pos1.x = 0;
        pos1.y = 0;
        ite_it2d_mat_transform(*src_mat, &pos1, &p0);

        /* right-top */
        pos1.x = ctx->op.bitblt.src_surf ? ctx->op.bitblt.src_rect.w
                                         : ctx->dst_rect.w;
        ite_it2d_mat_transform(*src_mat, &pos1, &p1);

        /* right-bottom */
        pos1.y = ctx->op.bitblt.src_surf ? ctx->op.bitblt.src_rect.h
                                         : ctx->dst_rect.h;
        ite_it2d_mat_transform(*src_mat, &pos1, &p2);

        /* left-bottom */
        pos1.x = 0;
        ite_it2d_mat_transform(*src_mat, &pos1, &p3);

        pos1.x = (int16_t)roundf(MIN(MIN(p0.x, p1.x), MIN(p2.x, p3.x)));
        pos1.y = (int16_t)roundf(MIN(MIN(p0.y, p1.y), MIN(p2.y, p3.y)));
        pos2.x = (int16_t)roundf(MAX(MAX(p0.x, p1.x), MAX(p2.x, p3.x)));
        pos2.y = (int16_t)roundf(MAX(MAX(p0.y, p1.y), MAX(p2.y, p3.y)));

        ctx->dst_rect.x += pos1.x;
        ctx->dst_rect.y += pos1.y;
        ctx->dst_rect.w = pos2.x - pos1.x + 1;
        ctx->dst_rect.h = pos2.y - pos1.y + 1;

        if (!ite_it2d_rect_has_intersect(&dst_surf_rect, &ctx->dst_rect))
        {
            return 0;
        }

        if (ctx->dst_rect.x < 0)
        {
            pos1.x += -ctx->dst_rect.x;
            ctx->dst_rect.x = 0;
        }

        if (ctx->dst_rect.y < 0)
        {
            pos1.y += -ctx->dst_rect.y;
            ctx->dst_rect.y = 0;
        }

        if (ctx->dst_rect.x + ctx->dst_rect.w >= dst_surf_rect.w)
        {
            ctx->dst_rect.w = dst_surf_rect.w - ctx->dst_rect.x - 1;
        }

        if (ctx->dst_rect.y + ctx->dst_rect.h >= dst_surf_rect.h)
        {
            ctx->dst_rect.h = dst_surf_rect.h - ctx->dst_rect.y - 1;
        }

        ite_it2d_mat_identity(*src_mat);

        ite_it2d_mat_translate(*src_mat, *dst_mat, dsc->pos.x - pos1.x,
                               dsc->pos.y - pos1.y);
        SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);

        if (angle != 0.0f)
        {
            if (dsc->center.x != 0.0f || dsc->center.y != 0.0f)
            {
                ite_it2d_mat_translate(*src_mat, *dst_mat, dsc->center.x,
                                       dsc->center.y);
                SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);

                ite_it2d_mat_rotate(*src_mat, *dst_mat, angle);
                SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);

                ite_it2d_mat_translate(*src_mat, *dst_mat, -dsc->center.x,
                                       -dsc->center.y);
                SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);
            }
            else
            {
                ite_it2d_mat_rotate(*src_mat, *dst_mat, angle);
                SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);
            }
        }

        if (dsc->scale_x != 1.0f || dsc->scale_y != 1.0f)
        {
            ite_it2d_mat_scale(*src_mat, *dst_mat, dsc->scale_x, dsc->scale_y);
            SWAP(src_mat, dst_mat, ite_it2d_matrix_t *);
        }
    }

    if (dsc->flags.inverse) {
        SWAP(ctx->dst_surf, ctx->op.bitblt.src_surf, ite_it2d_surface_t *);
        SWAP(ctx->dst_rect, ctx->op.bitblt.src_rect, ite_it2d_rect_t);
        memcpy(ctx->op.bitblt.transform_mat, *src_mat, sizeof(ite_it2d_matrix_t));
    } else {
        ret = ite_it2d_mat_inverse(*src_mat, ctx->op.bitblt.transform_mat);
        if (ret)
        {
            printf("inverse matrix fail: %d\n", ret);
            return ret;
        }
    }

    if ((ctx->dst_rect.x < 0) || (ctx->dst_rect.y < 0) ||
        (ctx->dst_rect.w > ctx->dst_surf->width) ||
        (ctx->dst_rect.h > ctx->dst_surf->height) || (ctx->dst_rect.w < 3) ||
        (ctx->dst_rect.h < 3))
    {
        if (ctx->flags.clipping)
        {
            ctx->clip_area.x1 = MAX(MAX(ctx->clip_area.x1, ctx->dst_rect.x), 0);
            ctx->clip_area.y1 = MAX(MAX(ctx->clip_area.y1, ctx->dst_rect.y), 0);
            ctx->clip_area.x2 = MIN(
                MIN(ctx->clip_area.x2, ctx->dst_rect.x + ctx->dst_rect.w - 1),
                ctx->dst_surf->width - 1);
            ctx->clip_area.y2 = MIN(
                MIN(ctx->clip_area.y2, ctx->dst_rect.y + ctx->dst_rect.h - 1),
                ctx->dst_surf->height - 1);
        }
        else
        {
            ctx->clip_area.x1   = MAX(ctx->dst_rect.x, 0);
            ctx->clip_area.y1   = MAX(ctx->dst_rect.y, 0);
            ctx->clip_area.x2   = MIN(ctx->dst_rect.x + ctx->dst_rect.w - 1,
                                      ctx->dst_surf->width - 1);
            ctx->clip_area.y2   = MIN(ctx->dst_rect.y + ctx->dst_rect.h - 1,
                                      ctx->dst_surf->height - 1);
            ctx->flags.clipping = true;
        }

        if (ctx->dst_rect.w < 3)
        {
            if (ctx->dst_rect.x + ctx->dst_rect.w > ctx->dst_surf->width)
            {
                return -ENOTSUP;
            }
            ctx->dst_rect.w = 3;
        }
        if (ctx->dst_rect.h < 3)
        {
            if (ctx->dst_rect.y + ctx->dst_rect.h > ctx->dst_surf->height)
            {
                return -ENOTSUP;
            }
            ctx->dst_rect.h = 3;
        }
    }

    ctx->flags.cmd = ITE_IT2D_CMD_BITBLT;
    if (ctx->op.bitblt.flags.color_key)
    {
        ctx->op.bitblt.rop3 = ITE_IT2D_ROP3_SRCAND;
    }
    else if (!ctx->op.bitblt.flags.rop3)
    {
        ctx->op.bitblt.rop3 = ITE_IT2D_ROP3_SRCCOPY;
    }
    ctx->op.bitblt.flags.tile_mode = dsc->flags.tile_mode;
    ite_it2d_enable_transforming(ctx, true);
    if (((dsc->angle > 45.0f) && (dsc->angle < 45.0f * 3)) ||
        ((dsc->angle > 45.0f * 5) && (dsc->angle < 45.0f * 7)))
    {
        ctx->op.bitblt.flags.scan_column_major = true;
    }
    else
    {
        ctx->op.bitblt.flags.scan_column_major = false;
    }

    if ((ctx->op.bitblt.flags.tile_mode != ITE_IT2D_TILE_FILL) ||
        (ctx->dst_surf->flags.format == ITE_IT2D_PIXEL_FORMAT_A_8))
    {
        ctx->op.bitblt.flags.force_transform = true;
    }
    else
    {
        ctx->op.bitblt.flags.force_transform = false;
    }

    ret = it2d_bitblt(ctx);

    if (dsc->flags.inverse) {
        SWAP(ctx->dst_surf, ctx->op.bitblt.src_surf, ite_it2d_surface_t *);
        SWAP(ctx->dst_rect, ctx->op.bitblt.src_rect, ite_it2d_rect_t);
    }
#if (CFG_CHIP_FAMILY == 9860)
    if (ctx->op.bitblt.src_surf && (ctx->op.bitblt.src_rect.w < 3) &&
        (ctx->op.bitblt.flags.tile_mode != ITE_IT2D_TILE_FILL))
    {
        ite_it2d_wait_surface_finish(ctx->op.bitblt.src_surf);
    }
#endif

    return ret;
}

/**
 * @brief Fills the destination rectangle with a gradient fill.
 *
 * @param ctx Pointer to the IT2D context.
 *
 * @return The number of pixels that were drawn.
 *
 * This function fills the destination rectangle with a gradient fill.
 * The gradient fill is a linear or radial gradient specified by the
 * ite_it2d_set_gradient() function. If the destination rectangle is
 * outside the destination surface, the function returns false.
 * Otherwise, it updates the destination rectangle and returns true.
 */
int ite_it2d_gradient_fill (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->dst_surf);

    if (!ite_it2d_calc_dst_rect(ctx))
    {
        return 0;
    }

    ctx->flags.cmd = ITE_IT2D_CMD_GRADFILL;

    return it2d_gradfill(ctx);
}

/**
 * @brief Draws a line on the destination surface.
 *
 * @param ctx Pointer to the IT2D context.
 *
 * @return The number of pixels that were drawn, or a negative error code.
 *
 * This function draws a line on the destination surface using the
 * specified line width and antialiasing settings. It calculates the
 * necessary parameters for rendering the line and checks for
 * clipping if the line extends beyond the destination rectangle.
 * If the line width or antialiasing is enabled, it computes the
 * line's length and adjusts the line parameters accordingly.
 * The function uses the current clipping area to ensure the line
 * is drawn within bounds, enabling clipping if necessary.
 */

int ite_it2d_line (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->dst_surf);

    if (!ite_it2d_calc_dst_rect(ctx))
    {
        return 0;
    }

    int32_t dx = ctx->op.line.line_area.x1 - ctx->op.line.line_area.x2;
    int32_t dy = ctx->op.line.line_area.y1 - ctx->op.line.line_area.y2;
    float   d  = (dx + dy) == 0 ? 1.0f : sqrtf((float)(dx * dx + dy * dy));
    ctx->op.line.line_p2 =
        (uint16_t)(d * (ctx->op.line.line_width + 1) / 2);
    if (ctx->op.line.line_p2 >= 8192)
    {
        return -ENOTSUP;
    }
    ctx->op.line.line_p1 = (uint32_t)((1 << 20) / d);

    int16_t dst_x2 = ctx->dst_rect.x + ctx->dst_rect.w - 1;
    int16_t dst_y2 = ctx->dst_rect.y + ctx->dst_rect.h - 1;

    if ((ctx->op.line.line_area.x1 < ctx->dst_rect.x) ||
        (ctx->op.line.line_area.y1 < ctx->dst_rect.y) ||
        (ctx->op.line.line_area.x2 > dst_x2) ||
        (ctx->op.line.line_area.y2 > dst_y2))
    {
        if (ctx->flags.clipping)
        {
            ctx->clip_area.x1 = MAX(MAX(ctx->clip_area.x1, ctx->dst_rect.x), 0);
            ctx->clip_area.y1 = MAX(MAX(ctx->clip_area.y1, ctx->dst_rect.y), 0);
            ctx->clip_area.x2 =
                MIN(MIN(ctx->clip_area.x2, dst_x2), ctx->dst_surf->width - 1);
            ctx->clip_area.y2 =
                MIN(MIN(ctx->clip_area.y2, dst_y2), ctx->dst_surf->height - 1);
        }
        else
        {
            ctx->clip_area.x1   = MAX(ctx->dst_rect.x, 0);
            ctx->clip_area.y1   = MAX(ctx->dst_rect.y, 0);
            ctx->clip_area.x2   = MIN(dst_x2, ctx->dst_surf->width - 1);
            ctx->clip_area.y2   = MIN(dst_y2, ctx->dst_surf->height - 1);
            ctx->flags.clipping = true;
        }
    }

    ctx->flags.cmd = ITE_IT2D_CMD_LINE;

    return it2d_line(ctx);
}

/**
 * @brief Execute a graphics command
 *
 * @param[in] ctx Pointer to the IT2D context
 *
 * @return 0 on success, negative error code on failure
 *
 * This function executes the graphics command stored in the context. It
 * checks the context's cmd field and calls the appropriate function.
 *
 * @sa ite_it2d_bitblt, ite_it2d_gradfill, ite_it2d_line
 */
int ite_it2d_exec (ite_it2d_context_t * ctx)
{
    assert(ctx);
    assert(ctx->dst_surf);

    switch (ctx->flags.cmd)
    {
        case ITE_IT2D_CMD_BITBLT:
            return it2d_bitblt(ctx);

        case ITE_IT2D_CMD_GRADFILL:
            return it2d_gradfill(ctx);

        case ITE_IT2D_CMD_LINE:
            return it2d_line(ctx);
    }

    return -ENOTSUP;
}

/**
 * @brief Presents the display surface by flipping the buffers.
 *
 * @param disp_surf Pointer to the display surface to present.
 *
 * @return Returns 0 on success.
 *
 * This function asserts that the display surface is valid and has a static
 * buffer. If double buffering is enabled, it flips the command queue buffer
 * index and updates the buffer pointer to the respective LCD base address.
 */

int ite_it2d_present (ite_it2d_surface_t * disp_surf)
{
    assert(disp_surf);
    assert(disp_surf->flags.static_buf);
#ifdef IT2D_DOUBLE_BUFFER_ENABLE
    ithCmdQFlip(it2d_data.lcd_index, ITH_CMDQ0_OFFSET);
    if (it2d_data.lcd_index == 1)
    {
        it2d_data.lcd_index = 0;
        disp_surf->buf      = (void *)ithLcdGetBaseAddrA();
    }
    else
    {
        it2d_data.lcd_index = 1;
        disp_surf->buf      = (void *)ithLcdGetBaseAddrB();
    }
#endif
    return 0;
}

/**
 * @brief Waits until the graphics engine is idle.
 *
 * @return Returns 0 on success.
 *
 * This function waits until the command queue is empty and the graphics engine
 * is idle. It is used to ensure that all graphics operations have completed
 * before performing operations that require the graphics engine to be idle,
 * such as reinitializing the graphics engine or rebooting the system.
 */
int ite_it2d_wait_idle (void)
{
    int ret = ithCmdQWaitEmpty(ITH_CMDQ0_OFFSET);
    if (ret < 0)
    {
        return ret;
    }

    while (sys_read32(it2d_config.base + REG_2D_ST1) & BIT(0))
    {
        sched_yield();
    }

    return 0;
}

/**
 * @brief Gets the current ID from the graphics engine
 *
 * @return Returns the current ID from the graphics engine
 *
 * This function returns the current ID from the graphics engine. The ID is
 * used to synchronize graphics operations. The ID is automatically
 * incremented by the graphics engine after each graphics command.
 */
int32_t ite_it2d_get_current_id (void)
{
    return (int32_t)sys_read32(it2d_config.base + REG_2D_ID1);
}

/**
 * @brief Waits for the specified graphics engine ID to finish processing.
 *
 * @param id The ID to wait for completion.
 *
 * @return Returns 0 on success.
 *
 * This function waits for the graphics engine to reach or exceed the specified
 * ID. If the current ID is negative and the specified ID is also negative, it
 * waits until the graphics engine is idle. If the current ID is less than the
 * specified ID, it continuously checks for the ID to match or exceed the specified
 * value, yielding the processor to other threads between checks.
 */

int ite_it2d_wait_id_finish (int32_t id)
{
    int32_t curr_id = ite_it2d_get_current_id();
	int ret = 0;

    if ((curr_id >= 0) && (id < 0))
    {
        ret = ite_it2d_wait_idle();
    }
    else if (curr_id < id)
    {
		uint32_t timeout = WAIT_BUSY_TIMEOUT;
		uint32_t retry_timeout = RETRY_TIMEOUT;

        /* id1 match register */
        for (;;)
        {
            curr_id = ite_it2d_get_current_id();
            if (curr_id >= id)
            {
                break;
            }
			if (timeout == 0UL) {
				uint32_t status;
				if (retry_timeout == 0UL) {
					ret = -EBUSY;
					break;
				}
				retry_timeout--;
                itclock_enable_2d_clock(REG_HOST_BASE);
				it2d_reset();
				ithDelay(1);
				status = sys_read32(REG_CMDQ_BASE + REG_CMDQ_SR1);
				if (status & (0x1 << ITH_CMDQ_OVGBUSY_BIT)) {
					ithCmdQReset(ITH_CMDQ0_OFFSET);
				}
				timeout = WAIT_BUSY_TIMEOUT;
				continue;
			}
			timeout--;
            sched_yield();
        }
    }

    return ret;
}

/**
 * @brief Resets the 2D graphics engine by writing the last ID to the ID1 register.
 *
 * This function writes the `last_id` from the `it2d_data` structure to the
 * `REG_2D_ID1` register at the base address specified in the `it2d_config`
 * structure. It is typically used to synchronize the graphics engine state
 * with the application state.
 */

static void it2d_reset (void)
{
    const struct it2d_config * cfg  = &it2d_config;
    struct it2d_data *         data = &it2d_data;

    sys_write32((uint32_t)data->last_id, cfg->base + REG_2D_ID1);
}

/**
 * @brief Enable the 2D graphics engine clock.
 *
 * This function is used to enable the clock for the 2D graphics engine. It
 * reads the current value of the clock enable register, sets the enable bit,
 * writes the register back, waits for a short time, and then clears the enable
 * bit to ensure the clock is enabled.
 *
 * @param[in] base The base address of the clock control register.
 */
static void itclock_enable_2d_clock (uint32_t base)
{
    uint32_t val = sys_read32(base + REG_CQ_CLK);
    sys_write32(0x42af8000 | val, base + REG_CQ_CLK);
    ithDelay(1);
    sys_write32(0x02af8000 | val, base + REG_CQ_CLK);
}

/**
 * @brief Initialize the 2D graphics engine.
 *
 * This function initializes the 2D graphics engine. It enables the hardware
 * flip, enables the clock for the 2D graphics engine, and resets the graphics
 * engine state.
 *
 * @return 0 on success, non-zero on failure.
 */
int it2d_init (void)
{
    const struct it2d_config * cfg  = &it2d_config;
    struct it2d_data *         data = &it2d_data;
    int                        ret  = 0;

    ithLcdEnableHwFlip();
    itclock_enable_2d_clock(REG_HOST_BASE);

    data->last_id   = 1;
    data->lcd_index = 1;

    it2d_reset();

    return 0;
}
