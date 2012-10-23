/* zrpc: RPC database+server for Zentific
 * Copyright (C) 2010, 2011 Zentific LLC and AUTHORS
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Zshare.h>
#include "zproxy.h"

static void usage(void);
static void zssl_init(void);
static void sighandler(int );
static void setup_signal_handler(void);
static int zconnect (const char *, const char *, int);

int proxyfd 			= -1;
int clientfd 			= -1;
int backend_host_fd 		= -1;

int debug 			= 0;
int verbose 			= 0;
int backendssl			= 0;
int frontendssl			= 0;
int verify 			= 0;

char *backendhost 		= NULL;
char *backendport 		= NULL;
char *frontendport 		= NULL;
char *frontendlisten 		= NULL;
char *pidpath 			= NULL;

static SSL_CTX *frontend_ctx	= NULL;
static SSL_CTX *backend_ctx 	= NULL;
static SSL *frontend_conn 	= NULL;
static SSL *backend_conn 	= NULL;

static char *cert		= NULL;
static char *key 		= NULL;

void ssl_display_errors(void){
	do {
		unsigned long err;

		err = ERR_get_error();
		if (!err) break;
		fprintf(stderr, "SSL error: %s\n", ERR_reason_error_string(err));
	} while (1);
}

int data_send(int fd, char *data, int len){

	struct pollfd writefds[1];
	writefds[0].events = POLLOUT;
	writefds[0].fd = fd;

	int ret = 0;
	int numwritten = 0;
	char *retry = NULL;
	for (retry = data; retry && len > 0; len -= numwritten, retry += numwritten){
		if (poll (writefds, 1, -1) == -1) {
			perror ("poll");
			return -1;
		}

		//TODO consider using MSG_NOSIGNAL as one of the flags here
		numwritten = send (fd, data, len, 0);

		if (numwritten < 0) {
			ret = numwritten;
			break;
		}

		ret += numwritten;
	}

	return ret;

}

void cleanup_exit(int ret){

	cleanup_ssl();

	zshare_pid_delete();

	exit(ret);

}

void cleanup_ssl(void){

	if(frontendssl){
		if(frontend_conn){
			SSL_shutdown(frontend_conn);
			SSL_free(frontend_conn);
		}
		if(frontend_ctx)
			SSL_CTX_free(frontend_ctx);
	}

	if(backendssl){
		if(backend_conn){
			SSL_shutdown(backend_conn);
			SSL_free(backend_conn);
		}
		if(backend_ctx)
			SSL_CTX_free(backend_ctx);
	}

	if(frontendssl||backendssl){
		printf("Attempting to clean up after SSL\n");
		ERR_free_strings();
	}

}

static void usage(void){
	printf("\nZentific polling daemon\n"
		"-------------------------------------------------------------------------\n"
		"Usage: zproxy [OPTIONS]\n\n"
		"-h --help\n"
		"\tThis message.\n"
		"-v --verbose\n"
		"\tDisplay verbose messages about SSL, client/backend, etc. Enables debug.\n"
		"-d --debug\n"
		"\tDebug mode, do not daemonize.\n"
		"-e --backendssl\n"
		"\tUse SSL connection to backend service.\n"
		"-s --frontendssl\n"
		"\tServe data via a SSL-enabled connection.\n"
		"-b --backendhost\n"
		"\tBackend IP or hostname hosting the service to be proxied.\n"
		"-p --backendport\n"
		"\tBackend port corresponding to the service to be proxied.\n"
		"-f --frontendport\n"
		"\tFrontend port corresponding to the exposed proxy.\n"
		"-l --frontendlisten\n"
		"\tFrontend port corresponding to the exposed proxy.\n"
		"-c --certpath\n"
		"\tAbsolute path to the SSL cert used by the proxy.\n"
		"-k --keypath\n"
		"\tAbsolute path to the (passwordless) SSL private key used by the proxy.\n"
		"-P --pidpath\n"
		"\tAbsolute path to the file intended to record the proxy PID.\n"
		);

}

static void sighandler(int signum){

	int ret = EXIT_FAILURE;

 	if (signum == -1) {
		fprintf(stderr, "Exiting due to errors.\n");
	} else {

		if (signum == SIGTERM || signum == SIGINT){
			fprintf(stderr, "Caught CTRL-C or SIGTERM.\n");
			ret = EXIT_SUCCESS;
		} else {
			fprintf(stderr, "Exiting due to errors.\n");
		}
	}

	cleanup_exit(ret);

}

static void setup_signal_handler(void) {

	printf("Setting signal handler\n");

	struct sigaction s;

	sigfillset(&s.sa_mask);
	sigdelset(&s.sa_mask, SIGSEGV);
	sigdelset(&s.sa_mask, SIGTERM);
	sigdelset(&s.sa_mask, SIGINT);
	s.sa_sigaction = NULL;
	s.sa_restorer = NULL;
	//pthread_sigmask(SIG_BLOCK, &sigs, NULL);

	s.sa_handler = sighandler;
	s.sa_flags = 0;
	if (sigaction(SIGTERM, &s, NULL) <0 || sigaction(SIGINT, &s, NULL) < 0){
		fprintf(stderr, "ERROR: failure setting signal handler\n");
	}

}

void ssl_teardown(SSL_CTX *ctx, SSL * conn ){
	SSL_SESSION *session = NULL;

	session = SSL_get_session (conn);

	if (session)
		SSL_CTX_remove_session (ctx, session);
	if (conn)
		SSL_free (conn);
}

static void zssl_init(void){

	printf("Initializing SSL\n");

	SSL_library_init();
	//OpenSSL_add_all_algorithms();
	//SSLeay_add_ssl_algorithms ();
	SSL_load_error_strings ();

	return;
}

SSL_CTX * ssl_ctx_new (int servermode, int usetls, const char * cert_file, const char *key_file){

#if OPENSSL_VERSION_NUMBER < 0x10000000
	SSL_METHOD      *meth = NULL;
#else
	const SSL_METHOD      *meth = NULL;
#endif
	SSL_CTX 	*ctx = NULL;

	//TODO debug print state value here to see what is what
	//	whether we need one or the other, per esx version, etc
	if(usetls) {
		if(servermode)
			meth = TLSv1_server_method();
		else
			meth = TLSv1_client_method();

		fprintf(stderr, "if err occurs after this msg, may need"
				" SSL_CTX_set_options(ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2)"
				" or similar\n");
	} else {
		if(servermode)
			meth = SSLv23_server_method();
		else
			meth = SSLv23_client_method();
	}

	if ( !(ctx = SSL_CTX_new (meth)) )
		goto out;

	//NOTE first step when state = 0, apply these options. what about after?
	//NOTE ALSO required when steve => 1
	//	do not add if(!state) again
	SSL_CTX_set_options (ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2 /* Work around all known bugs */);
	//NOTE SSL_OP_NO_SSLv2 is specifically required for zware

	if (cert_file){
      		if (SSL_CTX_use_certificate_file (ctx, cert_file, SSL_FILETYPE_PEM) <= 0)
			goto out;
		if (!key_file)
			key_file = cert_file;
		if (SSL_CTX_use_PrivateKey_file (ctx, key_file, SSL_FILETYPE_PEM) <= 0)
			goto out;
		if (!SSL_CTX_check_private_key (ctx))
			goto out;
	}

	SSL_CTX_set_verify (ctx, SSL_VERIFY_NONE, NULL);

	return ctx;

