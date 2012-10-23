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
#include "zentific.h"

#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <dlfcn.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Getopt.h>
#include <Azy.h>

#include "zconf.h"
#include "functions-zentific.h"
#include "zentific-defines.h"
#include "ztypes.h"
#include "db_sources.h"
#include "loop.h"

#define AZY_DEF "_module_def"

zentific_config *config;
DB *db;
int zrpc_log_dom = -1;
const char *true = NULL;
Eina_Bool reloading_modules = EINA_FALSE;
Eina_List *reload_modules_list = NULL;
static Eina_Array *servlet_modules, *tmp_servlet_modules;
static Azy_Server *server;
static Eina_Bool reload_modules;

static void cleanup_exit(int signum);
static Eina_Bool initialize(int argc, char **argv);
static void signals_handler(int sig);
static void signals_setup(void);
static Eina_Array *modules_load(void);
static void setup_sigsegv(void);
static void modules_reload_handler(DB *data, Eina_Bool success);
Eina_Bool modules_reload_begin(void);

/* sigsegv.c */
void signal_segv(int signum, siginfo_t* info, void*ptr);

static const Ecore_Getopt server_opts =
{
   "zrpc",
   "zrpc [options]",
   PACKAGE_VERSION,
   ZENTIFIC_COPYRIGHT"\n\n"ZENTIFIC_DISCLAIMER"\n\n"ZENTIFIC_CONTACT,
   "LGPL",
   "Zentific RPC daemon",
   EINA_TRUE, /* strict checking; fail on errors */
   {
      ECORE_GETOPT_STORE_TRUE('S', "selfhost", "Enable self-hosting mode"),
      ECORE_GETOPT_STORE_TRUE('s', "ssl", "Enable SSL"),
      ECORE_GETOPT_STORE_TRUE('X', "xmldebug", "Display rpc traces; implies -n"),
      ECORE_GETOPT_STORE_FALSE('n', "nodaemon", "Do not daemonize"),
      ECORE_GETOPT_STORE_STR('P', "pidfile", "Record PID in specified filename"),
      ECORE_GETOPT_STORE_STR('W', "webroot", "Web root used in self-hosting mode"),
      ECORE_GETOPT_STORE_STR('c', "cfgfile", "File to use as config file"),
      ECORE_GETOPT_STORE_STR('L', "logfile", "Destination path for log entries"),
      ECORE_GETOPT_STORE_STR('U', "uuid", "UUID for the vm in which zrpc runs, if any"),
      ECORE_GETOPT_STORE_INT('p', "port", "Bind to port X. Default: "S(DEFAULT_PORT)),
      ECORE_GETOPT_STORE_INT('l', "loglevel", "Record log entries of level or higher"),
      ECORE_GETOPT_VERSION('V', "version"),
      ECORE_GETOPT_COPYRIGHT('R', "copyright"),
      ECORE_GETOPT_LICENSE('C', "license"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
   }
};

static Eina_Bool server_start(Eina_Array *servlets){
	Eina_Iterator *i;
	Azy_Server_Module_Def *def;

	EINA_SAFETY_ON_NULL_RETURN_VAL(servlets, EINA_FALSE);

	server = azy_server_new(!!config->USE_SSL);

	EINA_SAFETY_ON_NULL_RETURN_VAL(server, EINA_FALSE);

	azy_server_addr_set(server, config->LISTEN);
	azy_server_port_set(server, config->PORT);
	if (config->USE_SSL)
		azy_server_cert_add(server, config->SSL_CERT);

	i = eina_array_iterator_new(servlets);
	EINA_ITERATOR_FOREACH(i, def)
		azy_server_module_add(server, def);
	eina_iterator_free(i);

	if (!azy_server_run(server))
		return EINA_FALSE;

	return EINA_TRUE;
}

static void server_client_disc(void *data __UNUSED__, int type __UNUSED__, void *ev __UNUSED__)
{
	if (reload_modules) modules_reload_begin();
}

int main(int argc, char **argv) {

	//TODO consider drop to lower privileges.
	//	keep in mind that this impacts PAM integration during runtime
	//	for simplicity's sake, this may be either or, not both.
	//		otherwise, could make a separate daemon running with
	//		higher privileges that handles PAM interation, etc..
	Eina_Array *servlets;

	config = config_new();
	initialize(argc, argv);
	servlets = modules_load();
	servlet_modules = tmp_servlet_modules;
	if ( config->XML_DEBUG ) {
		INF("RPC debug info enabled.");
		azy_rpc_log_enable();
	}
	ecore_event_handler_add(AZY_SERVER_CLIENT_DEL, (Ecore_Event_Handler_Cb)server_client_disc, NULL);
	signals_setup();
	true = eina_stringshare_add("true");
	server_start(servlets);

	cleanup_exit(0);

	return 0;
}


static Eina_Bool ismodule(Eina_Module *m, Eina_Array *servlets){
	const char *filename;
	char modstr[5010];
	Azy_Server_Module_Def_Cb cb;

	filename = eina_module_file_get(m);
	filename = strrchr(filename, '/') + 1;
	if (strcmp(filename + (strlen(filename) - 3), ".so") || strcmp(filename, "zrpc_base.so") || strcmp(filename, "libztype.so"))
		return EINA_FALSE;

	if (!eina_module_load(m)){
		ERR("Could not verify module named '%s'. Error: '%s'", filename, dlerror());
		return EINA_FALSE;
	}

	snprintf(modstr, sizeof(modstr), "%.*s"AZY_DEF, strlen(filename) - 3, filename);
	cb = eina_module_symbol_get(m, modstr);
	if (cb){
		eina_array_push(servlets, cb());
		return EINA_TRUE;
	}

	eina_module_unload(m);

	return EINA_FALSE;
}

static Eina_Array *modules_load(void)
{
	Eina_Module *baselib;
	Eina_Array *servlets;
	Eina_Array *(*base)(void);

	baselib = eina_module_new(ZRPC_SERVLET_PATH"/zrpc_base.so");
	if (!eina_module_load(baselib)){
		ERR("Could not load base servlet: %s", eina_error_msg_get(eina_error_get()));
		cleanup_exit(1);
	}
	base = eina_module_symbol_get(baselib, "zrpc_base_def");
	if (!base){
		ERR("Could not load base servlet!");
		cleanup_exit(1);
	}
	INF("Loading base servlets");
	servlets = base();
	if (!servlets){
		ERR("Could not load base servlet!");
		cleanup_exit(1);
	}
	tmp_servlet_modules = eina_module_list_get(NULL, ZRPC_SERVLET_PATH, EINA_FALSE, (Eina_Module_Cb)ismodule, servlets);
	if (!tmp_servlet_modules){
		INF("No external servlets found");
		tmp_servlet_modules = eina_array_new(1);
		eina_array_push(tmp_servlet_modules, baselib);
	}
	else
		eina_array_push(tmp_servlet_modules, baselib);
	INF("%u servlets discovered!", eina_array_count_get(servlets));
	return servlets;
}


static Eina_Bool initialize(int argc, char **argv) {
	char *cfg = NULL;
	Eina_Bool exit_option = EINA_FALSE;
	char *PID_FILE = NULL, *WEB_ROOT = NULL, *LOG_FILE = NULL, *MY_UUID = NULL;

	Ecore_Getopt_Value server_values[] = {
		ECORE_GETOPT_VALUE_BOOL(config->SELF_HOST),
		ECORE_GETOPT_VALUE_BOOL(config->USE_SSL),
		ECORE_GETOPT_VALUE_BOOL(config->XML_DEBUG),
		ECORE_GETOPT_VALUE_BOOL(config->DAEMONIZE),
		ECORE_GETOPT_VALUE_STR(PID_FILE),
		ECORE_GETOPT_VALUE_STR(WEB_ROOT),
		ECORE_GETOPT_VALUE_STR(cfg),
		ECORE_GETOPT_VALUE_STR(LOG_FILE),
		ECORE_GETOPT_VALUE_STR(MY_UUID),
		ECORE_GETOPT_VALUE_INT(config->PORT),
		ECORE_GETOPT_VALUE_INT(config->LOG_LEVEL),
		ECORE_GETOPT_VALUE_BOOL(exit_option),
		ECORE_GETOPT_VALUE_BOOL(exit_option),
		ECORE_GETOPT_VALUE_BOOL(exit_option),
		ECORE_GETOPT_VALUE_BOOL(exit_option)
	};
	eina_init();
	ecore_init();
	azy_init();
	azy_err_init();

	config_defaults_fill(config);

	zrpc_log_dom = eina_log_domain_register("zrpc", EINA_COLOR_GREEN);
	if (zrpc_log_dom < 0){
		EINA_LOG_ERR("Could not create log domain!");
		exit(1);
	}

	//FIXME not all config params are command-line-assignable
	// Validate configuration file and make sure that we CAN run here
	if (!cfg) cfg = (char*)config->CONF_FILE;
	if (!zshare_config_parse(cfg, (Zshare_Config_Parse_Cb)config_parse, config))
		exit(1);
	if (ecore_getopt_parse(&server_opts, server_values, argc, argv) < 0){
		ERR("Invalid options specified!");
		ecore_getopt_help(stderr, &server_opts);
		exit(1);
	}
	if (exit_option) exit(1);

	if (PID_FILE) eina_stringshare_replace(&config->PID_FILE, PID_FILE);
	if (WEB_ROOT) eina_stringshare_replace(&config->WEB_ROOT, WEB_ROOT);
	if (LOG_FILE) eina_stringshare_replace(&config->LOG_FILE, LOG_FILE);
	if (MY_UUID) eina_stringshare_replace(&config->MY_UUID, MY_UUID);
	if (config->XML_DEBUG)
		config->DAEMONIZE = EINA_FALSE;
	config_print(config);

	//options may have changed, update
	if (config->LOG_LEVEL == ZSHARE_LOG_VERBOSITY_NONE)
		eina_log_domain_level_set("zrpc", EINA_LOG_LEVEL_UNKNOWN);
	else
		eina_log_domain_level_set("zrpc", config->LOG_LEVEL);

	/* early pid check to avoid unnecessary initialization of db stuff if we're going to bail anyway */
	if (zshare_pid_check(config->PID_FILE)){
		ERR("Error creating pidfile, check permissions and file status!");
		exit(1);
	}

	// Final loading step is to make sure all required values are set
	// FIXME:function to check options.
	INF("Initializing..");

	if (!db_init(NULL, (Ecore_Event_Handler_Cb)error_handler, (Ecore_Event_Handler_Cb)result_handler, config))
		cleanup_exit(1);
	ecore_main_loop_begin();

	if (!db){
		ERR("Database object failure, cannot continue.");
		cleanup_exit(1);
	}

	// After we're sure we can run, detach from the TTY
	if (config->DAEMONIZE){
		INF("Attempting to daemonize.");
		if (!zshare_daemon_detach(1)){
			ERR("ERROR: Could not daemonize.");
			cleanup_exit(1);
		}
	} else {
		INF("Running in non-daemonized mode.");
	}

	//We're may not be heading into the bg, but create the PID file regardless
	//      need to do pidfile AFTER daemonizing to get right pid, but before closing descriptors (which would hide errors)
	switch (zshare_pid_create(config->PID_FILE))
	{
		case -1:
			ERR("ERROR: Could not write pid file. Check permissions and system status.");
			cleanup_exit(1);
		case 0:
			ERR("ERROR: Could not write pid file. Daemon already running or previously exited uncleanly.");
			exit(1);
		default:
			break;
	}


	//Verify running as root; if not, exit
	if (geteuid() != 0){
#ifdef DETECT_VM
		INF("INFO: Root privileges are required to accurately detect whether the server is running inside a VM.");
#endif
		//TODO move any privileged auth actions into the scheduler??
		INF("INFO: Root privileges are required for PAM authentication for any authentication module requiring access to protected resources (e.g. /etc/shadow).");
#ifdef DETECT_VM
	} else {
		// Detect whether we're running in a VM
		INF("Detecting virtualization platform.");

		//requires root privs
		config->IS_VM = running_as_vm();

		if ( config->IS_VM ) {
			//requires root privs
			config->MY_UUID = (char*)get_my_uuid();

			INF("Virtualization detected. Running in a vm, uuid=%s\n",
				config->MY_UUID);

			//TODO
			//save the uuid of this xmlrpc-server's vm into the db

			//FIXME and/or NOTE
			//	if using multiple frontend interface servers,
			//	it is not sufficient for each server process to be aware
			//	of its own VM uuid; it must be aware of the others also
			//	thus, insert this uuid into the db or set a flag.
		}
	}

#else
	}
#endif


	if (config->DAEMONIZE && (!zshare_daemon_closefds(0))){
		ERR("ERROR: Could not detach daemonize; daemon already running or previously exited uncleanly. Check pidfile.");
		cleanup_exit(-1);
	}

	signals_setup();

	INF("Initialization complete.");

	return EINA_FALSE;
}

