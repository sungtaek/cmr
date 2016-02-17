#include <stdio.h>
#include <stdlib.h>

#include "ortp/ortp.h"

#include "cmr/util.h"
#include "cmr/cmr.h"

cmr_t g_cmr;

static void _cmr_destroy_worker_pool();

int cmr_init(cmr_conf_t conf)
{
	int i;

	if(g_cmr.init) {
		return ERR_ALREADY_INITIALIZED;
	}
	g_cmr.conf = conf;
	g_cmr.portmgr = portmgr_create(conf.port_start, conf.port_end);
	if(!g_cmr.portmgr) {
		return ERR_SYSTEM_MEMORY;
	}

	for(i=0; i<conf.worker_count; i++) {
		// create worker
		cmr_worker_t *worker = cmr_worker_create(i);
		if(!worker) {
			_cmr_destroy_worker_pool();
			portmgr_destroy(g_cmr.portmgr);
			return ERR_SYSTEM_MEMORY;
		}

		// attach worker to cmr
		cmr_worker_set_next(worker, g_cmr.worker_pool);
		g_cmr.worker_pool = worker;
	}

	g_cmr.chan_hash = NULL;

	ortp_init();
	ortp_scheduler_init();

	g_cmr.init = 1;

	printf("cmr created!!\n");
	return SUCC;
}

int cmr_start()
{
	cmr_worker_t *worker = NULL;

	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	// start worker
	worker = g_cmr.worker_pool;
	while(worker) {
		cmr_worker_start(worker);
		printf("start worker %d ...\n", cmr_worker_get_idx(worker));
		worker = cmr_worker_get_next(worker);
	}
	
	return SUCC;
}

int cmr_stop()
{
	cmr_worker_t *worker = NULL;

	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	// stop worker
	worker = g_cmr.worker_pool;
	while(worker) {
		cmr_worker_stop(worker);
		printf("stop worker %d ...\n", cmr_worker_get_idx(worker));
		worker = cmr_worker_get_next(worker);
	}

	return SUCC;
}

int cmr_add_channel(cmr_chan_t *chan)
{
	cmr_worker_t *worker = NULL;
	cmr_worker_t *min_worker = NULL;

	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	if(!chan) {
		return ERR_INVALID_PARAM;
	}

	// find min_session worker
	worker = g_cmr.worker_pool;
	while(worker) {
		if(min_worker) {
			if(cmr_worker_get_chan_count(min_worker) > cmr_worker_get_chan_count(worker)) {
				min_worker = worker;
			}
		}
		else {
			min_worker = worker;
		}
		worker = cmr_worker_get_next(worker);
	}

	if(!min_worker) {
		return ERR_SESSION_FULL;
	}

	HASH_ADD(cmr_hh, g_cmr.chan_hash, id, sizeof(chan->id), chan);
	return cmr_worker_add_channel(min_worker, chan);
}

cmr_chan_t *cmr_get_channel(long long chan_id)
{
	cmr_chan_t *chan = NULL;

	if(!g_cmr.init) {
		return NULL;
	}

	HASH_FIND(cmr_hh, g_cmr.chan_hash, &chan_id, sizeof(chan_id), chan);
	return chan;
}

int cmr_get_all_channel(cmr_chan_t **chan_list, int list_len)
{
	int len=0;
	cmr_chan_t *chan = NULL, *tmp;

	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	HASH_ITER(cmr_hh, g_cmr.chan_hash, chan, tmp) {
		if(len < list_len) {
			chan_list[len++] = chan;
		}
		else {
			break;
		}
	}
	return len;
}

int cmr_get_all_channel_in_worker(int worker_idx, cmr_chan_t **chan_list, int list_len)
{
	int i, len=0;
	cmr_worker_t *worker = NULL;

	if(!g_cmr.init) {
		return ERR_NOT_INITIALIZED;
	}

	for(worker = g_cmr.worker_pool, i=0; worker && i<worker_idx; i++) {
		worker = worker->next;
	}

	if(worker) {
		len = cmr_worker_get_all_channel(worker, chan_list, list_len);
	}

	return len;
}

cmr_chan_t *cmr_remove_channel(long long chan_id)
{
	cmr_chan_t *chan = NULL;

	if(!g_cmr.init) {
		return NULL;
	}

	chan = cmr_get_channel(chan_id);
	if(chan) {
		if(chan->worker) {
			cmr_worker_remove_channel(chan->worker, chan_id);
		}
		HASH_DELETE(cmr_hh, g_cmr.chan_hash, chan);
	}

	return chan;
}

static void _cmr_destroy_worker_pool()
{
	if(g_cmr.init && g_cmr.worker_pool) {
		cmr_worker_t *worker = g_cmr.worker_pool;
		while(worker) {
			cmr_worker_t *next = cmr_worker_get_next(worker);
			cmr_worker_destroy(worker);
			worker = next;
		}
		g_cmr.worker_pool = NULL;
	}
}

