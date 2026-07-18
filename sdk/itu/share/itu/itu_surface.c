#include <sys/endian.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#ifdef CFG_UCL_LIBRARY
    #include "ucl/ucl.h"
#endif
#include "ite/itp.h"
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"
#ifdef CFG_BUILD_M2D
    #include "gfx/gfx.h"
#endif

#if CFG_CHIP_FAMILY != 970
    #include "speedy_comp.h"
#endif

#ifdef CFG_M2D_RESERVE_MEM_ENABLE
    #if defined(CFG_WIN32_SIMULATOR)
uint8_t * gfxVmemAlloc (uint32_t sizeInByte)
{
    // dummy for win32
}
void gfxVmemFree (uint8_t * ptr)
{
    // dummy for win32
}
    #endif
#endif
static void ituSurfaceErrorClearDCPSFLAG (int line)
{
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX]->flags & ITU_DRAWDCPS)
    {
        ituScene->surfpl[ituScene->dbuffIDX]->flags &= ~ITU_DRAWDCPS;
        ituLog("[SurfacePool][dcps][buffer %d][dcps fail --> force clear flag][%d]\n", ituScene->dbuffIDX, line);
        ituScene->dbuffIDX = ((ituScene->dbuffIDX + 1) % ITU_DCPS_BUFFER_COUNT);
    }
#endif
    return;
}

ITUSurface * ituSurfaceDecompress (ITUSurface * surf)
{
    ITUSurface * retSurf = NULL;
#ifdef CFG_M2D_RESERVE_MEM_ENABLE
    bool bUseM2DReserveMem = false;
    #if !defined(CFG_WIN32_SIMULATOR) && !defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE)
    bUseM2DReserveMem = true;
    #endif
#endif

    if (surf->flags & ITU_SPRITE_BUF)
    {
        if (ituScene->surfpl[0] == NULL)
        {
            uint32_t  vmemL       = itpVmemAlloc(ituScene->poolSize);
            uint8_t * bufL        = (uint8_t *)ithMapVram(vmemL, ituScene->poolSize, ITH_VRAM_READ);
            ituScene->vPoolBuf[0] = vmemL;
            ituScene->surfpl[0] =
                ituCreateSurface(CFG_LCD_WIDTH, CFG_LCD_HEIGHT, CFG_LCD_WIDTH * 4, ITU_ARGB8888, bufL, ITU_STATIC);
            ithUnmapVram(bufL, ituScene->poolSize);
        }
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX])
    {
        if (!(ituScene->surfpl[ituScene->dbuffIDX]->flags & ITU_DRAWDCPS))
        {
            // ITU_LOG_DBG("[SurfacePool][dcps][buffer %d][bypass]\n", ituScene->dbuffIDX );
            return NULL;
        }
    }
    else
    {
        ituLog("[SurfacePool][dcps][buffer %d got error!]\n", ituScene->dbuffIDX);
        return NULL;
    }

    if (!(surf->flags & ITU_COMPRESSED)) // workaround for no compress issue ex: size < 256 bytes
    {
        int       bufSize = 0;
        uint8_t * buf     = NULL;

        if (surf->format == ITU_RGB565A8)
        {
            bufSize = surf->pitch * surf->height + surf->width * surf->height;
        }
        else
        {
            bufSize = surf->pitch * surf->height;
        }

        if (ituScene->surfpl[ituScene->dbuffIDX])
        {
            ITUSurface * dcpsSurf = ituScene->surfpl[ituScene->dbuffIDX];
            ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], surf->width, surf->height, surf->pitch,
                              surf->format);
            ituScene->surfpl[ituScene->dbuffIDX]->flags |= ITU_SURFPOOL;
            buf = (uint8_t *)ithMapVram(ituScene->surfpl[ituScene->dbuffIDX]->addr, bufSize, ITH_VRAM_WRITE);
            (void)memcpy(buf, (const uint8_t *)surf->addr, bufSize);
            // ITU_LOG_DBG("[SurfacePool][dcps_raw] %d x %d [format: %d] [size %d]\n", surf->width, surf->height,
            // surf->format, bufSize );
            ithUnmapVram(buf, bufSize);

            if (ituScene->surfpl[ituScene->dbuffIDX]->flags & ITU_DRAWDCPS)
            {
                ituScene->surfpl[ituScene->dbuffIDX]->flags &= ~ITU_DRAWDCPS;
            }

            ituScene->dbuffIDX = ((ituScene->dbuffIDX + 1) % ITU_DCPS_BUFFER_COUNT);

            return dcpsSurf;
        }
        else
        {
            return NULL;
        }
    }