static void
modules_reload_handler(DB *data, Eina_Bool success){
	Azy_Server_Module *module;
	if (success){
		db_uninit(db);
		db = data;
		INF("DB module successfully reloaded.");
		INF("Modules successfully reloaded!");
	} else
		ERR("DB module reloading failed!");
	EINA_LIST_FREE(reload_modules_list, module){
		azy_server_module_events_rewind(module);
		azy_server_module_events_resume(module, EINA_TRUE);
	}
	reload_modules = EINA_FALSE;
	reloading_modules = EINA_FALSE;
}

Eina_Bool modules_reload_begin(void)
{
	Eina_Array *servlets;
	Eina_Iterator *i;
	Eina_Module *m;
	Eina_List *l, *ll;
	Azy_Server_Module_Def *s;

	reload_modules = EINA_TRUE;
	if (azy_server_clients_count(server)){
		INF("Deferring module reload until clients are finished");
		return EINA_TRUE;
	}

	INF("Reloading modules");
	servlets = modules_load();
	if (!servlets){
		ERR("Module reloading failed! Continuing with already loaded modules.");
		reload_modules = EINA_FALSE;
		return EINA_FALSE;
	}
	i = eina_array_iterator_new(servlet_modules);
	EINA_ITERATOR_FOREACH(i, m)
		eina_module_free(m);
	eina_iterator_free(i);
	INF("Modules successfully unloaded.");

	EINA_LIST_FOREACH_SAFE(azy_server_module_defs_get(server), l, ll, s)
		azy_server_module_del(server, s);
	i = eina_array_iterator_new(servlets);
	EINA_ITERATOR_FOREACH(i, s)
		azy_server_module_add(server, s);
	eina_iterator_free(i);
	servlet_modules = tmp_servlet_modules;
	INF("%i modules loaded", eina_array_count_get(servlet_modules));
	signals_setup();
	//tmp_db =
	if (!db_init((Zrpcdb_Connect_Cb)modules_reload_handler, (Ecore_Event_Handler_Cb)error_handler, (Ecore_Event_Handler_Cb)result_handler, config)){
	//if (!tmp_db)
		ERR("DB module could not be reloaded! Continuing with already loaded db module.");
		reload_modules = EINA_FALSE;
		return EINA_FALSE;
	}
	reloading_modules = EINA_TRUE;
	return EINA_TRUE;
}

