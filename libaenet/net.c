/* anet.c -- Basic TCP socket stuff made a bit less boring
 *
 */

#ifdef _WIN32
#include "Win32_Interop/Win32_Portability.h"
#include "Win32_Interop/win32_types.h"
#include "Win32_Interop/win32fixes.h"
#include "Win32_Interop/win32_wsiocp2.h"
#define ANET_NOTUSED(V) V
#include <Mstcpip.h>
#endif

//#include "fmacros.h"

#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include "net.h"

void net_set_error(char *err, const char *fmt, ...)
{
    va_list ap;
    if (!err) return;
    va_start(ap, fmt);
    vsnprintf(err, NET_ERR_LEN, fmt, ap);
    va_end(ap);
}

static inline int net_set_block(char *err, int fd, int non_block) {
    int flags;

    /* Set the socket blocking (if non_block is zero) or non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = win32_fcntl(fd, F_GETFL, 0)) == -1) {                                WIN_PORT_FIX /* fcntl default value for the 'flags' parameter */
        //anetSetError(err, "fcntl(F_GETFL): %s", strerror(errno));
        return NET_ERR;
    }

    if (non_block)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    if (win32_fcntl(fd, F_SETFL, flags) == -1) {
        //anetSetError(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return NET_ERR;
    }
    return NET_OK;
}

int net_non_block(char *err, int fd) {
    return net_set_block(err,fd,1);
}

int net_block(char *err, int fd) {
    return net_set_block(err,fd,0);
}

/* Set TCP keep alive option to detect dead peers. The interval option
 * is only used for Linux as we are using Linux-specific APIs to set
 * the probe send time, interval, and count. */
int net_keep_alive(char *err, int fd, int interval)
{
    int val = 1;

#ifdef _WIN32    
    if (win32_setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
        //net_set_error(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return NET_ERR;
    }

    struct tcp_keepalive alive; 
    DWORD dwBytesRet = 0; 
    alive.onoff = TRUE; 
    alive.keepalivetime = interval * 1000; 
    /* According to
     * http://msdn.microsoft.com/en-us/library/windows/desktop/ee470551(v=vs.85).aspx
     * On Windows Vista and later, the number of keep-alive probes (data
     * retransmissions) is set to 10 and cannot be changed.
     * So we set the keep alive interval as interval/10, as 10 probes will
     * be send before detecting an error */
    val = interval/10; 
    if (val == 0) val = 1; 
    alive.keepaliveinterval = val*1000; 
    if (FDAPI_WSAIoctl(fd, SIO_KEEPALIVE_VALS, &alive, sizeof(alive),
                       NULL, 0, &dwBytesRet, NULL, NULL) == SOCKET_ERROR) {
        //net_set_error(err,
        //             "WSAIotcl(SIO_KEEPALIVE_VALS) failed with error code %d\n",
        //             strerror(errno));
        return NET_ERR;
    } 
#else
    /* Default settings are more or less garbage, with the keepalive time
     * set to 7200 by default on Linux. Modify settings to make the feature
     * actually useful. */

    /* Send first probe after interval. */
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        anetSetError(err, "setsockopt TCP_KEEPIDLE: %s\n", strerror(errno));
        return ANET_ERR;
    }

    /* Send next probes after the specified interval. Note that we set the
     * delay as interval / 3, as we send three probes before detecting
     * an error (see the next setsockopt call). */
    val = interval/3;
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        anetSetError(err, "setsockopt TCP_KEEPINTVL: %s\n", strerror(errno));
        return ANET_ERR;
    }

    /* Consider the socket in error state after three we send three ACK
     * probes without getting a reply. */
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        anetSetError(err, "setsockopt TCP_KEEPCNT: %s\n", strerror(errno));
        return ANET_ERR;
    }
#endif

    return NET_OK;
}

static int net_set_tcp_no_delay(char * err, int fd, int val)
{
    if (win32_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1)
    {
        //net_set_error(err, "setsockopt TCP_NODELAY: %s", strerror(errno));
        return NET_ERR;
    }
    return NET_OK;
}

int net_enable_tcp_no_delay(char *err, int fd)
{
    return net_set_tcp_no_delay(err, fd, 1);
}

int net_disable_tcp_no_delay(char *err, int fd)
{
    return net_set_tcp_no_delay(err, fd, 0);
}


int net_set_send_buffer(char * err, int fd, int buffsize)
{
    if (win32_setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1)
    {
        //net_set_error(err, "setsockopt SO_SNDBUF: %s", strerror(errno));
        return NET_ERR;
    }
    return NET_OK;
}

int net_tcp_keep_alive(char *err, int fd)
{
    int yes = 1;
    if (win32_setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1)
	{
       // net_set_error(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return NET_ERR;
    }
    return NET_OK;
}

int net_send_timeout(char *err, int fd, long long ms)
{
    struct timeval tv;

    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
    if (win32_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
       // net_set_error(err, "setsockopt SO_SNDTIMEO: %s", strerror(errno));
        return NET_ERR;
    }
    return NET_OK;
}

