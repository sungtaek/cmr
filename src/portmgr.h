#ifndef _PORTMGR_H_
#define _PORTMGR_H_

#ifdef __cplusplus
extern "C" {
#endif

int portmgr_init(int start_port, int end_port);

int portmgr_alloc();
void portmgr_return(int port);


#ifdef __cplusplus
}
#endif


#endif /* _PORTMGR_H_ */

