#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "cmr/cmr.h"
#include "cmr/channel.h"

int main(int argc, char **argv)
{
	int ret, i;
	cmr_chan_t *chan;
	cmr_sess_t *sess[100];
	int sess_cnt = 0;

	cmr_conf_t conf;
	CMR_CONF_DEFAULT(conf);
	//conf.worker_count = 1;

	ret = cmr_init(conf);
	if(ret < 0) {
		printf("cmr_init fail: %d\n", ret);
		return -1;
	}

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
			sess[sess_cnt++] = cmr_sess_create(host, port, SESS_MODE_SENDRECV);
		}
	}

	chan = cmr_chan_create();

	for(i=0; i<sess_cnt; i++) {
		cmr_chan_add_session(chan, sess[i]);
	}

	cmr_add_channel(chan);

	ret = cmr_start();
	if(ret < 0) {
		printf("cmr_start fail: %d\n", ret);
		return -1;
	}


	while(1) {
		sleep(1);
	}

	return 0;
}
