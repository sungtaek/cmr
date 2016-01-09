#include <stdio.h>

#include "cmr/session.h"

cmr_sess_t *cmr_sess_create(const char *peer_ip, int peer_port, int mode)
{
	cmr_sess_t *sess = NULL;
	sess = (cmr_sess_t*)malloc(sizeof(cmr_sess_t));
	if(!sess) {
		return NULL;
	}

	sess->id = get_unique_id();
	sess->chan = NULL;
	sess->mode = mode;
	switch(mode) {
		case SESS_MODE_SENDONLY:
			sess->session = rtp_session_new(RTP_SESSION_SENDONLY);
			break;
		case SESS_MODE_RECVONLY:
			sess->session = rtp_session_new(RTP_SESSION_RECVONLY);
			break;
		case SESS_MODE_SENDRECV:
			sess->session = rtp_session_new(RTP_SESSION_SENDRECV);
			break;
	}
	if(!sess->session) {
		free(sess);
		return NULL;
	}
	// TODO: set local ip, port
	rtp_session_set_local_addr(sess->session, "0.0.0.0", 3600,3601);
	rtp_Session_set_remote_addr(sess->session, peer_ip, peer_port);

	return sess;
}

void cmr_sess_destroy(cmr_sess_t *sess)
{
	if(sess) {
		if(sess->chann) {
			cmr_chan_remove_session(sess->chan, sess->id);
		}
		rtp_session_destroy(sess->session);
		free(sess);
	}
}

int cmr_sess_set_mode(cmr_sess_t *sess, int mode)
{
	if(!sess) {
		return ERR_INVALID_PARAM;
	}

	if(mode != sess->mode) {
		switch(mode) {
			case SESS_MODE_SENDONLY:
				rtp_session_init(sess->session, RTP_SESSION_SENDONLY);
				break;
			case SESS_MODE_RECVONLY:
				rtp_session_init(sess->session, RTP_SESSION_RECVONLY);
				break;
			case SESS_MODE_SENDRECV:
				rtp_session_init(sess->session, RTP_SESSION_SENDRECV);
				break;
		}
	}
	return SUCC;
}