out:
	ERR_print_errors_fp(stderr);
	return NULL;
}

SSL * ssl_new_conn(SSL_CTX *ctx, int fd){

	SSL *conn;

	if(!ctx){
		fprintf(stderr, "SSL_CTX is null\n");
		return NULL;
	}

	if ( !(conn = (SSL *) SSL_new (ctx)) ){
		fprintf(stderr, "SSL_new failed\n");
		goto err;
	}

	SSL_clear(conn);

	if (!SSL_set_fd (conn, fd)){
		fprintf(stderr, "SSL_set_fd failed\n");
		goto err;
	}

	return conn;
err:
	ssl_display_errors();
	return NULL;
}

void ssl_setup_accept(SSL_CTX *ctx __attribute__ ((unused)), SSL *conn){
	int sslaccept = 0;
	X509 *cert;
	char *line;

	sslaccept = SSL_accept(conn);

	if(sslaccept < 0){
		ssl_display_errors();
		fprintf(stderr, "SSL_accept failed with error %d\n", SSL_get_error(conn, sslaccept));
		cleanup_ssl();
		exit(EXIT_FAILURE);
	}

	printf("SSL connection using %s\n", SSL_get_cipher (conn));

	/* if present, get certs and display some info about them */
	if ( (cert = SSL_get_peer_certificate(conn)) ) {
		printf("Server certs:\n");
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("Subject: %s\n", line);
		free(line);
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		printf("Issuer: %s\n", line);
		free(line);
		X509_free(cert);
	} else {
		printf("No certificates.\n");
	}
}

