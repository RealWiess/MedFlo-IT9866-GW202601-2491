#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#ifdef CFG_BUILD_FFMPEG
    #include "libavcodec/avcodec.h"
    #include "libavcodec/get_bits.h"
    #include "libavcodec/golomb.h"
    #include "libavformat/avformat.h"
#endif

//=============================================================================
//                              Constant Definition
//=============================================================================
#define FILE_MODE         0
#define VIDEO_BUF_SIZE    10 * 1024
#define AUDIO_BUF_SIZE    2048
#define INPUT_BUFFER_SIZE 512000

static Call_info * call_list     = NULL;
static char        start_code[4] = {0x00, 0x00, 0x01, 0xfc};

typedef struct _SenderData
{
    udp_config_t udp_conf;
    bool         run_flag;
} SenderData;

typedef struct _ReceiverData
{
    udp_config_t udp_conf;
    bool         run_flag;
} ReceiverData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
static void udp_sender_set_para (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SenderData *       d        = (SenderData *)f->data;
    udp_config_t *     udp_conf = (udp_config_t *)arg;
    struct sockaddr_in addr;
    memset(&(d->udp_conf), '\0', sizeof(udp_config_t));
    d->udp_conf.c_type = udp_conf->c_type;
    memcpy(d->udp_conf.group_ip, udp_conf->group_ip, 16);
    if (d->udp_conf.c_type == VIDEO_OUTPUT)
    {
        call_list = (Call_info *)udp_conf->remote_ip;
    }
    else if (d->udp_conf.c_type == AUDIO_OUTPUT)
    {
        d->udp_conf.remote_ip = udp_conf->remote_ip;
        // printf("++++++++ %s, %d\n", (char *)d->udp_conf.remote_ip,
        // d->udp_conf.remote_port);
    }
    d->udp_conf.remote_port = udp_conf->remote_port;
    d->udp_conf.cur_socket  = udp_conf->cur_socket;

    if (d->udp_conf.cur_socket == -1)
    {
        d->udp_conf.cur_socket = lwip_socket(AF_INET, SOCK_DGRAM, 0);
        if (d->udp_conf.cur_socket < 0)
        {
            printf("[udpsend_set_para]sockek create failure\n");
        }
        else
        {
            printf("[udpsend_set_para]udp socket create OK!\n");
        }
    }
}

static void udp_sender_get_socket (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SenderData * d  = (SenderData *)f->data;
    ((int *)arg)[0] = d->udp_conf.cur_socket;
}

static void audio_send (unsigned char * data, unsigned int len_size,
                        SenderData * send_data, struct sockaddr_in * addr,
                        int addr_len)
{
    const struct sockaddr * to = (struct sockaddr *)addr;

    // printf("audio_send %d\n", len_size);
    lwip_sendto(send_data->udp_conf.cur_socket, data, len_size, 0, to,
                addr_len);
    usleep(5000);
}

static void video_send (unsigned char * data, unsigned int len_size,
                        SenderData * send_data, struct sockaddr_in * addr,
                        int addr_len)
{
    int                     send_size = 0, remain_size = len_size;
    unsigned char *         tmp_buf = malloc(sizeof(char) * VIDEO_BUF_SIZE);
    const struct sockaddr * to      = (struct sockaddr *)addr;

    while (remain_size > 0)
    {
        if (send_size == 0)
        {
            memcpy(&tmp_buf[0], start_code, 4);
            tmp_buf[4] = (remain_size >> 24) & 0xFF;
            tmp_buf[5] = (remain_size >> 16) & 0xFF;
            tmp_buf[6] = (remain_size >> 8) & 0xFF;
            tmp_buf[7] = remain_size & 0xFF;
            //*(uint32_t *)(&tmp_buf[4]) = remain_size;
            if (remain_size > VIDEO_BUF_SIZE - 8)
            {
                memcpy(&tmp_buf[8], &data[send_size], VIDEO_BUF_SIZE - 8);
                lwip_sendto(send_data->udp_conf.cur_socket, tmp_buf,
                            VIDEO_BUF_SIZE, 0, to, addr_len);
                remain_size -= (VIDEO_BUF_SIZE - 8);
                send_size += (VIDEO_BUF_SIZE - 8);
            }
            else
            {
                memcpy(&tmp_buf[8], &data[send_size], remain_size);
                lwip_sendto(send_data->udp_conf.cur_socket, tmp_buf,
                            remain_size + 8, 0, to, addr_len);
                break;
            }
        }
        usleep(2000);
        if (remain_size > VIDEO_BUF_SIZE)
        {
            lwip_sendto(send_data->udp_conf.cur_socket, &data[send_size],
                        VIDEO_BUF_SIZE, 0, to, addr_len);
            remain_size -= VIDEO_BUF_SIZE;
            send_size += VIDEO_BUF_SIZE;
        }
        else
        {
            lwip_sendto(send_data->udp_conf.cur_socket, &data[send_size],
                        remain_size, 0, to, addr_len);
            remain_size = 0;
        }
    }
    free(tmp_buf);
}