static void signals_handler(int sig)
{
	static time_t last;
	time_t current;

	current = time(NULL);

	DBG( "Signal '%s'", strsignal(sig));

	switch (sig){
		case SIGQUIT:
		case SIGTERM:
		case SIGINT:
			cleanup_exit(sig);
			return;
		case SIGHUP:
			if (reload_modules)
				break;
			if (current - last < 5)
				modules_reload_begin();
			else config_reparse(&config);
			setup_sigsegv();
			break;
		default:
			WRN( "Unhandled signal: '%s'", strsignal(sig));
	}
	if (reload_modules)
		INF("Signal ignored until module reload completed");


	last = current;
}

static void gdb_segv(int signum, siginfo_t *info, void *ptr)
{
	char gdb[1024];
	size_t size;

	size = snprintf(gdb, sizeof(gdb), "gdb --pid=%i -ex 'thread apply all bt' -ex detach -ex quit", getpid());
	if (size >= sizeof(gdb))
		signal_segv(signum, info, ptr);
	else
		system(gdb);
	exit(1);
}

static void setup_sigsegv(void) {
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	if (!config->GDB_BT)
		action.sa_sigaction = signal_segv;
	else
		action.sa_sigaction = gdb_segv;
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGSEGV, &action, NULL) < 0)
		perror("sigaction");
}

