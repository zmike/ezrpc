/* zrpc: RPC database+server for Zentific
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Zentific LLC and AUTHORS
 *
 * All rights reserved.
 * Use is subject to license terms.
 *
 * Please visit http://zentific.com for news and updates
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */
#ifndef ZPROXY_H
#define ZPROXY_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <openssl/lhash.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <ctype.h>
#include <dirent.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//TCP_NODELAY definitions, etc
//	under bsd license
#include <netinet/tcp.h>

#include <getopt.h>

#define MAX_CONNECT_QUEUE 	32
#define SSL_ERR_STR_LEN	  	200 /* needed to be at least 120 per documentation in ERR_error_string */
#define DEFAULT_LISTEN_ADDR	"0.0.0.0"

#define DEFAULT_CERT_PATH 	"/opt/zentific/ssl/proxy.crt"
#define DEFAULT_KEY_PATH  	"/opt/zentific/ssl/proxy.key"

#define SETOPT(x) 	{ if(x) free(x); x=strdup(optarg); printf("%s%s=\t'%s'\n", #x, ((strlen(#x) >=8) ? "\t" : "\t\t"), x);  }
#define ERROPT(x) 	{ if(!x) { fprintf(stderr, "PARAM '" #x "' not set\n"); usage(); exit(1); } }
#define WARNOPT(x)	{ if(!x) fprintf(stderr, "PARAM '" #x "' not set. Using default.\n"); }

void proxy(void);
void cleanup_ssl(void);
void cleanup_exit(int);
void ssl_display_errors(void);
int data_send(int, char *, int );
void ssl_teardown(SSL_CTX *, SSL *);
void ssl_setup_conn(SSL_CTX *, SSL *);
void ssl_setup_accept(SSL_CTX *, SSL *);
SSL * ssl_new_conn(SSL_CTX *ctx, int fd);
SSL_CTX * ssl_ctx_new (int, int, const char *, const char *);

#endif
