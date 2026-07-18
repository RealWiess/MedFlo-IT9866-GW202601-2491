#include <sys/ioctl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include "lwip/ip.h"
#include "lwip/dns.h"
#include "ite/itp.h"
#include "ite/itv.h"
#include "ith/ith_video.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#define TCP_MIRROR_FLOW 1
#define SHOW_BITRATE 1
#define DROP_FRAME 1
#ifdef TCP_MIRROR_FLOW
#define RECV_BUF_LEN    512*1024
#define TMP_BUF_LEN    1024*1024
#else
#define RECV_BUF_LEN    2*1024
#endif
#define NAL_BUF_LEN    512*1024
#define MIRROR_VIDEO_NUM 6


#define MACRO_ERTN         { \
        printf("%f:%d, error returned!\n", __FILE__, __LINE__); \
        return (void *)-1; \
}

#define MACRO_NRTN         { \
        printf("%f:%d, error returned!\n", __FILE__, __LINE__); \
        return; \
}

typedef struct _mirror_video
{
    bool    new_nal;
    uint8_t *data;
    int     size;
}mirror_video;

static int  sockfd = 0, forClientSockfd = 0;
static bool mirror_is_init = false, mirror_pause = false, mirror_from_H = false, network_mirror_start = false, backup_is_init = false, backup_recving = false;
static pthread_t recv_thread, decode_thread, backup_recv_thread;
static uint32_t  lastTick = 0, check_tick = 0;
static uint8_t   window_state = 0, mirror_video_r_idx = 0, mirror_video_w_idx = 0;
static mirror_video mirror_video_nal[MIRROR_VIDEO_NUM] = {0}; 
static uint8_t* nalBuf = NULL;

static void display_on_LCD(AVFrame *picture)
{
    ITV_DBUF_PROPERTY prop = {0};

    while (1)
    {
        uint8_t *dbuf = itv_get_dbuf_anchor();
        if (dbuf != NULL)
        {
            uint32_t rdIndex = picture->opaque ? *(uint32_t *)picture->opaque : 0, check_cnt = 0;
            //uint8_t  *ya = picture->data[0];

            //printf("+++++++++++++++(%d, %d, %d, %d)\n", picture->width, picture->height, picture->linesize[0], picture->linesize[1]);
            //printf("(0x%x, 0x%x, 0x%x)\n", ya[0], ya[picture->width/2], ya[picture->width]);
            //for(int i = 0; i < picture->width/2; i++)
            //{
                //printf("(0x%x)\n", ya[picture->width*picture->height/2+i]);
                //if(ya[picture->width*picture->height/2+i] == 0x10)
                //    check_cnt++;
            //}
            //printf("\n\n");
            //if(check_cnt < picture->width/4)
            if(mirror_from_H)
            {
                if(window_state != 1)
                {
                    printf("show_mirror_window_W!!!!!!!!!\n");
                    show_mirror_window(1280, 480);
                }
                window_state = 1;
                prop.src_w    = picture->width;
                prop.src_h    = picture->height;
                prop.ya       = picture->data[0];
                prop.ua       = picture->data[1];
                prop.va       = picture->data[2];
                prop.pitch_y  = picture->linesize[0];
                prop.pitch_uv = picture->linesize[1];
                prop.bidx     = rdIndex;
                prop.format   = MMP_ISP_IN_NV12;
            }else
            {
                if(window_state != 2)
                {
                    printf("show_mirror_window_H!!!!!!!!!\n");
                    show_mirror_window(480, 1280);
                }
                window_state = 2;
                prop.src_w    = picture->width;
                prop.src_h    = picture->height;
                prop.ya       = picture->data[0];
                prop.ua       = picture->data[1];
                prop.va       = picture->data[2];
                prop.pitch_y  = picture->linesize[0];
                prop.pitch_uv = picture->linesize[1];
                prop.bidx     = rdIndex;
                prop.format   = MMP_ISP_IN_NV12;
            }

            itv_update_dbuf_anchor(&prop); // printf("display mark non-use %d\n", rdIndex);
            break;
        }
        else
        {
            //printf(">>>>>>>>>>>>>itv buffer full!!!!!!!!!!!!!!!!!!!wait\n");
            usleep(1000);
        }
    }
}

static void drop_display_on_LCD(AVFrame *picture)
{
    ITV_DBUF_PROPERTY prop = {0};

    //while (1)
    {
        uint8_t *dbuf = itv_get_dbuf_anchor();
        if (dbuf != NULL)
        {
            uint32_t rdIndex = picture->opaque ? *(uint32_t *)picture->opaque : 0, check_cnt = 0;
            //uint8_t  *ya = picture->data[0];

            //printf("+++++++++++++++(%d, %d, %d, %d)\n", picture->width, picture->height, picture->linesize[0], picture->linesize[1]);
            //printf("(0x%x, 0x%x, 0x%x)\n", ya[0], ya[picture->width/2], ya[picture->width]);
            //for(int i = 0; i < picture->width/2; i++)
            //{
                //printf("(0x%x)\n", ya[picture->width*picture->height/2+i]);
                //if(ya[picture->width*picture->height/2+i] == 0x10)
                //    check_cnt++;
            //}
            //printf("\n\n");
            //if(check_cnt < picture->width/4)
            if(mirror_from_H)
            {
                if(window_state != 1)
                {
                    printf("show_mirror_window_W!!!!!!!!!\n");
                    show_mirror_window(1280, 480);
                }
                window_state = 1;
                prop.src_w    = picture->width;
                prop.src_h    = picture->height;
                prop.ya       = picture->data[0];
                prop.ua       = picture->data[1];
                prop.va       = picture->data[2];
                prop.pitch_y  = picture->linesize[0];
                prop.pitch_uv = picture->linesize[1];
                prop.bidx     = rdIndex;
                prop.format   = MMP_ISP_IN_NV12;
            }else
            {
                if(window_state != 2)
                {
                    printf("show_mirror_window_H!!!!!!!!!\n");
                    show_mirror_window(480, 1280);
                }
                window_state = 2;
                prop.src_w    = picture->width;
                prop.src_h    = picture->height;
                prop.ya       = picture->data[0];
                prop.ua       = picture->data[1];
                prop.va       = picture->data[2];
                prop.pitch_y  = picture->linesize[0];
                prop.pitch_uv = picture->linesize[1];
                prop.bidx     = rdIndex;
                prop.format   = MMP_ISP_IN_NV12;
            }

            itv_update_dbuf_anchor(&prop); // printf("display mark non-use %d\n", rdIndex);
            //break;
        }
        //else
        {
            //printf(">>>>>>>>>>>>>itv buffer full!!!!!!!!!!!!!!!!!!!wait\n");
            //usleep(1000);
        }
    }
}

