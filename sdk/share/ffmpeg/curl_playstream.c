#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include "curl/curl.h"
#include "castor3player.h"

#define MIN_PLAYVIDEO_SIZE 512 * 1024
#define MAX_PATH_LENGTH 256

static bool b_ForceStopDownload = false;
static bool b_ForcePauseDownload = false;
static int cur_filesize = 0;
static bool isfile_playing = false;
static bool isfile_EOF = false;
static char url[MAX_PATH_LENGTH];
static char filepath[MAX_PATH_LENGTH];
static pthread_t task;
static MTAL_SPEC mtal_spec = {0};
static int stream_volume = 0;
static pthread_mutex_t curl_stream_mutex = PTHREAD_MUTEX_INITIALIZER;


static void EventHandler(PLAYER_EVENT nEventID, void *arg)
{
    switch(nEventID)
    {
        case PLAYER_EVENT_EOF:
            printf("File EOF\n");
			isfile_EOF = true;
            break;
        default:
            break;
    }
}

static size_t play_stream(void *buffer, size_t size, size_t nmemb, void *stream)
{
#ifndef _WIN32
    fwrite(buffer, size * nmemb, 1, stream);
	cur_filesize += size * nmemb;
	if(cur_filesize > MIN_PLAYVIDEO_SIZE && !isfile_playing)
	{
        printf("start play video streaming");
		isfile_playing = true;
		
		strlcpy(mtal_spec.srcname, filepath, sizeof(mtal_spec.srcname));
		mtal_spec.vol_level = stream_volume;
		mtal_pb_select_file(&mtal_spec);
        mtal_pb_play();
	}
    //printf("pkt size : %d\n",size * nmemb);
#endif

	return size * nmemb;
}

static void* curl_play_stream(void *arg)
{
	FILE *fp = NULL;
	CURL* curl = NULL;
	CURLM *curlm;
	CURLcode res = -1;
	int still_running = 0;

	cur_filesize = 0;
	b_ForceStopDownload = false;
	b_ForcePauseDownload = false;

    printf("+curl_play_stream(%s)\n", url);
#ifndef _WIN32
	fp = fopen(filepath, "wb+");
#endif
	if(fp != NULL)
	{
		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl = curl_easy_init();
		if (!curl)
	    {
	        printf("curl_easy_init() fail.\n");
	        res = -1;
	        goto end;
	    }
		
		curlm = curl_multi_init();
	    if (!curlm)
	    {
	        printf("curl_multi_init() fail.\n");
	        res = -1;
	        goto end;
	    }
		
		curl_easy_setopt(curl, CURLOPT_URL, url);
	    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, play_stream);
	    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_multi_add_handle(curlm, curl);
	    do{
			while(b_ForcePauseDownload) usleep(1000);
	        res = curl_multi_perform(curlm, &still_running);
	        if (CURLE_OK != res)
	        {
	            printf("curl_multi_perform() fail: %d\n", res);
	            break;
	        }
	        usleep(1000);
	    }while(still_running!=0 && !b_ForceStopDownload);

	    //res = curl_easy_perform(curl);
	    if (CURLE_OK != res || b_ForceStopDownload)
	    {
	        printf("Download file fail: %d\n",res);
	        res = -1;
	        goto end;
	    }
   
		printf("### Recv data finish ###\n");
end:
		if (curl)
	        curl_easy_cleanup(curl);
	   
	    if (curlm)
	        curl_multi_cleanup(curlm);

		curl_global_cleanup();
	}
	else
	{
		printf("Open File %s fail\n", filepath);
	}
	
    printf("-curl_play_stream()\n");
	
#ifndef _WIN32
	if(fp) 
		fclose(fp);
#endif

	pthread_exit(NULL);
	return 0;
}

bool curl_check_stream_playing()
{
	return isfile_playing;
}

bool curl_check_stream_EOF()
{
	return isfile_EOF;
}

void curl_set_stream_volume(int volume)
{
	stream_volume = volume;
}

void curl_set_stream_url(char *s_url)
{
	strlcpy(url, s_url, MAX_PATH_LENGTH);
}

void curl_set_download_filepath(char *d_path)
{
	strlcpy(filepath, d_path, MAX_PATH_LENGTH);
}

void curl_stream_start()
{
    pthread_attr_t  attr;
	pthread_mutex_lock(&curl_stream_mutex);

	isfile_playing = false;
	isfile_EOF = false;
	
#ifdef CFG_VIDEO_ENABLE
    mtal_pb_init(EventHandler);
#endif
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 16*1024);
	pthread_create(&task, &attr, curl_play_stream, NULL);
	
	pthread_mutex_unlock(&curl_stream_mutex);
}

void curl_stream_stop()
{
	pthread_mutex_lock(&curl_stream_mutex);
	b_ForceStopDownload = true;
#ifndef WIN32	
	if(task)
#endif		
	{
		pthread_join(task, NULL);
#ifndef WIN32		
		task = 0;
#endif
	}

#ifdef CFG_VIDEO_ENABLE
    mtal_pb_stop();
    mtal_pb_exit();
#endif

	isfile_playing = false;
	isfile_EOF = false;

	pthread_mutex_unlock(&curl_stream_mutex);
}

