#ifndef _CMR_SESSION_H_
#define _CMR_SESSION_H_

typedef struct _cmr_chan cmr_chan_t;

typedef struct _cmr_sess {
	cmr_t *cmr;
	cmr_chan_t *chan;
	long long id;
	struct _cmr_sess *next;
} cmr_sess_t;

#ifdef __cplusplus
extern "C" {
#endif

cmr_sess_t *cmr_sess_create(cmr_chan_t *chan);
cmr_sess_t *cmr_sess_get(cmr_chan_t *chan, long long sess_id);
int cmr_sess_get_all(cmr_chan_t *chan, cmr_sess_t **sess_list, int list_len);
void cmr_sess_destroy(cmr_sess_t *sess);

int cmr_sess_change_status(cmr_sess_t *sess, int status);

#ifdef __cplusplus
}
#endif

#endif /* _CMR_SESSION_H_ */
