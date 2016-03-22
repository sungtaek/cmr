#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include <mediastreamer2/msconference.h>


int main(int argc, char **argv)
{
	int ret, i;
	MSAudioConference *chan;
	MSAudioEndpoint *sess[100];
	AudioStream *stream = NULL;
	RtpProfile prof;
	MSAudioConferenceParams cf_params;
	int sess_cnt = 0;
	int loc_port = 49152;

	ms_init();
	ortp_init();
	av_profile_init(&prof);
	cf_params.samplerate = 44100;

	for(i=1; i<argc; i++) {
		char *host;
		int port;
		char v[128];
		char *tok;

		strcpy(v, argv[i]);

		if((tok = strtok(v, ":"))) {
			if(strcmp(tok, "0.0.0.0") != 0) {
				host = tok;
				if((tok = strtok(NULL, ":"))) {
					port = atoi(tok);
				}
				else {
					port = 60000;
				}
			}
			else {
				host = NULL;
				port = 0;
			}

			printf("create session[%s:%d]\n", (host)?host:"", port);
			if(!(stream = audio_stream_new(loc_port, loc_port+1, FALSE))) {
				printf("create audio_stream fail\n");
				return -1;
			}
			if((ret = audio_stream_start_full(stream, &prof
					, host, port, host, port+1
					, 11, 40, NULL, NULL, NULL, NULL, FALSE)) != 0) {
				printf("start audio_stream fail(ret:%d)\n", ret);
				return -1;
			}
			sess[sess_cnt++] = ms_audio_endpoint_get_from_stream(stream, TRUE);
			loc_port+=2;
		}
	}

	if(!(chan = ms_audio_conference_new(&cf_params))) {
		printf("create conference fail\n");
		return -1;
	}

	for(i=0; i<sess_cnt; i++) {
		ms_audio_conference_add_member(chan, sess[i]);
	}

	while(1) {
		sleep(1);
	}

	return 0;
}