void ssl_setup_conn(SSL_CTX *ctx, SSL *conn){
	int sslcret = SSL_connect (conn);

	if (sslcret > 0) return;

	fprintf(stderr, "SSL_connect failed. Error code %d with "
		"errno %d for SSL_connect return %d\n",
		SSL_get_error(conn, sslcret), errno, sslcret);

	ssl_display_errors();
	ssl_teardown(ctx, conn);
	fprintf(stderr, "exiting due to failure.\n");

	cleanup_ssl();
	exit(EXIT_FAILURE);
}

static int zconnect (const char *hostname, const char *service, int servermode){

	int sockfd = -1;

#ifdef HAVE_ADDRINFO
	struct addrinfo *in, *in0, hints;
#else
	struct hostent *host;
	struct servent *serv;
	struct sockaddr_in sin;
#endif

#ifdef HAVE_ADDRINFO
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo (hostname, service, &hints, &in0))
		goto out;

	for (in = in0; in; in = in->ai_next) {

		if( (sockfd = socket (in->ai_family, in->ai_socktype, in->ai_protocol)) < 0){
			perror("socket");
			fprintf(stderr, "trying again.\n");
			continue;
		}

		if (sockfd < 0)
			continue;
		if(servermode){
			if (bind (sockfd, in->ai_addr, in->ai_addrlen) < 0) {
				close(sockfd);
				sockfd = -1;
				perror("socket");
				fprintf(stderr, "trying again.\n");
				continue;
			}

			if( listen(sockfd, MAX_CONNECT_QUEUE) == -1 ){
				fprintf(stderr, "error beginning to listen on socket\n");
				close(sockfd);
				sockfd = -1;
				perror("listen");
				fprintf(stderr, "trying again.\n");
				continue;
			}

		} else{
			if (connect (sockfd, in->ai_addr, in->ai_addrlen) < 0) {
				close(sockfd);
				sockfd = -1;
				perror("connect");
				fprintf(stderr, "trying again.\n");
				continue;
			}
		}
		break;
	}

	//bail
	if (sockfd < 0)
		goto out;