#endif
    assert(surf->flags & ITU_COMPRESSED);

    if ((surf->flags & ITU_STATIC) && (surf->lockSize > 0)) // as reference count
    {
        assert(surf->lockAddr);                             // as cached decompressed surface
        surf->lockSize++;
        ituLog("[SurfacePool][dcps][buffer %d][return lockSize]\n", ituScene->dbuffIDX);
        return (ITUSurface *)surf->lockAddr;
    }

    if (surf->flags & ITU_JPEG)
    {
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        if (ituScene->surfpl[ituScene->dbuffIDX])
        {
            uint8_t * pbuf =
                (uint8_t *)ithMapVram(ituScene->vPoolBuf[ituScene->dbuffIDX], ituScene->poolSize, ITH_VRAM_READ);
            // ituDestroySurface(ituScene->surfpl);
            // ituScene->surfpl = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, pbuf,
            // ITU_STATIC);
            ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], surf->width, surf->height, surf->pitch,
                              surf->format);
            ituScene->surfpl[ituScene->dbuffIDX]->flags |= ITU_SURFPOOL;
            ithUnmapVram(pbuf, ituScene->poolSize);
            ituLog("[SurfacePool][dcps] %d x %d [jpeg format: %d]\n", surf->width, surf->height, surf->format);
        }
#endif

        if (surf->format == ITU_ARGB8888)
        {
            int      ret, jpegSize, alphaSize, alphaCompressSize;
            uint8_t *jpegData, *alphaData, *buf;
            uint32_t vmem;

            (void)memcpy(&jpegSize, (uint8_t *)surf->addr, 4);
            jpegData = (uint8_t *)surf->addr + 4;
            (void)memcpy(&alphaSize, jpegData + jpegSize, 4);
            alphaSize = be32toh(alphaSize);
            (void)memcpy(&alphaCompressSize, jpegData + jpegSize + 4, 4);
            alphaCompressSize = be32_to_cpu(alphaCompressSize);
            alphaData         = (uint8_t *)jpegData + jpegSize;

#ifdef CFG_M2D_RESERVE_MEM_ENABLE
            if (bUseM2DReserveMem)
            {
                buf = gfxVmemAlloc(alphaSize); // use m2d reserve memory
            }
            else
            {
#endif
#if !defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) || defined(ITU_SURFACE_DEBUG_JPEGBUFF)
                vmem = itpVmemAlloc(alphaSize);
#else
            vmem = ituScene->vAlphaBuf[ituScene->dbuffIDX];
#endif
                if (!vmem)
                {
                    ituSurfaceErrorClearDCPSFLAG(__LINE__);
                    ITU_LOG_ERR("out of memory: %d\n", alphaSize);
                    return NULL;
                }
                buf = (uint8_t *)ithMapVram(vmem, alphaSize, ITH_VRAM_WRITE);
#ifdef CFG_M2D_RESERVE_MEM_ENABLE
            }
#endif

#if defined(CFG_DCPS_ENABLE) && !defined(CFG_ITU_UCL_ENABLE)
            // hardware decompress
    #ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
            {
                unsigned long cmdqMode = (unsigned long)ITP_DCPS_USE_CMDQ; // open dcps with cmd queue (no waiting)
                ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_INIT, &cmdqMode);   // ITP_DCPS_BRFLZ);	 gCurrDcpsMode
            }
    #else
            ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_INIT, NULL);
    #endif

    #if CFG_CHIP_FAMILY != 970
            if (surf->flags & ITU_SPEEDY)
            {
                uint8_t dcpsMode = ITP_DCPS_SPEEDY_MODE; // ITP_DCPS_SPEEDY_MODE=2
                ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_SET_MODE, &dcpsMode);
                ret = write(ITP_DEVICE_DECOMPRESS, alphaData + 8, alphaCompressSize);
            }
            else
            {
                ret = write(ITP_DEVICE_DECOMPRESS, alphaData, alphaCompressSize);
            }
    #else
            ret = write(ITP_DEVICE_DECOMPRESS, alphaData, alphaCompressSize);
    #endif // CFG_CHIP_FAMILY != 970

            if (ret != alphaCompressSize)
            {
                ituSurfaceErrorClearDCPSFLAG(__LINE__);
                ITU_LOG_ERR("decompress write error: %d != %d\n", ret, alphaCompressSize);
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
                if (bUseM2DReserveMem)
                {
                    gfxVmemFree(buf); // use m2d reserve memory
                }
                else
    #endif
                {
                    ithUnmapVram(buf, alphaSize);
    #if !defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) || defined(ITU_SURFACE_DEBUG_JPEGBUFF)
                    itpVmemFree(vmem);
    #endif
                }
                return NULL;
            }

            ret = read(ITP_DEVICE_DECOMPRESS, buf, alphaSize);
            if (ret != alphaSize)
            {
                ituSurfaceErrorClearDCPSFLAG(__LINE__);
                ITU_LOG_ERR("decompress read error: %d != %d\n", ret, alphaSize);
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
                if (bUseM2DReserveMem)
                {
                    gfxVmemFree(buf); // use m2d reserve memory
                }
                else
    #endif
                {
                    ithUnmapVram(buf, alphaSize);
    #if !defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) || defined(ITU_SURFACE_DEBUG_JPEGBUFF)
                    itpVmemFree(vmem);
    #endif
                }
                return NULL;
            }
            ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_EXIT, NULL);

