#ifndef _CMR_H_
#define _CMR_H_

typedef struct _cmr_conf cmr_conf_t;

#include "cmr/channel.h"
#include "cmr/session.h"

struct _cmr_conf {
	char *local_host;
	char *public_host;
	int worker_count;
	int max_sess_per_chan;
	int max_sess_per_thr;
	int port_start;
	int port_end;
	int packet_size;
	int inc_ts;
};

#define CMR_CONF_DEFAULT(_c) { \
	(_c).local_host = "0.0.0.0"; \
	(_c).public_host = "0.0.0.0"; \
	(_c).worker_count = 16; \
	(_c).max_sess_per_chan = -1; \
	(_c).max_sess_per_thr = -1; \
	(_c).port_start = 49152; \
	(_c).port_end = 65535; \
	(_c).packet_size = 160; \
	(_c).inc_ts = 160; \
}

#ifdef __cplusplus
extern "C" {
#endif

int cmr_init(cmr_conf_t conf);
int cmr_start();
int cmr_stop();
int cmr_is_init();
cmr_conf_t cmr_get_conf();

// channel functions
int cmr_add_channel(cmr_chan_t *chan);
cmr_chan_t *cmr_get_channel(unsigned long chan_id);
int cmr_get_all_channel(cmr_chan_t ***chan_list, int list_len);
cmr_chan_t *cmr_remove_channel(unsigned long chan_id);
int cmr_get_channel_count();

cmr_sess_t *cmr_get_session(unsigned long sess_id);
int cmr_get_session_count();


#ifdef __cplusplus
}
#endif


#endif /* _CMR_H_ */

