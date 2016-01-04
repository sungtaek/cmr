#ifndef _CMR_SESSION_H_
#define _CMR_SESSION_H_

#include "ortp/rtpsession.h"

#define SESS_MODE_NONE		0
#define SESS_MODE_SEND_ONLY	1
#define SESS_MODE_RECV_ONLY	2
#define SESS_MODE_SEND_RECV	3

typedef struct _cmr_sess {
	long long id;
	char *peer_ip;
	unsigned short peer_port;
	int mode;
	RtpSession *session;
	struct _cmr_sess *next;
} cmr_sess_t;

#ifdef __cplusplus
extern "C" {
#endif

cmr_sess_t *cmr_sess_create(char *peer_ip, unsigned short peer_port, int mode);
void cmr_sess_destroy(cmr_sess_t *sess);

int cmr_sess_set_mode(cmr_sess_t *sess, int mode);

#ifdef __cplusplus
}
#endif

#endif /* _CMR_SESSION_H_ */