#else
            // software decompress
    #if CFG_CHIP_FAMILY != 970
            if (surf->flags & ITU_SPEEDY)
            {
                myLZParaType para = {1, 11, 4096, 6};
                ret               = myLZ_depack(alphaData + 8 + 14, buf, alphaSize, 4096, &para);
                if (ret != alphaSize)
                {
                    ituSurfaceErrorClearDCPSFLAG(__LINE__);
                    ITU_LOG_ERR("internal error - decompression speedy failed\n");
        #ifdef CFG_M2D_RESERVE_MEM_ENABLE
                    if (bUseM2DReserveMem)
                    {
                        gfxVmemFree(buf); // use m2d reserve memory
                    }
                    else
        #endif
                    {
                        ithUnmapVram(buf, alphaSize);
        #if !defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) || defined(ITU_SURFACE_DEBUG_JPEGBUFF)
                        itpVmemFree(vmem);
        #endif
                    }

                    return NULL;
                }
            }
            else
    #endif // CFG_CHIP_FAMILY != 970
            {
    #ifdef CFG_UCL_LIBRARY
                if (ucl_init() != UCL_E_OK)
    #else
                if (1)
    #endif
                {
                    ituSurfaceErrorClearDCPSFLAG(__LINE__);
                    ITU_LOG_ERR("internal error - ucl_init() failed !!!\n");
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
                    if (bUseM2DReserveMem)
                    {
                        gfxVmemFree(buf); // use m2d reserve memory
                    }
                    else
    #endif
                    {
                        ithUnmapVram(buf, alphaSize);
    #if !defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) || defined(ITU_SURFACE_DEBUG_JPEGBUFF)
                        itpVmemFree(vmem);
    #endif
                    }
                    return NULL;
                }

    #ifdef CFG_UCL_LIBRARY
                ret = alphaSize;
                if (ucl_nrv2e_decompress_8((const ucl_bytep)alphaData + 8, alphaCompressSize, buf, &ret, NULL) !=
                        UCL_E_OK ||
                    ret != alphaSize)
    #else
                if (1)
    #endif
                {
                    ituSurfaceErrorClearDCPSFLAG(__LINE__);
                    ITU_LOG_ERR("internal error - decompression failed\n");
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
                    if (bUseM2DReserveMem)
                    {
                        gfxVmemFree(buf); // use m2d reserve memory
                    }
                    else
    #endif
                    {
                        ithUnmapVram(buf, alphaSize);
    #if !defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) || defined(ITU_SURFACE_DEBUG_JPEGBUFF)
                        itpVmemFree(vmem);
    #endif
                    }
                    return NULL;
                }
            }
#endif // defined(CFG_DCPS_ENABLE) && !defined(CFG_ITU_UCL_ENABLE)

            // ithFlushDCacheRange(buf, alphaSize);

            retSurf = ituJpegAlphaLoad(surf->width, surf->height, buf, jpegData, jpegSize);

#ifdef CFG_M2D_RESERVE_MEM_ENABLE
            if (bUseM2DReserveMem)
            {
                gfxVmemFree(buf);
            }
            else