int net_generic_resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len,
                       int flags)
{
    struct addrinfo hints, *info;
    int rv;
    memset(&hints,0,sizeof(hints));
    if (flags & NET_IP_ONLY) hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;  /* specify socktype to avoid dups */

    if ((rv = win32_getaddrinfo(host, NULL, &hints, &info)) != 0) {
        net_set_error(err, "%s", gai_strerror(rv));
        return NET_ERR;
    }
    if (info->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)info->ai_addr;
        //inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, ipbuf_len);
    } 
	win32_freeaddrinfo(info);
    return NET_OK;
}

int net_resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len) {
    return net_generic_resolve(err,host,ipbuf,ipbuf_len,NET_IP_ONLY);
}

int net_resolve_ip(char *err, char *host, char *ipbuf, size_t ipbuf_len) {
    return net_generic_resolve(err,host,ipbuf,ipbuf_len,NET_IP_ONLY);
}

static int net_set_reuse_addr(char *err, int fd) {
	int yes = 1;
	/* Make sure connection-intensive things like the redis benckmark
	* will be able to close/open sockets a zillion of times */
	if (win32_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		//net_set_error(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
		return NET_ERR;
	}
	return NET_OK;
}

static int net_create_socket(char *err, int domain) 
{
	int s;
	if ((s = win32_socket(domain, SOCK_STREAM, IF_WIN32(IPPROTO_TCP, 0))) == -1) {
		//net_set_error(err, "create socket error: %s", strerror(errno));
		return NET_ERR;
	}

	/* Make sure connection-intensive things like the redis benckmark
	* will be able to close/open sockets a zillion of times */
	if (net_set_reuse_addr(err, s) == NET_ERR) {
		close(s);
		return NET_ERR;
	}
	return s;

}

#define NET_CONNECT_NONE 0
#define NET_CONNECT_NONBLOCK 1
#define NET_CONNECT_BE_BINDING 2 /* Best effort binding. */
#ifdef _WIN32
static int net_tcp_generic_connect(char *err, char *addr, int port,
                                 char *source_addr, int flags) {
    int fd;
    SOCKADDR_STORAGE socketStorage;

    if (ParseStorageAddress(addr, port, &socketStorage) == FALSE) {
        return NET_ERR;
    }

    if ((fd = net_create_socket(err, socketStorage.ss_family)) == NET_ERR) {
        return NET_ERR;
    }

    // Workaround for getpeername failing to retrieve the endpoint address
    FDAPI_SaveSocketAddrStorage(fd, &socketStorage);

    if (WSIOCP_SocketConnect(fd, &socketStorage) == SOCKET_ERROR) {
        if ((errno == WSAEWOULDBLOCK || errno == WSA_IO_PENDING)) errno = EINPROGRESS;
        if (errno == EINPROGRESS && flags & NET_CONNECT_NONBLOCK) {
            return fd;
        }

		//net_set_error(err, "connect: %d\n", errno);
        close(fd);
        return NET_ERR;
    }
	return fd;
}
#endif

int net_tcp_connect(char *err, char *addr, int port)
{
    return net_tcp_generic_connect(err,addr,port,NULL,NET_CONNECT_NONE);
}

int net_tcp_non_block_connect(char *err, char *addr, int port)
{
    return net_tcp_generic_connect(err,addr,port,NULL,NET_CONNECT_NONBLOCK);
}

int net_tcp_non_block_bind_connect(char *err, char *addr, int port, char *source_addr)
{
    return net_tcp_generic_connect(err,addr,port,source_addr,NET_CONNECT_NONBLOCK);
}

int net_tcp_non_block_best_effort_bind_connect(char *err, char *addr, int port, char *source_addr)
{
    return net_tcp_generic_connect(err,addr,port,source_addr, NET_CONNECT_NONBLOCK|NET_CONNECT_BE_BINDING);
}

int net_unix_generic_connect(char *err, char *path, int flags)
{
    ANET_NOTUSED(err);
    ANET_NOTUSED(path);
    ANET_NOTUSED(flags);
    return NET_ERR;
}

int net_uxnix_connect(char *err, char *path)
{
    return net_unix_generic_connect(err,path,NET_CONNECT_NONE);
}

int net_unix_non_block_connect(char *err, char *path)
{
    return net_unix_generic_connect(err,path,NET_CONNECT_NONBLOCK);
}

int net_read(int fd, char *buf, int count)
{
	int nread, totlen = 0;
    while(totlen != count) {
        nread = win32_read(fd,buf,count-totlen);
        if (nread == 0) return totlen;
        if (nread == -1) return -1;
        totlen += nread;
        buf += nread;
    }
    return totlen;
}

