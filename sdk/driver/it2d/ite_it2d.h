/*
 * Copyright (c) 2022 ITE Tech. Inc.
 *
 */

#ifndef ITE_IT2D_H_
#define ITE_IT2D_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    ITE_IT2D_PIXEL_FORMAT_RGBA_8888 = 0x00,
    ITE_IT2D_PIXEL_FORMAT_ARGB_8888 = 0x01,
    ITE_IT2D_PIXEL_FORMAT_BGRA_8888 = 0x02,
    ITE_IT2D_PIXEL_FORMAT_ABGR_8888 = 0x03,
    ITE_IT2D_PIXEL_FORMAT_RGBA_5551 = 0x04,
    ITE_IT2D_PIXEL_FORMAT_ARGB_1555 = 0x05,
    ITE_IT2D_PIXEL_FORMAT_BGRA_5551 = 0x06,
    ITE_IT2D_PIXEL_FORMAT_ABGR_1555 = 0x07,
    ITE_IT2D_PIXEL_FORMAT_RGBA_4444 = 0x08,
    ITE_IT2D_PIXEL_FORMAT_ARGB_4444 = 0x09,
    ITE_IT2D_PIXEL_FORMAT_BGRA_4444 = 0x0A,
    ITE_IT2D_PIXEL_FORMAT_ABGR_4444 = 0x0B,
    ITE_IT2D_PIXEL_FORMAT_RGB_565   = 0x0C,
    ITE_IT2D_PIXEL_FORMAT_BGR_565   = 0x0D,
    ITE_IT2D_PIXEL_FORMAT_A_8       = 0x0E,
    ITE_IT2D_PIXEL_FORMAT_A_4       = 0x0F,
    ITE_IT2D_PIXEL_FORMAT_A_2       = 0x10,
    ITE_IT2D_PIXEL_FORMAT_A_1       = 0x11,
    ITE_IT2D_PIXEL_FORMAT_XARGB     = 0x12,
    ITE_IT2D_PIXEL_FORMAT_XABGR     = 0x13,
} ite_it2d_pixel_format_t;

/**
 * @brief 2D point data structure.
 *
 * @details This structure stores an (x, y) coordinate pair.
 */
typedef struct
{
    int16_t x; /**< @brief X-coordinate */
    int16_t y; /**< @brief Y-coordinate */
} ite_it2d_point_t;

/**
 * @brief 2D bounding box data structure.
 *
 * @details This structure stores a bounding box defined by two points.
 *          The first point (x1, y1) is the top-left corner of the box,
 *          and the second point (x2, y2) is the bottom-right corner of the box.
 */
typedef struct
{
    int16_t x1; /**< @brief X-coordinate of top-left corner */
    int16_t y1; /**< @brief Y-coordinate of top-left corner */
    int16_t x2; /**< @brief X-coordinate of bottom-right corner */
    int16_t y2; /**< @brief Y-coordinate of bottom-right corner */
} ite_it2d_area_t;

/**
 * @brief 2D rectangle data structure.
 *
 * @details This structure stores a rectangle defined by its top-left corner (x, y),
 *          and its width and height.
 */
typedef struct
{
    int16_t x; /**< @brief X-coordinate of top-left corner */
    int16_t y; /**< @brief Y-coordinate of top-left corner */
    int16_t w; /**< @brief Width of the rectangle */
    int16_t h; /**< @brief Height of the rectangle */
} ite_it2d_rect_t;

/**
 * @brief Color data structure.
 *
 * @details This structure stores a color as an RGB value, with an additional
 *          alpha channel.
 */
typedef union {
    /**
     * @brief Color components.
     *
     * @details Each component is represented as an 8-bit unsigned integer.
     */
    struct
    {
        uint8_t blue;  /**< @brief Blue component */
        uint8_t green; /**< @brief Green component */
        uint8_t red;   /**< @brief Red component */
        uint8_t alpha; /**< @brief Alpha component */
    } ch;
    uint32_t full; /**< @brief The entire color as a 32-bit unsigned integer */
} ite_it2d_color_t;

typedef float ite_it2d_matrix_t[3][3];

