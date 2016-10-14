
#include "export.h"

int write_to_client(CLIENT * c) 
{
	SDK_CONTENT * content = &g_sdk_content;
	int fd = c->fd;
	int len;
	if (c->buf_len)
	{
		//len = WSASend(fd, c->buf + c->bufpos, c->buf_len, NULL);
		len = WSIOCP_SocketSend(fd, c->buf + c->bufpos, c->buf_len,
			content->el, c, c->buf, NULL);
		if (0 >= len)
		{
			printf("%s: send failed %d\n", __func__, 2);
			return FALSE;
		}
		c->bufpos += len;
		c->buf_len -= len;
	}

	if (! c->buf_len)
	{
		c->bufpos = 0;
		//zfree(c->buf);
		//get_a_data;
	}
	return 1;
}

BOOL get_thread_message(MSG *msg)
{
	int ret = FALSE;
	//while (GetMessage(msg, NULL, 0, 0))
	//PeekMessage(&msg, NULL, 0, 0,PM_REMOVE);
	while (PeekMessage(msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg->hwnd)
		{
			TranslateMessage(msg);
			DispatchMessage(msg);
		}
		else
		{
			ret = TRUE;
			break;
		}
	}
	return ret;
}

int msg_handler()
{
	MSG msg;
	int ret = FALSE;
	unsigned char * data;
	get_thread_message(&msg);
	switch (msg.message)
	{
	case WM_DATA_COME:
		read_data_from_server_do(msg.wParam, msg.lParam);
		break;
	default:
		break;
	}
	//g_sdk_content.log_func(SDK_DEBUG_INFO, "status", __LINE__, "Wait for login resut!\n");
	return ret;
}

void read_data_from_server(AE_EVENT_LOOP * el, int fd, void * privdata, int mask)
{
	CLIENT * c = (CLIENT *)privdata;
	SDK_CONTENT * content = &g_sdk_content;
	//UNUSED(el);
	//UNUSED(mask);
	if (fd == content->login.fd)
	{
		if (!PostThreadMessage(content->login.thread_id, WM_DATA_COME, (DWORD)fd, (DWORD)privdata))
		{
			//ret = ERROR_INTERNAL;
		}
	}
	else if (fd == content->status.fd)
	{
		if (!PostThreadMessage(content->status.thread_id, WM_DATA_COME, (DWORD)fd, (DWORD)privdata))
		{
			//ret = ERROR_INTERNAL;
		}
	}
	else
	{
		OutputDebugString("³ö´íÁË");
	}
}

void read_data_from_server_do(int fd, void * privdata)
{
	CLIENT * c = (CLIENT *)privdata;
	int len;
	char buffer[1024] = { 0 };
	len = (int)win32_read(fd, buffer, 1024 - 1);                          WIN_PORT_FIX /* cast (int) */
		if (len == -1)
		{
			if (errno == EAGAIN)
			{
				len = 0;
			}
			else
			{
				return;
			}
		}
		else if (len == 0)
		{
			return;
		}

	WIN32_ONLY(WSIOCP_QueueNextRead(fd);)
		//print_buf(c->buffer,len);
		OutputDebugStringA("recv msg from server!\n");
	//if (0 >= len)
	//{
	//	closesocket(fd);
	//	goto error;
	//}
error:
	closesocket(fd);
}
