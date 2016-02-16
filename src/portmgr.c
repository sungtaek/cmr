#include <stdio.h>

#include "cmr/portmgr.h"

portmgr_t *portmgr_create(int start_port, int end_port)
{
	int i;
	portmgr_t *portmgr = NULL;

	portmgr = (portmgr_t*)malloc(sizeof(portmgr_t));
	if(!portmgr) {
		return NULL;
	}

	portmgr->start_port = start_port;
	portmgr->end_port = end_port;
	portmgr->range = (end_port+1-start_port) - ((end_port+1-start_port)%2);
	if(portmgr->range < 2) {
		printf("invalid port range[%d ~ %d]\n",
				start_port, end_port);
		free(portmgr);
		return NULL;
	}
	portmgr->idx = 0;
	portmgr->pool = (char*)malloc(portmgr->range);
	if(!portmgr->pool) {
		free(portmgr);
		return NULL;
	}
	for(i=0; i<portmgr->range; i++) {
		portmgr->pool[i] = 0;
	}
	cmr_mutex_init(&portmgr->lock, NULL);

	return portmgr;
}

void portmgr_destroy(portmgr_t *portmgr)
{
	if(portmgr) {
		free(portmgr->pool);
		cmr_mutex_destroy(&portmgr->lock);
		free(portmgr);
	}
}

int portmgr_alloc(portmgr_t *portmgr)
{
	int port = 0;
	int cur = -1;

	if(!portmgr) {
		return ERR_INVALID_PARAM;
	}

	cmr_mutex_lock(&portmgr->lock);
	cur = portmgr->idx;
	while(portmgr->pool[portmgr->idx]) {
		portmgr->idx = (portmgr->idx + 2)%portmgr->range;
		if(cur == portmgr->idx)
			break;
	}
	if(portmgr->pool[portmgr->idx]) {
		port = ERR_PORT_FULL;
	}
	else {
		port = portmgr->start_port + portmgr->idx;
		portmgr->pool[portmgr->idx] = 1;
		portmgr->pool[portmgr->idx+1] = 1;
	}
	cmr_mutex_unlock(&portmgr->lock);

	printf("alloc port[%d,%d]\n", port, port+1);
	return port;
}

void portmgr_return(portmgr_t *portmgr, int port)
{
	int idx;

	if(!portmgr) {
		return;
	}

	if(port < portmgr->start_port || port > portmgr->end_port) {
		return;
	}
	idx = port - portmgr->start_port;

	cmr_mutex_lock(&portmgr->lock);
	portmgr->pool[idx] = 0;
	portmgr->pool[idx+1] = 0;
	cmr_mutex_unlock(&portmgr->lock);

	printf("return port[%d,%d]\n", port, port+1);
}