typedef enum
{
    ITE_IT2D_ROP3_BLACKNESS   = 0x00,
    ITE_IT2D_ROP3_NOTSRCERASE = 0x11,
    ITE_IT2D_ROP3_NOTSRCCOPY  = 0x33,
    ITE_IT2D_ROP3_SRCERASE    = 0x44,
    ITE_IT2D_ROP3_DSTINVERT   = 0x55,
    ITE_IT2D_ROP3_SRCINVERT   = 0x66,
    ITE_IT2D_ROP3_SRCAND      = 0x88,
    ITE_IT2D_ROP3_PAINTINVERT = 0x5A,
    ITE_IT2D_ROP3_MERGEPAINT  = 0xBB,
    ITE_IT2D_ROP3_MERGECOPY   = 0xC0,
    ITE_IT2D_ROP3_SRCCOPY     = 0xCC,
    ITE_IT2D_ROP3_SRCPAINT    = 0xEE,
    ITE_IT2D_ROP3_PATCOPY     = 0xF0,
    ITE_IT2D_ROP3_PATPAINT    = 0xFB,
    ITE_IT2D_ROP3_WHITENESS   = 0xFF
} ite_it2d_rop3_t;

/**
 * @brief Enumerates the various tile modes that can be used for filling a
 *        rectangle with a pattern.
 *
 * @details When drawing a rectangle with a pattern, the library will need to
 *          fill the entire target area with the pattern. This is done by
 *          "tiling" the pattern, i.e., repeating it in the horizontal and/or
 *          vertical direction. The mode determines how the pattern is tiled.
 */
typedef enum
{
    /**
     * @brief Fill the target area with the pattern.
     *
     * @details The pattern is repeated in both the horizontal and vertical
     *          direction as many times as is necessary to fill the target
     *          area.
     */
    ITE_IT2D_TILE_FILL    = 0x0,

    /**
     * @brief Pad the target area with a solid color.
     *
     * @details The pattern is used to fill the target area as far as possible
     *          with the pattern. Any remaining space is filled with the
     *          background color.
     */
    ITE_IT2D_TILE_PAD     = 0x1,

    /**
     * @brief Repeat the pattern.
     *
     * @details The pattern is repeated in both the horizontal and vertical
     *          direction as many times as is necessary to fill the target
     *          area. When the pattern reaches the edge of the target area, it
     *          wraps around to the opposite edge.
     */
    ITE_IT2D_TILE_REPEAT  = 0x2,

    /**
     * @brief Reflect the pattern.
     *
     * @details The pattern is repeated in both the horizontal and vertical
     *          direction as many times as is necessary to fill the target
     *          area. When the pattern reaches the edge of the target area, it
     *          is reflected back into the target area, as if the edge were a
     *          mirror.
     */
    ITE_IT2D_TILE_REFLECT = 0x3
} ite_it2d_tile_mode_t;

/**
 * @brief Enumerates the direction of the gradient used for filling a rectangle.
 *
 * When filling a rectangle with a gradient, the gradient can be drawn in
 * one of three directions. This enumeration specifies which direction is
 * to be used.
 */
typedef enum
{
    /**
     * @brief The gradient is drawn horizontally.
     *
     * When this direction is used, the gradient is drawn horizontally,
     * from left to right. The color of the gradient changes linearly from
     * the start color to the end color as the position moves from left to
     * right.
     */
    ITE_IT2D_GRAD_DIR_HOR  = 0x0,

    /**
     * @brief The gradient is drawn vertically.
     *
     * When this direction is used, the gradient is drawn vertically, from
     * top to bottom. The color of the gradient changes linearly from the
     * start color to the end color as the position moves from top to bottom.
     */
    ITE_IT2D_GRAD_DIR_VER  = 0x1,

    /**
     * @brief The gradient is drawn diagonally.
     *
     * When this direction is used, the gradient is drawn diagonally, from
     * top-left to bottom-right. The color of the gradient changes linearly
     * from the start color to the end color as the position moves from
     * top-left to bottom-right.
     */
    ITE_IT2D_GRAD_DIR_DIAG = 0x2
} ite_it2d_grad_dir_t;

/**
 * @brief Enumerates the way the destination alpha is calculated.
 *
 * @details When blitting a source surface onto a destination surface,
 *          the destination alpha needs to be calculated. This enumeration
 *          specifies how the destination alpha should be calculated.
 */
typedef enum
{
    /**
     * @brief Use the default behaviour.
     *
     * @details When this mode is used, the destination alpha is not
     *          changed. This is the default behaviour.
     */
    ITE_IT2D_ALPHA_DEFAULT = 0x0,

    /**
     * @brief Use the source alpha.
     *
     * @details When this mode is used, the destination alpha is set to
     *          the source alpha. This means that the destination surface
     *          will have the same alpha values as the source surface.
     */
    ITE_IT2D_ALPHA_SRC     = 0x1,

    /**
     * @brief Use the destination alpha.
     *
     * @details When this mode is used, the destination alpha is not
     *          changed. This means that the destination surface will have
     *          the same alpha values as before the blit operation.
     */
    ITE_IT2D_ALPHA_DST     = 0x2,

    /**
     * @brief Use the maximum of the destination and source alphas.
     *
     * @details When this mode is used, the destination alpha is set to
     *          the maximum of the destination alpha and the source alpha.
     *          This means that the destination surface will have the
     *          maximum alpha value of the two surfaces.
     */
    ITE_IT2D_ALPHA_MAX_DST_SRC = 0x3
} ite_it2d_alpha_dst_t;

