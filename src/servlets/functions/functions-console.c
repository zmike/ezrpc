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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 */
#include "functions.h"
#include "functions-console.h"
#include "functions-vm.h"
#include "functions-user.h"
#include "functions-session.h"
#include "functions-zentific.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <inttypes.h>

extern zentific_config *config;

/*
 * get_free_tcp_port_near
 * This returns the next available tcp port
 *
 * Input:
 * uint16_t start	- Find a free port starting from this port
 *
 * Output:
 * uint16_t start	- An unused port
 */
static uint16_t get_free_tcp_port_between(uint16_t start, uint16_t end){

	uint16_t port;
	int	sock;
	struct sockaddr_in addr;

	port = start;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		return (0);
	}

	/* FIXME do we really want to use INADDR_ANY here?
 	 *	yay for semantics.
 	 */
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	/* FIXME: blocking */
	while (bind(sock, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
		port++;
		addr.sin_port = htons(port);

		if (port > end){
			//ERROR: could not find free port in range supplied
			//FIXME log error
			port = -1;
			break;
		}

	}

	/*
	 * Now close the socket and use the port
	 * NOTE: cant trust socket reuse flags, so
	 * 	deal with the race and hope for the best
	 * 	TODO investigate alternatives that make some sort of sense
	 */
	close(sock);
	return (port);
}

/*
 * try to find a (mostly likely) free port which can be reused
 *
 * Idea: Open a TCP socket and let UNIX find a free port, close the socket
 *    and print the port number.
 *    Normally UNIX assignes port numbers one after the other.
 *    Therefore the next application should not get the same port as we.
 *    This is not absolutely safe but should work in practice.
 *
 * 26 Jan 1996, hot@informatik.tu-chemnitz.de
 *
 */
static uint16_t get_random_free_tcp_port(void){

	int ret = -1;

	struct sockaddr_in tcp_addr; 	/* TCP address to be forwarded by SSH */

	struct hostent *hp;	 	/* for gethostbyname */

	socklen_t addr_len;	/* length of a socket address */
	int tcp_sockfd;		/* TCP socket descriptor */

	/*
	* resolve "localhost"
	*/
	/* FIXME: blocking */
	if ((hp = gethostbyname("localhost")) == NULL)
	{
		fprintf(stderr, "can't resolve localhost\n");
		return -1;
	}

	/*
	* create a TCP listening socket
	*/

	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_sockfd < 0)
	{
		perror("can't open stream socket for server");
		return -1;
	}

	/* old code.
 	 *
	 * bzero((char *) &tcp_addr, sizeof(tcp_addr));
	 * tcp_addr.sin_family   = AF_INET;
	 * bcopy((char *)hp->h_addr,(char *)&tcp_addr.sin_addr, hp->h_length);
	 * tcp_addr.sin_port	= htons(0);
	 *
 	 * ..because bcopy/bzero not in the C standard, gnu extension..
 	 */
	memset(&tcp_addr, 0, sizeof(tcp_addr));
	tcp_addr.sin_family   = AF_INET;
	strncpy((char *)hp->h_addr,(char *)&tcp_addr.sin_addr, hp->h_length);
	tcp_addr.sin_port	= htons(0);

	/*
	* bind the socket
	*/
	/* FIXME: blocking */
	if (bind(tcp_sockfd, (struct sockaddr *) &tcp_addr, sizeof(tcp_addr)) < 0)
	{
		fprintf(stderr, "can't bind local address for server\n");
		return -1;
	}

	/*
	* read the binding back
	*/

	addr_len = (socklen_t)sizeof(tcp_addr);
	getsockname(tcp_sockfd, (struct sockaddr *) &tcp_addr, &addr_len);

	/*
	* set the return to the free port number
	*/

	ret = ntohs(tcp_addr.sin_port);

	/*
	* close the socket (and hope nobody will "steal" it till SSH grabs it)
	*/

	close(tcp_sockfd);

	return ret;
}

