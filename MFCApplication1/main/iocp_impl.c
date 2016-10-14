/*
 */
#include "export.h"

int init_iocp()
{
	SDK_CONTENT * c = &g_sdk_content;
	c->el = ae_create_event_loop(1024);
	return 1;
}

int do_iocp_main()
{
	SDK_CONTENT * c = &g_sdk_content;
	while (!c->el->stop) 
	{
		ae_process_events(c->el, AE_ALL_EVENTS);
	}
	//ae_delete_event_loop(c->el);
	return 1;
}

int deinit_iocp()
{
	SDK_CONTENT * c = &g_sdk_content;
	return TRUE;
}
 
