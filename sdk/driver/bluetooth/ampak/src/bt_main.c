#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "ite/ith.h"
#include "ite/itp.h"
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"
#include "uart/uart.h"
#include "nimble/hal/hal_uart.h"

typedef struct bt_uart {
    UART_OBJ    info;
    uint32_t    reg_on;
} bt_bus_t;

bt_bus_t                bt_bus = {
    .info           = {
        .port       = TEST_ITH_PORT,
        .parity     = 0,
        .txPin      = TEST_GPIO_TX,
        .rxPin      = TEST_GPIO_RX,
        .baud       = TEST_BAUDRATE,    // default 115200
        .timeout    = 0,
        .mode       = UART0_MODE,       // UART_INTR_MODE , UART_DMA_MODE, UART_FIFO_MODE
        .forDbg     = false,
    },
    .reg_on         = CFG_GPIO_BT_REG_ON,
};

extern int              brcm_patch_ram_length;
extern const char       brcm_patch_version[];
extern const uint8_t    brcm_patchram_buf[];

typedef void (*hci_callback_t) (void *);

char name[32] = "iTE_";

static void
printf_array (
    uint8_t     * arrayAddr,
    uint16_t    len)
{
    (void)printf("\r\n*********************************\r\n");
    uint16_t i = 0;

    do
    {
        (void)printf("%02X ", ((unsigned char *)arrayAddr)[i++]);
        if (i % 16 == 0)
        {
            (void)printf("\r\n");
        }
    } while (i < len);

    (void)printf("\r\n*********************************\r\n");
}

hci_callback_t
hci_read_bd_addr_cb (
    void * parm)
{
    uint8_t * p = (uint8_t *) parm;
    if (p[6] == 0) // SUCCESS
    {
        (void)printf("Local MAC:%02x%02x%02x%02x%02x%02x\r\n", p[12], p[11], p[10], p[9], p[8], p[7]);
    }
    int     i;
    char    tmp[4] = "";
    for (i = 12; i > 6; i--)
    {
        snprintf(tmp, sizeof(tmp), "%02X", p[i]);
        strlcat(name, tmp, sizeof(name));
    }
    (void)printf("name: %s\n", name);
}

hci_callback_t
hci_hdc_downing_cb (
    void * parm)
{
    uint8_t * p = (uint8_t *) parm;
    printf_array(p, p[2] + 3); // include Opcode, length
}

int
bt_bus_receive (
    uint8_t     * data_in,
    uint32_t    size)
{
    int     recv_len    = 0;
    int     temp_len    = 0;
    uint8_t i;

    do
    {
        recv_len += read(TEST_PORT, &data_in[recv_len], size - recv_len);
    } while (recv_len < size);

    return (recv_len == size)? 0 : 1;
}

static void
send_hci_one_cmd (
    const uint8_t   * cmd,
    int             length,
    int             delay_ms,
    hci_callback_t  cb)
{
    unsigned char   recv_buf[10];

    uint32_t        r = write(TEST_PORT, cmd, length);

    do
    {
        r = bt_bus_receive(recv_buf, 1);
        if (recv_buf[0] != 0x04)
        {
            (void)printf("error packet :%x\r\n", recv_buf[0]);
        }
    } while (recv_buf[0] != 0x04);

    r       = bt_bus_receive(recv_buf + 1, 2);
    length  = recv_buf[2];
    r       = bt_bus_receive(recv_buf + 3, length);

    usleep(delay_ms * 1000);

    if (cb)
    {
        cb(&recv_buf);
    }
}

static int
bt_bus_init (
    bt_bus_t * bus)
{
    /* Initial BT Power Pin(output) */
    ithGpioSetOut(bt_bus.reg_on);
    ithGpioSetMode(bt_bus.reg_on, ITH_GPIO_MODE0);
    ithGpioClear(bt_bus.reg_on);       // chipset powerOff
    usleep(150 * 1000);

    /* PowerOn and waiting chipset standby */
    ithGpioSet(bt_bus.reg_on);
    usleep(250 * 1000);

    /* Open BT Uart */
    itpRegisterDevice(TEST_PORT, &TEST_DEVICE);
    ioctl(TEST_PORT, ITP_IOCTL_INIT, (void *)&bt_bus.info);

    usleep(10 * 1000);

    return 0;
}