/**
 * @brief Structure for describing a 2D surface.
 *
 * This structure is used to describe a 2D surface, which is a region of
 * memory that contains a 2D image.
 *
 * @note The structure is defined such that it can be used by the IT2D
 *       driver to describe a surface.
 */
typedef struct
{
    /**
     * @brief The width of the surface in pixels.
     *
     * This field specifies the width of the surface in pixels. It is
     * used to determine the size of the surface.
     */
    int16_t  width;

    /**
     * @brief The height of the surface in pixels.
     *
     * This field specifies the height of the surface in pixels. It is
     * used to determine the size of the surface.
     */
    int16_t  height;

    /**
     * @brief The pitch of the surface in bytes.
     *
     * This field specifies the pitch of the surface in bytes. It is
     * used to determine the size of the surface.
     */
    uint16_t pitch;

    /**
     * @brief Bit field of flags for the surface.
     *
     * This field is a bit field of flags that describe the surface.
     * The flags are as follows:
     *
     * * format: The format of the surface. Valid values are:
     *   - IT2D_SURFACE_FORMAT_RGB565
     *   - IT2D_SURFACE_FORMAT_RGB888
     *   - IT2D_SURFACE_FORMAT_YUV422
     * * static_buf: Whether the surface is a static buffer or not.
     * * buf_offset: The offset of the surface buffer in bytes.
     * * custom: Custom flags for the surface.
     */
    struct
    {
        /**
         * @brief The format of the surface.
         *
         * This field specifies the format of the surface. Valid values
         * are:
         * - IT2D_SURFACE_FORMAT_RGB565
         * - IT2D_SURFACE_FORMAT_RGB888
         * - IT2D_SURFACE_FORMAT_YUV422
         */
        uint16_t format : 5;

        /**
         * @brief Whether the surface is a static buffer or not.
         *
         * This field specifies whether the surface is a static buffer
         * or not. If it is a static buffer, the surface will not be
         * allocated or deallocated by the driver. Instead, the caller
         * is responsible for allocating and deallocating the surface.
         */
        uint16_t static_buf : 1;

        /**
         * @brief The offset of the surface buffer in bytes.
         *
         * This field specifies the offset of the surface buffer in
         * bytes. It is used to determine the location of the surface
         * buffer.
         */
        uint16_t buf_offset : 5;

        /**
         * @brief Custom flags for the surface.
         *
         * This field specifies custom flags for the surface. These
         * flags are used to specify custom behavior for the surface.
         */
        uint16_t custom : 5;
    } flags;

    /**
     * @brief The buffer for the surface.
     *
     * This field specifies the buffer for the surface. It is used to
     * store the surface data.
     */
    uint8_t * buf;

    /**
     * @brief User data for the surface.
     *
     * This field specifies user data for the surface. It is used to
     * store custom data for the surface.
     */
    void *    user_data;

    /**
     * @brief The ID of the surface.
     *
     * This field specifies the ID of the surface. It is used to
     * identify the surface.
     */
    int32_t   id;
} ite_it2d_surface_t;

/**
 * @brief Enumerated type for the different graphics commands.
 *
 * This enumeration is used to describe the type of graphics command that
 * is being executed. It is used to determine which set of commands should
 * be used to execute the command.
 *
 * @note The values of the enumeration are chosen such that they are
 *       consistent with the values of the IT2D_CMD register.
 */
typedef enum
{
    /**
     * @brief No graphics command.
     *
     * This value is used to indicate that no graphics command should be
     * executed.
     */
    ITE_IT2D_CMD_NULL = 0x0,

    /**
     * @brief Bitblt graphics command.
     *
     * This value is used to indicate that a bitblt graphics command should
     * be executed.
     */
    ITE_IT2D_CMD_BITBLT = 0x1,

    /**
     * @brief Gradient fill graphics command.
     *
     * This value is used to indicate that a gradient fill graphics command
     * should be executed.
     */
    ITE_IT2D_CMD_GRADFILL = 0x2,

    /**
     * @brief Line graphics command.
     *
     * This value is used to indicate that a line graphics command should
     * be executed.
     */
    ITE_IT2D_CMD_LINE = 0x3
} ite_it2d_cmd_t;

/**
 * @brief The transformation descriptor structure.
 *
 * This structure is used to describe the transformation needed to perform a
 * graphics command, such as a bitblt, gradient fill, or line drawing.
 *
 * @note The structure is used to describe an affine transformation of the
 *       source surface, which is the surface that will be used to draw the
 *       destination surface.
 */