#else
	memset (&sin, 0, sizeof (sin));

	if(!hostname){
		fprintf(stderr, "invalid param: hostname is null\n");
		goto out;
	}

	if ( !(host = gethostbyname (hostname)) ){
		fprintf(stderr, "gethostbyname err\n");
		goto out;
	}

	memcpy (&sin.sin_addr, host->h_addr, host->h_length);

	serv = getservbyname (service, "tcp");

	if (serv)
		sin.sin_port = serv->s_port;
	else if (isdigit (service[0]))
		sin.sin_port = htons (atoi (service));

	sin.sin_family = AF_INET;

	if( (sockfd = socket (sin.sin_family, SOCK_STREAM, 0)) < 0 ){
		perror("socket");
		sockfd = -1;
		goto out;
	}


	if(servermode){
		if (bind (sockfd, (struct sockaddr *)&sin, sizeof (sin)) < 0){
			close (sockfd);
			perror("connect");
			sockfd = -1;
			goto out;
		}

		if( listen(sockfd, MAX_CONNECT_QUEUE) == -1 ){
			close(sockfd);
			perror("listen");
			fprintf(stderr, "error beginning to listen on socket\n");
			sockfd = -1;
			goto out;
		}

	} else{
		if (connect (sockfd, (struct sockaddr *)&sin, sizeof (sin)) < 0){
			close (sockfd);
			perror("connect");
			sockfd = -1;
			goto out;
		}
	}

#endif

	const int optvfalse = 0;
	const int optvtrue = 1;
	if (servermode) {
		setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &optvtrue, sizeof(optvtrue));
  		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optvtrue, sizeof(optvtrue));

		setsockopt (sockfd, SOL_SOCKET, SO_KEEPALIVE, &optvfalse, sizeof(optvfalse));
	} else {
		setsockopt (sockfd, SOL_SOCKET, SO_KEEPALIVE, &optvfalse, sizeof(optvfalse));
  		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optvtrue, sizeof(optvtrue));
	}

out:
	return sockfd;
}