static void f_sender_init (IteFilter * f)
{
    printf("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SenderData * d = (SenderData *)ite_new(SenderData, 1);
    f->data        = d;
}

static void f_sender_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SenderData * d = (SenderData *)(f->data);
    if (d != NULL)
    {
        close(d->udp_conf.cur_socket);
        printf("[sender_uninit]socket close OK!\n");
        free(d);
    }
}

static void f_sender_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SenderData *       d = (SenderData *)f->data;
    IteQueueblk        blk;
    mblk_ite *         m = NULL;
    struct sockaddr_in addr;
    int                addr_len = sizeof(struct sockaddr_in);
    void (*data_send)(unsigned char * data, unsigned int len_size,
                      SenderData * send_data, struct sockaddr_in * addr,
                      int addr_len);
    switch (d->udp_conf.c_type)
    {
        case AUDIO_OUTPUT:
            data_send = audio_send;
            break;
        case VIDEO_OUTPUT:
            data_send = video_send;
            break;
    }
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(d->udp_conf.remote_port);
    // addr.sin_addr.s_addr = inet_addr(d->udp_conf.remote_ip);
    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            m = (mblk_ite *)blk.datap;
            if (d->udp_conf.c_type == VIDEO_OUTPUT)
            {
                for (int i = 0; i < max_call_num; i++)
                {
                    if (call_list[i].call_start)
                    {
                        addr.sin_addr.s_addr = inet_addr(call_list[i].call_ip);
                        data_send(m->b_rptr, m->b_wptr - m->b_rptr, d, &addr,
                                  addr_len);
                        usleep(1000);
                    }
                }
            }
            else if (d->udp_conf.c_type == AUDIO_OUTPUT)
            {
                addr.sin_addr.s_addr = inet_addr((char *)d->udp_conf.remote_ip);
                data_send(m->b_rptr, m->b_wptr - m->b_rptr, d, &addr, addr_len);
                usleep(1000);
            }
            freemsg_ite(m);
            m = NULL;
            // usleep(1000);
        }
        usleep(1000);
    }
    // ite_mblk_queue_flush(f->input[0].Qhandle);
    return;
}

static IteMethodDes udp_send_methods[] = {
    {  ITE_FILTER_UDP_SEND_SET_PARA,   udp_sender_set_para},
    {ITE_FILTER_UDP_SEND_GET_SOCKET, udp_sender_get_socket},
    {                             0,                  NULL}
};

// clang-format off
IteFilterDes FilterUDPsend = {
    ITE_FILTER_UDP_SEND_ID,
    f_sender_init,
    f_sender_uninit,
    f_sender_process,
    udp_send_methods
};
// clang-format on

