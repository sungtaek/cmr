#include <stdio.h>

#include "cmr/util.h"

#include "portmgr.h"

typedef struct _portmgr {
	int init;
	int start_port;
	int end_port;
	int range;
	int idx;
	char *pool;
	cmr_mutex_t lock;
} portmgr_t;

portmgr_t g_portmgr;

int portmgr_init(int start_port, int end_port)
{
	int i;

	if(g_portmgr.init) {
		return ERR_ALREADY_INITIALIZED;
	}

	g_portmgr.start_port = start_port;
	g_portmgr.end_port = end_port;
	g_portmgr.range = (end_port+1-start_port) - ((end_port+1-start_port)%2);
	if(g_portmgr.range < 2) {
		printf("invalid port range[%d ~ %d]\n",
				start_port, end_port);
		return ERR_INVALID_PARAM;
	}
	g_portmgr.idx = 0;
	g_portmgr.pool = (char*)malloc(g_portmgr.range);
	if(!g_portmgr.pool) {
		return ERR_SYSTEM_MEMORY;
	}
	for(i=0; i<g_portmgr.range; i++) {
		g_portmgr.pool[i] = 0;
	}
	cmr_mutex_init(&g_portmgr.lock, NULL);

	g_portmgr.init = 1;

	return SUCC;
}

int portmgr_alloc()
{
	int port = 0;
	int cur = -1;

	if(!g_portmgr.init) {
		return ERR_NOT_INITIALIZED;
	}

	cmr_mutex_lock(&g_portmgr.lock);
	cur = g_portmgr.idx;
	while(g_portmgr.pool[g_portmgr.idx]) {
		g_portmgr.idx = (g_portmgr.idx + 2)%g_portmgr.range;
		if(cur == g_portmgr.idx)
			break;
	}
	if(g_portmgr.pool[g_portmgr.idx]) {
		port = ERR_PORT_FULL;
	}
	else {
		port = g_portmgr.start_port + g_portmgr.idx;
		g_portmgr.pool[g_portmgr.idx] = 1;
		g_portmgr.pool[g_portmgr.idx+1] = 1;
	}
	cmr_mutex_unlock(&g_portmgr.lock);

	printf("alloc port[%d,%d]\n", port, port+1);
	return port;
}

void portmgr_return(int port)
{
	int idx;

	if(!g_portmgr.init) {
		return;
	}

	if(port < g_portmgr.start_port || port > g_portmgr.end_port) {
		return;
	}
	idx = port - g_portmgr.start_port;

	cmr_mutex_lock(&g_portmgr.lock);
	g_portmgr.pool[idx] = 0;
	g_portmgr.pool[idx+1] = 0;
	cmr_mutex_unlock(&g_portmgr.lock);

	printf("return port[%d,%d]\n", port, port+1);
}

