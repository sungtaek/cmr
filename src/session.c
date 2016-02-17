#include <stdio.h>

#include "ortp/rtpsession.h"

#include "cmr/cmr.h"
#include "cmr/session.h"

cmr_sess_t *cmr_sess_create(const char *peer_ip, int peer_port, int mode)
{
	int port = -1;

	if(!g_cmr.init) {
		return NULL;
	}

	cmr_sess_t *sess = NULL;
	sess = (cmr_sess_t*)malloc(sizeof(cmr_sess_t));
	if(!sess) {
		return NULL;
	}

	sess->id = gen_unique_id();
	sess->chan = NULL;
	sess->mode = mode;
	sess->raw_sess = rtp_session_new(RTP_SESSION_SENDRECV);
	if(!sess->raw_sess) {
		free(sess);
		return NULL;
	}

	port = portmgr_alloc(g_cmr.portmgr);
	if(port < 0) {
		rtp_session_destroy(sess->raw_sess);
		free(sess);
		return NULL;
	}

	rtp_session_set_local_addr(sess->raw_sess, g_cmr.conf.local_host, port, port+1);
	rtp_session_set_remote_addr(sess->raw_sess, peer_ip, peer_port);
	cmr_rwlock_init(&sess->lock, NULL);

	return sess;
}

void cmr_sess_destroy(cmr_sess_t *sess)
{
	int port = -1;

	if(!g_cmr.init) {
		return;
	}

	if(sess) {
		port = rtp_session_get_local_port(sess->raw_sess);

		if(sess->chan) {
			cmr_chan_remove_session(sess->chan, sess->id);
		}
		rtp_session_destroy(sess->raw_sess);
		cmr_rwlock_destroy(&sess->lock);
		free(sess);

		portmgr_return(g_cmr.portmgr, port);
	}
}

int cmr_sess_set_mode(cmr_sess_t *sess, int mode)
{
	if(!sess) {
		return ERR_INVALID_PARAM;
	}

	cmr_rwlock_wrlock(&sess->lock);
	sess->mode = mode;
	cmr_rwlock_unlock(&sess->lock);

	return SUCC;
}

int cmr_sess_get_mode(cmr_sess_t *sess)
{
	int mode;

	if(!sess) {
		return ERR_INVALID_PARAM;
	}

	cmr_rwlock_rdlock(&sess->lock);
	mode = sess->mode;
	cmr_rwlock_unlock(&sess->lock);

	return mode;
}

