#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "libxml/xpathInternals.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "openssl/sha.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "onvif/onvif.h"

#define SHA1_DIGEST_SIZE    20


//=============================================================================
//                              Function Declaration
//=============================================================================

static int Base64Encode(const void *data, int data_len, char *buffer) 
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());

    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data, data_len);
    BIO_ctrl(bio, BIO_CTRL_FLUSH, 0, NULL);

    BUF_MEM *bptr = NULL;
    BIO_get_mem_ptr(bio, &bptr);

    size_t slen = bptr->length;
    memcpy(buffer, bptr->data, slen);
    buffer[slen] = '\0';

    BIO_free_all(bio);
    
    return slen;
}

static void xaddrsExtractHost(char *xaddrs, char *host, int *host_port)
{
    char tmp[128] = {0}, tmp2[128] = {0};
    char port_buf[128] = {0};
    int tmp_len = 0;
    char *sub = NULL;
    int start = 0, end = 0;
    int i;

    // ignore "http://"
    sub = strstr(xaddrs, "//");
    start = sub - xaddrs + 2;
    tmp_len = strlen(xaddrs);
    
    for(i = 0; i < tmp_len - start; i++) {
        tmp[i] = xaddrs[i + start];
    }
    tmp[i] = '\0';

    sub = strstr(tmp, "/");
    end = sub - tmp;

    // get "192.168.190.31" or "192.168.190.31:port"
    for(i = 0; i < end; i++) {
        tmp2[i] = tmp[i];
    }
    tmp2[i] = '\0';

    sub = strstr(tmp2, ":");
    if(sub == NULL) {
        strcpy(host, tmp2);
        strcpy(port_buf, "80");
    }
    else {
        start = sub - tmp2;
        for (i = 0; i < start; i++) {
            host[i] = tmp2[i];
        }
        host[i] = '\0';
        
        tmp_len = strlen(tmp2);
        for (i = start + 1; i < tmp_len; i++) {
            port_buf[i - (start + 1)] = tmp2[i];
        }
        port_buf[i - (start + 1)] = '\0';
    }
    
    *host_port = atoi(port_buf);

}

static void addHttpHeader(xmlDocPtr doc, xmlNodePtr root, char *xaddrs, char *post_type, char *cmd, int cmd_length) 
{
    char xml[8192] = {0};
    int size = 0, xml_size_length = 0;
    xmlOutputBufferPtr outputbuffer = xmlAllocOutputBuffer(NULL);
    char xml_size[6] = {0};
    char host[128] = {0};
    int port = 0;
    int i = 0, s = 0;
    char content[] =
    "User-Agent: Generic\r\n"
    "Connection: Close\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Content-Type: application/soap+xml; charset=utf-8;\r\n"
    "Host: ";
    char content_length[] = "\r\nContent-Length: ";    
    int post_type_len = 0, host_len = 0, content_len = 0, content_length_size = 0, cmd_size = 0;
    char http_terminate[5] = {0};

    // http header end's format "\r\n\r\n"
    http_terminate[0] = '\r';
    http_terminate[1] = '\n';
    http_terminate[2] = '\r';
    http_terminate[3] = '\n';
    http_terminate[4] = '\0';

    // Dump xml node to outputbuffer format
    xmlNodeDumpOutput(outputbuffer, doc, root, 0, 0, NULL);
    size = xmlOutputBufferGetSize(outputbuffer);
    
    if(size > 8191) {
        DEBUG_PRINT("[%s] ERROR: xmlOutputBufferGetSize too big, size= %d\n", __FUNCTION__, size);
        return;
    }
    else{
        // outputbuffer format transfer to char format
        strcpy(xml, (char*)xmlOutputBufferGetContent(outputbuffer));
    }

    xmlOutputBufferFlush(outputbuffer);
    xmlOutputBufferClose(outputbuffer);
    xmlFreeDoc(doc);

    sprintf(xml_size, "%d", size);
    xml_size_length = strlen(xml_size) + 1;

    // xaddrs filter to host address
    xaddrsExtractHost(xaddrs, host, &port);

    post_type_len = strlen(post_type) + 1; // +1 is "\r"
    host_len = strlen(host) + 1;
    content_len = sizeof(content);
    content_length_size = sizeof(content_length);
    cmd_size = post_type_len + host_len + content_len + content_length_size + xml_size_length + size + 1;

    /**********************************************************/
    /* Build Http header                                      */
    /*                                                        */
    /* POST /onvif/device_service HTTP/1.1\r\n                */
    /* User-Agent: Generic\r\n                                */
    /* Connection: Close\r\n                                  */
    /* Accept-Encoding: gzip, deflate\r\n                     */
    /* Content-Type: application/soap+xml; charset=utf-8;\r\n */
    /* Host: 192.168.190.31\r\n                               */
    /* Content-Length: 205\r\n\r\n                            */
    /**********************************************************/
    for(i = 0; i < post_type_len - 1; i++)
        cmd[i] = post_type[i];
    s = i;
    for(i = 0; i < content_len - 1; i++)
        cmd[s + i] = content[i];
    s = s + i;
    for(i = 0; i< host_len - 1; i++)
        cmd[s + i] = host[i];
    s = s + i;
    for(i = 0; i < content_length_size - 1; i++)
        cmd[s + i] = content_length[i];
    s = s + i;
    for(i = 0; i < xml_size_length - 1; i++)
        cmd[s + i] = xml_size[i];
    s = s + i;
    for(i = 0; i < 4; i++)
        cmd[s + i] = http_terminate[i];

    // add soap-xml cmd payload
    s = s + i;
    for(i = 0; i < size; i++)
        cmd[s + i] = xml[i];
    
    cmd[cmd_size] = '\0';

    //DEBUG_PRINT("[%s] cmd=%s\n", __FUNCTION__, cmd);
}

