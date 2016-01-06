#ifndef _CMR_SESSION_H_
#define _CMR_SESSION_H_

#include "ortp/rtpsession.h"

typedef struct _cmr_sess cmr_sess_t;

#include "cmr/util.h"
#include "cmr/channel.h"

#define SESS_MODE_SEND_ONLY	1
#define SESS_MODE_RECV_ONLY	2
#define SESS_MODE_SEND_RECV	3

struct _cmr_sess {
	long long id;
	cmr_chan_t *chan;
	int mode;
	RtpSession *session;
	UT_hash_handle chan_hh;
};

#ifdef __cplusplus
extern "C" {
#endif

cmr_sess_t *cmr_sess_create(const char *peer_ip, int peer_port, int mode);
void cmr_sess_destroy(cmr_sess_t *sess);

int cmr_sess_set_mode(cmr_sess_t *sess, int mode);

#ifdef __cplusplus
}
#endif

#endif /* _CMR_SESSION_H_ */
