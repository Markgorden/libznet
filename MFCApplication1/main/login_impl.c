/*
 */
#include "export.h"

int init_login()
{
	SDK_CONTENT * c = &g_sdk_content;
	c->el = ae_create_event_loop(1024);
	return 1;
}

void do_login_main()
{
	SDK_CONTENT * c = &g_sdk_content;
	// open a tcp connect 
	char error_net[256] = { 0 };
	c->login.fd = net_tcp_non_block_connect(error_net,"10.36.65.179",8080);
    net_enable_tcp_no_delay(NULL,c->login.fd);
	if (ae_create_file_event(c->el,c->login.fd,AE_READABLE,read_data_from_server,&c->login) == AE_ERR)
	{
		closesocket(c->login.fd);
		return NULL;
	}

	while (!c->el->stop) 
	{
		write_to_client(&c->login);
		msg_handler();
	}
}

void deinit_login()
{
	SDK_CONTENT * c = &g_sdk_content;
	ae_delete_event_loop(c->el);
	closesocket(c->login.fd);
	return;
}