static void addDigestHeader(xmlNodePtr root, xmlNsPtr ns_env, char *username, char *password, time_t offset) 
{
    unsigned char nonce_buffer[SHA1_DIGEST_SIZE];
    unsigned char nonce_result[SHA1_DIGEST_SIZE];
    char nonce_base64[128] = {0};
    char time_buffer[128] = {0};
    char digest_base64[128] = {0};
    time_t now;
    size_t time_buffer_length;
    unsigned char hash[SHA1_DIGEST_SIZE] = {0};
    SHA_CTX ctx;
    unsigned int digest_chunk_size = SHA1_DIGEST_SIZE;
    unsigned char digest_result[128] = {0};
    int i = 0;

    /**************************************************************/
    /* Digest = BASE64( SHA1( BASE64(Nonce) + Date + Password ) ) */
    /**************************************************************/

    // get random and unique number
    srand(time(NULL));
    for(i = 0; i < SHA1_DIGEST_SIZE; i++) {
        nonce_buffer[i] = (unsigned char)rand();
    }

    // use openssl/evb base64 algorithm encode Nonce
    Base64Encode(nonce_buffer, SHA1_DIGEST_SIZE, nonce_result);
    strcpy(nonce_base64, (const char *)nonce_result);

    // get IPCam's time
    now = time(NULL);
    now = now + offset;
    time_buffer_length = strftime(time_buffer, 128, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    time_buffer[time_buffer_length] = '\0';

    // use openssl/sha1 SHA1 algorithm encode (Nonce_base64 + IPCam_time + Password)
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, nonce_buffer, SHA1_DIGEST_SIZE);
    SHA1_Update(&ctx, (const unsigned char *)time_buffer, strlen(time_buffer));
    SHA1_Update(&ctx, (const unsigned char *)password, strlen(password));
    SHA1_Final(hash, &ctx);
    
    // use openssl/evb base64 algorithm encode SHA1_hash
    Base64Encode(hash, digest_chunk_size, digest_result);    
    strcpy(digest_base64, (const char *)digest_result);

    DEBUG_PRINT("[%s] Nonce=%s Time=%s Digest=%s\n", __FUNCTION__, nonce_base64, time_buffer, digest_base64);

    // create auth xml header
    xmlNsPtr ns_wsse = xmlNewNs(root, BAD_CAST "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd", BAD_CAST "wsse");
    xmlNsPtr ns_wsu = xmlNewNs(root, BAD_CAST "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd", BAD_CAST "wsu");
    xmlNodePtr header = xmlNewTextChild(root, ns_env, BAD_CAST "Header", NULL);
    xmlNodePtr security = xmlNewTextChild(header, ns_wsse, BAD_CAST "Security", NULL);
    xmlNewProp(security, BAD_CAST "SOAP-ENV:mustUnderstand", BAD_CAST "1");
    xmlNodePtr username_token = xmlNewTextChild(security, ns_wsse, BAD_CAST "UsernameToken", NULL);
    xmlNewTextChild(username_token, ns_wsse, BAD_CAST "Username", BAD_CAST username);
    xmlNodePtr pwd = xmlNewTextChild(username_token, ns_wsse, BAD_CAST "Password", BAD_CAST digest_base64);
    xmlNewProp(pwd, BAD_CAST "Type", BAD_CAST "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest");
    xmlNodePtr nonce = xmlNewTextChild(username_token, ns_wsse, BAD_CAST "Nonce", BAD_CAST nonce_base64);
    xmlNewProp(nonce, BAD_CAST "EncodingType", BAD_CAST "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary");
    xmlNewTextChild(username_token, ns_wsu, BAD_CAST "Created", BAD_CAST time_buffer);
}