Eina_Bool _saveGraphicalConsolePort(Zentific_Ctx *ctx, const char *uuid, int tunnelport, int externalport){
	db->saveGraphicalConsolePort(db, ctx, uuid, tunnelport, externalport);
	CTX_RETURN;
}
Eina_Bool _getGraphicalConsoleTunnelPort(Zentific_Ctx *ctx, const char *uuid){
	db->getGraphicalConsoleTunnelPort(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getGraphicalConsolePort(Zentific_Ctx *ctx, const char *uuid){
	db->getGraphicalConsolePort(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _saveTextConsolePort(Zentific_Ctx *ctx, const char *uuid, int externalport){
	db->saveTextConsolePort(db, ctx, uuid, externalport);
	CTX_RETURN;
}

Eina_Bool _getTextConsolePort(Zentific_Ctx *ctx, const char *uuid){
	db->getTextConsolePort(db, ctx, uuid);
	CTX_RETURN;
}


/* if this is called, console is okay */
static Eina_Bool
console_cb_timer(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;

	ctx->dead = EINA_TRUE;
	azy_server_module_events_rewind(pctx->parent);
	azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	return EINA_FALSE;
}

static void
console_port_find(Zentific_Ctx *ctx, Ecore_Thread *thread __UNUSED__){
	int port;

	port = get_random_free_tcp_port();
	ctx->data2 = (intptr_t*)port;
}

static void
console_port_find_end(Zentific_Ctx *ctx, Ecore_Thread *thread __UNUSED__){
	ctx->count++;
	REWIND;
	RESUME;
}

//FIXME FIXME
//	To support multiple frontend servers:
//	both connectTextConsole and connectGraphicalConsole
//	will need to place the externally exposed port
//	into the database (by reading pid files),
//	along with the zentific frontend host from
//	which the console is being proxied.
//	Do this in the console lock functions
Eina_Bool _connectTextConsole(Zentific_Ctx *ctx, SESSION *session, const char *uuid){
	const char *node;
	intptr_t *id;
	int port;
	Eina_Strbuf *cmd;
	Zentific_Ctx *zctx;
	//NOTE check for a console lock outside of this
	// function too
	switch (ctx->count){
		case 0:
			_hasConsoleLock(ctx, uuid, "text");
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			if (!ctx->error){
				ctx->error = AZY_ERR(CONSOLE_LOCKED);
				break;
			}
			_acquireConsoleLock(ctx, uuid, session, "text");
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 2:
			if (ctx->error){
				ERR("Text console tunnel could not be established, %s.", uuid);
				ctx->error = AZY_ERR(CONSOLE_CONNECT_FAILED);
				break;
			}
			_getVMparentAddress(ctx, uuid);
			CTX_VALUE_RETURN;
			break;
		case 3:
			node = ctx->data = CTX_PARAM_GET;
			if (ctx->error){
				eina_stringshare_del(node);
				break;
			}
			_getVMstat(ctx, uuid, "domid");
			CTX_INT_RETURN;
			break;
		case 4:
			if (ctx->error){
				eina_stringshare_del(ctx->data);
				break;
			}
			ecore_thread_run((Ecore_Thread_Cb)console_port_find,
				(Ecore_Thread_Cb)console_port_find_end,
				(Ecore_Thread_Cb)console_port_find_end, ctx);
			break;
		case 5:
			node = ctx->data;
			port = (intptr_t)ctx->data2;
			if (port < 0){
				ERR("Graphical console tunnel could not be established, %s.", uuid);
				ctx->error = AZY_ERR(CONSOLE_CONNECT_FAILED);
				break;
			}
			id = CTX_PARAM_GET;
			//TODO hook for firewalling
			cmd = eina_strbuf_new();
			eina_strbuf_append_printf(cmd, //"unset LANG && "
				"/opt/zentific/bin/shellinaboxd -b -q -u zentific -c /opt/zentific/ssl "
				//add option to reverse tty colors in context menu
				"--user-css Normal:+/opt/zentific/share/doc/shellinabox/white-on-black.css,"
				"Reverse:-/opt/zentific/share/doc/shellinabox/black-on-white.css "
				//"-s /WEB_PATH_GOES_HERE:SSH:"
				"-s /%s-%s:SSH:"
				//HOSTNAME-${SSH PORT ON THAT HOST}-${VMID}
				"%s-22_%i"
				" --pidfile %s/run/zentific/zentific-console_%s_%s_%d.pid"
				//set the port that shellinabox will run on
				" -p %d",
				session->session_id, uuid,
				node, //nodeport,
				(int)((intptr_t)id),
				config->VAR_PATH, session->session_id, uuid, port,
				port);
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->parent = ctx;
			ctx->retval = ecore_exe_run(eina_strbuf_string_get(cmd), zctx);
			eina_strbuf_free(cmd);
			eina_stringshare_del(node);
			ctx->data = (intptr_t*)port;
			if (!ctx->retval){
				_releaseConsoleLock(ctx, uuid, session->session_id, "text");
				free(zctx);
				CTX_CB_ADD(cb_bool_rewind);
				eina_stringshare_del(ctx->data);
			} else {
				char buf[128];
				snprintf(buf, sizeof(buf), "text:%s:%s", uuid, session->session_id);
				ecore_exe_tag_set(ctx->retval, buf);
				zctx->retval = ecore_timer_add(0.5, (Ecore_Task_Cb)console_cb_timer, zctx);
			}
			ctx->count++;
			break;
		case 6:
			if (!ctx->retval){
				ctx->error = AZY_ERR(CONSOLE_CONNECT_FAILED);
				break;
			}
			ctx->retval = NULL;
			if (ctx->error) break;
			port = (intptr_t)ctx->data;
			INF("Text console tunnel successfully established, external port %d, %s.", port, uuid);
			_saveTextConsolePort(ctx, uuid, port);
			CTX_CB_ADD(cb_bool_rewind);
		case 7:
			ctx->error = 0;
			port = (intptr_t)ctx->data;
			AZY_RETVAL(port);
			RESUME;
		default:
			break;
	}
	CTX_RETURN;
}

static void
console_graphical_port_find(Zentific_Ctx *ctx, Ecore_Thread *thread __UNUSED__){
	const char *str;
	int externalport;

	str = ctx->data;
	if (str){
		uint16_t start, end;

		if (sscanf(str, "%"SCNi16"-%"SCNi16, &start, &end) == 2)
			externalport = get_free_tcp_port_between(start, end);
		else
			externalport = get_random_free_tcp_port();
		eina_stringshare_del(str);
	} else
		externalport = get_random_free_tcp_port();

	ctx->data = (intptr_t*)externalport;
}

Eina_Bool _connectGraphicalConsole(Zentific_Ctx *ctx, SESSION *session, const char *uuid){
/*
 * ssh -Nnf -M -S /tmp/ssh-tunnel-control_UUID_SESSION_%h-%p-%r.ctl sam04005@host
 * ssh -S /tmp/ssh-tunnel-control_UUID_SESSION_%h-%p-%r.ctl -O check sam04005@host
 * 	returns pid of the previous master session
 *
 * else:
 * setsid sh -c 'echo $$ >/var/run/ssh.pid; exec ssh -N -L 10.0.0.1:139:20.250.205.96:139 srvusr@srvb'
 * sh -c 'kill -INT $(cat /var/run/ssh.pid)

 */
	const char *str;
	int externalport, tunnelport;
	Eina_Strbuf *cmd;
	Eina_Bool success;
	Zentific_Ctx *zctx;

	switch (ctx->count){
		case 0:
			_hasConsoleLock(ctx, uuid, "vnc");
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			if (!ctx->error){
				ctx->error = AZY_ERR(CONSOLE_LOCKED);
				break;
			}
			_getConfig(ctx, "console_port_range");
			CTX_VALUE_RETURN;
			break;
		case 2:
			ctx->error = 0;
			ctx->data = CTX_PARAM_GET;
			_getVMattribute(ctx, uuid, "vncport");
			CTX_INT_RETURN;
			break;
		case 3:
			ctx->retval = CTX_PARAM_GET;
			if (ctx->error){
				ERR("Graphical console tunnel could not be established, %s.", uuid);
				ctx->error = AZY_ERR(CONSOLE_CONNECT_FAILED);
				eina_stringshare_del(ctx->data);
				break;
			}
			_acquireConsoleLock(ctx, uuid, session, "vnc");
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 4:
			if (ctx->error){
				ERR("Graphical console lock could not be established, %s.", uuid);
				ctx->error = AZY_ERR(DB_FAIL);
				eina_stringshare_del(ctx->data);
				break;
			}
			ecore_thread_run((Ecore_Thread_Cb)console_graphical_port_find,
				(Ecore_Thread_Cb)console_port_find_end,
				(Ecore_Thread_Cb)console_port_find_end, ctx);
			break;
		case 5:
			externalport = (intptr_t)ctx->data;
			if (externalport < 0){
				ERR("Graphical console tunnel could not be established, %s.", uuid);
				ctx->error = AZY_ERR(CONSOLE_CONNECT_FAILED);
				break;
			}
			ecore_thread_run((Ecore_Thread_Cb)console_port_find,
				(Ecore_Thread_Cb)console_port_find_end,
				(Ecore_Thread_Cb)console_port_find_end, ctx);
			break;
		case 6:
			externalport = (intptr_t)ctx->data;
			tunnelport = (intptr_t)ctx->data2;
			if ((tunnelport < 0) || (tunnelport == externalport)) {
				ERR("Graphical console tunnel could not be established, %s.", uuid);
				ctx->error = AZY_ERR(CONSOLE_CONNECT_FAILED);
				break;
			}
			_getVMparentAddress(ctx, uuid);
			CTX_VALUE_RETURN;
			break;
		case 7:
			if (ctx->error) break;
			externalport = (intptr_t)ctx->data;
			tunnelport = (intptr_t)ctx->data2;
			str = CTX_PARAM_GET;
			//FIXME hook for firewalling
			cmd = eina_strbuf_new();
			eina_strbuf_append_printf(cmd,
				//NOTE: we're either running as root or as zentific user anyway,
				//	so kill the sudo
				//" sudo ssh -C -N"
				//
				//compression (ssh -C) is really only useful over slow links and
				//	manpage) may slow things down on fast links. remove it.
				//" ssh -C -Nn"
				"%s -Nnf"
				" -M -S %s/run/zentific/ssh_%s_%d_%%h-%%p-%%r.pid"
				//NOTE shortened because SSH complained the ControlPath was too long
				//" -M -S /tmp/zentific-tunnel_%s_%s_%d_%%h-%%p-%%r.pid"
				" -i %s/zentific_tunnel-key"
				" -o UserKnownHostsFile=%s/zentific_tunnel-known_hosts"
				" -o StrictHostKeyChecking=no"
				" -L %d:127.0.0.1:%i"
				" zentific_tunnel@%s ",
				//NOTE shortened because SSH complained the ControlPath was too long
				//sessionid, uuid, tunnelport, tunnelport, vncport, nodehost);
				config->SSH_PATH,
				config->VAR_PATH,
				uuid, tunnelport,
				config->KEY_PATH,
				config->KEY_PATH,
				tunnelport, (int)((intptr_t)ctx->retval /* vncport */),
				str /* puuid */);
			eina_stringshare_del(str);
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->parent = ctx;
			ctx->retval = ecore_exe_run(eina_strbuf_string_get(cmd), zctx);
			eina_strbuf_free(cmd);
			if (!ctx->retval){
				_releaseConsoleLock(ctx, uuid, session->session_id, "vnc");
				free(zctx);
				CTX_CB_ADD(cb_bool_rewind);
				eina_stringshare_del(ctx->data);
			} else {
				char buf[128];
				snprintf(buf, sizeof(buf), "vnc:%s:%s", uuid, session->session_id);
				ecore_exe_tag_set(ctx->retval, buf);
				zctx->retval = ecore_timer_add(0.5, (Ecore_Task_Cb)console_cb_timer, zctx);
			}
			ctx->retval = (intptr_t*)tunnelport;
			ctx->count++;
			break;
		case 8:
			if (!ctx->retval){
				ERR("Graphical console backend ssh tunnel could not be established, %s.", uuid);
				ctx->error = AZY_ERR(CONSOLE_CONNECT_FAILED);
				break;
			}
			if (ctx->error) {
				ERR("Graphical console backend ssh tunnel could not be established, %s.", uuid);
				break;
			}
			INF("Graphical console backend ssh tunnel successfully established, %s.", uuid);
			_saveGraphicalConsolePort(ctx, uuid,
				(int)((intptr_t)ctx->retval) /* tunnelport */, (int)((intptr_t)ctx->data) /* externalport */);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 9:
			ctx->error = 0;
			cmd = eina_strbuf_new();
			eina_strbuf_append_printf(cmd,
				"/opt/zentific/utils/zproxy"
				" -s -l 0.0.0.0 -b 127.0.0.1 -p %d -f %d"
				" -P %s/run/zentific/zentific-zproxy_%s_%s_%d.pid",
				(int)((intptr_t)ctx->retval) /* tunnelport */, (int)((intptr_t)ctx->data) /* externalport */,
				config->VAR_PATH,
				session->session_id, uuid, (int)((intptr_t)ctx->retval) /* tunnelport */);
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->parent = ctx;
			success = !!ecore_exe_pipe_run(eina_strbuf_string_get(cmd), ECORE_EXE_USE_SH, zctx);
			eina_strbuf_free(cmd);
			if (!success){
				_releaseConsoleLock(ctx, uuid, session->session_id, "vnc");
				ctx->retval = NULL;
				free(zctx);
				CTX_CB_ADD(cb_bool_rewind);
				eina_stringshare_del(ctx->data);
			} else {
				char buf[128];
				snprintf(buf, sizeof(buf), "vnc:%s:%s", uuid, session->session_id);
				ecore_exe_tag_set(ctx->retval, buf);
				zctx->retval = ecore_timer_add(0.5, (Ecore_Task_Cb)console_cb_timer, zctx);
			}
			ctx->count++;
			break;
		case 10:
			if (!ctx->retval){
				ERR("Graphical console backend ssh tunnel could not be established, %s.", uuid);
				ctx->error = AZY_ERR(CONSOLE_CONNECT_FAILED);
				break;
			}
			if (ctx->error) {
				ERR("Graphical console backend ssh tunnel could not be established, %s.", uuid);
				break;
			}
			INF("Graphical console frontend zproxy successfully established, port %d, %s.", (int)((intptr_t)ctx->data) /* externalport */, uuid);
			AZY_RET((int)((intptr_t)ctx->data) /* externalport */);
			RESUME;
		default:
			break;
	}

	CTX_RETURN;
}

static void
console_disconnect_text(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	const char *uuid, *session;
	Eina_Strbuf *textpid;
	intptr_t *externalport;
	int recordedpid;
	FILE *fp;

	uuid = ctx->data;
	session = ctx->retval;
	switch (ctx->count){
		case 0:
			_hasConsoleLock(ctx, uuid, "text");
			CTX_BOOL_JOB(console_disconnect_text);
			break;
		case 1:
			if (ctx->error) {
				ctx->error = 0;
				goto out;
			}
			if (!ctx->retval){
				db->getConsoleLockSession(db, ctx, uuid, "text");
				CTX_VALUE_JOB(console_disconnect_text);
				break;
			}
		case 2:
			if (!ctx->retval){
				ctx->retval = CTX_PARAM_GET;
				if (ctx->error) break;
			}
			_getTextConsolePort(ctx, uuid);
			CTX_INT_JOB(console_disconnect_text);
			break;
		case 3:
			if (ctx->error) break;
			externalport = CTX_PARAM_GET;
			textpid = eina_strbuf_new();
			eina_strbuf_append_printf(textpid, "%s/run/zentific/zentific-console_%s_%s_%d.pid",
				config->VAR_PATH, session, uuid, externalport ? *externalport : 0);

			fp = fopen(eina_strbuf_string_get(textpid), "r");
			if ((!fp) || (fscanf(fp, "%d", &recordedpid) != 1)) {
				ERR("Unable to read pidfile for text console for %s", uuid);
				if (fp) fclose(fp);
				eina_strbuf_free(textpid);
				goto out;
			}
			fclose(fp);
			unlink(eina_strbuf_string_get(textpid));
			INF("Terminating text console %s.", eina_strbuf_string_get(textpid));
			if (kill((pid_t)recordedpid, SIGTERM)) {
				int e = errno;
				/* error occurred */
				if (e == EPERM){
					/* permission denied */
					ctx->error = AZY_ERR(CONSOLE_DISCONNECT_FAILED);
					ERR("Permission denied when attempting to terminate text console (%s)",
						strerror(e));
				} else {
					/* wrong pid / not running */
					ctx->error = AZY_ERR(CONSOLE_DAEMON_NOT_FOUND);
					ERR("Console daemon not running at pid %i! (%s)",
						recordedpid, strerror(e));
				}
				break;
			}
			//FIXME flock here?
			db->releaseConsoleLock(db, ctx, uuid, session, "text");
			CTX_BOOL_JOB(console_disconnect_text);
			break;
		case 4:
			ctx->error = 0;
			goto out;
		default:
			break;
	}
	if (!ctx->error) return;
out:
	eina_stringshare_del(session);
	eina_stringshare_del(uuid);
	pctx->error = ctx->error;
	free(ctx);
	if (--pctx->count == 2){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

static void
console_disconnect_vnc_thread(Zentific_Ctx *ctx, Ecore_Thread *thread __UNUSED__){
	const char *uuid, *node, *session;
	Eina_Strbuf *pidpath, *sshtunnelcmd;
	intptr_t *port;
	int recordedpid;
	struct stat s;
	FILE *fp;

	uuid = ctx->data;
	session = ctx->retval;
	node = ctx->data2;
	port = CTX_PARAM_GET;

	pidpath = eina_strbuf_new();
	eina_strbuf_append_printf(pidpath,
		"%s/run/zentific/zentific-zproxy_%s_%s_%d.pid",
		config->VAR_PATH, session, uuid, port ? *port : 0);
	if (stat(eina_strbuf_string_get(pidpath), &s)){
		ERR("Stunnel pid path %s not present for vm %s console."
			" (session %s). Tunnel dead?",
			eina_strbuf_string_get(pidpath), uuid, session);
		goto zpidfail;
	}
	fp = fopen(eina_strbuf_string_get(pidpath), "r");
	if (!fp)
		ERR("Unable to read pidfile for vnc console for %s", uuid);
	else if (fscanf(fp, "%d", &recordedpid) != 1)
		ERR("Unable to parse pid from pidfile for vnc console for %s.", uuid);
	else {
		INF("Terminating vnc console %s.",
			eina_strbuf_string_get(pidpath));

		if (kill((pid_t)recordedpid, SIGTERM))
			ERR("Unable to kill zproxy(%d) related to the vnc console for %s. Errno: %s",
				recordedpid, uuid, strerror(errno));
	}
	INF("Deleting pidfile for vnc console for %s", uuid);
	unlink(eina_strbuf_string_get(pidpath));
	if (fp) fclose(fp);
	eina_strbuf_string_free(pidpath);
zpidfail:
			//the other half of the tunnel
	eina_strbuf_append_printf(pidpath, "%s/run/zentific/ssh_%s_%d_%s-%d-zentific_tunnel.pid",
		config->VAR_PATH, uuid, port ? *port : 0, node, 22 /* sshport FIXME this should be configurable */);

	sshtunnelcmd = eina_strbuf_new();
	eina_strbuf_append_printf(sshtunnelcmd,
		"%s -S %s -O check zentific_tunnel@%s 2>&1", config->SSH_PATH, eina_strbuf_string_get(pidpath), node);

	fp = popen(eina_strbuf_string_get(sshtunnelcmd), "r");
	if (!fp)
		ERR("Unable to read pidfile for vnc console for %s", uuid);
	else if (fscanf(fp, "Master running (pid=%d)", &recordedpid) != 1)
		ERR("Unable to parse pid from pidfile for vnc console for %s.", uuid);
	else {
		INF("Terminating vnc console %s.", eina_strbuf_string_get(pidpath));
		if (kill((pid_t)recordedpid, SIGTERM)) {
			int e = errno;
			/* error occurred */
			if (e == EPERM){
				/* permission denied */
				ctx->error = AZY_ERR(CONSOLE_DISCONNECT_FAILED);
				ERR("Permission denied when terminating console tunnel (%s)",
					strerror(e));
			} else {
				/* wrong pid / not running */
				ctx->error = AZY_ERR(CONSOLE_DAEMON_NOT_FOUND);
				ERR("Console tunnel not running at pid %i! (%s)",
					recordedpid, strerror(e));
			}
		}
	}
	if (fp) pclose(fp);

	eina_stringshare_del(node);
	//NOTE ssh deletes the control socket (what we're calling the tunnel "pidfile") as soon as its killed
	eina_strbuf_free(sshtunnelcmd);
	eina_strbuf_free(pidpath);
}

static void console_disconnect_vnc(Zentific_Ctx *ctx);

static void
console_disconnect_vnc_thread_end(Zentific_Ctx *ctx, Ecore_Thread *thread __UNUSED__){
	db->releaseConsoleLock(db, ctx, ctx->data, ctx->retval, "vnc");
	CTX_BOOL_JOB(console_disconnect_vnc);
}

static void
console_disconnect_vnc(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	const char *uuid, *node, *session;

	uuid = ctx->data;
	session = ctx->retval;
	node = ctx->data2;
	switch (ctx->count){
		case 0:
			_hasConsoleLock(ctx, uuid, "vnc");
			CTX_BOOL_JOB(console_disconnect_text);
			break;
		case 1:
			if (ctx->error) {
				ctx->error = 0;
				goto out;
			}
			if (!ctx->retval){
				db->getConsoleLockSession(db, ctx, uuid, "text");
				CTX_VALUE_JOB(console_disconnect_text);
				break;
			}
		case 2:
			if (!ctx->retval){
				ctx->retval = CTX_PARAM_GET;
				if (ctx->error) break;
			}
			_getConsoleNodeHost(ctx, uuid, "vnc");
			CTX_VALUE_JOB(console_disconnect_vnc);
			break;
		case 3:
			ctx->data2 = CTX_PARAM_GET;
			if (ctx->error) {
				eina_stringshare_del(ctx->data2);
				break;
			}
			_getGraphicalConsoleTunnelPort(ctx, uuid);
			CTX_JOB_SEND(console_disconnect_vnc_thread);
			break;
		case 4:
			if (ctx->error) {
				eina_stringshare_del(ctx->data2);
				break;
			}

			ecore_thread_run((Ecore_Thread_Cb)console_disconnect_vnc_thread,
				(Ecore_Thread_Cb)console_disconnect_vnc_thread_end,
				(Ecore_Thread_Cb)console_disconnect_vnc_thread_end, ctx);
			break;
		case 5:
			goto out;
		default:
			break;

	}
	if (!ctx->error) return;
out:
	eina_stringshare_del(ctx->retval);
	eina_stringshare_del(ctx->data);
	pctx->error = ctx->error;
	free(ctx);
	if (--pctx->count == 2){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

Eina_Bool _disconnectConsole(Zentific_Ctx *ctx, const char *uuid, const char *type){
	/* NOTE there is a possibility that the
		db could contain bad data here;
		however lacking information about the lock
		we cannot with precision kill the related
		daemons. only fallback available is a
		walk through /var/run/zentific to knock
		off all items observed for the given uuid.

		-This fallback has merit. FIXME
	*/
	Zentific_Ctx *zctx;
	switch (ctx->count){
		case 0:
			if (zshare_strcmp(type, "text") && zshare_strcmp(type, "vnc")){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}

			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->parent = ctx;
			zctx->data = (void*)eina_stringshare_add(uuid);
			if (type[0] == 't')
				console_disconnect_text(zctx);
			else
				console_disconnect_vnc(zctx);
			ctx->count = 3;
			break;
		case 2:
			if (ctx->error) break;
			AZY_RETVAL(EINA_TRUE);
		default:
			break;
	}
	CTX_RETURN;
	//FIXME if multiple frontend hosts are to be
	//	supported, need to do a frontend host
	//	lookup here. modifiy getConsoleLock to
	//	return host/sessid of lock?
	//	in this case, need to ssh to other frontend
	//	to issue the kill and pidfile removal
	//
	//	OR
	//
	//	make a Session. or VM. destroyConsole so that
	//	we can make an xmlrpc call FROM this host TO
	//	remote frontend
	//
	//	OR some combination thereof.

	//FIXME hook here to bring down firewall rules associated with this console session
	//	what if session ip has changed since rules were created? we
	//	lose that data... perhaps we should store the client ip
	//	at time of console creation
}

//returns sessionid
Eina_Bool _getConsoleLockSession(Zentific_Ctx *ctx, const char *uuid, const char *type){
	db->getConsoleLockSession(db, ctx, uuid, type);
	CTX_RETURN;
}

Eina_Bool _getConsoleClientHost(Zentific_Ctx *ctx, const char *uuid, const char *type){
	db->getConsoleClientHost(db, ctx, uuid, type);
	CTX_RETURN;
}

Eina_Bool _getConsoleNodeHost(Zentific_Ctx *ctx, const char *uuid, const char *type){
	db->getConsoleNodeHost(db, ctx, uuid, type);
	CTX_RETURN;
}

Eina_Bool _getConsoleLockUser(Zentific_Ctx *ctx, const char *uuid, const char *type){
	const char *session_id;
	switch (ctx->count){
		case 0:
			db->getConsoleLockSession(db, ctx, uuid, type);
			CTX_VALUE_RETURN;
			break;
		case 1:
			session_id = CTX_PARAM_GET;
			if (ctx->error){
				eina_stringshare_del(session_id);
				break;
			}
			db->getUserNameBySession(db, ctx, session_id);
			eina_stringshare_del(session_id);
		default:
			break;
	}
	CTX_RETURN;
}


Eina_Bool _hasConsoleLock(Zentific_Ctx *ctx, const char *uuid, const char *type){
	db->hasConsoleLock(db, ctx, uuid, type);
	CTX_RETURN;
}

Eina_Bool _acquireConsoleLock(Zentific_Ctx *ctx, const char *uuid, SESSION *session, const char *type){
	const char *node;

	switch (ctx->count){
		case 0:
			_getVMparentAddress(ctx, uuid);
			CTX_VALUE_RETURN;
			break;
		case 1:
			node = CTX_PARAM_GET;
			if (ctx->error){
				eina_stringshare_del(node);
				break;
			}
			db->acquireConsoleLock(db, ctx, uuid, session, type, node);
			eina_stringshare_del(node);
		default:
			break;
	}
	CTX_RETURN;
}

Eina_Bool _releaseConsoleLock(Zentific_Ctx *ctx, const char *uuid, const char *sessionid, const char *type){
	db->releaseConsoleLock(db, ctx, uuid, sessionid, type);
	CTX_RETURN;
}



//FIXME getting vif/disk/vm stats over time and across a potential *set* of physical hosts will require tracking of start/stop on hostA,B,C. ie. getNetIfStats will sum up bytes tx/rx when on hostA last week and hostB from migration on sunday until today at XX:YY
//therefore, nodechildren will need non-unique uuid, and extra timestamp fields for start/stop


Eina_Bool _getConsoleLockVmsBySession(Zentific_Ctx *ctx, const char *session_id){
	db->getConsoleLockVmsBySession(db, ctx, session_id);
	CTX_RETURN;
}

//FIXME not exposed in api. should it be?
Eina_Bool _getConsoleType(Zentific_Ctx *ctx, const char *uuid, const char *session_id){
	db->getConsoleType(db, ctx, uuid, session_id);
	CTX_RETURN;
}

Eina_Bool _getSupportedConsoles(Zentific_Ctx *ctx, const char *uuid){
	intptr_t *port;
	Eina_List *consoles = NULL;
	const char *type;

	switch (ctx->count){
		case 0:
			_getVMattribute(ctx, uuid, "vncport");
			CTX_INT_RETURN;
			break;
		case 1:
			if (ctx->error) break;
			port = CTX_PARAM_GET;
			if (port && (*port > 0))
				consoles = eina_list_append(consoles, eina_stringshare_add("graphical"));
			//TODO FIXME
			//	for xen pv, assume yes
			//	for xen hvm, check whether serial='somepty' is set
			//		may require poller changes
			consoles = eina_list_append(consoles, eina_stringshare_add("text"));
			AZY_RETVAL(consoles);
			EINA_LIST_FREE(consoles, type)
				eina_stringshare_del(type);
		default:
			break;
	}
	CTX_RETURN;
}

//FIXME not exposed in api. should it be?
Eina_Bool _disconnectConsolesBySession(Zentific_Ctx *ctx, const char *session_id){
	//FIXME do we care much about return here?
	Eina_List *uuids;
	const char *uuid;
	Zentific_Ctx *zctx;

	switch (ctx->count){
		case 0:
			_getConsoleLockVmsBySession(ctx, session_id);
			CTX_VALUE_RETURN;
			break;
		case 1:
			uuids = CTX_PARAM_GET;
			ctx->count = 2 + (2 * eina_list_count(uuids));
			EINA_LIST_FREE(uuids, uuid){
				zctx = calloc(1, sizeof(Zentific_Ctx));
				zctx->parent = ctx;
				zctx->data = (void*)eina_stringshare_ref(uuid);
				zctx->retval = (void*)eina_stringshare_ref(session_id);
				console_disconnect_text(zctx);
				zctx = calloc(1, sizeof(Zentific_Ctx));
				zctx->parent = ctx;
				zctx->data = (void*)uuid;
				zctx->retval = (void*)eina_stringshare_ref(session_id);
				console_disconnect_vnc(zctx);
			}
		case 2:
			if (ctx->error) break;
			AZY_RETVAL(EINA_TRUE);
			RESUME;
		default:
			break;
	}

	CTX_RETURN;
}