typedef struct
{
    /**
     * @brief Position of the top-left corner of the source surface.
     *
     * This is the position of the top-left corner of the source surface in the
     * coordinate space of the destination surface.
     */
    ite_it2d_point_t  pos;

    /**
     * @brief Center of rotation if rotation is used.
     *
     * This is the center of rotation if the angle is not zero. If the angle is
     * zero, this is ignored.
     */
    ite_it2d_point_t  center;

    /**
     * @brief Scale factor in the X direction.
     *
     * This is the scale factor to apply in the X direction. A value of 1.0 means
     * no scaling. A value of 2.0 means double the size of the source surface.
     */
    float             scale_x;

    /**
     * @brief Scale factor in the Y direction.
     *
     * This is the scale factor to apply in the Y direction. A value of 1.0 means
     * no scaling. A value of 2.0 means double the size of the source surface.
     */
    float             scale_y;

    /**
     * @brief Angle of rotation.
     *
     * This is the angle of rotation to apply to the source surface. A value of
     * 0.0 means no rotation.
     */
    float             angle;

    /**
     * @brief Flags.
     *
     * The flags are used to describe how the transformation should be applied.
     */
    struct
    {
        /**
         * @brief Tile mode.
         *
         * This is the tile mode to use. The tile mode is used to describe how
         * the source surface should be repeated to fill the destination surface.
         */
        uint8_t        tile_mode : 2;

        /**
         * @brief Auto destination rectangle.
         *
         * If this flag is set, the destination rectangle will be automatically
         * computed based on the source surface and the transformation.
         */
        uint8_t        auto_dst_rect : 1;

        uint8_t        inverse : 1;
    } flags;
} ite_it2d_transform_dsc_t;

/**
 * @brief The context structure used to describe a command.
 *
 * This structure contains all the information needed to perform a graphics
 * command, such as a bitblt, gradient fill, or line drawing.
 */
