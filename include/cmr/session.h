#ifndef _CMR_SESSION_H_
#define _CMR_SESSION_H_

#include "ortp/rtpsession.h"

typedef struct _cmr_sess cmr_sess_t;

#include "cmr/cmr.h"
#include "cmr/util.h"
#include "cmr/channel.h"

#define SESS_MODE_SENDONLY	1
#define SESS_MODE_RECVONLY	2
#define SESS_MODE_SENDRECV	3

struct _cmr_sess {
	long long id;
	cmr_chan_t *chan;
	int mode;
	RtpSession *raw_sess;
	cmr_rwlock_t lock;
	UT_hash_handle chan_hh;
};

#define cmr_sess_set_chan(_s, _c) (_s)->chan = (_c)
#define cmr_sess_get_id(_s) (_s)->id
#define cmr_sess_get_chan(_s) (_s)->chan

#ifdef __cplusplus
extern "C" {
#endif

cmr_sess_t *cmr_sess_create(cmr_t *cmr, const char *peer_ip, int peer_port, int mode);
void cmr_sess_destroy(cmr_sess_t *sess);

int cmr_sess_set_mode(cmr_sess_t *sess, int mode);
int cmr_sess_get_mode(cmr_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* _CMR_SESSION_H_ */