int main (int argc, char **argv) {

	//opterr=0 means dont print error via getopt if option parameter is missing
	extern int opterr;
	int i 		= 0;
	int opt_ind 	= 1;
	opterr = 0;

	static struct option long_options[] = {
		{ "help",		no_argument,		0,	'h' },
		{ "debug",		no_argument,      	0,	'd' },
		{ "verbose",		no_argument,      	0,	'v' },
		{ "backendssl",		no_argument,    	0,	'e' },
		{ "frontendssl",	no_argument,	      	0,	's' },
		{ "backendhost",	required_argument,      0,	'b' },
		{ "backendport",	required_argument,      0,	'p' },
		{ "frontendport",	required_argument,	0,	'f' },
		{ "frontendlisten",	required_argument,      0,	'l' },
		{ "certpath",		required_argument,      0,	'c' },
		{ "keypath",		required_argument,      0,	'k' },
		{ "pidpath",		required_argument,      0,	'P' },
		{ 0, 0, 0, 0 },
	};

	backendssl 	= 0;
	frontendssl 	= 0;
	cert 		= strdup(DEFAULT_CERT_PATH);
	key 		= strdup(DEFAULT_KEY_PATH);
	frontendlisten 	= strdup(DEFAULT_LISTEN_ADDR);

	debug 	= 0;
	verbose = 0;

	while (i != -1) {
		i = getopt_long(argc, argv, ":hdvb:p:esf:l:c:k:P:", long_options, &opt_ind);
		switch (i) {
			case 'c': /* cert path */
				SETOPT(cert);
				break;
			case 'k': /* key */
				SETOPT(key);
				break;
			case 'P': /* pid file */
				SETOPT(pidpath);
				break;
			case 'l': /* frontend listen */
				SETOPT(frontendlisten);
				break;
			case 'f': /* frontend port */
				SETOPT(frontendport);
				break;
			case 'v': /* verbose */
				verbose = 1;
				debug = 1;
				break;
			case 'e': /* backend ssl */
				backendssl = 1;
				break;
			case 'b': /* backend host */
				SETOPT(backendhost);
				break;
			case 's': /* frontend ssl */
				frontendssl = 1;
				break;
			case 'p': /* backend port */
				SETOPT(backendport);
				break;
			case 'd':
				debug = 1;
				break;
			case 'h': /* help */
				usage();
				exit(0);
				break;
			case ':':
				if (!optarg & (i != -1)) {
					fprintf(stderr, "ERROR: Option '%c' requires an argument.\n\n", optopt);
					usage();
					exit(1);
				}
				break;
			case '?':
				if (optopt) fprintf(stderr, "Bad option %c\n", optopt);
				else fprintf(stderr, "Bad option %s\n", argv[optind]);
				usage();
				exit(1);
				break;
			default:
				break;
		}
	}

	WARNOPT(frontendlisten);

	ERROPT(frontendport);
	ERROPT(backendhost);
	ERROPT(backendport);
	ERROPT(pidpath);

	if(frontendssl){
		WARNOPT(key);
		WARNOPT(cert);

		int fd = -1;
		struct stat filestat;
		if (stat(key, &filestat) != 0){
			fprintf(stderr, "ERROR: SSL key at path '%s' does not exist.\n", key);
			cleanup_exit(EXIT_FAILURE);
		}

		fd = open(key, O_RDONLY);
		if(errno == EACCES) {
			fprintf(stderr, "ERROR: SSL key at path '%s' is not readable.\n", key);
			cleanup_exit(EXIT_FAILURE);
		}
		if(fd > 0) close(fd);

		if (stat(cert, &filestat) != 0){
			fprintf(stderr, "ERROR: SSL cert at path '%s' does not exist.\n", cert);
			cleanup_exit(EXIT_FAILURE);
		}

		fd = open(cert, O_RDONLY);
		if(errno == EACCES) {
			fprintf(stderr, "ERROR: SSL cert at path '%s' is not readable.\n", cert);
			cleanup_exit(EXIT_FAILURE);
		}
		if(fd > 0) close(fd);

	}

	if (zshare_pid_check(pidpath)){
		fprintf(stderr, "PID file (%s) already exists. Exiting.\n", pidpath);
		exit(1);
	}

	if(!debug){
		if( (!zshare_daemon_detach(1))){
			fprintf(stderr, "Could not daemonize. Exiting\n");
			exit(EXIT_FAILURE);
		}
	} else {
		printf("Running in debug mode; not daemonizing\n");
	}

	if(verbose)
		printf("Running in verbose mode; displaying connection and transfer information\n");

	switch (zshare_pid_create(pidpath))
	{
		case -1:
			fprintf(stderr, "ERROR: Could not write pid file. Check permissions and system status.");
			cleanup_exit(EXIT_FAILURE);
		case 0:
			fprintf(stderr, "ERROR: Could not write pid file. Daemon already running or previously exited uncleanly.");
			exit(EXIT_FAILURE);
		default:
			break;
	}

	setup_signal_handler();

	if(!debug){
		if(!zshare_daemon_closefds(0)){
			fprintf(stderr, "ERROR: Could not detach daemon.");
			exit(EXIT_FAILURE);
		}
	}

	proxy();

	exit(0);
}

