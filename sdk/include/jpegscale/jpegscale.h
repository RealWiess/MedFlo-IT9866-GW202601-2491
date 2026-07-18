#ifndef JPEGSCALE_H
#define JPEGSCALE_H

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                Constant Definition
//=============================================================================
typedef enum JPEGSCALE_FORMAT_TAG
{
    JPEGSCALE_FORMAT_YUV422,
    JPEGSCALE_FORMAT_YUV420
} JPEGSCALE_FORMAT;

typedef enum JPEGSCALE_RESULT_TAG
{
    JPEGSCALE_SUCCESS,
    JPEGSCALE_ERR_INVALID_INPUT_PARAM,
    JPEGSCALE_ERR_FILE_OPERATION,
    JPEGSCALE_ERR_MEM_ALLOCATION,
    JPEGSCALE_ERR_JPEG_OPERATION
} JPEGSCALE_RESULT;

//=============================================================================
//                Macro Definition
//=============================================================================
#if defined(WIN32)

    #define JPEGSCALE_EXPORTS
    #if defined(JPEGSCALE_EXPORTS)
        #define JPEGSCALE_API __declspec(dllexport)
    #else
        #define JPEGSCALE_API __declspec(dllimport)
    #endif

#else
    #define JPEGSCALE_API extern
#endif

//=============================================================================
//                Structure Definition
//=============================================================================
typedef void *JPEGSCALE_TEMP;

//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================
JPEGSCALE_API JPEGSCALE_RESULT
JpegScale(
    char             *inputFile,
    char             *outputFile,
    float            widthPercent,
    float            heightPercent,
    JPEGSCALE_FORMAT format);

JPEGSCALE_API JPEGSCALE_RESULT
JpegLoadFile(
    char             *inputFile,
    float            widthPercent,
    float            heightPercent,
    JPEGSCALE_FORMAT format,
    JPEGSCALE_TEMP   *temp,
    uint32_t         *width,
    uint32_t         *height,
    uint32_t         *header_width,
    uint32_t         *header_height);

JPEGSCALE_API JPEGSCALE_RESULT
JpegSaveFile(
    char             *outputFile,
    JPEGSCALE_FORMAT format,
    JPEGSCALE_TEMP   temp,
    uint32_t         width,
    uint32_t         height,
    uint32_t         header_width,
    uint32_t         header_height);

JPEGSCALE_API JPEGSCALE_RESULT
PngScaleToJpeg(
	char			 *inputFile,
	char			 *outputFile,
	float			 widthPercent,
	float			 heightPercent,
	JPEGSCALE_FORMAT format);

#ifdef __cplusplus
}
#endif

#endif /* JPEGSCALE_H */