typedef struct
{
    /**
     * @brief Destination rectangle.
     *
     * This is the rectangle of the destination surface that will be affected
     * by the command.
     */
    ite_it2d_rect_t      dst_rect;

    /**
     * @brief Clipping area.
     *
     * This is the area of the destination surface that should be clipped
     * before performing the command.
     */
    ite_it2d_area_t      clip_area;

    /**
     * @brief Foreground color.
     *
     * This is the color that will be used for the command.
     */
    ite_it2d_color_t     fg_color;

    /**
     * @brief Mask position.
     *
     * This is the position of the mask surface relative to the destination
     * surface.
     */
    ite_it2d_point_t     mask_pos;

    /**
     * @brief Destination surface.
     *
     * This is the surface that will be affected by the command.
     */
    ite_it2d_surface_t * dst_surf;

    /**
     * @brief Mask surface.
     *
     * This is the surface that will be used as a mask during the command.
     */
    ite_it2d_surface_t * mask_surf;

    /**
     * @brief Command union.
     *
     * This union contains the command-specific data.
     */
    union {
        /**
         * @brief Bitblt command.
         *
         * This structure contains the data needed to perform a bitblt.
         */
        struct
        {
            /**
             * @brief Source surface.
             *
             * This is the surface that will be used as the source for the
             * bitblt.
             */
            ite_it2d_surface_t * src_surf;

            /**
             * @brief Transformation matrix.
             *
             * This is the matrix that will be used to transform the source
             * surface.
             */
            ite_it2d_matrix_t    transform_mat;

            /**
             * @brief Source rectangle.
             *
             * This is the rectangle of the source surface that should be used.
             */
            ite_it2d_rect_t      src_rect;

            /**
             * @brief ROP3 value.
             *
             * This is the ROP3 value that should be used for the bitblt.
             */
            uint8_t              rop3;

            /**
             * @brief Flags.
             *
             * This is a bitfield that contains flags that affect the behavior
             * of the command.
             */
            struct
            {
                /**
                 * @brief Tile mode.
                 *
                 * This is the mode that should be used to tile the source
                 * surface.
                 */
                uint16_t tile_mode : 2;

                /**
                 * @brief Transforming.
                 *
                 * This flag indicates whether the transformation matrix should
                 * be used.
                 */
                uint16_t transforming : 1;

                /**
                 * @brief Foreground color.
                 *
                 * This flag indicates whether the foreground color should be
                 * used.
                 */
                uint16_t fg_color : 1;

                /**
                 * @brief Interpolation.
                 *
                 * This flag indicates whether interpolation should be used.
                 */
                uint16_t interpolation : 1;

                /**
                 * @brief Color key.
                 *
                 * This flag indicates whether color keying should be used.
                 */
                uint16_t color_key : 1;

                /**
                 * @brief Scan column major.
                 *
                 * This flag indicates whether the scan should be done in column
                 * major order.
                 */
                uint16_t scan_column_major : 1;

                /**
                 * @brief Force transform.
                 *
                 * This flag indicates whether the transformation matrix should
                 * be forced to be used, even if it is the identity matrix.
                 */
                uint16_t force_transform : 1;

                uint16_t rop3: 1;
            } flags;
        } bitblt;

        /**
         * @brief Gradient fill command.
         *
         * This structure contains the data needed to perform a gradient fill.
         */
        struct
        {
            /**
             * @brief End color.
             *
             * This is the color that should be used at the end of the gradient.
             */
            ite_it2d_color_t end_color;

            /**
             * @brief Direction.
             *
             * This is the direction of the gradient.
             */
            uint8_t          direction;
        } gradfill;

        /**
         * @brief Line drawing command.
         *
         * This structure contains the data needed to draw a line.
         */
        struct
        {
            /**
             * @brief Line area.
             *
             * This is the area of the line that should be drawn.
             */
            ite_it2d_area_t line_area;

            /**
             * @brief Line positions.
             *
             * These are the positions of the two endpoints of the line.
             */
            uint32_t        line_p1;
            uint16_t        line_p2;

            /**
             * @brief Line width.
             *
             * This is the width of the line that should be drawn.
             */
            uint8_t         line_width;

            /**
             * @brief Flags.
             *
             * This is a bitfield that contains flags that affect the behavior
             * of the line drawing.
             */
            struct
            {
                /**
                 * @brief Antialiasing.
                 *
                 * This flag indicates whether antialiasing should be used.
                 */
                uint8_t antialiasing : 1;

                /**
                 * @brief Last pixel.
                 *
                 * This flag indicates whether the last pixel of the line
                 * should be drawn.
                 */
                uint8_t last_pixel : 1;

                /**
                 * @brief Alter algorithm.
                 *
                 * This flag indicates whether an alternate algorithm should be
                 * used to draw the line.
                 */
                uint8_t alter_algorithm : 1;
            } flags;
        } line;

    } op;

    /**
     * @brief Flags.
     *
     * This is a bitfield that contains flags that affect the behavior of the
     * command.
     */
    struct
    {
        /**
         * @brief Command type.
         *
         * This is the type of the command.
         */
        uint16_t cmd : 2;

        /**
         * @brief Alpha destination.
         *
         * This flag indicates whether the alpha channel should be used as the
         * destination.
         */
        uint16_t alpha_dst : 2;

        /**
         * @brief Clipping.
         *
         * This flag indicates whether clipping should be used.
         */
        uint16_t clipping : 1;

        /**
         * @brief Masking.
         *
         * This flag indicates whether masking should be used.
         */
        uint16_t masking : 1;

        /**
         * @brief Blending.
         *
         * This flag indicates whether blending should be used.
         */
        uint16_t blending : 1;

        /**
         * @brief Constant alpha.
         *
         * This flag indicates whether a constant alpha value should be used.
         */
        uint16_t const_alpha : 1;

        /**
         * @brief Dithering.
         *
         * This flag indicates whether dithering should be used.
         */
        uint16_t dithering : 1;
    } flags;

    /**
     * @brief Constant alpha value.
     *
     * This is the constant alpha value that should be used if the
     * const_alpha flag is set.
     */
    uint8_t const_alpha;

} ite_it2d_context_t;

/**
 * @brief Compares two rectangles.
 *
 * @param a First rectangle.
 * @param b Second rectangle.
 *
 * @return true if rectangles are equal, false otherwise.
 *
 * @details Two rectangles are considered equal if their positions and sizes
 *          are equal.
 */
static inline bool ite_it2d_rect_is_equal (const ite_it2d_rect_t * a,
                                           const ite_it2d_rect_t * b)
{
    return (a && b && (a->x == b->x) && (a->y == b->y) && (a->w == b->w) &&
            (a->h == b->h))
               ? true
               : false;
}

bool ite_it2d_rect_has_intersect (const ite_it2d_rect_t * a,
                                  const ite_it2d_rect_t * b);

/**
 * Create an ite_it2d_color_t object from separate color components.
 *
 * @param r The red component of the color (0-255).
 * @param g The green component of the color (0-255).
 * @param b The blue component of the color (0-255).
 * @param a The alpha component of the color (0-255).
 *
 * @return The ite_it2d_color_t object representing the color.
 */