static void * video_receive_thread (IteFilter * f)
{
    ReceiverData *     d = (ReceiverData *)f->data;
    struct sockaddr_in remote_addr;
    mblk_ite *         m           = NULL;
    IteQueueblk        blk_output0 = {0};
    IteQueueblk        blk_output1 = {0};
    int                len         = sizeof(struct sockaddr_in);
    int                ret         = 0;
    unsigned char *    buf_ptr     = NULL;
    unsigned int       frame_size = 0, frame_size_count = 0, blk_len_tmp = 0;
    unsigned char *    buf        = NULL;
    bool               start_recv = false;
    fd_set             readfds;
    struct timeval     timeout = {0};

    buf                        = malloc(sizeof(char) * VIDEO_BUF_SIZE);
    while (f->run)
    {
        FD_ZERO(&readfds);
        FD_SET(d->udp_conf.cur_socket, &readfds);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 5000;
        lwip_select(d->udp_conf.cur_socket + 1, &readfds, NULL, NULL, &timeout);

        memset(buf, '\0', VIDEO_BUF_SIZE);
        if (FD_ISSET(d->udp_conf.cur_socket, &readfds))
        {
            ret = lwip_recvfrom(d->udp_conf.cur_socket, buf, VIDEO_BUF_SIZE, 0,
                                (struct sockaddr *)&remote_addr, &len);
            buf_ptr = buf;
            // printf("receiver=%d\n", ret);
            if (ret > 0)
            {
                if (memcmp(buf_ptr, start_code, 4) == 0)
                {
                    if (m)
                    {
                        freemsg_ite(m);
                        m = NULL;
                    }

                    frame_size = frame_size_count = blk_len_tmp = 0;
                    frame_size = (buf_ptr[4] << 24) | (buf_ptr[5] << 16) |
                                 (buf_ptr[6] << 8) | buf_ptr[7];
                    // printf("get frame=%d\n", frame_size);
                    ret -= 8;
                    buf_ptr += 8;
                    if (ret < 0)
                    {
                        start_recv = false;
                    }
                    else
                    {
                        start_recv = true;
                    }
                }
                if (start_recv)
                {
                    blk_len_tmp = ret;
                    if (m == NULL)
                    {
                        m = allocb_ite(frame_size);
                        memcpy(m->b_wptr, buf_ptr, blk_len_tmp);
                        m->b_wptr += blk_len_tmp;
                        frame_size_count += blk_len_tmp;
                    }
                    else
                    {
                        if (frame_size_count + blk_len_tmp > frame_size)
                        {
                            if (m)
                            {
                                freemsg_ite(m);
                                m = NULL;
                            }

                            start_recv = false;
                            frame_size = frame_size_count = blk_len_tmp = 0;
                            continue;
                        }

                        memcpy(m->b_wptr, buf_ptr, blk_len_tmp);
                        m->b_wptr += blk_len_tmp;
                        frame_size_count += blk_len_tmp;
                    }
                    buf_ptr += blk_len_tmp;
                    if (frame_size != 0 && frame_size == frame_size_count)
                    {
                        start_recv = false;
                        if (m != NULL)
                        {
                            blk_output0.datap = m;
                            ite_queue_put(f->output[0].Qhandle, &blk_output0);

                            if (f->output[1].Qhandle)
                            {
                                mblk_ite * data = NULL;
                                data            = allocb_ite(frame_size);
                                memcpy(data->b_rptr, m->b_rptr, frame_size);
                                data->b_wptr += frame_size;
                                blk_output1.datap = data;
                                ite_queue_put(f->output[1].Qhandle,
                                              &blk_output1);
                            }
                        }

                        frame_size = frame_size_count = blk_len_tmp = 0;
                        m                                           = NULL;
                    }
                }
                else
                {
                    printf("unknow data:ret = %d\n", ret);
                    if (m)
                    {
                        freemsg_ite(m);
                        m = NULL;
                    }
                    start_recv = false;
                    frame_size = frame_size_count = blk_len_tmp = 0;
                    continue;
                }
            }
            usleep(1000);
        }
        // usleep(1000);
    }
    if (m)
    {
        freemsg_ite(m);
        m = NULL;
    }
    if (buf)
    {
        free(buf);
    }
    return NULL;
}

static void * audio_receive_thread (IteFilter * f)
{
    ReceiverData *     d         = (ReceiverData *)f->data;
    mblk_ite *         m         = NULL;
    IteQueueblk        blk       = {0};
    unsigned char *    buf       = NULL;
    int                recv_size = 0;
    struct sockaddr_in remote_addr;
    int                len = sizeof(struct sockaddr_in);
    fd_set             readfds;
    struct timeval     timeout = {0};

    buf                        = malloc(sizeof(char) * AUDIO_BUF_SIZE);
    while (f->run)
    {
        FD_ZERO(&readfds);
        FD_SET(d->udp_conf.cur_socket, &readfds);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 5000;
        lwip_select(d->udp_conf.cur_socket + 1, &readfds, NULL, NULL, &timeout);

        memset(buf, '\0', AUDIO_BUF_SIZE);
        if (FD_ISSET(d->udp_conf.cur_socket, &readfds))
        {
            recv_size =
                lwip_recvfrom(d->udp_conf.cur_socket, buf, AUDIO_BUF_SIZE, 0,
                              (struct sockaddr *)&remote_addr, &len);
            // printf("audio recv %d\n", recv_size);
            if (m == NULL)
            {
                m = allocb_ite(recv_size);
            }
            if (m != NULL)
            {
                memcpy(m->b_wptr, buf, recv_size);
                m->b_wptr += recv_size;
                blk.datap = m;
                ite_queue_put(f->output[0].Qhandle, &blk);
                m = NULL;
            }
            usleep(5000);
        }
        usleep(1000);
    }
    if (buf)
    {
        free(buf);
    }
    // ite_mblk_queue_flush(f->output[0].Qhandle);
    return NULL;
}