static xmlDocPtr sendCommand(char *cmd, char *xaddrs) {
    xmlDocPtr reply = NULL;
    int onvif_socket = 0, recv_length = 0, port = 0;
    struct sockaddr_in serv_addr;
    char buffer[4096] = {0};
    char host[128] = {0};
    char http_header[1024] = {0};
    char *substr = NULL;
    int xml_start = 0, length_start = 0;
    char str_xml_length[1024] = {0};
    int xml_length = 0, segment_length = 0;
    char xml_reply[65536] = {0};
    int i = 0;
    char http_terminate[5] = {0};

    // http header end's format "\r\n\r\n"
    http_terminate[0] = '\r';
    http_terminate[1] = '\n';
    http_terminate[2] = '\r';
    http_terminate[3] = '\n';
    http_terminate[4] = '\0';

    // xaddrs filter to host address and port
    xaddrsExtractHost(xaddrs, host, &port);

    // create TCP connection for IPcam
    if((onvif_socket = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DEBUG_PRINT("[%s] ERROR: tcp socket cannot create\n", __FUNCTION__);
        return NULL;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(host);

    if(lwip_connect(onvif_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        lwip_close(onvif_socket);
        
        DEBUG_PRINT("[%s] ERROR: tcp connect fail\n", __FUNCTION__);
        return NULL;
    }

    // send request-cmd to IPCam
    if(lwip_send(onvif_socket, cmd, strlen(cmd), 0) < 0) {
        lwip_close(onvif_socket);
        
        DEBUG_PRINT("[%s] ERROR: send failure\n", __FUNCTION__);
        return NULL;
    }

    // receive response from IPCam
    recv_length = lwip_recv(onvif_socket, buffer, 4096, 0);

    substr = strstr(buffer, http_terminate);
    if(substr == NULL) {
        lwip_close(onvif_socket);
        
        DEBUG_PRINT("[%s] ERROR: Not found http header, size= %d\n", __FUNCTION__, recv_length);
        return NULL;
    }

    // ignore http-terminate
    xml_start = substr - buffer + 4;
    if(xml_start >= 1024) {
        lwip_close(onvif_socket);
        
        DEBUG_PRINT("[%s] ERROR: http header(size= %d) is too big\n", __FUNCTION__, xml_start);
        return NULL;
    }

    // get http header    
    for(i = 0; i < xml_start; i++) {
      http_header[i] = buffer[i];
    }
    http_header[xml_start] = '\0';

    substr = strstr(http_header, "Content-Length:");
    if(substr == NULL) {
        lwip_close(onvif_socket);
        
        DEBUG_PRINT("[%s] ERROR: Not found \"Content-Length:\" field\n", __FUNCTION__);
        return NULL;
    }

    // ignore "Content-Length:"
    length_start = substr - http_header + 15;
    if((xml_start - length_start) > 1024) {
        lwip_close(onvif_socket);
        
        DEBUG_PRINT("[%s] ERROR: http header endline is too big\n", __FUNCTION__);
        return NULL;
    }

    // get "Content-Length" value, namely xml payload's size        
    for(i = length_start; i < xml_start; i++) {
        if(http_header[i] == '\r' && http_header[i+1] == '\n') {
            str_xml_length[i - length_start] = '\0';
            break;
        }
        else {
            str_xml_length[i - length_start] = http_header[i];
        }
    }

    // xml_length is xml payload's size
    xml_length = (int) strtol(str_xml_length, (char **)NULL, 10);
    if(xml_length > 65536) {
        lwip_close(onvif_socket);
        
        DEBUG_PRINT("[%s] ERROR: Payload(size=%d) is too big\n", __FUNCTION__, xml_length);
        return NULL;
    }
    

    // xml_reply is packet's xml payload
    for(i = 0; i < recv_length - xml_start; i++) {
        xml_reply[i] = buffer[i + xml_start];
    }

    // check if exist TCP packet segment
    segment_length = recv_length - xml_start;
    while(segment_length < xml_length) {
        // receive the next TCP packet segment
        recv_length = lwip_recv(onvif_socket, buffer, 4096, 0);

        for(i = 0; i < recv_length; i++) {
            xml_reply[i + segment_length] = buffer[i];
        }
        segment_length = segment_length + recv_length;
    }
    xml_reply[xml_length] = '\0';


    // close socket
    lwip_close(onvif_socket);

    // transfer to xml format
    reply = xmlParseMemory(xml_reply, xml_length);

    return reply;
}


static int getXmlValue(xmlDocPtr doc, xmlChar *xpath, char *buf, int buf_length) 
{
    xmlChar *keyword = NULL;
    xmlXPathContextPtr context = NULL;
    xmlXPathObjectPtr result = NULL;
    
    // new a xpath context
    context = xmlXPathNewContext(doc);    
    if(context == NULL) {
        return -1;
    }
    
    // register namespace to xpath
    xmlXPathRegisterNs(context, BAD_CAST "s", BAD_CAST "http://www.w3.org/2003/05/soap-envelope");
    xmlXPathRegisterNs(context, BAD_CAST "trt", BAD_CAST "http://www.onvif.org/ver10/media/wsdl");
    xmlXPathRegisterNs(context, BAD_CAST "tt", BAD_CAST "http://www.onvif.org/ver10/schema");
    xmlXPathRegisterNs(context, BAD_CAST "tds", BAD_CAST "http://www.onvif.org/ver10/device/wsdl");
    xmlXPathRegisterNs(context, BAD_CAST "ter", BAD_CAST "http://www.onvif.org/ver10/error");
    xmlXPathRegisterNs(context, BAD_CAST "timg", BAD_CAST "http://www.onvif.org/ver20/imaging/wsdl");
    xmlXPathRegisterNs(context, BAD_CAST "wsa5", BAD_CAST "http://www.w3.org/2005/08/addressing");
    xmlXPathRegisterNs(context, BAD_CAST "wsnt", BAD_CAST "http://docs.oasis-open.org/wsn/b-2");
    xmlXPathRegisterNs(context, BAD_CAST "d", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery");
    xmlXPathRegisterNs(context, BAD_CAST "a", BAD_CAST "http://schemas.xmlsoap.org/ws/2004/08/addressing");

    // use xpath to find node
    result = xmlXPathEvalExpression(xpath, context);

    // free xpath context
    xmlXPathFreeContext(context);
    
    if(result == NULL) {
        // element no found
        DEBUG_PRINT("[%s] ERROR: Not found Xpath(%s)\n", __FUNCTION__, xpath);
        return -2;
    }

    if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        if((strcmp((char*) xpath, "//s:Body//s:Fault//s:Code//s:Subcode//s:Value") != 0) && (strcmp((char*) xpath, "//s:Body//s:Fault//s:Reason//s:Text") != 0)) {
            DEBUG_PRINT("[%s] ERROR: Node should be exist\n", __FUNCTION__);
        }

        return -3;
    }

    if(result) {
        // get node's value
        keyword = xmlNodeListGetString(doc, result->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
        if(keyword != NULL) {
            if(strlen((char*) keyword) > buf_length-1) {
                DEBUG_PRINT("[%s] ERROR: Overflow risk\n", __FUNCTION__);
                xmlXPathFreeObject(result);
                free(keyword);
                return -4;
            }
            else {
                strcpy(buf, (char*) keyword);
            }
        }
    }

    xmlXPathFreeObject(result);
    if(keyword != NULL)
        free(keyword);
    
    return 0;
}

static void getScopeField(char *scope, char *field_name, char *value) 
{
    char *field = NULL, *mark = NULL;
    char field_contents[1024] = {0};
    int length = 0, offset = 0;
    int i = 0, j = 0;
    int result = 0;

    // ex: scope="onvif://www.onvif.org/location/country/Taiwan onvif://www.onvif.org/name/ITE"
    field = strstr(scope, field_name);
    if(field != NULL) {
        field = field + strlen(field_name);

        // ex: field="onvif://www.onvif.org/name/ITE onvif://www.onvif.org/hardware/9862"
        mark = strstr(field, " ");
        if(mark != NULL) {
            length = mark - field;
            strncpy(field_contents, field, length);
        }
        else {
            strcpy(field_contents, field);
        }

        length = strlen(field_contents);

        for(i = 0; i < length; i++) {
            j = i - offset;
            if(field_contents[i] == '%') {
                // ex: field_contents=%87
                char middle[3] = {0};
                char buf[10] = {0};
                char *ptr;
                i++; 
                offset++;
                middle[0] = field_contents[i];
                i++; 
                offset++;
                middle[1] = field_contents[i];
                
                result = strtol(middle, &ptr, 16);               
                snprintf(buf, sizeof(buf), "%d", result);
                strncpy(&value[j], buf, strlen(buf) + 1);
            }
            else {
                // ex: field_contents=ITE
                value[j] = field_contents[i];
            }
        }
        value[length] = '\0';
    }
}

static int checkForXmlErrorMsg(xmlDocPtr doc, char *error_msg) {

    if(getXmlValue(doc, BAD_CAST "//s:Body//s:Fault//s:Code//s:Subcode//s:Value", error_msg, 1024) == 0) {
        return -1;
    }
    else if(getXmlValue(doc, BAD_CAST "//s:Body//s:Fault//s:Reason//s:Text", error_msg, 1024) == 0) {
        return -1;
    }
    
    return 0;
}

static void getCameraName(int index, onvifConn *onvif_session, onvifData *onvif_data) 
{
    xmlDocPtr xml_input = xmlParseMemory(onvif_session->buf[index], onvif_session->buf_len[index]);
    char scopes[8192] = {0};
    char temp_mf[1024] = {0};
    char temp_hd[1024] = {0};

    // get IPcam manufacturer's info   
    getXmlValue(xml_input, "//s:Body//d:ProbeMatches//d:ProbeMatch//d:Scopes", scopes, 8192);

    getScopeField(scopes, "onvif://www.onvif.org/name/", temp_mf);
    getScopeField(scopes, "onvif://www.onvif.org/hardware/", temp_hd);

    // found manufacturer
    if(strlen(temp_mf) > 0) {
        strcat(onvif_data->camera_name, temp_mf);
    }

    // found IPcam module number
    if(strlen(temp_hd) > 0) {
        if (strstr(temp_mf, temp_hd) == NULL) {
            strcat(onvif_data->camera_name, " ");
            strcat(onvif_data->camera_name, temp_hd);
        }
    }

    // Not found Camera name
    if(strlen(onvif_data->camera_name) == 0) {
        strcpy(onvif_data->camera_name, "UNKNOWN CAMERA");
    }

    xmlFreeDoc(xml_input);
}

static void getXAddrs(int index, onvifConn *onvif_session, onvifData *onvif_data) 
{
    char *sub = NULL;
    int mark = 0;
    xmlDocPtr xml_input = xmlParseMemory(onvif_session->buf[index], onvif_session->buf_len[index]);
    
    if(getXmlValue(xml_input, BAD_CAST "//s:Body//d:ProbeMatches//d:ProbeMatch//d:XAddrs", onvif_data->xaddrs, 1024) == 0) {
        sub = strstr(onvif_data->xaddrs, " ");
        if(sub != NULL) {
            mark = sub - onvif_data->xaddrs;
            onvif_data->xaddrs[mark] = '\0';
        }

        // copy xaddr to device_service
        strcpy(onvif_data->device_service, onvif_data->xaddrs);
    }
    
    xmlFreeDoc(xml_input);
}

static void getOnvifService(char *service, bool post) {
    int mark = 0, i = 0;
    char temp_buf[128] = {0};
    int length = strlen(service);
    char *sub = NULL;

    sub = strstr(service, "//");
    
    if(sub != NULL) {
        // ingore "http://"
        mark = sub - service;
        mark = mark + 2;

        for(i = 0; i < length - mark; i++) {
            service[i] = service[i + mark];
        }
        service[i] = '\0';

        // use the first address of xaddrs
        sub = strstr(service, " ");
        if(sub != NULL) {
            mark = sub - service;
            service[mark] = '\0';
        }

        length = strlen(service);
        sub = strstr(service, "/");
        if(sub != NULL) {
            // ignore IP address
            mark = sub - service;
            for (i=0; i< length - mark; i++) {
                service[i] = service[i + mark];
            }
            service[i] = '\0';

            // Add http post cmd
            if(post) {
                strcat(temp_buf, "POST ");
                strcat(temp_buf, service);
                strcat(temp_buf, " HTTP/1.1\r\n");
                strcpy(service, "");
                strcpy(service, temp_buf);
            }
        }
    }
    else {
        DEBUG_PRINT("[%s] ERROR: service is not found\n", __FUNCTION__);
    }
}

static int getTimeOffset(onvifData *onvif_data) {
    int result = 0;
    char cmd[4096] = {0};
    char hour[16] = {0}, min[16] = {0}, sec[16] = {0}, year[16] = {0}, month[16] = {0}, day[16] = {0}, daylight[16] = {0};
    int isDst = 0;
    time_t now, utc_time_ite, utc_time_ipcam, offset;
    struct tm *utc_ite, *utc_ipcam;

    // create GetSystemDateAndTime header
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewDocNode(doc, NULL, BAD_CAST "Envelope", NULL);
    xmlDocSetRootElement(doc, root);
    xmlNsPtr ns_env = xmlNewNs(root, BAD_CAST "http://www.w3.org/2003/05/soap-envelope", BAD_CAST "SOAP-ENV");
    xmlNsPtr ns_tds = xmlNewNs(root, BAD_CAST "http://www.onvif.org/ver10/device/wsdl", BAD_CAST "tds");
    xmlSetNs(root, ns_env);
    xmlNodePtr body = xmlNewTextChild(root, ns_env, BAD_CAST "Body", NULL);
    xmlNewTextChild(body, ns_tds, BAD_CAST "GetSystemDateAndTime", NULL);
    
    // create http header
    addHttpHeader(doc, root, onvif_data->xaddrs, onvif_data->device_service, cmd, 4096);

    // send cmd and receive response
    xmlDocPtr reply = sendCommand(cmd, onvif_data->xaddrs);

    if(reply != NULL) {
        getXmlValue(reply, BAD_CAST "//s:Body//tds:GetSystemDateAndTimeResponse//tds:SystemDateAndTime//tt:UTCDateTime//tt:Time//tt:Hour", hour, 16);
        getXmlValue(reply, BAD_CAST "//s:Body//tds:GetSystemDateAndTimeResponse//tds:SystemDateAndTime//tt:UTCDateTime//tt:Time//tt:Minute", min, 16);
        getXmlValue(reply, BAD_CAST "//s:Body//tds:GetSystemDateAndTimeResponse//tds:SystemDateAndTime//tt:UTCDateTime//tt:Time//tt:Second", sec, 16);
        getXmlValue(reply, BAD_CAST "//s:Body//tds:GetSystemDateAndTimeResponse//tds:SystemDateAndTime//tt:UTCDateTime//tt:Date//tt:Year", year, 16);
        getXmlValue(reply, BAD_CAST "//s:Body//tds:GetSystemDateAndTimeResponse//tds:SystemDateAndTime//tt:UTCDateTime//tt:Date//tt:Month", month, 16);
        getXmlValue(reply, BAD_CAST "//s:Body//tds:GetSystemDateAndTimeResponse//tds:SystemDateAndTime//tt:UTCDateTime//tt:Date//tt:Day", day, 16);
        getXmlValue(reply, BAD_CAST "//s:Body//tds:GetSystemDateAndTimeResponse//tds:SystemDateAndTime//tt:DaylightSavings", daylight, 16);

        DEBUG_PRINT("[%s] IPCam Data= %s:%s:%s Time= %s:%s:%s daylight=%s\n", __FUNCTION__, year, month, day, hour, min, sec, daylight);

        // check daylight saving time
        if(strcmp(daylight, "true") == 0) {
            isDst = 1;
        }

        // get ITE time
        now = time(NULL);
        utc_ite = gmtime(&now);
        utc_ite->tm_isdst = -1;        
        utc_time_ite = mktime(utc_ite);
        
        DEBUG_PRINT("[%s] ITE Data= %d:%d:%d Time= %d:%d:%d day_saving=%d\n", __FUNCTION__, utc_ite->tm_year + 1900, utc_ite->tm_mon + 1, utc_ite->tm_mday, utc_ite->tm_hour, utc_ite->tm_min, utc_ite->tm_sec, utc_ite->tm_isdst);

        // create IPcam time
        utc_ipcam = localtime(&now);
        utc_ipcam->tm_year = atoi(year) - 1900;
        utc_ipcam->tm_mon = atoi(month) - 1;
        utc_ipcam->tm_mday = atoi(day);
        utc_ipcam->tm_hour = atoi(hour);
        utc_ipcam->tm_min = atoi(min);
        utc_ipcam->tm_sec = atoi(sec);
        utc_ipcam->tm_isdst = isDst;
        utc_time_ipcam = mktime(utc_ipcam);

        // calulate offset between ITE and IPCam
        offset = utc_time_ipcam - utc_time_ite;
        onvif_data->time_offset = offset;

        // check error        
        result = checkForXmlErrorMsg(reply, onvif_data->error_msg);
        if(result == -1) {
            DEBUG_PRINT("[%s] ERROR: %s\n", __FUNCTION__, onvif_data->error_msg);
        }
        
        xmlFreeDoc(reply);              
    }
    else {
        result = -1;
        DEBUG_PRINT("[%s] ERROR: No XML reply\n", __FUNCTION__);
        strcpy(onvif_data->error_msg, "No XML reply");
    }

    return result;
}

static int getCapabilities(onvifData *onvif_data)
{
    int result = 0;
    char cmd[4096] = {0};

    // create GetCapabilities header
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewDocNode(doc, NULL, BAD_CAST "Envelope", NULL);
    xmlDocSetRootElement(doc, root);
    xmlNsPtr ns_env = xmlNewNs(root, BAD_CAST "http://www.w3.org/2003/05/soap-envelope", BAD_CAST "SOAP-ENV");
    xmlNsPtr ns_tds = xmlNewNs(root, BAD_CAST "http://www.onvif.org/ver10/device/wsdl", BAD_CAST "tds");
    xmlSetNs(root, ns_env);
    xmlNodePtr body = xmlNewTextChild(root, ns_env, BAD_CAST "Body", NULL);
    xmlNodePtr capabilities = xmlNewTextChild(body, ns_tds, BAD_CAST "GetCapabilities", NULL);
    xmlNewTextChild(capabilities, ns_tds, BAD_CAST "Category", "All");    

    // create http header
    addHttpHeader(doc, root, onvif_data->xaddrs, onvif_data->device_service, cmd, 4096);
    
    xmlDocPtr reply = sendCommand(cmd, onvif_data->xaddrs);
    if(reply != NULL) {
        // get Media service
        if(getXmlValue(reply, BAD_CAST "//s:Body//tds:GetCapabilitiesResponse//tds:Capabilities//tt:Media//tt:XAddr", onvif_data->media_service, 128) == 0) {
            getOnvifService(onvif_data->media_service, true);
        }
        else {
            DEBUG_PRINT("[%s] ERROR: Not found Media Capability\n", __FUNCTION__);
            result  = -1;
            xmlFreeDoc(reply);
            return result;
        }

        // check error
        result = checkForXmlErrorMsg(reply, onvif_data->error_msg);
        if(result == -1) {
            DEBUG_PRINT("[%s] ERROR: %s\n", __FUNCTION__, onvif_data->error_msg);
        }
        
        xmlFreeDoc(reply);
    }
    else {
        result = -1;
        DEBUG_PRINT("[%s] ERROR: No XML reply\n", __FUNCTION__);
        strcpy(onvif_data->error_msg, "No XML reply");
    }

    return result;
}

static int getProfiles(onvifData *onvif_data) {
    int result = 0;
    char cmd[4096] = {0};
    char encode_type[32] = {0}, width[32] = {0}, height[32] = {0}, frame_rate[32] = {0};

    // create GetProfiles header
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewDocNode(doc, NULL, BAD_CAST "Envelope", NULL);
    xmlDocSetRootElement(doc, root);
    xmlNsPtr ns_env = xmlNewNs(root, BAD_CAST "http://www.w3.org/2003/05/soap-envelope", BAD_CAST "SOAP-ENV");
    xmlNsPtr ns_trt = xmlNewNs(root, BAD_CAST "http://www.onvif.org/ver10/media/wsdl", BAD_CAST "trt");
    xmlSetNs(root, ns_env);
    
    // add Digest header
    addDigestHeader(root, ns_env, onvif_data->username, onvif_data->password, onvif_data->time_offset);
    
    xmlNodePtr body = xmlNewTextChild(root, ns_env, BAD_CAST "Body", NULL);
    xmlNewTextChild(body, ns_trt, BAD_CAST "GetProfiles", NULL);

    // create http header
    addHttpHeader(doc, root, onvif_data->xaddrs, onvif_data->media_service, cmd, 4096);
    
    xmlDocPtr reply = sendCommand(cmd, onvif_data->xaddrs);
    if(reply != NULL) {
        // get encode type
        if(getXmlValue(reply, BAD_CAST "//s:Body//trt:GetProfilesResponse//trt:Profiles//tt:VideoEncoderConfiguration//tt:Encoding", encode_type, 32) == 0) {
            strcpy(onvif_data->encode_type, encode_type);
        }
        else {
            DEBUG_PRINT("[%s] ERROR: Not found video's encode type\n", __FUNCTION__);
        }

        // get width
        if(getXmlValue(reply, BAD_CAST "//s:Body//trt:GetProfilesResponse//trt:Profiles//tt:VideoEncoderConfiguration//tt:Resolution//tt:Width", width, 32) == 0) {
            onvif_data->width = atoi(width);
        }
        else {
            DEBUG_PRINT("[%s] ERROR: Not found video's width\n", __FUNCTION__);
        }

        // get height
        if(getXmlValue(reply, BAD_CAST "//s:Body//trt:GetProfilesResponse//trt:Profiles//tt:VideoEncoderConfiguration//tt:Resolution//tt:Height", height, 32) == 0 ) {
            onvif_data->height = atoi(height);
        }
        else {
            DEBUG_PRINT("[%s] ERROR: Not found video's height\n", __FUNCTION__);
        }

        // get frame rate
        if(getXmlValue(reply, BAD_CAST "//s:Body//trt:GetProfilesResponse//trt:Profiles//tt:VideoEncoderConfiguration//tt:RateControl//tt:FrameRateLimit", frame_rate, 32) == 0) {
            onvif_data->frame_rate = atoi(frame_rate);
        }
        else {
            DEBUG_PRINT("[%s] ERROR: Not found video's frame rate\n", __FUNCTION__);
        }

        // check error
        result = checkForXmlErrorMsg(reply, onvif_data->error_msg);
        if(result == -1) {
            DEBUG_PRINT("[%s] ERROR: %s\n", __FUNCTION__, onvif_data->error_msg);
        }
        
        xmlFreeDoc(reply);
    }
    else {
        result = -1;
        DEBUG_PRINT("[%s] ERROR: No XML reply\n", __FUNCTION__);
        strcpy(onvif_data->error_msg, "No XML reply");
    }
    
    return result;
}

static void GetUuid(char *uuid)
{
    int i;
    srand(time(NULL));
    strcpy(uuid, "urn:uuid:");
    
    for(i = 0; i < 16; i++) {
        char buf[3];
        sprintf(buf, "%02x", (unsigned char) rand());
        strcat(uuid, buf);
        
        if(i == 3 || i == 5 || i == 7 || i == 9)
            strcat(uuid, "-");
    }
}

static void GetDiscoveryXmlv1(char *buffer, int buf_size, char *uuid)
{
    // makeup WS-Discovery
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root = xmlNewDocNode(doc, NULL, BAD_CAST "Envelope", NULL);
    xmlDocSetRootElement(doc, root);
    xmlNewProp(root, BAD_CAST "xmlns:SOAP-ENV", BAD_CAST "http://www.w3.org/2003/05/soap-envelope");
    xmlNewProp(root, BAD_CAST "xmlns:a", BAD_CAST "http://schemas.xmlsoap.org/ws/2004/08/addressing");
    xmlNsPtr ns_soap = xmlNewNs(root, NULL, BAD_CAST "SOAP-ENV");
    xmlNsPtr ns_a = xmlNewNs(root, NULL, BAD_CAST "a");
    xmlSetNs(root, ns_soap);

    // makupu SOAP Header
    xmlNodePtr header = xmlNewTextChild(root, ns_soap, BAD_CAST "Header", NULL);
    xmlNodePtr action = xmlNewTextChild(header, ns_a, BAD_CAST "Action", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe");
    xmlNewProp(action, BAD_CAST "SOAP-ENV:mustUnderstand", BAD_CAST "1");
    xmlNodePtr messageid = xmlNewTextChild(header, ns_a, BAD_CAST "MessageID", BAD_CAST uuid);
    xmlNodePtr replyto = xmlNewTextChild(header, ns_a, BAD_CAST "ReplyTo", NULL);
    xmlNodePtr address = xmlNewTextChild(replyto, ns_a, BAD_CAST "Address", BAD_CAST "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous");
    xmlNodePtr to = xmlNewTextChild(header, ns_a, BAD_CAST "To", BAD_CAST "urn:schemas-xmlsoap-org:ws:2005:04:discovery");
    xmlNewProp(to, BAD_CAST "SOAP-ENV:mustUnderstand", BAD_CAST "1");

    // makeup SOAP Boby and add Probe message
    xmlNodePtr body = xmlNewTextChild(root, ns_soap, BAD_CAST "Body", NULL);
    xmlNodePtr probe = xmlNewTextChild(body, NULL, BAD_CAST "Probe", NULL);
    xmlNewProp(probe, BAD_CAST "xmlns:p", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery");
    xmlNsPtr ns_p = xmlNewNs(probe, NULL, BAD_CAST "p");
    xmlSetNs(probe, ns_p);
    xmlNodePtr types = xmlNewTextChild(probe, NULL, BAD_CAST "Types", BAD_CAST "dp0:NetworkVideoTransmitter");
    xmlNewProp(types, BAD_CAST "xmlns:d", BAD_CAST "http://schemas.xmlsoap.org/ws/2005/04/discovery");
    xmlNewProp(types, BAD_CAST "xmlns:dp0", BAD_CAST "http://www.onvif.org/ver10/network/wsdl");
    xmlNsPtr ns_d = xmlNewNs(types, NULL, BAD_CAST "d");
    xmlSetNs(types, ns_d);

    // xml format transfer to buffer
    xmlOutputBufferPtr outputbuffer = xmlAllocOutputBuffer(NULL);
    xmlNodeDumpOutput(outputbuffer, doc, root, 0, 0, NULL);
    int size = xmlOutputBufferGetSize(outputbuffer);
    strcpy(buffer, (char*)xmlOutputBufferGetContent(outputbuffer));
    
    xmlOutputBufferFlush(outputbuffer);
    xmlOutputBufferClose(outputbuffer);
    xmlFreeDoc(doc);
}

static void GetDiscoveryXmlv2(char *buffer)
{
    char *xml_string = "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:a=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\"><s:Header><a:Action s:mustUnderstand=\"1\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</a:Action><a:MessageID>uuid:6bbdae2d-f229-42c8-a27b-93880fb80826</a:MessageID><a:ReplyTo><a:Address>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:Address></a:ReplyTo><a:To s:mustUnderstand=\"1\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</a:To></s:Header><s:Body><Probe xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\"><d:Types xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:dp0=\"http://www.onvif.org/ver10/device/wsdl\">dp0:Device</d:Types></Probe></s:Body></s:Envelope>";
    strcpy(buffer, xml_string);
}

void OnviInit(onvifConn *conn) 
{
    // init Onvif conn
    memset(conn, 0, sizeof(onvifConn));    

    // init xml parser
    xmlInitParser();
}

void OnvifDeinit(void)
{
    // stop xml parser
    xmlCleanupParser();
}

int OnvifDiscovery(onvifConn *conn) 
{
    int ret = 0;
    struct sockaddr_in onvif_address;
    int onvif_socket, loopback = 0;
    char onvif_msg[1024] = {0};
    int msg_size = 0;
    unsigned int address_size = 0;
    struct timeval tv;
    int keep_receive = 1;
    int cam_count = 0;

    // set discovery_type and Uuid
    conn->discovery_type = 1; //use default type=1
    GetUuid(conn->uuid);

    // get Onvif discovery payload
    memset(onvif_msg, 0, sizeof(char)*1024);
    if(conn->discovery_type == 1) {
        // Standard Onvif Format
        GetDiscoveryXmlv1(onvif_msg, 1024, conn->uuid);
        msg_size = strlen(onvif_msg);
    }
    else if(conn->discovery_type == 2) {
        // non-Standard Onvif Format
        GetDiscoveryXmlv2(onvif_msg);
        msg_size = strlen(onvif_msg);
    }
    else{
        DEBUG_PRINT("[%s] ERROR: No support Discovery-type\n", __FUNCTION__);
        return -1;
    }

    // open multicast socket
    onvif_socket = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if(onvif_socket < 0) {
        DEBUG_PRINT("[%s] ERROR: Onvif broadcast socket create failure\n", __FUNCTION__);
        return -1;
    }

    // set receive's timeout
    tv.tv_sec = 500;  //FIXME: tv_sec = 500 is 500 ms
    tv.tv_usec = 0;
    ret = lwip_setsockopt(onvif_socket, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
    if(ret < 0) {
        DEBUG_PRINT("[%s] ERROR: setsockopt(SO_RCVTIMEO)\n", __FUNCTION__);
        lwip_close(onvif_socket);
        return -1;
    }

    // disable the loopback of outgoing multicast pkg
    ret = lwip_setsockopt(onvif_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback));
    if(ret < 0) {
        DEBUG_PRINT("[%s] ERROR: setsockopt(IP_MULTICAST_LOOP)\n", __FUNCTION__);
        lwip_close(onvif_socket);
        return -1;
    }

    // start to send onvif-discovery pkg
    memset(&onvif_address, 0, sizeof(struct sockaddr_in));
    onvif_address.sin_family = AF_INET;
    onvif_address.sin_port = htons(3702);
    onvif_address.sin_addr.s_addr = inet_addr("239.255.255.250");
    address_size = sizeof(onvif_address);
    ret = lwip_sendto(onvif_socket, onvif_msg, msg_size, 0, (struct sockaddr*)&onvif_address, address_size);
    if(ret < 0) {
        DEBUG_PRINT("[%s] ERROR: sendto failure\n", __FUNCTION__);
        lwip_close(onvif_socket);
        return -1;
    }

    // receive IPCam's response
    while(keep_receive) {
        conn->buf_len[cam_count] = lwip_recvfrom(onvif_socket, conn->buf[cam_count], sizeof(conn->buf[cam_count]), 0, (struct sockaddr*) &onvif_address, &address_size);
        DEBUG_PRINT("[%s] (%d)recvifrom ret=%d\n", __FUNCTION__, cam_count, conn->buf_len[cam_count]);
    
        if(conn->buf_len[cam_count] > 0) {
            // Get one IPCam's response
            cam_count++;
        }
        else {
            // Timeout or Error
            keep_receive = 0;
            if(conn->buf_len[cam_count] < 0) {
                //DEBUG_PRINT("[%s] ERROR: recvfrom failure(timeout)\n", __FUNCTION__);
                //lwip_close(broadcast_socket);
                //return -1;
            }
        }
    }

    // stop conn
    lwip_close(onvif_socket);

    return cam_count;
}

void OnvifDataParser(int camIdx, onvifConn *conn, onvifData *data)
{
    // reset onvifData
    memset(data, 0, sizeof(onvifData));

    // get IPCam name
    getCameraName(camIdx, conn, data);

    // get xAddrs
    getXAddrs(camIdx, conn, data);

    // get device service
    getOnvifService(data->device_service, true);

    // get time offset
    getTimeOffset(data);
}

int OnvifGetProfile(onvifData *data)
{
    int result = 0;

    // get IPCam capabilities
    result = getCapabilities(data);
    if(result == 0) {
        // get IPCam profiles
        result = getProfiles(data);
    }

    return result;
}

int OnvifGetxAddrsNum(char *xAddrs)
{
    int ret = 0;
    int i = 0, xAddrsLen = 0;

    if(xAddrs == NULL) {
        DEBUG_PRINT("[%s] ERROR: xAddrs is invaild\n", __FUNCTION__);
        return 0;
    }

    xAddrsLen = strlen(xAddrs);
    for(i = 0; i < xAddrsLen; i++) {
        if(xAddrs[i] == ' ') {
            // There are the another IP address in xAddrs list.
            ret++;
        }
    }

    return ret + 1;
}

void OnvifTransferAddr(int idx, char *ip, char *xAddrs)
{
    int count = 0, length = 0;
    char dupstr[1024] = {0};
    char *space = " ";
    char *substr = NULL, *saveptr = NULL;
    char *mark = NULL;
    int i;

    if(xAddrs == NULL) {
        DEBUG_PRINT("[%s] ERROR: xAddrs is invaild\n", __FUNCTION__);
        return;
    }

    strcpy(dupstr, xAddrs);

    if(strstr(dupstr, space) != NULL) {
        // More than one IP address exist in xAddrs list
        
        substr = strtok_r(dupstr, space, &saveptr);
        do {
            // Find the idx IP address
            if(count == idx)
                break;
            
            substr = strtok_r(NULL, space, &saveptr);
            count++;
        } while(substr);

        //DEBUG_PRINT("[%s] count=%d substr=%s\n", __FUNCTION__, count, substr);

        // parse "http://"
        substr = substr + 7;

        // ex: substr=192.168.1.1/device/service mark=/device/service
        mark = strstr(substr, "/");
        if(mark != NULL) {
            length = mark - substr;
            strncpy(ip, substr, length);
        }
        else {
            strcpy(ip, substr);
        }      
    }
    else {
        // The only one IP address exist in xAddrs list
        
        length = strlen(dupstr);

        // parse "http://"
        substr = strstr(dupstr, "//");
        substr = substr + 2;

        for(i = 0; i < length - (substr - dupstr); i++) {
            if(substr[i] == '/') {
                ip[i] = '\0';
                break;
            }
            else {
                ip[i] = substr[i];
            }
        }
    }

}

void OnvifDataPrint(onvifData *data)
{
    printf("****IPCam****\n");
    printf("Name: %s\n", data->camera_name);
    printf("\tXaddr: %s\n", data->xaddrs);
    printf("\tUsername: %s\n", data->username);
    printf("\tPassword: %s\n", data->password);
    printf("\tDevice Service: %s\n", data->device_service);
    printf("\tMedia Service: %s\n", data->media_service);
    printf("\tError: %s\n", data->error_msg);
    printf("\tVideo Encode Type: %s\n", data->encode_type);
    printf("\tVideo Width: %d\n", data->width);
    printf("\tVideo Height: %d\n", data->height);
    printf("\tVideo Frame Rate: %d\n", data->frame_rate);
    printf("*************\n");
}


