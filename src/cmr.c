#include <stdio.h>
#include <stdlib.h>

#include "cmr/util.h"
#include "cmr/cmr.h"

cmr_t *cmr_create(cmr_conf_t conf)
{
	cmr_t *cmr = (cmr_t *)malloc(sizeof(cmr_t));
	cmr->conf = conf;
	printf("cmr created!!\n");
	return cmr;
}

int cmr_start(cmr_t *cmr)
{
	printf("run %d worker...\n", cmr->conf.worker_count);
	return SUCC;
}

int cmr_stop(cmr_t *cmr)
{
	return SUCC;
}

void cmr_destroy(cmr_t *cmr)
{
	free(cmr);
}