static inline ite_it2d_color_t ite_it2d_color_make (uint8_t r, uint8_t g,
                                                    uint8_t b, uint8_t a)
{
    ite_it2d_color_t c = {
        {b, g, r, a}
    };
    return c;
}

int    ite_it2d_bits_per_pixel (ite_it2d_pixel_format_t format);
void   ite_it2d_init_transform_dsc (ite_it2d_transform_dsc_t * dsc);
void   ite_it2d_mat_identity (ite_it2d_matrix_t mat);
void   ite_it2d_mat_translate (const ite_it2d_matrix_t src_mat,
                               ite_it2d_matrix_t dst_mat, float x, float y);
void   ite_it2d_mat_scale (const ite_it2d_matrix_t src_mat,
                           ite_it2d_matrix_t dst_mat, float x, float y);
void   ite_it2d_mat_rotate (const ite_it2d_matrix_t src_mat,
                            ite_it2d_matrix_t dst_mat, float angle);
void   ite_it2d_mat_transform (const ite_it2d_matrix_t  mat,
                               const ite_it2d_point_t * src_pt,
                               ite_it2d_point_t *       dst_pt);
int    ite_it2d_mat_inverse (const ite_it2d_matrix_t src_mat,
                             ite_it2d_matrix_t       dst_mat);

int    ite_it2d_create_display_surface (ite_it2d_surface_t * disp_surf);
int    ite_it2d_create_surface (ite_it2d_surface_t * surf);
int    ite_it2d_destroy_surface (ite_it2d_surface_t * surf);
int    ite_it2d_wait_surface_finish (ite_it2d_surface_t * surf);

void * ite_it2d_alloc_buffer (uint32_t size);
void   ite_it2d_free_buffer (void * buf);

void   ite_it2d_ctx_init (ite_it2d_context_t * ctx);

void   ite_it2d_set_dst_surface (ite_it2d_context_t * ctx,
                                 ite_it2d_surface_t * surf);
void   ite_it2d_set_src_surface (ite_it2d_context_t * ctx,
                                 ite_it2d_surface_t * surf);

/**
 * @brief Set the mask surface for the 2D engine.
 *
 * @param ctx  2D context
 * @param surf Mask surface
 *
 * @note This function should be called before executing any operations that
 * utilize the mask surface.
 */
static inline void ite_it2d_set_mask_surface (ite_it2d_context_t * ctx,
                                              ite_it2d_surface_t * surf)
{
    ctx->mask_surf = surf;
}

void ite_it2d_set_dst_rect (ite_it2d_context_t * ctx, ite_it2d_rect_t * rect);

/**
 * @brief Set the destination rectangle's position for the 2D engine.
 *
 * @param ctx 2D context
 * @param x   Destination rectangle's x-coordinate
 * @param y   Destination rectangle's y-coordinate
 *
 * @note This function should be called before executing any operations that
 * utilize the destination rectangle's position.
 */

static inline void ite_it2d_set_dst_pos (ite_it2d_context_t * ctx, int16_t x,
                                         int16_t y)
{
    ctx->dst_rect.x = x;
    ctx->dst_rect.y = y;
}

/**
 * @brief Set the destination rectangle's size for the 2D engine.
 *
 * @param ctx 2D context
 * @param w   Destination rectangle's width
 * @param h   Destination rectangle's height
 *
 * @note This function should be called before executing any operations that
 * utilize the destination rectangle's size.
 */

static inline void ite_it2d_set_dst_size (ite_it2d_context_t * ctx, int16_t w,
                                          int16_t h)
{
    ctx->dst_rect.w = w;
    ctx->dst_rect.h = h;
}

/**
 * @brief Set the position of the mask for the 2D engine.
 *
 * @param ctx 2D context
 * @param x   Mask x-coordinate
 * @param y   Mask y-coordinate
 *
 * @note This function should be called before executing any operations that
 * utilize the mask position.
 */

static inline void ite_it2d_set_mask_pos (ite_it2d_context_t * ctx, int16_t x,
                                          int16_t y)
{
    ctx->mask_pos.x = x;
    ctx->mask_pos.y = y;
}

void ite_it2d_set_clip_area (ite_it2d_context_t * ctx, ite_it2d_area_t * area);

