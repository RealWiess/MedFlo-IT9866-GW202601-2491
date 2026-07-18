import sys

with open('bt_log.c', 'r', encoding='utf-8') as f:
    content = f.read()

old_func = '''static void* HttpUploadTask(void* arg) {
    while (1) {
        char ip[16] = "0.0.0.0";
        bt_wifi_get_ip(ip, sizeof(ip));
        if (strcmp(ip, "0.0.0.0") == 0) {
            usleep(1000000); // 1 second if WiFi not connected
            continue;
        }

        GatewayBleLog current_log;
        bool found = false;
        int log_idx = -1;

        pthread_mutex_lock(&s_log_mutex);
        int index = s_log_head;
        for (int i = 0; i < s_log_count; i++) {
            if (!s_log_buffer[index].uploaded) {
                current_log = s_log_buffer[index];
                log_idx = index;
                found = true;
                break;
            }
            index = (index + 1) % MAX_LOG_COUNT;
        }
        pthread_mutex_unlock(&s_log_mutex);

        if (!found) {
            usleep(500000); // 500 ms wait if no data
            continue;
        }

        // Format JSON payload
        struct tm *tm_info = localtime((time_t*)&current_log.timestamp);
        char time_str[30];
        if (tm_info) {
            strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S+08:00", tm_info);
        } else {
            snprintf(time_str, sizeof(time_str), "2026-07-03T00:00:00+08:00");
        }

        char data_hex[65] = {0};
        for (int i = 0; i < current_log.raw_len && i < 32; i++) {
            sprintf(&data_hex[i*2], "%02X", current_log.raw_data[i]);
        }
        if (current_log.raw_len == 0) {
            sprintf(data_hex, "%02X%02X", current_log.status_byte, current_log.rolling_counter);
        }

        char payload[512];
        snprintf(payload, sizeof(payload), 
            "[\n {\n  \"channel\":924000000,\n  \"sf\":10,\n  \"time\":\"%s\",\n  \"gwip\":\"%s\",\n  \"gwid\":\"00005813d34aed65\",\n  \"repeater\":\"00000000ffffffff\",\n  \"systype\":161,\n  \"rssi\":%d,\n  \"snr\":23,\n  \"snr_max\":33.8,\n  \"snr_min\":20,\n  \"macAddr\":\"0000%02X%02X%02X%02X%02X%02X\",\n  \"data\":\"%s\",\n  \"frameCnt\":%d,\n  \"fport\":3\n }\n]",
            time_str, ip, current_log.rssi, 
            current_log.mac[5], current_log.mac[4], current_log.mac[3], current_log.mac[2], current_log.mac[1], current_log.mac[0],
            data_hex, current_log.rolling_counter
        );

        // Send via libcurl
        CURL *curl = curl_easy_init();
        if(curl) {
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, "https://mqtt.noonspace.com/mainssl/modules/MySpace/index.php?sn=mqtt&pg=ZC4876");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

            printf("[HTTP] Uploading BLE log to Noonspace...\\n");
            CURLcode res = curl_easy_perform(curl);
            
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if(res == CURLE_OK) {
                printf("[HTTP] Upload SUCCESS.\\n");
                // Mark as uploaded
                pthread_mutex_lock(&s_log_mutex);
                s_log_buffer[log_idx].uploaded = true;
                pthread_mutex_unlock(&s_log_mutex);
            } else {
                printf("[HTTP] Upload FAILED, curl error code: %d\\n", res);
            }
        }
        
        usleep(100000); // 100 ms between uploads
    }
    return NULL;
}'''