#endif
            {
                ithUnmapVram(buf, alphaSize);
#if !defined(CFG_ITU_DECOMPRESS_SURFACE_INPLACE) || defined(ITU_SURFACE_DEBUG_JPEGBUFF)
                itpVmemFree(vmem);
#endif
            }
        }
        else // JPEG_565
        {
#ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
            retSurf = ituJpegLoad(surf->width, surf->height, (uint8_t *)surf->addr, surf->size, 0);
#else
            if (ituScene->surfpl[ituScene->dbuffIDX])
            {
                retSurf = ituJpegLoad(surf->width, surf->height, (uint8_t *)surf->addr, surf->size, 0);
            }
#endif
        }
    }
    else
    {
        int       ret, bufSize;
        uint32_t  compressedSize, vmem = 0;
        uint8_t * buf;

        if (surf->format == ITU_RGB565A8)
        {
            bufSize = surf->pitch * surf->height + surf->width * surf->height;
        }
        else
        {
            bufSize = surf->pitch * surf->height;
        }

        // use for cppcheck
        if (bufSize < 0)
        {
            (void)printf("[nothing][dummy][%d]\n", (int)vmem);
        }

#ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        if (surf->flags & ITU_SPRITE_BUF)
        {
            ituReplaceSurface(ituScene->surfpl[0], surf->width, surf->height, surf->pitch, surf->format);
            buf = (uint8_t *)ithMapVram(ituScene->surfpl[0]->addr, bufSize, ITH_VRAM_WRITE);
        }
        else
        {
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
            if (bUseM2DReserveMem)
            {
                buf = gfxVmemAlloc(bufSize); // use m2d reserve memory
            }
            else
    #endif
            {
                vmem = itpVmemAlloc(bufSize);
                if (!vmem)
                {
                    ITU_LOG_WARN("out of memory: %d, try to reset font cache...\n", surf->pitch * surf->height);
                    ituFtResetCache();
                    vmem = itpVmemAlloc(bufSize);
                    if (!vmem)
                    {
                        ituSurfaceErrorClearDCPSFLAG(__LINE__);
                        ITU_LOG_ERR("out of memory: %d\n", surf->pitch * surf->height);
                        return NULL;
                    }
                }
    #if CFG_CPU_WB
                // The mapped memory area may be used by CPU and related cache may be dirty before.
                // Therefore, we need to invalidate the cache range to ensure no related cache flush while engine
                // writing this area simultaneously.
                ithInvalidateDCacheRange((void *)vmem, bufSize);
    #endif

                buf = (uint8_t *)ithMapVram(vmem, bufSize, ITH_VRAM_WRITE);
            }
        }
#else
        if (ituScene->surfpl[ituScene->dbuffIDX])
        {
            uint8_t * pbuf =
                (uint8_t *)ithMapVram(ituScene->vPoolBuf[ituScene->dbuffIDX], ituScene->poolSize, ITH_VRAM_READ);
            // ituDestroySurface(ituScene->surfpl);
            // ituScene->surfpl = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, pbuf,
            // ITU_STATIC);
            ituReplaceSurface(ituScene->surfpl[ituScene->dbuffIDX], surf->width, surf->height, surf->pitch,
                              surf->format);
            ituScene->surfpl[ituScene->dbuffIDX]->flags |= ITU_SURFPOOL;
            ithUnmapVram(pbuf, ituScene->poolSize);
            buf = (uint8_t *)ithMapVram(ituScene->surfpl[ituScene->dbuffIDX]->addr, bufSize, ITH_VRAM_WRITE);
            ituLog("[SurfacePool][dcps] %d x %d [format: %d] [size %d]\n", surf->width, surf->height, surf->format,
                   bufSize);
        }
#endif

        compressedSize = surf->size;
        ituLog("decompress surface size %d to %d\n", (int)compressedSize, bufSize);

#if defined(CFG_DCPS_ENABLE) && !defined(CFG_ITU_UCL_ENABLE)
        // hardware decompress
    #ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        {
            unsigned long cmdqMode = (unsigned long)ITP_DCPS_USE_CMDQ; // open dcps with cmd queue (no waiting)
            ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_INIT, &cmdqMode);   // ITP_DCPS_BRFLZ);	 gCurrDcpsMode
        }
    #else
        {
            // unsigned long cmdqMode = (unsigned long)ITP_DCPS_USE_CMDQ_WAIT;
            ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_INIT, NULL);
            // ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_INIT, &cmdqMode);
        }
    #endif

    #if CFG_CHIP_FAMILY != 970
        if (surf->flags & ITU_SPEEDY)
        {
            uint8_t dcpsMode = ITP_DCPS_SPEEDY_MODE; // ITP_DCPS_SPEEDY_MODE=2
            ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_SET_MODE, &dcpsMode);
        }
    #endif // CFG_CHIP_FAMILY != 970

        surf->lockSize = ithBswap32(bufSize);
        surf->parent   = (ITUSurface *)ithBswap32(compressedSize);
        ret            = write(ITP_DEVICE_DECOMPRESS, (uint8_t *)surf->addr, compressedSize);
        if (ret != compressedSize)
        {
            ituSurfaceErrorClearDCPSFLAG(__LINE__);
            ITU_LOG_ERR("decompress write error: %d != %lu\n", ret, compressedSize);
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
            if (bUseM2DReserveMem)
            {
                gfxVmemFree(buf); // use m2d reserve memory
            }
            else
    #endif
            {
                ithUnmapVram(buf, bufSize);
    #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                itpVmemFree(vmem);
    #endif
            }
            return NULL;
        }
        // else
        //	(void)printf("---[%d]---bufsize[%d]--compressedSize[%d]\n", __LINE__, bufSize, compressedSize);

        ret = read(ITP_DEVICE_DECOMPRESS, buf, bufSize);
        if (ret != bufSize)
        {
            ituSurfaceErrorClearDCPSFLAG(__LINE__);
            ITU_LOG_ERR("decompress read error: %d != %d\n", ret, bufSize);
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
            if (bUseM2DReserveMem)
            {
                gfxVmemFree(buf); // use m2d reserve memory
            }
            else
    #endif
            {
                ithUnmapVram(buf, bufSize);
    #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                itpVmemFree(vmem);
    #endif
            }
            return NULL;
        }
        ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_EXIT, NULL);
        surf->lockSize = 0;
