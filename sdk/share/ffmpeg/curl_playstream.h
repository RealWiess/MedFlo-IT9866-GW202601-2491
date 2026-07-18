#ifndef CURL_PLAYSTRAM_H
#define CURL_PLAYSTRAM_H

bool curl_check_stream_playing();
bool curl_check_stream_EOF();
void curl_set_stream_volume(int volume);
void curl_set_stream_url(char *s_url);
void curl_set_download_filepath(char *d_path);
void curl_stream_start();
void curl_stream_stop();

#endif