/**
 * @brief Set the position of the clip area for the 2D engine.
 *
 * @param ctx 2D context
 * @param x   Clip area x-coordinate
 * @param y   Clip area y-coordinate
 *
 * @note The clip area's size is implicitly set to the current size of the clip
 * area. The clip area's end position is set to the specified x and y plus the
 * current size of the clip area.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_set_clip_pos (ite_it2d_context_t * ctx, int16_t x,
                                          int16_t y)
{
    ctx->clip_area.x1 = x;
    ctx->clip_area.y1 = y;
}

/**
 * @brief Set the size of the clip area for the 2D engine.
 *
 * @param ctx 2D context
 * @param w   Clip area width
 * @param h   Clip area height
 *
 * @note The clip area's position is implicitly set to the current position of
 * the clip area. The clip area's end position is set to the current position
 * plus the specified width and height.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_set_clip_size (ite_it2d_context_t * ctx, int16_t w,
                                           int16_t h)
{
    ctx->clip_area.x2 = ctx->clip_area.x1 + w - 1;
    ctx->clip_area.y2 = ctx->clip_area.y1 + h - 1;
}

/**
 * @brief Set the foreground color for the 2D engine.
 *
 * @param ctx 2D context
 * @param color Foreground color.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_set_fg_color (ite_it2d_context_t * ctx,
                                          ite_it2d_color_t     color)
{
    ctx->fg_color = color;
}

/**
 * @brief Set the alpha blending destination for the 2D engine.
 *
 * @param ctx 2D context
 * @param dst Alpha blending destination.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_set_alpha_dst (ite_it2d_context_t * ctx,
                                           ite_it2d_alpha_dst_t dst)
{
    ctx->flags.alpha_dst = (uint8_t)dst;
}

/**
 * @brief Set the constant alpha for the 2D engine.
 *
 * @param ctx 2D context
 * @param alpha Constant alpha value.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_set_const_alpha (ite_it2d_context_t * ctx,
                                             uint8_t              alpha)
{
    ctx->const_alpha = alpha;
}

void ite_it2d_set_src_rect (ite_it2d_context_t * ctx, ite_it2d_rect_t * rect);

/**
 * @brief Set the source rectangle's position for the 2D engine.
 *
 * @param ctx 2D context
 * @param x   Source rectangle's x-coordinate
 * @param y   Source rectangle's y-coordinate
 */
static inline void ite_it2d_set_src_pos (ite_it2d_context_t * ctx, int16_t x,
                                         int16_t y)
{
    ctx->op.bitblt.src_rect.x = x;
    ctx->op.bitblt.src_rect.y = y;
}

/**
 * @brief Set the source rectangle's size for the 2D engine.
 *
 * @param ctx 2D context
 * @param w   Source rectangle's width
 * @param h   Source rectangle's height
 */
static inline void ite_it2d_set_src_size (ite_it2d_context_t * ctx, int16_t w,
                                          int16_t h)
{
    ctx->op.bitblt.src_rect.w = w;
    ctx->op.bitblt.src_rect.h = h;
}

/**
 * @brief Set the ROP3 (blitting operation) for the 2D engine.
 *
 * @param ctx   The 2D context.
 * @param rop3  The ROP3 to set.
 *
 * The ROP3 is used to determine how the source pixels are combined with the
 * destination pixels.
 */
static inline void ite_it2d_set_rop3 (ite_it2d_context_t * ctx,
                                      ite_it2d_rop3_t      rop3)
{
    ctx->op.bitblt.rop3 = (uint8_t)rop3;
}

void ite_it2d_set_transform (ite_it2d_context_t * ctx, ite_it2d_matrix_t * mat);

/**
 * @brief Set the end color for the upcoming 2D gradient fill operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] color End color of the gradient fill.
 *
 * @note This function should be called before submitting a 2D gradient fill
 *       operation.
 */
static inline void ite_it2d_set_gradient_end_color (ite_it2d_context_t * ctx,
                                                    ite_it2d_color_t     color)
{
    ctx->op.gradfill.end_color = color;
}

/**
 * @brief Set the gradient fill direction for the upcoming 2D gradient operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] dir Direction of the gradient fill.
 *
 * @note This function must be called before initiating a gradient fill operation.
 */

static inline void ite_it2d_set_gradient_dir (ite_it2d_context_t * ctx,
                                              ite_it2d_grad_dir_t  dir)
{
    ctx->op.gradfill.direction = (uint8_t)dir;
}

void ite_it2d_set_line_area (ite_it2d_context_t * ctx, ite_it2d_area_t * area);

/**
 * @brief Set the start position of the line for the upcoming 2D line operation.
 *
 * @param ctx 2D context
 * @param x   Start x coordinate
 * @param y   Start y coordinate
 */
static inline void ite_it2d_set_line_start (ite_it2d_context_t * ctx, int16_t x,
                                            int16_t y)
{
    ctx->op.line.line_area.x1 = x;
    ctx->op.line.line_area.y1 = y;
}

/**
 * @brief Set the end position of the line for the upcoming 2D line operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] x X-coordinate of the end position.
 * @param[in] y Y-coordinate of the end position.
 *
 * @note This function should be called before submitting a 2D line operation.
 */
