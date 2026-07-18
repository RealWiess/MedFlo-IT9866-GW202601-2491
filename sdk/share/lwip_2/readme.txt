LWIP v2.1.2

<2019/11/19>
****[Ethernet]****
1. Refine some functions as:
/********************************/
   #sdk/share/
           curl/
           dhcps/
           eXosip2/
           lwip/
           mbedtls/
           mediastreamer2/
           polarssl/
           sntp/

   #sdk/include/
           lwip/
           netif/
           corec/

   #sdk/driver/itp/
           itp_ethernet.c
           itp_socket.c

   #openrtos/include/
           inet.c
           if.h
           in.h
           poll.h
           select.h
           uio.h
           lwipopts.h
           netdb.h
/********************************/
2. Enable LWIP_RAW
3. Add IP conflict detection
4. Rename ip_addr to ip_addr_t
5. Etharp for "MIDEA issue"

****[WIFI]****
1. Add packet.c/packet.h
2. Add compiler option of LWIP_PACKET and some functions about WIFI
3. Add API of NGPL WIFI in wifipktif.c


<2019/12/23>
****[Ethernet]****
1. None
****[WIFI]****
1. Support for NGPL WIFI


<2019/12/27>
****[Ethernet]****
1. Support ECM and NCM (from Irene Lin)
****[WIFI]****
1. None


<2020/07/13>
****[Ethernet]****
1. Fixed the lwip_getsockname can't get local IP by using UDP  in linphone(Actually, getsockname get IP = 0.0.0.0 by using UDP,  it's not a bug)
2. Update LWIP 2.1.X - 1bb6e7f52de1cd86be0eed31e348431edc2cd01e
****[WIFI]****
1. None


<2021/11/15>
****[Ethernet]****
1. Update to STABLE-2_1_3_RELEASE
****[WIFI]****
1. None

<2022/09/15>
****[Ethernet]****
1. None
****[WIFI]****
1. Remove all modified codes about LWIP_PACKET

<2022/12/13>
****[Ethernet]****
1. Update to 2022/05/09 master(239918ccc173cb2c2a62f41a40fd893f57faf1d6)
****[WIFI]****
1. None

<2023/12/27>
****[Ethernet]****
1. Update to STABLE-2_2_0_RELEASE
****[WIFI]****
1. None