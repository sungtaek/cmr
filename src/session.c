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
		case SESS_MODE_SENDONLY:
			sess->raw_sess = rtp_session_new(RTP_SESSION_SENDONLY);
			break;
		case SESS_MODE_RECVONLY:
			sess->raw_sess = rtp_session_new(RTP_SESSION_RECVONLY);
			break;
		case SESS_MODE_SENDRECV:
			sess->raw_sess = rtp_session_new(RTP_SESSION_SENDRECV);
			break;
		default:
			printf("invalid mode(%d)\n", mode);
			free(sess);
			return NULL;
	}
	if(!sess->raw_sess) {
		free(sess);
		return NULL;
	}

	// TODO: set local ip, alloc  port
	rtp_session_set_local_addr(sess->raw_sess, "0.0.0.0", 3600,3601);
	rtp_Session_set_remote_addr(sess->raw_sess, peer_ip, peer_port);

	return sess;
}

void cmr_sess_destroy(cmr_sess_t *sess)
{
	if(sess) {
		// TODO: check namespace
		if(sess->chan) {
			cmr_chan_remove_session(sess->chan, sess->id);
		}
		rtp_session_destroy(sess->raw_sess);
		free(sess);
	}
}

int cmr_sess_set_mode(cmr_sess_t *sess, int mode)
{
	if(!sess) {
		return ERR_INVALID_PARAM;
	}

	// TODO: check namespace
	// TODO: check rtp_session_init
	if(mode != sess->mode) {
		switch(mode) {
			case SESS_MODE_SENDONLY:
				rtp_session_init(sess->raw_sess, RTP_SESSION_SENDONLY);
				break;
			case SESS_MODE_RECVONLY:
				rtp_session_init(sess->raw_sess, RTP_SESSION_RECVONLY);
				break;
			case SESS_MODE_SENDRECV:
				rtp_session_init(sess->raw_sess, RTP_SESSION_SENDRECV);
				break;
		}
	}
	return SUCC;
}