static inline void ite_it2d_set_line_end (ite_it2d_context_t * ctx, int16_t x,
                                          int16_t y)
{
    ctx->op.line.line_area.x2 = x;
    ctx->op.line.line_area.y2 = y;
}

/**
 * @brief Set the line width for the upcoming 2D line operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] width Width of the line to be set.
 */

static inline void ite_it2d_set_line_width (ite_it2d_context_t * ctx,
                                            uint8_t              width)
{
    ctx->op.line.line_width = width;
}

static inline void ite_it2d_enable_line_antialiasing(ite_it2d_context_t *ctx, bool en)
{
	ctx->op.line.flags.antialiasing = en;
}

/**
 * @brief Enable or disable clipping for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable clipping.
 *
 * @note This function should be called before submitting a 2D operation.
 *
 * @note If clipping is enabled, the 2D engine will perform clipping
 *       based on the defined clip area.
 */

static inline void ite_it2d_enable_clipping (ite_it2d_context_t * ctx, bool en)
{
    ctx->flags.clipping = en;
}

/**
 * @brief Enable or disable masking for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable masking.
 *
 * @note This function should be called before submitting a 2D operation.
 *
 * @note If masking is enabled, the 2D engine will perform pixel masking
 *       for the source surface and the destination surface.
 */
static inline void ite_it2d_enable_masking (ite_it2d_context_t * ctx, bool en)
{
    ctx->flags.masking = en;
}

/**
 * @brief Enable or disable blending for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable blending.
 *
 * @note This function should be called before submitting a 2D operation.
 *
 * @note If blending is enabled, the 2D engine will perform alpha blending
 *       for the source surface and the destination surface.
 */
static inline void ite_it2d_enable_blending (ite_it2d_context_t * ctx, bool en)
{
    ctx->flags.blending = en;
}

/**
 * @brief Enable or disable constant alpha for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable constant alpha.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_enable_const_alpha (ite_it2d_context_t * ctx,
                                                bool                 en)
{
    ctx->flags.const_alpha = en;
}

/**
 * @brief Enable or disable dithering for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable dithering.
 *
 * @note This function only takes effect when the source surface and the destination
 *       surface have the same color depth.
 *
 * @note Currently, we only support dithering for 1-bit, 2-bit and 4-bit color depth.
 */
static inline void ite_it2d_enable_dithering (ite_it2d_context_t * ctx, bool en)
{
    ctx->flags.dithering = en;
}

/**
 * @brief Enable or disable transforming for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable transforming.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_enable_transforming (ite_it2d_context_t * ctx,
                                                 bool                 en)
{
    ctx->op.bitblt.flags.transforming = en;
}

/**
 * @brief Enable or disable foreground color for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable foreground color.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_enable_fg_color (ite_it2d_context_t * ctx, bool en)
{
    ctx->op.bitblt.flags.fg_color = en;
}

/**
 * @brief Enable or disable interpolation for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable interpolation.
 *
 * @note This function should be called before submitting a 2D operation.
 */

static inline void ite_it2d_enable_interpolation (ite_it2d_context_t * ctx,
                                                  bool                 en)
{
    ctx->op.bitblt.flags.interpolation = en;
}

/**
 * @brief Enable or disable color keying for the following 2D operation.
 *
 * @param[in] ctx 2D context.
 * @param[in] en Enable or disable color keying.
 *
 * @note This function should be called before submitting a 2D operation.
 */
static inline void ite_it2d_enable_color_key (ite_it2d_context_t * ctx, bool en)
{
    ctx->op.bitblt.flags.color_key = en;
}

static inline void ite_it2d_enable_rop3(ite_it2d_context_t *ctx, bool en)
{
    ctx->op.bitblt.flags.rop3 = en;
}

int     ite_it2d_fill (ite_it2d_context_t * ctx);
int     ite_it2d_copy (ite_it2d_context_t * ctx);
int     ite_it2d_transform_copy (ite_it2d_context_t *       ctx,
                                 ite_it2d_transform_dsc_t * dsc);
int     ite_it2d_bitblt (ite_it2d_context_t * ctx);
int     ite_it2d_gradient_fill (ite_it2d_context_t * ctx);
int     ite_it2d_line (ite_it2d_context_t * ctx);
int     ite_it2d_exec (ite_it2d_context_t * ctx);

int     ite_it2d_present (ite_it2d_surface_t * disp_surf);
int     ite_it2d_wait_idle (void);
int32_t ite_it2d_get_current_id (void);
int     ite_it2d_wait_id_finish (int32_t id);

int     it2d_init (void);

#ifdef __cplusplus
}
#endif

#endif /* ITE_IT2D_H_ */
