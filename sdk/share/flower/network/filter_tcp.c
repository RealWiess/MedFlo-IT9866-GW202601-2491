#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "errno.h"
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
    tcp_config_t tcp_conf;
    bool         run_flag;
} SenderData;

typedef struct _ReceiverData
{
    tcp_config_t tcp_conf;
    bool         run_flag;
} ReceiverData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
static void tcp_sender_set_para (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SenderData *       d        = (SenderData *)f->data;

    tcp_config_t *     tcp_conf = (tcp_config_t *)arg;
    struct sockaddr_in addr;
    int                val = 1;

    memset(&(d->tcp_conf), '\0', sizeof(tcp_config_t));
    d->tcp_conf.c_type      = tcp_conf->c_type;
    d->tcp_conf.remote_port = tcp_conf->remote_port;
    d->tcp_conf.cur_socket  = tcp_conf->cur_socket;
    call_list               = (Call_info *)tcp_conf->remote_ip;

    if (d->tcp_conf.cur_socket == -1)
    {
        d->tcp_conf.cur_socket = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (d->tcp_conf.cur_socket < 0)
        {
            printf("[tcprecv_set_para]sockek create failure\n");
        }
        else
        {
            printf("[tcprecv_set_para]tcp socket create OK!\n");
        }
        memset(&addr, '\0', sizeof(struct sockaddr_in));
        printf(
            "YC: d->tcp_conf.cur_socket = %d, d->tcp_conf.remote_port = %d\n",
            d->tcp_conf.cur_socket, d->tcp_conf.remote_port);
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(d->tcp_conf.remote_port);

        if (lwip_setsockopt(d->tcp_conf.cur_socket, SOL_SOCKET, SO_REUSEADDR,
                            &val, sizeof(int)) == -1)
        {
            printf("setsockopt error\n");
        }

        if (lwip_bind(d->tcp_conf.cur_socket, (struct sockaddr *)&addr,
                      sizeof(struct sockaddr_in)) < 0)
        {
            printf("[tcprecv_set_para]bind tcp  error, errno = %d\n", errno);
        }
        else
        {
            printf("[tcprecv_set_para]bind tcp ok! \n");
        }
    }
}

static void tcp_sender_get_socket (IteFilter * f, void * arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    SenderData * d  = (SenderData *)f->data;
    ((int *)arg)[0] = d->tcp_conf.cur_socket;
}

static void audio_send (unsigned char * data, unsigned int len_size,
                        SenderData * send_data, unsigned int call_index)
{
    int ret = 0;
    // printf("audio_send %d\n", len_size);
    ret     = lwip_send(send_data->tcp_conf.cur_cli_socket[call_index], data,
                        len_size, 0);
    // printf("%s, %d, ret = %d\n", __FUNCTION__, __LINE__, ret);
    usleep(5000);
}

