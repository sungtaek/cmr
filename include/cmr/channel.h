#ifndef _CMR_CHANNEL_H_
#define _CMR_CHANNEL_H_

#include "cmr/session.h"

typedef struct _cmr cmr_t;

typedef struct _cmr_chan {
	cmr_t *cmr;
	long long id;
	// sess_hash;
	cmr_sess_t *sess_link;
	int sess_count;
	struct _cmr_chan *next;
} cmr_chan_t;

#ifdef __cplusplus
extern "C" {
#endif

cmr_chan_t *cmr_chan_create(cmr_t *cmr);
cmr_chan_t *cmr_chan_get(cmr_t *cmr, long long chan_id);
int cmr_chan_get_all(cmr_t *cmr, cmr_chan_t **chan_list, int list_len);
int cmr_chan_get_all_in_worker(cmr_t *cmr, int worker_idx, cmr_chan_t **chan_list, int list_len);
void cmr_chan_destroy(cmr_chan_t *chan);

#ifdef __cplusplus
}
#endif

#endif /* _CMR_CHANNEL_H_ */
