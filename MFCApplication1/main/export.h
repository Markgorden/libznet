#ifndef _COE_LIB_STATUS_H_
#define _COE_LIB_STATUS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../../libaenet/win32_Interop/win32fixes.h"
#include "../../libaenet/win32_Interop/win32_wsiocp.h"
#include "../../libaenet/ae.h"
#include "../../libaenet/net.h"

	// internal thread message
#define WM_USER                             0x0400
#define WM_DATA_COME     				   (WM_USER + 2100)

typedef struct __tag_sdk_client
{
	DWORD thread_id;
	char server_ip[512];
	int server_port;
	SOCKET fd;
	unsigned char temp_buffer[4];
	int len_of_temp_data;
	/* send buffer */
	char * buf;
	unsigned int bufpos;
	unsigned int buf_len;
}CLIENT;

typedef struct __tag_sdk_content
{
	AE_EVENT_LOOP * el;
	CLIENT login;
	CLIENT status;
}SDK_CONTENT;

extern SDK_CONTENT g_sdk_content;

// status_impl.c
int init_status();
int do_status_main();
int deinit_status();

// login_impl.c
int init_login();
void do_login_main();
void deinit_login();

// iocp_impl.c
int init_iocp();
int do_iocp_main();
int deinit_iocp();

// protocol.c
void read_data_from_server(AE_EVENT_LOOP * el, int fd, void * privdata, int mask);
void read_data_from_server_do(int fd, void * privdata);
int msg_handler();
int write_to_client(CLIENT * c);

#ifndef __linux
#endif
#ifdef __cplusplus
}
#endif
#endif