#else
        // software decompress
    #if CFG_CHIP_FAMILY != 970
        if (surf->flags & ITU_SPEEDY)
        {
            if (compressedSize == bufSize)
            {
                (void)memcpy((void *)buf, (const void *)surf->addr, bufSize);
            }
            else
            {
                myLZParaType para = {4, 11, 4096, 6};

                /* if (surf->format == ITU_RGB565 || surf->format == ITU_ARGB1555 || surf->format == ITU_ARGB4444)
                {
                    para.bytePerUnit = 2;
                }
                else if (surf->format == ITU_MONO)
                {
                    para.bytePerUnit = 1;
                } */

                // always use 4 butes per unit for speedy (sync with Joseph modified under drawrocker speedy export)
                para.bytePerUnit  = 4;

                ret               = myLZ_depack((const void *)(surf->addr + 14), (void *)buf, bufSize, 4096, &para);
                if (ret != bufSize)
                {
                    ituSurfaceErrorClearDCPSFLAG(__LINE__);
                    ITU_LOG_ERR("internal error - decompression speedy failed\n");
        #ifdef CFG_M2D_RESERVE_MEM_ENABLE
                    if (bUseM2DReserveMem)
                    {
                        gfxVmemFree(buf); // use m2d reserve memory
                    }
                    else
        #endif
                    {
                        ithUnmapVram(buf, bufSize);
        #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                        itpVmemFree(vmem);
        #endif
                    }
                    return NULL;
                }
            }
        }
        else
    #endif // CFG_CHIP_FAMILY != 970
        {
    #ifdef CFG_UCL_LIBRARY
            if (ucl_init() != UCL_E_OK)
    #else
            if (0)
    #endif
            {
                ituSurfaceErrorClearDCPSFLAG(__LINE__);
                ITU_LOG_ERR("internal error - ucl_init() failed !!!\n");
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
                if (bUseM2DReserveMem)
                {
                    gfxVmemFree(buf); // use m2d reserve memory
                }
                else
    #endif
                {
                    ithUnmapVram(buf, bufSize);
    #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                    itpVmemFree(vmem);
    #endif
                }
                return NULL;
            }
            else
            {
                uint32_t        i = 0;
                uint32_t        o = 0;
                uint32_t        srcSize;
                uint32_t        dstSize;
                uint32_t        MaxUclBlockSize = 0x400000;
                const uint8_t * ptr             = (uint8_t *)surf->addr;
                uint8_t *       dstBuf          = (uint8_t *)buf;

                do
                {
                    dstSize = ((ptr[i] << 24) | (ptr[i + 1] << 16) | (ptr[i + 2] << 8) | (ptr[i + 3]));
                    i += 4;
                    srcSize = ((ptr[i] << 24) | (ptr[i + 1] << 16) | (ptr[i + 2] << 8) | (ptr[i + 3]));
                    i += 4;

                    if (!srcSize || !dstSize || (dstSize > MaxUclBlockSize))
                    {
                        if (dstSize > MaxUclBlockSize)
                        {
                            ITU_LOG_ERR("internal error - decompression failed, dst size > 4MB\n");
                        }
                        break;
                    }
    #ifdef CFG_UCL_LIBRARY
                    ret = dstSize;
                    if (ucl_nrv2e_decompress_8((const ucl_bytep)&ptr[i], srcSize, dstBuf, &ret, NULL) != UCL_E_OK ||
                        ret != dstSize)
    #else
                    if (0)
    #endif
                    {
                        ituSurfaceErrorClearDCPSFLAG(__LINE__);
                        ITU_LOG_ERR("internal error - decompression failed %d\n", (int)&dstBuf);
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
                        if (bUseM2DReserveMem)
                        {
                            gfxVmemFree(buf); // use m2d reserve memory
                        }
                        else
    #endif
                        {
                            ithUnmapVram(buf, bufSize);
    #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
                            itpVmemFree(vmem);
    #endif
                        }
                        return NULL;
                    }

                    i += srcSize;
                    o += dstSize;
                    dstBuf = (uint8_t *)&buf[o];
                } while (i < compressedSize);
            }
        }
