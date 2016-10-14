/*
 */
#include "export.h"

int init_status()
{
	SDK_CONTENT * c = &g_sdk_content;
	return 1;
}

int do_status_main()
{
	// Á¬½ÓÍøÂç¡£
	// open a tcp connect 
	SDK_CONTENT * c = &g_sdk_content;
	// wait login success.
	char error_net[256] = { 0 };
	c->status.fd = net_tcp_non_block_connect(error_net, c->status.server_ip,c->status.server_port);
	net_enable_tcp_no_delay(NULL,c->status.fd);
	if (ae_create_file_event(c->el,c->status.fd,AE_READABLE, read_data_from_server,&c->status) == AE_ERR)
	{
		closesocket(c->status.fd);
		OutputDebugStringA("Can't create the socket event.");
		return FALSE;
	}
	while (!c->el->stop) 
	{
		write_to_client(&c->login);
		msg_handler();
	}
	return 1;
}

int deinit_status()
{
	SDK_CONTENT * c = &g_sdk_content;
	closesocket(c->status.fd);
	return TRUE;
}
 