static void init_codec(void)
{
}

static void* udp_backup_task(void* arg)
{
    fd_set readfds;
    struct timeval timeout = {0};
    uint32_t recv_size = 0;
    uint8_t* recvBuf = (uint8_t*) malloc(RECV_BUF_LEN);

    backup_is_init = true;
    
    while(1)
    {
        //printf("++++++++++\n");
        if(!network_mirror_start)
        {
            if(mirror_is_init && backup_recving)
            {    
                if(sockfd)
                    close(sockfd);
                backup_recving = false;
            }
            else
            {
                while(mirror_is_init)
                    usleep(1000); //wait udp recv task close
                backup_recving = true;
                //printf("+++++++++++111111\n");
                FD_ZERO(&readfds);
    		    FD_SET(sockfd,&readfds);
    		    timeout.tv_sec = 0;
    		    timeout.tv_usec = 5000;
    		    select(sockfd+1,&readfds,NULL,NULL,&timeout);
                if(FD_ISSET(sockfd,&readfds))
                {
                    recv_size = recv(sockfd, recvBuf, RECV_BUF_LEN, 0);
                    printf("UDP_BACK recv = %d\n", recv_size);
                }
            }
        }
        usleep(1000);
    }
    if(recvBuf)
        free(recvBuf);
    
    //pthread_exit(NULL);
   
    return NULL;
}

