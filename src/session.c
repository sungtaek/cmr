#include <stdio.h>

#include "ortp/rtpsession.h"

#include "cmr/session.h"

cmr_sess_t *cmr_sess_create(const char *peer_ip, int peer_port, int mode)
{
	cmr_sess_t *sess = NULL;

	sess = (cmr_sess_t*)malloc(sizeof(cmr_sess_t));
	if(!sess) {
		return NULL;
	}

	sess->id = gen_unique_id();
	sess->chan = NULL;
	sess->mode = mode;
	switch(mode) {
		case SESS_MODE_SEND_ONLY:
			sess->session = rtp_session_new(RTP_SESSION_SENDONLY);
			break;
		case SESS_MODE_RECV_ONLY:
			sess->session = rtp_session_new(RTP_SESSION_RECVONLY);
			break;
		case SESS_MODE_SEND_RECV:
			sess->session = rtp_session_new(RTP_SESSION_SENDRECV);
			break;
		default:
			printf("invalid mode(%d)\n", mode);
			free(sess);
			return NULL;
	}
	if(!sess->session) {
		free(sess);
		return NULL;
	}

	// TODO: alloc local port
	rtp_session_set_local_addr(sess->session, "0.0.0.0", 1, 2);
	rtp_session_set_remote_addr(sess->session, peer_ip, peer_port);

	return sess;
}

void cmr_sess_destroy(cmr_sess_t *sess)
{
}

int cmr_sess_set_mode(cmr_sess_t *sess, int mode)
{
}