static void video_send (unsigned char * data, unsigned int len_size,
                        SenderData * send_data, unsigned int call_index)
{
    int             ret       = 0;
    int             send_size = 0, remain_size = len_size;
    unsigned char * tmp_buf = malloc(sizeof(char) * VIDEO_BUF_SIZE);
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
                ret = lwip_send(send_data->tcp_conf.cur_cli_socket[call_index],
                                tmp_buf, VIDEO_BUF_SIZE, 0);
                // printf("%s, %d, ret = %d\n", __FUNCTION__, __LINE__, ret);
                remain_size -= (VIDEO_BUF_SIZE - 8);
                send_size += (VIDEO_BUF_SIZE - 8);
            }
            else
            {
                memcpy(&tmp_buf[8], &data[send_size], remain_size);
                ret = lwip_send(send_data->tcp_conf.cur_cli_socket[call_index],
                                tmp_buf, remain_size + 8, 0);
                // printf("%s, %d, ret = %d\n", __FUNCTION__, __LINE__, ret);
                break;
            }
        }
        usleep(2000);
        if (remain_size > VIDEO_BUF_SIZE)
        {
            ret = lwip_send(send_data->tcp_conf.cur_cli_socket[call_index],
                            &data[send_size], VIDEO_BUF_SIZE, 0);
            // printf("%s, %d, ret = %d\n", __FUNCTION__, __LINE__, ret);
            remain_size -= VIDEO_BUF_SIZE;
            send_size += VIDEO_BUF_SIZE;
        }
        else
        {
            ret = lwip_send(send_data->tcp_conf.cur_cli_socket[call_index],
                            &data[send_size], remain_size, 0);
            // printf("%s, %d, ret = %d\n", __FUNCTION__, __LINE__, ret);
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
        close(d->tcp_conf.cur_socket);
        for (int i = 0; i < max_call_num; i++)
        {
            if (d->tcp_conf.cur_cli_socket[i] > 0)
            {
                close(d->tcp_conf.cur_cli_socket[i]);
            }
        }
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
    struct sockaddr_in remote_addr;
    int                addr_len = sizeof(struct sockaddr_in);
    int                recv_timeout, send_timeout;
    struct timeval     timeout = {1, 0}; // 1 sec
    void (*data_send)(unsigned char * data, unsigned int len_size,
                      SenderData * send_data, unsigned int call_index);

    memset(&remote_addr, '\0', sizeof(struct sockaddr_in));

    switch (d->tcp_conf.c_type)
    {
        case AUDIO_OUTPUT:
            data_send = audio_send;
            break;
        case VIDEO_OUTPUT:
            data_send = video_send;
            break;
    }

    send_timeout = lwip_setsockopt(d->tcp_conf.cur_socket, SOL_SOCKET,
                                   SO_SNDTIMEO, &timeout, sizeof(timeout));
    recv_timeout = lwip_setsockopt(d->tcp_conf.cur_socket, SOL_SOCKET,
                                   SO_RCVTIMEO, &timeout, sizeof(timeout));

    if (lwip_listen(d->tcp_conf.cur_socket, 10) == -1)
    {
        printf("Socket Listen Fail, errno = %d\n", errno);
    }

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            m = (mblk_ite *)blk.datap;
            if (d->tcp_conf.c_type == VIDEO_OUTPUT && call_list != NULL)
            {
                for (int i = 0; i < max_call_num; i++)
                {
                    if (call_list[i].call_start)
                    {
                        if (d->tcp_conf.connect_accepted[i] == false)
                        {
                            d->tcp_conf.cur_cli_socket[i] = lwip_accept(
                                d->tcp_conf.cur_socket,
                                (struct sockaddr *)&remote_addr, &addr_len);
                            if (d->tcp_conf.cur_cli_socket[i] == -1)
                            {
                                printf("Socket Accept Fail\n");
                                continue;
                            }
                            d->tcp_conf.connect_accepted[i] = true;
                            printf("Socket Accept Success\n");
                        }
                        data_send(m->b_rptr, m->b_wptr - m->b_rptr, d, i);
                        usleep(1000);
                    }
                }
            }
            else if (d->tcp_conf.c_type == AUDIO_OUTPUT)
            {
                if (d->tcp_conf.connect_accepted[0] == false)
                {
                    d->tcp_conf.cur_cli_socket[0] =
                        lwip_accept(d->tcp_conf.cur_socket,
                                    (struct sockaddr *)&remote_addr, &addr_len);
                    if (d->tcp_conf.cur_cli_socket[0] == -1)
                    {
                        printf("Socket Accept Fail\n");
                        freemsg_ite(m);
                        m = NULL;
                        usleep(1000);
                        continue;
                    }
                    d->tcp_conf.connect_accepted[0] = true;
                    printf("Socket Accept Success\n");
                }
                data_send(m->b_rptr, m->b_wptr - m->b_rptr, d, 0);
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

static IteMethodDes tcp_send_methods[] = {
    {  ITE_FILTER_TCP_SEND_SET_PARA,   tcp_sender_set_para},
    {ITE_FILTER_TCP_SEND_GET_SOCKET, tcp_sender_get_socket},
    {                             0,                  NULL}
};

// clang-format off
IteFilterDes FilterTCPsend = {
    ITE_FILTER_TCP_SEND_ID,
    f_sender_init,
    f_sender_uninit,
    f_sender_process,
    tcp_send_methods
};
// clang-format on

static void * video_receive_thread (IteFilter * f)
{
    ReceiverData *     d = (ReceiverData *)f->data;
    struct sockaddr_in addr;
    int                addr_len    = sizeof(struct sockaddr_in);
    mblk_ite *         m           = NULL;
    IteQueueblk        blk_output0 = {0};
    IteQueueblk        blk_output1 = {0};
    int                recv_timeout, send_timeout;
    struct timeval     timeout    = {3, 0}; // 3 sec
    int                ret        = 0;
    unsigned char *    buf_ptr    = NULL;
    unsigned int       frame_size = 0, frame_size_count = 0, blk_len_tmp = 0;
    unsigned char *    buf           = NULL;
    unsigned char *    tmp_buf       = NULL;
    unsigned int       tmp_bufsize   = 0;
    bool               start_recv    = false;
    int                connect_count = 0;
    fd_set             readfds;
    struct timeval     select_timeout = {0};

    buf                               = malloc(sizeof(char) * VIDEO_BUF_SIZE);
    tmp_buf                           = malloc(sizeof(char) * VIDEO_BUF_SIZE);

    send_timeout = lwip_setsockopt(d->tcp_conf.cur_socket, SOL_SOCKET,
                                   SO_SNDTIMEO, &timeout, sizeof(timeout));
    recv_timeout = lwip_setsockopt(d->tcp_conf.cur_socket, SOL_SOCKET,
                                   SO_RCVTIMEO, &timeout, sizeof(timeout));

CONNECT_RETRY:
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(d->tcp_conf.remote_port);
    addr.sin_addr.s_addr = inet_addr(d->tcp_conf.remote_ip);
    // printf("remote ip = %s, remote port = %d\n", d->tcp_conf.remote_ip,
    // d->tcp_conf.remote_port );

    if (lwip_connect(d->tcp_conf.cur_socket, (struct sockaddr *)&addr,
                     addr_len) == -1)
    {
        printf("Connect Fail, errno = %d\n", errno);
        connect_count++;
        if (connect_count > 400)
        {
            goto END;
        }
        lwip_close(d->tcp_conf.cur_socket);
        d->tcp_conf.cur_socket = lwip_socket(AF_INET, SOCK_STREAM, 0);
        usleep(5000);
        goto CONNECT_RETRY;
    }

    while (f->run)
    {
        FD_ZERO(&readfds);
        FD_SET(d->tcp_conf.cur_socket, &readfds);
        select_timeout.tv_sec  = 0;
        select_timeout.tv_usec = 5000;
        lwip_select(d->tcp_conf.cur_socket + 1, &readfds, NULL, NULL,
                    &select_timeout);
        if (FD_ISSET(d->tcp_conf.cur_socket, &readfds))
        {
            if (ret > 0)
            {
                int i = 0;
#if 0
                for (i = 0; i < ret; i++)
                {
                    printf("%x ", *(buf_ptr + i));
                }
                printf("\n");
#endif
                tmp_bufsize = ret;
                memcpy(&tmp_buf[0], buf_ptr, tmp_bufsize);
                ret = 0;
            }

            ret = lwip_recv(d->tcp_conf.cur_socket, buf, VIDEO_BUF_SIZE, 0);
            // printf("%s, %d, ret = %d\n", __FUNCTION__, __LINE__, ret);
            if (ret == -1)
            {
                printf("recv video timeout\n");
                usleep(1000);
                continue;
            }

            if (tmp_bufsize != 0)
            {
                memcpy(tmp_buf + tmp_bufsize, buf, ret);
                ret += tmp_bufsize;
                buf_ptr     = &tmp_buf[0];
                tmp_bufsize = 0;
            }
            else
            {
                buf_ptr = &buf[0];
            }

            while (ret > 8)
            {
                if (frame_size_count == 0)
                {
                    if (memcmp(buf_ptr, start_code, 4) == 0)
                    {
                        frame_size = frame_size_count = blk_len_tmp = 0;
                        frame_size = (buf_ptr[4] << 24) | (buf_ptr[5] << 16) |
                                     (buf_ptr[6] << 8) | buf_ptr[7];

                        ret -= 8;
                        buf_ptr += 8;
                        start_recv = true;
                    }
                    else
                    {
                        ret = 0;
                        break;
                    }
                }
                if (start_recv)
                {
                    blk_len_tmp = ret;
                    if (m == NULL)
                    {
                        m = allocb_ite(frame_size);
                        if (blk_len_tmp > frame_size)
                        {
                            blk_len_tmp = frame_size;
                        }
                        memcpy(m->b_wptr, buf_ptr, blk_len_tmp);
                        m->b_wptr += blk_len_tmp;
                        frame_size_count += blk_len_tmp;

                        ret -= blk_len_tmp;
                        buf_ptr += blk_len_tmp;
                    }
                    else
                    {
                        if ((frame_size_count + blk_len_tmp) > frame_size)
                        {
                            blk_len_tmp = frame_size - frame_size_count;
                        }

                        memcpy(m->b_wptr, buf_ptr, blk_len_tmp);
                        m->b_wptr += blk_len_tmp;
                        frame_size_count += blk_len_tmp;

                        ret -= blk_len_tmp;
                        buf_ptr += blk_len_tmp;
                    }

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
            }
        }
        usleep(1000);
    }

END:
    if (m)
    {
        freemsg_ite(m);
        m = NULL;
    }
    if (buf)
    {
        free(buf);
    }
    if (tmp_buf)
    {
        free(tmp_buf);
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
    struct sockaddr_in addr;
    int                addr_len = sizeof(struct sockaddr_in);
    int                recv_timeout, send_timeout;
    struct timeval     timeout       = {3, 0}; // 3 sec
    int                connect_count = 0;
    fd_set             readfds;
    struct timeval     select_timeout = {0};

    send_timeout = lwip_setsockopt(d->tcp_conf.cur_socket, SOL_SOCKET,
                                   SO_SNDTIMEO, &timeout, sizeof(timeout));
    recv_timeout = lwip_setsockopt(d->tcp_conf.cur_socket, SOL_SOCKET,
                                   SO_RCVTIMEO, &timeout, sizeof(timeout));

CONNECT_RETRY:
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(d->tcp_conf.remote_port);
    addr.sin_addr.s_addr = inet_addr(d->tcp_conf.remote_ip);
    // printf("remote ip = %s, remote port = %d\n", d->tcp_conf.remote_ip,
    // d->tcp_conf.remote_port );

    if (lwip_connect(d->tcp_conf.cur_socket, (struct sockaddr *)&addr,
                     addr_len) == -1)
    {
        printf("Connect Fail, errno = %d\n", errno);
        connect_count++;
        if (connect_count > 400)
        {
            goto END;
        }
        lwip_close(d->tcp_conf.cur_socket);
        d->tcp_conf.cur_socket = lwip_socket(AF_INET, SOCK_STREAM, 0);
        usleep(5000);
        goto CONNECT_RETRY;
    }

    buf = malloc(sizeof(char) * AUDIO_BUF_SIZE);
    while (f->run)
    {
        FD_ZERO(&readfds);
        FD_SET(d->tcp_conf.cur_socket, &readfds);
        select_timeout.tv_sec  = 0;
        select_timeout.tv_usec = 5000;
        lwip_select(d->tcp_conf.cur_socket + 1, &readfds, NULL, NULL,
                    &select_timeout);
        if (FD_ISSET(d->tcp_conf.cur_socket, &readfds))
        {
            memset(buf, '\0', AUDIO_BUF_SIZE);

            recv_size =
                lwip_recv(d->tcp_conf.cur_socket, buf, AUDIO_BUF_SIZE, 0);
            // printf("audio recv %d\n", recv_size);
            if (recv_size == -1)
            {
                printf("recv audio timeout\n");
                usleep(1000);
                continue;
            }

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
        }
        usleep(5000);
    }
END:
    if (buf)
    {
        free(buf);
    }
    // ite_mblk_queue_flush(f->output[0].Qhandle);
    return NULL;
}

static void tcp_receiver_set_para (IteFilter * f, void * arg)
{
    ReceiverData *     d        = (ReceiverData *)f->data;

    tcp_config_t *     tcp_conf = (tcp_config_t *)arg;
    struct sockaddr_in addr;
    memset(&(d->tcp_conf), '\0', sizeof(tcp_config_t));
    d->tcp_conf.c_type      = tcp_conf->c_type;
    d->tcp_conf.remote_ip   = tcp_conf->remote_ip;
    d->tcp_conf.remote_port = tcp_conf->remote_port;
    d->tcp_conf.cur_socket  = tcp_conf->cur_socket;

    if (d->tcp_conf.cur_socket == -1)
    {
        d->tcp_conf.cur_socket = lwip_socket(AF_INET, SOCK_STREAM, 0);
        // printf("d->tcp_conf.cur_socket = %d\n", d->tcp_conf.cur_socket );
        if (d->tcp_conf.cur_socket < 0)
        {
            printf("[tcpsend_set_para]sockek create failure\n");
        }
        else
        {
            printf("[tcpsend_set_para]tcp socket create OK!\n");
        }
    }
}

static void tcp_receiver_get_socket (IteFilter * f, void * arg)
{
    ReceiverData * d = (ReceiverData *)f->data;
    ((int *)arg)[0]  = d->tcp_conf.cur_socket;
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
        lwip_close(d->tcp_conf.cur_socket);
        printf("[receiver_uninit]socket close OK!\n");
        free(d);
    }
}

static void f_receiver_process (IteFilter * f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    ReceiverData * d = (ReceiverData *)f->data;
    switch (d->tcp_conf.c_type)
    {
        case AUDIO_INPUT:
            audio_receive_thread(f);
            break;
        case VIDEO_INPUT:
            video_receive_thread(f);
            break;
    }
}

static IteMethodDes tcp_recv_methods[] = {
    {  ITE_FILTER_TCP_RECV_SET_PARA,   tcp_receiver_set_para},
    {ITE_FILTER_TCP_RECV_GET_SOCKET, tcp_receiver_get_socket},
    {                             0,                    NULL}
};

// clang-format off
IteFilterDes FilterTCPrecv = {
    ITE_FILTER_TCP_RECV_ID,
    f_receiver_init,
    f_receiver_uninit,
    f_receiver_process,
    tcp_recv_methods
};
// clang-format on