static void* stream_decode_task(void* arg)
{    
    // ffmpeg
    AVCodec        *codec;
    AVCodecContext *c       = NULL;
    AVFrame        *picture = NULL;
    AVDictionary   *dict    = NULL;
    // ffmpeg
    AVPacket       avpkt;
    int            got_picture = 0, decode_len = 0, w = 0, len;

#ifdef CFG_VIDEO_ENABLE
	//ituFrameFuncInit();
#endif // CFG_VIDEO_ENABLE
    //itv_set_video_window(100, 0, 300, 480);
    itv_set_pb_mode(1);

    avcodec_register_all(); // avcodec_init();
    codec = avcodec_find_decoder(CODEC_ID_H264);
    if (!codec)
    {
        MACRO_ERTN;
    }
    c               = avcodec_alloc_context3(codec); // avcodec_alloc_context();

#ifdef _DEBUG
    c->debug        = FF_DEBUG_PICT_INFO | FF_DEBUG_SKIP | FF_DEBUG_STARTCODE | FF_DEBUG_PTS | FF_DEBUG_MMCO | FF_DEBUG_BUGS | FF_DEBUG_BUFFERS | FF_DEBUG_THREADS;
#endif

    picture         = avcodec_alloc_frame();
    assert(picture);
    avcodec_get_frame_defaults(picture);
    picture->opaque = malloc(sizeof(uint32_t));
    if (codec->capabilities & CODEC_CAP_TRUNCATED)
        c->flags |= CODEC_FLAG_TRUNCATED;

    if (avcodec_open2(c, codec, &dict) /*avcodec_open(c, codec)*/ < 0)
    {
        MACRO_ERTN;
    }
#ifdef _DEBUG
    av_log_set_level(AV_LOG_DEBUG);
#endif

    mirror_video_r_idx = 0;
    mirror_pause = false; 

    while(network_mirror_start)
    {
        if(mirror_video_nal[mirror_video_r_idx].new_nal)
        {
            if(!mirror_pause)
            {
                avpkt.data = mirror_video_nal[mirror_video_r_idx].data;
                avpkt.size = mirror_video_nal[mirror_video_r_idx].size;

                len = avcodec_decode_video2(c, picture, &got_picture, &avpkt);
                if (got_picture)    
                {
                    //printf("#######H264 decode ok\n");
#ifdef DROP_FRAME 
                    drop_display_on_LCD(picture);
#else 
                    display_on_LCD(picture);
#endif
                }
            }
            if(mirror_video_nal[mirror_video_r_idx].data)
            {
                free(mirror_video_nal[mirror_video_r_idx].data);
                mirror_video_nal[mirror_video_r_idx].data = NULL;
            }
            mirror_video_nal[mirror_video_r_idx].new_nal = false;
            mirror_video_r_idx++;
            mirror_video_r_idx = mirror_video_r_idx%MIRROR_VIDEO_NUM;
            usleep(1000);
        }else
            usleep(1000);
    }
    while(mirror_is_init)
        usleep(1000); //wait recv task close
    // ffmpeg de-initial
    avcodec_close(c);
    av_free(c);
    if (picture->opaque)
        free(picture->opaque);
    av_free(picture);
    itv_set_pb_mode(0);

    for(int i = 0; i < MIRROR_VIDEO_NUM; i++)
    {
        mirror_video_nal[i].new_nal = false;
        if(mirror_video_nal[i].data)
        {
            free(mirror_video_nal[i].data);
            mirror_video_nal[i].data = NULL;
        }
    }
    //pthread_exit(NULL);
   
    return NULL;
}
#ifdef TCP_MIRROR_FLOW
static void* tcp_mirror_task(void* arg)
{
    FILE* f = NULL;
    int k = 0;
    uint8_t  message[13] = {0x4F, 0x4B, 0x68, 0x01, 0xBC, 0x02, 0xBC, 0x02, 0x68, 0x01, 0x1E, 0xB8, 0x0B}; /*{OK,V_W,V_H,H_W,H_H,FPS,bps}*/
    //uint8_t  message[2] = {0x4F, 0x4B}; //return OK to APP
    uint8_t  frame_cnt = 0, buf_end_data[16] = {NULL};
    uint8_t *recvBuf = (uint8_t*) malloc(RECV_BUF_LEN), *tmpBuf = (uint8_t*) malloc(TMP_BUF_LEN);
    uint32_t nal_tick = 0, sec_size = 0, recv_size = 0, w_idx = 0, r_idx = 0, remaining_size = 0, parse_size = 0, head_nal_size = 0, recv_nal_size = 0, ori_w = 0, ori_h = 0, enc_w = 0, enc_h = 0;
    bool     got_nal = false, mirror_start = false, first_Iframe_get = true;
    fd_set readfds;
    struct timeval timeout = {0};
    struct sockaddr_in remote_addr;
    int len = sizeof(struct sockaddr_in); 
    int recv_sock = -1, err_num = 0;

    mirror_video_w_idx = 0;
    memset(tmpBuf,'\0',TMP_BUF_LEN);

    //memset(recvBuf,'\0',RECV_BUF_LEN);
    //memset(nalBuf,'\0',NAL_BUF_LEN);
    //f = fopen("E:/testtcp.264", "wb");
    while(network_mirror_start)
    {
        if( mirror_start && itpGetTickDuration(check_tick) > 2000)
        {
            printf("#########no video input!!!\n");  
            check_tick = itpGetTickCount();
            mirror_start = false;
            w_idx = 0;
            r_idx = 0;
        //    if(recv_sock)
        //        close(recv_sock);
        } 

        //memset(recvBuf,'\0',RECV_BUF_LEN);
        if(!mirror_start)
        {
            FD_ZERO(&readfds);
    		FD_SET(sockfd,&readfds);
    		timeout.tv_sec = 0;
    		timeout.tv_usec = 10000;
    		select(sockfd+1,&readfds,NULL,NULL,&timeout);
            
            if(FD_ISSET(sockfd,&readfds))
            {
                recv_sock = accept(sockfd, (struct sockaddr*)&remote_addr, &len);
                if (recv_sock < 0)
                {
                    printf("ERROR at %s[%d]\n", __FUNCTION__, __LINE__);
                }
                else
                {
                    int optval = 1;
                    int nRecvBuf = 512 * 1024; 
                    setsockopt(recv_sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval) );
                    setsockopt(recv_sock, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(nRecvBuf));
                    recv_size = recv(recv_sock,recvBuf,RECV_BUF_LEN,0);
                }
                if (recv_size < 0) 
                {
                    printf ("###could not read TCP start!!\n");
                }else
                {
                    printf("###Received data form %s : %d\n", inet_ntoa(remote_addr.sin_addr), htons(remote_addr.sin_port));
                    /*for(int i = 0; i < recv_size; i++)
                        printf("0x%x\n", recvBuf[i]);
                    printf("\n");*/

            	    if(recvBuf[0] == 0xaa && recvBuf[1] == 0xbb && recvBuf[2] == 0xcc && recvBuf[3] == 0x4)
            	    {
                	    ori_w = recvBuf[12] | (recvBuf[13] << 8);
                        ori_h = recvBuf[14] | (recvBuf[15] << 8);
                        enc_w = ori_w / 2;
                        enc_h = ori_h / 2;
                            /***ITE decode only support 16 alignment***/
                            if (enc_w % 16 != 0)
                				enc_w = enc_w - (enc_w % 16);
                            if (enc_h % 16 != 0)
                				enc_h = enc_h - (enc_h % 16);
                            
                        message[2] = enc_w & 0xFF;
                        message[3] = (enc_w & 0xFF00) >> 8;
                        message[4] = enc_h & 0xFF;
                        message[5] = (enc_h & 0xFF00) >> 8;
                        message[6] = enc_h & 0xFF;
                        message[7] = (enc_h & 0xFF00) >> 8;
                        message[8] = enc_w & 0xFF;
                        message[9] = (enc_w & 0xFF00) >> 8;
                        if (send(recv_sock, message, sizeof(message), 0) < 0) 
                        {
                            printf("Could not send OK!!\n");
                        }
                        mirror_start = true;
                        recv_size -= 16;
                        check_tick = itpGetTickCount();
                        printf("====>network_mirror_start\n");
                    }
                    if(recv_size > 0)
                    {
                        memcpy(tmpBuf, recvBuf + 16, recv_size - 16);
                        w_idx += (recv_size - 16);
                        w_idx = w_idx % TMP_BUF_LEN;
                        recv_size = 0;
                    }
                }
            }
            continue;
        }
		else
        {
            //FD_ZERO(&readfds);
    		//FD_SET(recv_sock,&readfds);
    		//timeout.tv_sec = 0;
    		//timeout.tv_usec = 10000;
    		//select(recv_sock+1,&readfds,NULL,NULL,&timeout);

            //if(FD_ISSET(recv_sock,&readfds))
            if(recv_size == 0)
            {
                FD_ZERO(&readfds);
        		FD_SET(recv_sock,&readfds);
        		timeout.tv_sec = 0;
        		timeout.tv_usec = 500;
        		select(recv_sock+1,&readfds,NULL,NULL,&timeout);

                if(FD_ISSET(recv_sock,&readfds))
            {
                recv_size = recv(recv_sock,recvBuf,RECV_BUF_LEN,0);
                parse_size = 0;
                //printf("====>network_mirror size=%d, data<=========\n", recv_size);
                //for(int i = 0; i<recv_size; i++)
                //{
                //    printf("0x%x,",recvBuf[i]);
                //}
                //printf("\n");
            }
		}
		}
        
        if(recv_size != -1 && recv_size > 0)
        {
            check_tick = itpGetTickCount();
            if(w_idx >= r_idx)
            {
                if((TMP_BUF_LEN - w_idx) + r_idx >= recv_size)
                {
                    if(TMP_BUF_LEN - w_idx >= recv_size)
                    {
                        memcpy(tmpBuf + w_idx, recvBuf, recv_size);
                        w_idx += recv_size;
                        if(w_idx >= TMP_BUF_LEN)
                            w_idx = w_idx - TMP_BUF_LEN;
                        recv_size = 0;
                        //printf("ccccccccc111(%d, %d, %d)\n", recv_size, w_idx, r_idx);
                    }else
                    {
                        memcpy(tmpBuf + w_idx, recvBuf, TMP_BUF_LEN - w_idx);
                        memcpy(tmpBuf, recvBuf + (TMP_BUF_LEN - w_idx), recv_size - (TMP_BUF_LEN - w_idx));
                        w_idx = recv_size - (TMP_BUF_LEN - w_idx);
                        if(w_idx >= TMP_BUF_LEN)
                            w_idx = w_idx - TMP_BUF_LEN;
                        recv_size = 0;
                        //printf("ccccccccc222(%d, %d)\n", w_idx, r_idx);
                    }
                }else
                    printf("###111TMP_BUF full(remain=%d, need=%d)\n", (TMP_BUF_LEN - w_idx) + r_idx, recv_size);
            }else
            {
                if((r_idx - w_idx) >= recv_size)
                {
                    memcpy(tmpBuf + w_idx, recvBuf, recv_size);
                    w_idx += recv_size;
                    if(w_idx >= TMP_BUF_LEN)
                        w_idx = w_idx - TMP_BUF_LEN;
                    recv_size = 0;
                    //printf("ccccccccc333(%d, %d)\n", w_idx, r_idx);
                }else
                    printf("###222TMP_BUF full(remain=%d, need=%d)\n", r_idx - w_idx, recv_size);
            }
            //recv_size = 0;
        }//else
        //    usleep(1000);

        if(w_idx >= r_idx)
        {
            //printf("ccccccccc666(%d, %d)\n", w_idx, r_idx);
            parse_size = w_idx - r_idx;
            err_num = 0;
        }else
        {
            //printf("ccccccccc777(%d, %d)\n", w_idx, r_idx);
            parse_size = TMP_BUF_LEN - r_idx + w_idx;
            err_num = 0;
        }
NEXT_NAL:        
        if(parse_size > 16)
        {
            if(TMP_BUF_LEN - r_idx < 16)
            {  
                memcpy(buf_end_data, tmpBuf + r_idx, TMP_BUF_LEN - r_idx);
                memcpy(buf_end_data + (TMP_BUF_LEN - r_idx), tmpBuf, 16 - (TMP_BUF_LEN - r_idx));
                if(buf_end_data[0] == 0xaa && buf_end_data[1] == 0xbb && buf_end_data[2] == 0xcc && buf_end_data[3] == 0x6)
                {
                    ori_w = buf_end_data[12] | (buf_end_data[13] << 8);
                    ori_h = buf_end_data[14] | (buf_end_data[15] << 8);
                    printf("====>ORI W_H info =(%d, %d)<=========\n", ori_w, ori_h);
                    if(ori_w > ori_h)
                        mirror_from_H = true;
                    else
                        mirror_from_H = false;
                    r_idx = 16 - (TMP_BUF_LEN - r_idx);
                }else if(buf_end_data[0] == 0xaa && buf_end_data[1] == 0xbb && buf_end_data[2] == 0xcc && buf_end_data[3] == 0x0)
                {
                    //NAL size from header
                    head_nal_size = (buf_end_data[11] << 24) | (buf_end_data[10] << 16) | (buf_end_data[9] << 8) | buf_end_data[8];
                    head_nal_size -= 4;
                    //printf("head nal time =%d(%x, %x, %x, %x)\n", (recvBuf[15] << 24) | (recvBuf[14] << 16) | (recvBuf[13] << 8) | recvBuf[12], recvBuf[15], recvBuf[14], recvBuf[13], recvBuf[12]);
                    //fwrite(recvBuf+13, 1, recv_size-13, f);
                    //k++;
                    if(parse_size >= (head_nal_size + 16))
                    {
                        r_idx = 16 - (TMP_BUF_LEN - r_idx);
                            
                        nalBuf = (uint8_t*) malloc(NAL_BUF_LEN);
                        //memset(nalBuf,'\0',NAL_BUF_LEN);
                        
                        memcpy(nalBuf, tmpBuf + r_idx, head_nal_size);
                        recv_nal_size+=head_nal_size;
                        r_idx += head_nal_size;
                        if(r_idx >= TMP_BUF_LEN)
                            r_idx = r_idx - TMP_BUF_LEN;
                        //printf("ccccccccc8(%d)\n", recv_nal_size);
#ifdef SHOW_BITRATE     

                        if((nalBuf[4]&0x0F) == 0x5 && sec_size != 0)            
                        {
                            if(first_Iframe_get)
                                first_Iframe_get = false;
                            else
                            {
                                //printf("+++++++++++---(%d, %d, %d)\n", sec_size, frame_cnt, itpGetTickDuration(nal_tick));
                                printf("++++++++++++++(TCP=0x%x)bitrate=%f Mbps(%dms), FPS=%d\n", nalBuf[4], ((double)(sec_size*8*1000/(1024*1024))/itpGetTickDuration(nal_tick)), itpGetTickDuration(nal_tick), frame_cnt * 1000/itpGetTickDuration(nal_tick));                    
                            }
                            nal_tick = itpGetTickCount();  
                            sec_size = 0;            
                            frame_cnt = 0;
                        }
                        sec_size += head_nal_size;
                        frame_cnt++;
#endif                        
                    }//else
                    //    usleep(1000);
                }else
                {
                    printf(">>>>>>>>111>>>>>>>>DATA ERROR JUMP=%d(%x, %x, %x)!!!!!!!!!!!!!!!!!!!\n", parse_size, buf_end_data[0], buf_end_data[1], buf_end_data[2]);
                    r_idx++;
                    if(r_idx >= TMP_BUF_LEN)
                        r_idx = r_idx - TMP_BUF_LEN;
                    parse_size -= 1;
                    err_num++;
                    if(err_num >= 16)
                    {
                        //err_num = 0;
                        continue;
                    }
                    else
                        goto NEXT_NAL;
                }
            }
            else if(tmpBuf[r_idx] == 0xaa && tmpBuf[r_idx + 1] == 0xbb && tmpBuf[r_idx + 2] == 0xcc && tmpBuf[r_idx + 3] == 0x6)
            {
                //printf("====>ORI W_H info =%d, data<=========\n", recv_size);
                //for(int i = 0; i<recv_size; i++)
                //{
                //    printf("0x%x,",recvBuf[i]);
                //}
                //printf("\n");
                ori_w = tmpBuf[r_idx + 12] | (tmpBuf[r_idx + 13] << 8);
                ori_h = tmpBuf[r_idx + 14] | (tmpBuf[r_idx + 15] << 8);
                printf("====>ORI W_H info =(%d, %d)<=========\n", ori_w, ori_h);
                if(ori_w > ori_h)
                    mirror_from_H = true;
                else
                    mirror_from_H = false;
                r_idx += 16;
                if(r_idx >= TMP_BUF_LEN)
                    r_idx = r_idx - TMP_BUF_LEN;
                continue;
                //if(parse_size - 16 > 16)
                //{
                //    parse_size -= 16;
                //    goto NEXT_NAL;
                //}
            }
            else if(tmpBuf[r_idx] == 0xaa && tmpBuf[r_idx + 1] == 0xbb && tmpBuf[r_idx + 2] == 0xcc && tmpBuf[r_idx + 3] == 0x0)
            {
                //printf("ccccccccc555(%d, %d)\n", w_idx, r_idx);
                //NAL size from header
                head_nal_size = (tmpBuf[r_idx + 11] << 24) | (tmpBuf[r_idx + 10] << 16) | (tmpBuf[r_idx + 9] << 8) | tmpBuf[r_idx + 8];
                head_nal_size -= 4;
                //printf("head nal time =%d(%x, %x, %x, %x)\n", (recvBuf[15] << 24) | (recvBuf[14] << 16) | (recvBuf[13] << 8) | recvBuf[12], recvBuf[15], recvBuf[14], recvBuf[13], recvBuf[12]);
                //fwrite(recvBuf+13, 1, recv_size-13, f);
                //k++;
                if(parse_size >= (head_nal_size + 16))
                {
                    nalBuf = (uint8_t*) malloc(NAL_BUF_LEN);
                    //memset(nalBuf,'\0',NAL_BUF_LEN);
                    if((r_idx + 16 + head_nal_size) > TMP_BUF_LEN)
                    {
                        memcpy(nalBuf, tmpBuf + r_idx + 16, TMP_BUF_LEN - (r_idx + 16));
                        memcpy(nalBuf + (TMP_BUF_LEN - (r_idx + 16)), tmpBuf, head_nal_size - (TMP_BUF_LEN - (r_idx + 16)));
                        recv_nal_size+=head_nal_size;
                        r_idx = head_nal_size - (TMP_BUF_LEN - (r_idx + 16));
                        if(r_idx >= TMP_BUF_LEN)
                            r_idx = r_idx - TMP_BUF_LEN;
                    }else
                    {
                        memcpy(nalBuf, tmpBuf + r_idx + 16, head_nal_size);
                        recv_nal_size+=head_nal_size;
                        r_idx += (head_nal_size + 16);
                        if(r_idx >= TMP_BUF_LEN)
                            r_idx = r_idx - TMP_BUF_LEN;
                    }
                    //printf("ccccccccc8(%d)\n", recv_nal_size);
#ifdef SHOW_BITRATE                         
                    if((nalBuf[4]&0x0F) == 0x5 && sec_size != 0)            
                    {
                        if(first_Iframe_get)
                            first_Iframe_get = false;
                        else
                        {
                            //printf("+++++++++++---(%d, %d, %d)\n", sec_size, frame_cnt, itpGetTickDuration(nal_tick));
                            printf("++++++++++++++(TCP=0x%x)bitrate=%f Mbps(%dms), FPS=%d\n", nalBuf[4], ((double)(sec_size*8*1000/(1024*1024))/itpGetTickDuration(nal_tick)), itpGetTickDuration(nal_tick), frame_cnt * 1000/itpGetTickDuration(nal_tick));                    
                        }                  
                        nal_tick = itpGetTickCount();  
                        sec_size = 0;            
                        frame_cnt = 0;
                    }
                    sec_size += head_nal_size;
                    frame_cnt++;
#endif 
                }//else
                //    usleep(1000);
            }else
            {
                printf(">>>>>>>222>>>>>>>>>DATA ERROR JUMP=%d(%x, %x, %x, %x)!!!!!!!!!!!!!!!!!!!\n", parse_size, tmpBuf[r_idx], tmpBuf[r_idx + 1], tmpBuf[r_idx + 2], tmpBuf[r_idx + 3]);
                r_idx++;
                if(r_idx >= TMP_BUF_LEN)
                    r_idx = r_idx - TMP_BUF_LEN;
                parse_size -= 1;
                err_num++;
                if(err_num >= 16)
                {
                    //err_num = 0;
                    continue;
                }
                else
                    goto NEXT_NAL;
            }

            if(recv_nal_size != 0 && recv_nal_size == head_nal_size)
            {
                //printf("====>NAL=%d, data<=========\n", recv_nal_size);
                //for(int i = 0; i<recv_nal_size; i++)
                //{
                //   printf("0x%x,",nalBuf[i]);
                //}
                //printf("\n");
                
                //fwrite(nalBuf, 1, recv_nal_size, f);
                //k++;
                if(itpGetTickDuration(lastTick) > 100)
                {
                    printf("#########no video too long(%dms)##################################!!!\n", itpGetTickDuration(lastTick));    
                }   
                lastTick = itpGetTickCount();
                while(mirror_video_nal[mirror_video_w_idx].new_nal == true)
                {
                    usleep(1000);
                }
                if(mirror_video_nal[mirror_video_w_idx].new_nal != true)
                {
                    mirror_video_nal[mirror_video_w_idx].data = nalBuf;
                    mirror_video_nal[mirror_video_w_idx].size = recv_nal_size;
                    mirror_video_nal[mirror_video_w_idx].new_nal = true;
                    mirror_video_w_idx++;
                    mirror_video_w_idx = mirror_video_w_idx%MIRROR_VIDEO_NUM;
                    nalBuf = NULL;
                }else
                {
                    printf("NAL data Q not empty!!!\n");
                    if(nalBuf)
                        free(nalBuf);
                }
                recv_nal_size = 0;
                //if(parse_size - (head_nal_size + 16) > 16)
                //{
                //    parse_size -= (head_nal_size + 16);
                    //usleep(2000);
                //    goto NEXT_NAL;
                //}else
                    usleep(1000);
                //continue;
            }
            //if(k > 300)
            //{
            //    printf("STOOPPPPPPPPPPPPPPPPPP\n");
            //    fclose(f);
            //    break;
            //}
        }else
            usleep(1000);
    }

    if(recvBuf)
        free(recvBuf);
    if(nalBuf)
        free(nalBuf);
    if(tmpBuf)
        free(tmpBuf);
    if(mirror_start)
        close(recv_sock);
    if(sockfd)
        close(sockfd);
    //printf("STOOPPPPPPPPPPPPPPPPPP\n");
    //fclose(f);
    
    mirror_is_init = false;
    //pthread_exit(NULL);
   
    return NULL;
}
#else
static void* udp_mirror_task(void* arg)
{
    FILE* f = NULL;
    int k = 0;
    uint8_t  message[13] = {0x4F, 0x4B, 0x68, 0x01, 0xBC, 0x02, 0xBC, 0x02, 0x68, 0x01, 0x1E, 0xB8, 0x0B}; /*{OK,V_W,V_H,H_W,H_H,FPS,bps}*/
    //uint8_t  message[2] = {0x4F, 0x4B}; //return OK to APP
    uint8_t  frame_cnt = 0;
    uint8_t* recvBuf = (uint8_t*) malloc(RECV_BUF_LEN);
    uint32_t nal_tick = 0, sec_size = 0, recv_size = 0, head_nal_size = 0, recv_nal_size = 0, ori_w = 0, ori_h = 0, enc_w = 0, enc_h = 0;
    bool     got_nal = false, mirror_start = false, first_Iframe_get = true;
    fd_set readfds;
    struct timeval timeout = {0};
    struct sockaddr_in remote_addr;
    int len = sizeof(struct sockaddr_in);    

    mirror_video_w_idx = 0;

    //memset(recvBuf,'\0',RECV_BUF_LEN);
    //memset(nalBuf,'\0',NAL_BUF_LEN);
    //f = fopen("E:/test111.264", "wb");
    while(network_mirror_start)
    {
        if(itpGetTickDuration(lastTick) > 2000)
        {
            //printf("#########no video input!!!\n");    
            mirror_start = false;
        }
        FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 5000;
		select(sockfd+1,&readfds,NULL,NULL,&timeout); 

        memset(recvBuf,'\0',RECV_BUF_LEN);
        if(!mirror_start)
        {
            if(FD_ISSET(sockfd,&readfds))
            {
                recv_size = recvfrom(sockfd, recvBuf, RECV_BUF_LEN, 0, (struct sockaddr*)&remote_addr, (socklen_t *)&len);
                if (recv_size < 0) 
                {
                    printf ("###could not read UDP start!!\n");
                }else
                {
                    printf("###Received data form %s : %d\n", inet_ntoa(remote_addr.sin_addr), htons(remote_addr.sin_port));
                    /*for(int i = 0; i < recv_size; i++)
                        printf("0x%x\n", recvBuf[i]);
                    printf("\n");*/

                    //remote_addr.sin_family = AF_INET;
            	    //remote_addr.sin_port = htons(11111);
            	    //remote_addr.sin_addr.s_addr = inet_addr("192.168.43.1");
            	    if(recvBuf[0] == 0xaa && recvBuf[1] == 0xbb && recvBuf[2] == 0xcc && recvBuf[3] == 0x4)
            	    {
            	    ori_w = recvBuf[12] | (recvBuf[13] << 8);
                    ori_h = recvBuf[14] | (recvBuf[15] << 8);
                    enc_w = ori_w / 2;
                    enc_h = ori_h / 2;
                        /***ITE decode only support 16 alignment***/
                        if (enc_w % 16 != 0)
            				enc_w = enc_w - (enc_w % 16);
                        if (enc_h % 16 != 0)
            				enc_h = enc_h - (enc_h % 16);
                        
                    message[2] = enc_w & 0xFF;
                    message[3] = (enc_w & 0xFF00) >> 8;
                    message[4] = enc_h & 0xFF;
                    message[5] = (enc_h & 0xFF00) >> 8;
                    message[6] = enc_h & 0xFF;
                    message[7] = (enc_h & 0xFF00) >> 8;
                    message[8] = enc_w & 0xFF;
                    message[9] = (enc_w & 0xFF00) >> 8;
                    if (sendto(sockfd, message, sizeof(message), 0, (struct sockaddr*)&remote_addr, len) < 0) 
                    {
                        printf("Could not send OK!!\n");
                    }
                    mirror_start = true;
                    recv_size = 0;
                    lastTick = itpGetTickCount();
                    printf("====>network_mirror_start\n");
                }
                    else if(recvBuf[0] == 0xaa && recvBuf[1] == 0xbb && recvBuf[2] == 0xcc && (recvBuf[3] == 0x0 || recvBuf[3] == 0x6))
                    {
                        mirror_start = true;
                        recv_size = 0;
                        lastTick = itpGetTickCount();
                    }
                }
            }
            continue;
        }
		if(FD_ISSET(sockfd,&readfds))
        {
            recv_size = recv(sockfd, recvBuf, RECV_BUF_LEN, 0);
            lastTick = itpGetTickCount();
            //printf("====>network_mirror size=%d, data<=========\n", recv_size);
            //for(int i = 0; i<recv_size; i++)
            //{
            //    printf("0x%x,",recvBuf[i]);
            //}
            //printf("\n");
            if(recvBuf[0] == 0xaa && recvBuf[1] == 0xbb && recvBuf[2] == 0xcc && recvBuf[3] == 0x6)
        {
                //printf("====>ORI W_H info =%d, data<=========\n", recv_size);
                //for(int i = 0; i<recv_size; i++)
                //{
                //    printf("0x%x,",recvBuf[i]);
                //}
                printf("\n");
                ori_w = recvBuf[12] | (recvBuf[13] << 8);
                ori_h = recvBuf[14] | (recvBuf[15] << 8);
                if(ori_w > ori_h)
                    mirror_from_H = true;
                else
                    mirror_from_H = false;
                recv_size = 0;
            }
		}
        if(recv_size > 0)
        {
            if(recvBuf[16] == 0x0 && recvBuf[17] == 0x0 && recvBuf[18] == 0x0 && recvBuf[19] == 0x1)
            {
                //if(recvBuf[12] == 0x1)
                //    mirror_from_H = true;
                //else
                //    mirror_from_H = false;
                if(recv_nal_size != 0)
                {
                    printf("XXXXXX1\n");
                    recv_nal_size = 0;
                    if(nalBuf)
                    {
                        free(nalBuf);
                        nalBuf = NULL;
                    }
                }
                nalBuf = (uint8_t*) malloc(NAL_BUF_LEN);
                
                //NAL size from header
                head_nal_size = (recvBuf[11] << 24) | (recvBuf[10] << 16) | (recvBuf[9] << 8) | recvBuf[8];
                head_nal_size -= 4;
                //printf("head nal time =%d(%x, %x, %x, %x)\n", (recvBuf[15] << 24) | (recvBuf[14] << 16) | (recvBuf[13] << 8) | recvBuf[12], recvBuf[15], recvBuf[14], recvBuf[13], recvBuf[12]);
                //fwrite(recvBuf+13, 1, recv_size-13, f);
                //k++;
                memcpy(nalBuf, recvBuf+16, recv_size-16);
                recv_nal_size+=recv_size-16;
#ifdef SHOW_BITRATE     
                if((nalBuf[4]&0x0F) == 0x5 && sec_size != 0)            
                {
                    if(first_Iframe_get)
                        first_Iframe_get = false;
                    else
                    {
                        //printf("+++++++++++---(%d, %d, %d)\n", sec_size, frame_cnt, itpGetTickDuration(nal_tick));
                        printf("++++++++++++++(UDP=0x%x)bitrate=%f Mbps(%dms), FPS=%d\n", nalBuf[4], ((double)(sec_size*8*1000/(1024*1024))/itpGetTickDuration(nal_tick)), itpGetTickDuration(nal_tick), frame_cnt * 1000/itpGetTickDuration(nal_tick));                    
                    }
                    nal_tick = itpGetTickCount();  
                    sec_size = 0;            
                    frame_cnt = 0;
                }
                sec_size += (recv_size-16);
                frame_cnt++;
#endif                 
            }else
            {
                if(recv_nal_size != 0)
                {
                    //fwrite(recvBuf, 1, recv_size, f);
                    //k++;
                    memcpy(nalBuf + recv_nal_size, recvBuf, recv_size);
                    recv_nal_size += recv_size;
#ifdef SHOW_BITRATE
                    sec_size += recv_size;
#endif
                }
                else
                    printf("XXXXXX2\n");
            }
            if(recv_nal_size == head_nal_size)
            {
                //printf("====>NAL=%d, data<=========\n", recv_nal_size);
                //for(int i = 0; i<recv_nal_size; i++)
                //{
                 //   printf("0x%x,",nalBuf[i]);
                //}
                //printf("\n");
                
                //fwrite(nalBuf, 1, recv_nal_size, f);
                //k++;
                while(mirror_video_nal[mirror_video_w_idx].new_nal == true)
                {
                    usleep(1000);
                }
                mirror_video_nal[mirror_video_w_idx].data = nalBuf;
                mirror_video_nal[mirror_video_w_idx].size = recv_nal_size;
                mirror_video_nal[mirror_video_w_idx].new_nal = true;
                mirror_video_w_idx = ++mirror_video_w_idx%MIRROR_VIDEO_NUM;
                nalBuf = NULL;

                recv_nal_size = 0;
            }
            recv_size = 0;
            //if(k > 3000)
            //{
            //    printf("STOOPPPPPPPPPPPPPPPPPP\n");
            //    fclose(f);
            //    break;
            //}
        }
        
        usleep(1000);
    }

    if(recvBuf)
        free(recvBuf);
    if(nalBuf)
        free(nalBuf);
    //if(sockfd)
    //    close(sockfd);
    //printf("STOOPPPPPPPPPPPPPPPPPP\n");
    //fclose(f);
    
    mirror_is_init = false;
    //pthread_exit(NULL);
   
    return NULL;
}
#endif