static void udp_receiver_set_para (IteFilter * f, void * arg)
{
    ReceiverData *     d        = (ReceiverData *)f->data;
    udp_config_t *     udp_conf = (udp_config_t *)arg;
    struct sockaddr_in addr;
    pthread_attr_t     attr;
    int                err = 0;
    memset(&(d->udp_conf), '\0', sizeof(udp_config_t));
    d->udp_conf.c_type = udp_conf->c_type;
    memcpy(d->udp_conf.group_ip, udp_conf->group_ip, 16);
    d->udp_conf.remote_port = udp_conf->remote_port;
    d->udp_conf.cur_socket  = udp_conf->cur_socket;

    if (d->udp_conf.cur_socket == -1)
    {
        d->udp_conf.cur_socket = lwip_socket(AF_INET, SOCK_DGRAM, 0);
        if (d->udp_conf.cur_socket < 0)
        {
            printf("[udprecv_set_para]sockek create failure\n");
        }
        else
        {
            printf("[udprecv_set_para]udp socket create OK!\n");
        }
        memset(&addr, '\0', sizeof(struct sockaddr_in));
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port        = htons(d->udp_conf.remote_port);
        if (lwip_bind(d->udp_conf.cur_socket, (struct sockaddr *)&addr,
                      sizeof(struct sockaddr_in)) < 0)
        {
            printf("[udprecv_set_para]bind udp  error!\n");
        }
        else
        {
            printf("[udprecv_set_para]bind udp ok! \n");
        }
    }

    if (d->udp_conf.cur_socket != -1)
    {
        if (d->udp_conf.group_ip[0] != '\0' && d->udp_conf.enable_multicast)
        {
            d->udp_conf.mreq.imr_multiaddr.s_addr =
                inet_addr(d->udp_conf.group_ip);
            d->udp_conf.mreq.imr_interface.s_addr = htonl(INADDR_ANY);

            err = lwip_setsockopt(d->udp_conf.cur_socket, IPPROTO_IP,
                                  IP_ADD_MEMBERSHIP, &(d->udp_conf.mreq),
                                  sizeof(d->udp_conf.mreq));
            if (err < 0)
            {
                perror("[udprecv_set_para]setsockopt():IP_ADD_MEMBERSHIP");
            }
        }
    }
}

static void udp_receiver_get_socket (IteFilter * f, void * arg)
{
    ReceiverData * d = (ReceiverData *)f->data;
    ((int *)arg)[0]  = d->udp_conf.cur_socket;
}

static void f_receiver_init (IteFilter * f)
{
    printf("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    ReceiverData * d = (ReceiverData *)ite_new(ReceiverData, 1);
    f->data          = d;
}

static void f_receiver_uninit (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    ReceiverData * d = (ReceiverData *)(f->data);
    if (d != NULL)
    {
        if (d->udp_conf.group_ip[0] != '\0' && d->udp_conf.enable_multicast)
        {
            lwip_setsockopt(d->udp_conf.cur_socket, IPPROTO_IP,
                            IP_DROP_MEMBERSHIP, &(d->udp_conf.mreq),
                            sizeof(d->udp_conf.mreq));
        }
        lwip_close(d->udp_conf.cur_socket);
        printf("[receiver_uninit]socket close OK!\n");
        free(d);
    }
}

static void f_receiver_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    ReceiverData * d = (ReceiverData *)f->data;
    switch (d->udp_conf.c_type)
    {
        case AUDIO_INPUT:
            audio_receive_thread(f);
            break;
        case VIDEO_INPUT:
            video_receive_thread(f);
            break;
    }
}

static IteMethodDes udp_recv_methods[] = {
    {  ITE_FILTER_UDP_RECV_SET_PARA,   udp_receiver_set_para},
    {ITE_FILTER_UDP_RECV_GET_SOCKET, udp_receiver_get_socket},
    {                             0,                    NULL}
};

// clang-format off
IteFilterDes FilterUDPrecv = {
    ITE_FILTER_UDP_RECV_ID,
    f_receiver_init,
    f_receiver_uninit,
    f_receiver_process,
    udp_recv_methods
};
// clang-format on