void proxy(void){

	int frontendsslinit = 0;

	if(frontendssl || backendssl)
		zssl_init();

	proxyfd = zconnect(frontendlisten, frontendport, 1);
	if ( proxyfd < 0 ) {
		printf("could not start client-facing proxy connection. aborting\n");

		cleanup_exit(EXIT_FAILURE);
	}

restartbackend:
	backend_host_fd = zconnect (backendhost, backendport, 0);
	if (backend_host_fd >= 0){
		fprintf(stderr, "connection succeeded.\n");
	} else if (backend_host_fd < 0){
		perror ("zconnect");
		fprintf(stderr, "connection failed. aborting.\n");
		//FIXME ssl cleanup
		cleanup_exit(EXIT_FAILURE);
	}

	//restart tls for the actual console connection
	if(backendssl) {
		if ( !(backend_ctx = ssl_ctx_new (0, 0, cert, key)) ){
			printf("ssl_ctx_new failed\n");

			ssl_display_errors();
			cleanup_ssl();
			cleanup_exit(EXIT_FAILURE);
		}

		//negotiation
		backend_conn = ssl_new_conn(backend_ctx, backend_host_fd);

		ssl_setup_conn(backend_ctx, backend_conn);

	} else {
		printf("backend does not use SSL\n");
	}

	/*TODO :  refactor into smaller functions if reasonable  */

	int  nbuffer, wrote;
	struct pollfd pollfds[3]; //vmware_auth socket, server socket, client socket
	char buffer[BUFSIZ];
	socklen_t addr_size;
	struct sockaddr_in client_addr;

	//consider pollfds[blah].events = POLLIN | POLLOUT | POLLHUP | POLLPRI;
	pollfds[0].fd = backend_host_fd;
	pollfds[0].events = POLLIN | POLLHUP ;

	pollfds[1].fd = proxyfd;
	pollfds[1].events = POLLIN | POLLHUP ;

//awaitclient:
	addr_size = sizeof(struct sockaddr);
	clientfd = accept(proxyfd, (struct sockaddr *)&client_addr, &addr_size);

	unsigned int len = sizeof(client_addr);
	char ip[INET_ADDRSTRLEN];
	if (!getpeername(clientfd, (struct sockaddr*) &client_addr, &len))
		printf("New connection from %s:%d\n",
			inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN),
			ntohs(client_addr.sin_port));

	if(frontendssl){
		if(!frontendsslinit){
			if ( !(frontend_ctx = ssl_ctx_new (1, 0, cert, key)) ){
				fprintf(stderr, "ssl_ctx_new failed\n");

				ssl_display_errors();
				cleanup_ssl();
				cleanup_exit(EXIT_FAILURE);
			}
			frontendsslinit = 1;
		}

		if(frontend_conn)
			SSL_free(frontend_conn);

		frontend_conn = ssl_new_conn(frontend_ctx, clientfd);

		ssl_setup_accept(frontend_ctx, frontend_conn);
	} else {
		printf("frontend does not use SSL\n");
	}

	//pollfds[2].events = POLLIN | POLLOUT | POLLHUP | POLLPRI;
	pollfds[2].fd = clientfd;
	pollfds[2].events = POLLIN | POLLHUP ;

	while (1) {
		memset(buffer, 0, BUFSIZ);

		if (poll (pollfds, 3, 100) == -1 && errno != EINTR) {
			perror ("poll");
			cleanup_exit(EXIT_FAILURE);
		}

		//maybe check for poll retval == 0 (i.e. no fds to inspect)
		//	and just continue;

		/* bail if either side has disconnected */
		//FIXME add a GOTO at end of this function() to
		//	consolidate exit code?
		if (pollfds[0].revents & POLLHUP){
			//FIXME i think this means it disconnected
			fprintf(stderr, "backend disconnected\n");

			if(backendssl)
				ssl_teardown(backend_ctx, backend_conn);

			goto cleanup;
		} else if (pollfds[1].revents & POLLHUP){
			//FIXME i think this means it disconnected
			fprintf(stderr, "proxy server port disconnected\n");

			if(backendssl)
				ssl_teardown(backend_ctx, backend_conn);

			goto cleanup;;
		} else if (pollfds[2].revents & POLLHUP ){
			//FIXME i think this means it disconnected
			fprintf(stderr, "client hungup\n");

			//close old socket
			close(clientfd);
			close(backend_host_fd);

			//wait for reconnect
			//NOTE: assumption is that - if no reconnect occurs - this proxy will be
			//	reaped on user logout OR by automated
			//	stale session cleanup
			//goto awaitclient;
			goto restartbackend;

		}

/*
 * for now, this is just a 1:1 client:host proxy, with a new instance fired up
 * 	on demand.
 *  	braindead, but it gets the job done
 *
 * this inspection of the proxyfd is for debugging only. as written, we cant
 * 	accept() here anyway because it would blow away any other client connection
 *
 * to handle many clients simultaaneously connecting to one or many backends,
 * accept() should be in its own thread, spawning new threads for read/write as
 * 	 new clients connect for new backends.
 * 	 this will be very useful when we have a generic proxy server using console
 * 	 'tickets'
		if (pollfds[1].revents & POLLIN){
			//incoming on frontendport
			printf("stuff to read from proxy port\n");
			clientfd = accept(proxyfd, (struct sockaddr *)&client_addr, &addr_size);

			data_send(clientfd, "\nHEY YOU\n", 10);

			pollfds[2].fd = clientfd;
			//pollfds[2].events = POLLIN | POLLOUT | POLLHUP | POLLPRI;
			pollfds[2].events = POLLIN | POLLHUP ;
			pollfds[1].revents =0;
		}
*/

		if (pollfds[2].revents & POLLIN){
			//incoming on clientport
			if(verbose)
				printf("stuff to read from client\n");

			if(frontendssl){
				nbuffer = SSL_read (frontend_conn, buffer, sizeof buffer -1);

				if(nbuffer < 0){
					ssl_display_errors();
					perror("SSL_read");
					cleanup_ssl();
					cleanup_exit(EXIT_FAILURE);
				}

			} else {
				nbuffer = read(clientfd, buffer, sizeof buffer -1);

				if(nbuffer < 0){
					perror("read");
					fprintf(stderr, "connection failed. aborting.\n");
					cleanup_exit(EXIT_FAILURE);
				}
			}

			if(nbuffer == 0){
				if(verbose)
					printf("EOF reached from client. assuming hangup. back to accept()\n");

				//close old sockets
				//FIXME ssl shutdown
				close(clientfd);
				close(backend_host_fd);

				goto restartbackend;
			}

			if(backendssl) {
				wrote = SSL_write(backend_conn, buffer, nbuffer);
			} else {
				wrote = data_send(backend_host_fd, buffer, nbuffer);
			}

			if(verbose)
				printf("client sent len %d, we wrote %d to backend\n", nbuffer, wrote);
		}

		if (pollfds[0].revents & POLLIN){
			if(verbose)
				printf("stuff to read from backend\n");

			if(backendssl){
				if (!backend_conn){
					printf("TLS session ended abruptly. exiting\n");
					cleanup_exit(EXIT_FAILURE);
				}
				nbuffer = SSL_read (backend_conn, buffer, sizeof buffer -1);

				if(nbuffer < 0){
					ssl_display_errors();
					perror("SSL_read");
					cleanup_ssl();
					cleanup_exit(EXIT_FAILURE);
				}

			} else {
				nbuffer = read(backend_host_fd, buffer, sizeof buffer -1);

				if(nbuffer < 0){
					perror("read");
					fprintf(stderr, "connection failed. aborting.\n");
					cleanup_exit(EXIT_FAILURE);
				}

			}

			if(nbuffer == 0) {
				if(verbose)
					fprintf(stderr, "EOF reached from backend. assuming hangup. restarting backend\n");

				if(backendssl)
					ssl_teardown(backend_ctx, backend_conn);

				close(backend_host_fd);

				if(frontendssl)
					ssl_teardown(frontend_ctx, frontend_conn);

				close(clientfd);

				goto restartbackend;
			}

			if(frontendssl){
				wrote = SSL_write(frontend_conn, buffer, nbuffer);

				if(wrote < 0){
					ssl_display_errors();
					perror("SSL_read");
					cleanup_ssl();
					cleanup_exit(EXIT_FAILURE);
				}

			} else {
				wrote = data_send(clientfd, buffer, nbuffer);

				if(wrote < 0){
					perror("read");
					fprintf(stderr, "connection failed. aborting.\n");
					cleanup_exit(EXIT_FAILURE);
				}

			}
			if(verbose)
				printf("backend sent len %d we wrote %d to client\n", nbuffer, wrote);
		}

	}

cleanup:
	if(backendssl || frontendssl){
			ssl_display_errors();
			cleanup_ssl();
	}
	close(clientfd);
	close(proxyfd);
	close(backend_host_fd);

	cleanup_exit(EXIT_SUCCESS);
}

/*
	  		if (tls_conn && SSL_pending(tls_conn)){
			    printf("ssl pending\n");
			    goto readtop;
			}

*/