int net_write(int fd, char *buf, int count)
{
	int nwritten, totlen = 0;
    while(totlen != count) {
        nwritten = win32_write(fd,buf,count-totlen);
        if (nwritten == 0) return totlen;
        if (nwritten == -1) return -1;
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}


static int net_listen(char *err, int s, struct sockaddr *sa, socklen_t len, int backlog) {
	if (win32_bind(s, sa, len) == -1) {
		net_set_error(err, "bind: %s", strerror(errno));
		close(s);
		return NET_ERR;
	}

#ifdef _WIN32
    if (WSIOCP_Listen(s, backlog) == SOCKET_ERROR) {
#else
    if (listen(s, backlog) == -1) {
#endif
		net_set_error(err, "listen: %s", strerror(errno));
		close(s);
		return NET_ERR;
	}
	return NET_OK;
}

static int _net_tcp_server(char *err, int port, char *bindaddr, int af, int backlog)
{
    int s, rv;
    char _port[6];  /* strlen("65535") */
    struct addrinfo hints, *servinfo, *p;
    snprintf(_port,6,"%d",port);
    memset(&hints,0,sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    
    if ((rv = win32_getaddrinfo(bindaddr,_port,&hints,&servinfo)) != 0)
	{
       // net_set_error(err, "%s", gai_strerror(rv));
        return NET_ERR;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((s = win32_socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1)
            continue;
        if (net_set_reuse_addr(err,s) == NET_ERR) goto error;
        if (net_listen(err,s,p->ai_addr,p->ai_addrlen,backlog) == NET_ERR) goto error;
        goto end;
    }
    if (p == NULL) 
	{
       // net_set_error(err, "unable to bind socket");
        goto error;
    }

error:
    s = NET_ERR;
end:
	win32_freeaddrinfo(servinfo);
    return s;
}

int net_tcp_server(char *err, int port, char *bindaddr, int backlog)
{
    return _net_tcp_server(err, port, bindaddr, AF_INET, backlog);
}

static int net_generic_accept(char *err, int s, struct sockaddr *sa, int *len)
{
    int fd;
    while(1) 
	{
        fd = win32_accept(s,sa,len);
        if (fd == -1) 
		{
            if (errno == EINTR)
                continue;
            else {
                //net_set_error(err, "accept: %s", strerror(errno));
                return NET_ERR;
            }
        }
        break;
    }
    return fd;
}

int net_tcp_accept(char *err, int s, char *ip, size_t ip_len, int *port) 
{
    int fd;
    struct sockaddr_storage sa;
    int salen = sizeof(sa);
    if ((fd = net_generic_accept(err,s,(struct sockaddr*)&sa,&salen)) == -1)
        return NET_ERR;
    if (sa.ss_family == AF_INET) 
	{
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) win32_inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        if (port) *port = win32_ntohs(s->sin_port);
    } 
    return fd;
}

int net_peer_to_string(int fd, char *ip, size_t ip_len, int *port) 
{
    struct sockaddr_storage sa;
    int salen = sizeof(sa);

    if (win32_getpeername(fd,(struct sockaddr*)&sa,&salen) == -1) goto error;
    if (ip_len == 0) goto error;

    if (sa.ss_family == AF_INET) 
	{
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
        if (ip) win32_inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        if (port) *port = win32_ntohs(s->sin_port);
    }
	else 
	/*if (sa.ss_family == AF_UNIX) 
	{
        if (ip) strncpy(ip,"/unixsocket",ip_len);
        if (port) *port = 0;
    } 
	else*/ 
	{
        goto error;
    }
    return 0;

error:
    if (ip)
	{
        if (ip_len >= 2) 
		{
            ip[0] = '?';
            ip[1] = '\0';
        }
		else if (ip_len == 1)
		{
            ip[0] = '\0';
        }
    }
    if (port) *port = 0;
    return -1;
}

int net_format_addr(char *buf, size_t buf_len, char *ip, int port)
{
    return snprintf(buf,buf_len, strchr(ip,':') ? "[%s]:%d" : "%s:%d", ip, port);
}

int net_format_peer(int fd, char *buf, size_t buf_len)
{
    char ip[46];
    int port;

    net_peer_to_string(fd,ip,sizeof(ip),&port);
    return net_format_addr(buf, buf_len, ip, port);
}

int net_sock_name(int fd, char *ip, size_t ip_len, int *port) 
{
    struct sockaddr_storage sa;
    int salen = sizeof(sa);

    if (win32_getsockname(fd,(struct sockaddr*)&sa,&salen) == -1)
	{
        if (port) *port = 0;
        ip[0] = '?';
        ip[1] = '\0';
        return -1;
    }
    if (sa.ss_family == AF_INET)
	{
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
       // if (ip) inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
        if (port) *port = win32_ntohs(s->sin_port);
    }
    return 0;
}

int net_format_sock(int fd, char *fmt, size_t fmt_len)
{
    char ip[46];
    int port;
    net_sock_name(fd,ip,sizeof(ip),&port);
    return net_format_addr(fmt, fmt_len, ip, port);
}