#endif  // defined(CFG_DCPS_ENABLE) && !defined(CFG_ITU_UCL_ENABLE)

        // ithFlushDCacheRange(buf, bufSize);
#ifdef CFG_M2D_RESERVE_MEM_ENABLE
        if (bUseM2DReserveMem && buf)
        {
    #ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
            unsigned int flags = surf->flags & ~(ITU_COMPRESSED | ITU_STATIC | ITU_SPEEDY);
            flags |= ITU_M2DVMEM;
            retSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, buf, flags);
    #endif
        }
        else
#endif
        {
            ithUnmapVram(buf, bufSize);

#ifndef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
            if (!(surf->flags & ITU_SPRITE_BUF))
            {
                unsigned int flags = surf->flags & ~(ITU_COMPRESSED | ITU_STATIC | ITU_SPEEDY);
                retSurf =
                    ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, (uint8_t *)vmem, flags);
            }
#endif
        }
    }

    if (surf->flags & ITU_SPRITE_BUF)
    {
        return ituScene->surfpl[0];
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (ituScene->surfpl[ituScene->dbuffIDX])
    {
        ITUSurface * dcpsSurf = ituScene->surfpl[ituScene->dbuffIDX];
        // (void)printf("[dcps to buff-%d] ", ituScene->dbuffIDX);

        if (ituScene->surfpl[ituScene->dbuffIDX]->flags & ITU_DRAWDCPS)
        {
            ituScene->surfpl[ituScene->dbuffIDX]->flags &= ~ITU_DRAWDCPS;
            ituLog("[SurfacePool][dcps][buffer %d clear dcps flag]\n", ituScene->dbuffIDX);
        }

        ituScene->dbuffIDX = ((ituScene->dbuffIDX + 1) % ITU_DCPS_BUFFER_COUNT);
        ituLog("[SurfacePool][dcps][change buffidx to %d]\n", ituScene->dbuffIDX);

        if (retSurf)
        {
            return retSurf;
        }
        else
        {
            return dcpsSurf;
        }
    }
#endif

    if (retSurf && (surf->flags & ITU_STATIC) && (surf->lockSize == 0))
    {
        surf->lockSize++;
        surf->lockAddr  = (uint8_t *)retSurf;
        retSurf->parent = surf;
    }
    return retSurf;
}

void ituSurfaceRelease (ITUSurface * surf)
{
    ITUSurface* parentSurf = NULL;
    
    if (surf != NULL)
    {
        parentSurf = (ITUSurface*)surf->parent;
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
    if (surf->flags & ITU_SURFPOOL)
    {
        ituLog("[SurfacePool] ituSurfaceRelease bypass!\n");
        return;
    }
#endif

    if (parentSurf)
    {
        if (parentSurf->flags & ITU_STATIC)
        {
            assert(parentSurf->flags & ITU_COMPRESSED);
            assert(parentSurf->lockSize);

            if (--parentSurf->lockSize == 0)
            {
                ituDestroySurface(surf);
            }
        }
    }
    else
    {
#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
        // for 983x all surface that created by yourself should use free by yourself
        return;
#else
        ituDestroySurface(surf);
#endif
    }
}
