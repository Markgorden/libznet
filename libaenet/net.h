/* anet.c -- Basic TCP socket stuff made a bit less boring
 *
 */

#ifndef __NET_H
#define __NET_H

#include "Macros.h"


#define NET_OK 0
#define NET_ERR -1
#define NET_ERR_LEN 256

/* Flags used with certain functions. */
#define NET_NONE 0
#define NET_IP_ONLY (1<<0)

int net_tcp_connect(char * err, char * addr, int port);
int net_tcp_non_block_connect(char * err, char * addr, int port);
int net_tcp_non_block_bind_connect(char * err, char * addr, int port, char * source_addr);
int net_tcp_non_block_best_effort_bind_connect(char * err, char * addr, int port, char * source_addr);
int net_unix_connect(char * err, char * path);
int net_unix_non_block_connect(char * err, char *path);
int net_read(int fd, char *buf, int count);
int net_resolve(char *err, char *host, char *ipbuf, size_t ipbuf_len);
int net_resolve_ip(char *err, char *host, char *ipbuf, size_t ipbuf_len);
int net_tcp_server(char *err, int port, char *bindaddr, int backlog);
//int net_unix_server(char *err, char *path, mode_t perm, int backlog);
int net_tcp_accept(char *err, int serversock, char *ip, size_t ip_len, int *port);
int net_unix_accept(char *err, int serversock);
int net_write(int fd, char *buf, int count);
int net_non_block(char * err, int fd);
int net_block(char * err, int fd);

int net_enable_tcp_no_delay(char *err, int fd);
int net_disable_tcp_no_delay(char *err, int fd);
int net_tcp_keep_alive(char *err, int fd);
int net_send_timeout(char *err, int fd, long long ms);
int net_peer_to_string(int fd, char *ip, size_t ip_len, int *port);
int net_keep_alive(char *err, int fd, int interval);
int net_sock_name(int fd, char *ip, size_t ip_len, int *port);
int net_format_addr(char *fmt, size_t fmt_len, char *ip, int port);
int net_format_peer(int fd, char *fmt, size_t fmt_len);
int net_format_sock(int fd, char *fmt, size_t fmt_len);

#endif