new_func = '''#define BATCH_SIZE 50

static void* HttpUploadTask(void* arg) {
    while (1) {
        char ip[16] = "0.0.0.0";
        bt_wifi_get_ip(ip, sizeof(ip));
        if (strcmp(ip, "0.0.0.0") == 0) {
            usleep(1000000); // 1 second if WiFi not connected
            continue;
        }

        GatewayBleLog batch_logs[BATCH_SIZE];
        int batch_indices[BATCH_SIZE];
        int batch_count = 0;

        pthread_mutex_lock(&s_log_mutex);
        int index = s_log_head;
        for (int i = 0; i < s_log_count; i++) {
            if (!s_log_buffer[index].uploaded) {
                batch_logs[batch_count] = s_log_buffer[index];
                batch_indices[batch_count] = index;
                batch_count++;
                if (batch_count >= BATCH_SIZE) {
                    break;
                }
            }
            index = (index + 1) % MAX_LOG_COUNT;
        }
        pthread_mutex_unlock(&s_log_mutex);

        if (batch_count == 0) {
            usleep(500000); // 500 ms wait if no data
            continue;
        }

        // Allocate a large payload buffer (approx 400 bytes per log, so 20KB is safe for 50 logs)
        char *payload = malloc(20480);
        if (!payload) {
            printf("[HTTP] Failed to allocate memory for batch payload!\\n");
            usleep(1000000);
            continue;
        }
        
        strcpy(payload, "[\\n");
        int offset = 2; // length of "[\\n"
        
        for (int i = 0; i < batch_count; i++) {
            struct tm *tm_info = localtime((time_t*)&batch_logs[i].timestamp);
            char time_str[30];
            if (tm_info) {
                strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S+08:00", tm_info);
            } else {
                snprintf(time_str, sizeof(time_str), "2026-07-03T00:00:00+08:00");
            }

            char data_hex[65] = {0};
            for (int j = 0; j < batch_logs[i].raw_len && j < 32; j++) {
                sprintf(&data_hex[j*2], "%02X", batch_logs[i].raw_data[j]);
            }
            if (batch_logs[i].raw_len == 0) {
                sprintf(data_hex, "%02X%02X", batch_logs[i].status_byte, batch_logs[i].rolling_counter);
            }

            char obj[400];
            snprintf(obj, sizeof(obj), 
                " {\\n  \\"channel\\":924000000,\\n  \\"sf\\":10,\\n  \\"time\\":\\"%s\\",\\n  \\"gwip\\":\\"%s\\",\\n  \\"gwid\\":\\"00005813d34aed65\\",\\n  \\"repeater\\":\\"00000000ffffffff\\",\\n  \\"systype\\":161,\\n  \\"rssi\\":%d,\\n  \\"snr\\":23,\\n  \\"snr_max\\":33.8,\\n  \\"snr_min\\":20,\\n  \\"macAddr\\":\\"0000%02X%02X%02X%02X%02X%02X\\",\\n  \\"data\\":\\"%s\\",\\n  \\"frameCnt\\":%d,\\n  \\"fport\\":3\\n }",
                time_str, ip, batch_logs[i].rssi, 
                batch_logs[i].mac[5], batch_logs[i].mac[4], batch_logs[i].mac[3], batch_logs[i].mac[2], batch_logs[i].mac[1], batch_logs[i].mac[0],
                data_hex, batch_logs[i].rolling_counter
            );
            
            strcpy(payload + offset, obj);
            offset += strlen(obj);
            
            if (i < batch_count - 1) {
                strcpy(payload + offset, ",\\n");
                offset += 2;
            }
        }
        strcpy(payload + offset, "\\n]");

        // Send via libcurl
        CURL *curl = curl_easy_init();
        if(curl) {
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, "https://mqtt.noonspace.com/mainssl/modules/MySpace/index.php?sn=mqtt&pg=ZC4876");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Allow 10s for large payload

            printf("[HTTP] Uploading %d BLE logs to Noonspace...\\n", batch_count);
            CURLcode res = curl_easy_perform(curl);
            
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if(res == CURLE_OK) {
                printf("[HTTP] Batch Upload SUCCESS.\\n");
                // Mark all as uploaded
                pthread_mutex_lock(&s_log_mutex);
                for (int i = 0; i < batch_count; i++) {
                    s_log_buffer[batch_indices[i]].uploaded = true;
                }
                pthread_mutex_unlock(&s_log_mutex);
            } else {
                printf("[HTTP] Batch Upload FAILED, curl error code: %d\\n", res);
            }
        }
        
        free(payload);
        usleep(100000); // 100 ms between batch uploads
    }
    return NULL;
}'''

if old_func in content:
    content = content.replace(old_func, new_func)
    with open('bt_log.c', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Successfully patched bt_log.c")
else:
    print("Could not find the old function in bt_log.c. Here is a snippet of what we found:")
    print(content[:500])