static void signals_setup(void)
{
	sigset_t sigs = {{0}};
	struct sigaction s;

	sigfillset(&sigs);
	sigdelset(&sigs, SIGSEGV);
	sigdelset(&sigs, SIGTERM);
	sigdelset(&sigs, SIGINT);
	sigdelset(&sigs, SIGHUP);

	sigprocmask(SIG_BLOCK, &sigs, NULL);

	memset(&s, 0, sizeof(struct sigaction));
	s.sa_handler = signals_handler;
	s.sa_flags = SA_RESTART;
	sigemptyset(&s.sa_mask);

	sigaction(SIGTERM, &s, NULL);
	sigaction(SIGINT, &s, NULL);
	sigaction(SIGQUIT, &s, NULL);
	sigaction(SIGHUP, &s, NULL);
	setup_sigsegv();
}

// Called on a SIGINT so we can clean up before exitting
void cleanup_exit(int signum) {

	INF("Cleaning up before exiting.");

	INF("Stopping server.");

	if (server)
		azy_server_stop(server);

#ifndef ZDEV_DEBUG
/* this obfuscates valgrind logs, so don't do it in devel mode */
#if 0
	if (servlet_modules){
		Eina_Iterator *i;
		Eina_Module *m;

		i = eina_array_iterator_new(servlet_modules);
		EINA_ITERATOR_FOREACH(i, m)
			eina_module_free(m);
	}
#endif
	db_uninit(db);
#endif

	//signum == -1 when pidfile exists and we're exiting due to
	//	already-running daemon or previous unclean shutdown pending investigation
	if (signum != -1){
		INF("Removing pidfile.");
		zshare_pid_delete();
	}

	if (signum == 1){
		ERR("Exiting due to errors.");
	} else {
		INF("Exiting cleanly.");
	}

	// Since we overrode the default SIGINT action, we have to force the exit on
	// our own now.
	exit(signum);
}