void network_mirror_init(void)
{
    if(mirror_is_init)
    {
        //network_mirror_start();
        return 0;
    }
    
    mirror_is_init = true;
    while(backup_recving)
        usleep(1000);

    network_mirror_start = true;
    
    struct sockaddr_in myaddr;
    int reuse = 1;
    int nRecvBuf = 1 * 1024 * 1024;       //接收緩存設定成32k,避免掉封包

    struct sockaddr_in addr;

#ifdef TCP_MIRROR_FLOW
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        printf("Fail to create a socket.\n");
        return 0;
    }

    struct timeval timeout={0,1000};//1000us
    setsockopt(sockfd, SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
    
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(nRecvBuf));
    
    memset(&addr,'\0',sizeof(struct sockaddr_in));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(11111);
    if( bind (sockfd,(struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0 ){
        printf("[NetworkCallInit]bind error!\n");      
    }
    else
        printf("[NetworkCallInit]bind ok! \n");

    if (listen(sockfd, 10) < 0)
    {
        printf("[NetworkCallInit]listen error! \n");
    }
#else   
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1){
        printf("Fail to create a socket.\n");
        return 0;
    }

    struct timeval timeout={0,1000};//1000us
    setsockopt(sockfd, SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
    
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(nRecvBuf));
    
    //socket link
    memset(&myaddr,'\0',sizeof(struct sockaddr_in));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(11111);
    
	if( bind (sockfd,(struct sockaddr *)&myaddr, sizeof(struct sockaddr_in)) < 0 ){
		printf("udprecv bind udp  error!\n"); 
        return 0;
	}
	else
		printf("udprecv bind udp ok! \n");
#endif
    {
        //mirror_is_init = true;
        //network_mirror_start = true;
        window_state = 0;
        pthread_attr_t attr, attr_decode, attr_backup;
	    struct sched_param param;

        pthread_attr_init(&attr_decode);
        pthread_attr_setdetachstate(&attr_decode, PTHREAD_CREATE_DETACHED);
        pthread_create(&decode_thread, &attr_decode, stream_decode_task, NULL);
        
#ifdef TCP_MIRROR_FLOW
        pthread_attr_init(&attr);
    	param.sched_priority = sched_get_priority_min(1) + 2;
	    pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&recv_thread, &attr, tcp_mirror_task, NULL);
#else        
        pthread_attr_init(&attr);
    	param.sched_priority = sched_get_priority_min(1) + 2;
	    pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&recv_thread, &attr, udp_mirror_task, NULL);
            
        if(!backup_is_init)
        {
            pthread_attr_init(&attr_backup);
            pthread_attr_setdetachstate(&attr_backup, PTHREAD_CREATE_DETACHED);
            pthread_create(&backup_recv_thread, &attr_backup, udp_backup_task, NULL);
        }
#endif
        printf("====>network_mirror_init\n\n");
    }
}

void network_mirror_stop(void)
{
    //mirror_is_init = false;
    network_mirror_start = false;
}