static void
bt_update_baudrate (
    uint32_t    new_baudrate,
    uint8_t     send_cmd_to_controller)
{
    uint8_t hci_change_baudrate[] = {0x01, 0x18, 0xfc, 0x06, 0x00, 0x00, 0xc0, 0xc6, 0x2d, 0x00};   // 0x2DC6C0 = 3M baudrate

    if (new_baudrate > 3000000)                                                                     // max speed
    {
        new_baudrate = 3000000;
    }

    if (send_cmd_to_controller)
    {
        /* Baudrate is loaded LittleEndian */
        hci_change_baudrate[6]  = (new_baudrate      ) & 0xFF;
        hci_change_baudrate[7]  = (new_baudrate >>  8) & 0xFF;
        hci_change_baudrate[8]  = (new_baudrate >> 16) & 0xFF;
        hci_change_baudrate[9]  = (new_baudrate >> 24) & 0xFF;

        /* Send the command to controller */
        send_hci_one_cmd(hci_change_baudrate, sizeof(hci_change_baudrate), 50, NULL);
    }

    (void)printf("bt bus reconfig: %d\r\n", new_baudrate);
    /* bt bus reconfig */
    bt_bus.info.baud = new_baudrate;
    ioctl(TEST_PORT, ITP_IOCTL_RESET, (void *)&bt_bus.info);
}

static void
bt_download_hcd (
    void)
{
    const uint8_t   hci_reset[]                         = {0x01, 0x03, 0x0c, 0x00};
    const uint8_t   hci_vendor_download_minidriver[]    = {0x01, 0x2e, 0xfc, 0x00};
    const uint8_t   hci_read_bd_addr[]                  = {0x01, 0x09, 0x10, 0x00};

    uint8_t         buf[360];
    uint16_t        length;
    uint32_t        offset = 0;

    (void)printf("HCD version:%s\r\n", brcm_patch_version);

    send_hci_one_cmd(hci_reset, sizeof(hci_reset), 200, NULL);

    bt_update_baudrate(1000000, 1); // 3M

    send_hci_one_cmd(hci_vendor_download_minidriver, sizeof(hci_vendor_download_minidriver), 200, NULL);

    const uint8_t   * p = brcm_patchram_buf;
    uint16_t        idx = 0;
    do
    {
        length  = (p[2] & 0xff) + 3;    // include:OpCode(2B), Length(1B)
        memcpy(&buf[1], p, length);
        buf[0]  = 0x01;                 // HCI command
        send_hci_one_cmd(buf, length + 1, 0, 0);
        p       += length;
        idx     += length;
        offset  += length;
#ifdef DBG
        (void)printf("bt downloading... total:%d len:%d offset:%d\r\n", brcm_patch_ram_length, length, offset);
#endif
    } while (idx < (brcm_patch_ram_length - 1));

    /* when download finish chip will reset , waiting it ready */
    usleep(300 * 1000);
    (void)printf("download done\r\n");

    bt_update_baudrate(115200, 0); // default 115200

    send_hci_one_cmd(hci_reset, sizeof(hci_reset), 200, NULL);

    bt_update_baudrate(1000000, 1);

    send_hci_one_cmd(hci_read_bd_addr, sizeof(hci_read_bd_addr), 20, (void *)&hci_read_bd_addr_cb);
}

void *
bt_init (
    void * arg)
{
    uint8_t i = 0;

    (void)printf("BT HCD download begin\r\n");

    bt_bus_init(&bt_bus);
    bt_download_hcd();

#ifdef DBG
    (void)printf("=========================================================\r\n");
    (void)printf("=======                BT RF test mode            =======\r\n");
    (void)printf("=======                                           =======\r\n");
    (void)printf("======= WARN: wifi should turn off during BT Test =======\r\n");
    (void)printf("=========================================================\r\n");
    (void)printf("go ahead for testing...\r\n");
#endif
}
