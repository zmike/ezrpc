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

#include "zconf.h"
#include "zentific-defines.h"
#include "db_sources.h"

/* SIGHUP handler */
void config_reparse(zentific_config **zconf) {
	zentific_config *new;
	DBG( "Handling a SIGHUP: reparsing configuration file");

	/* avoid losing current config */
	new = config_new();
	if (!new) return;
	if (!zshare_config_parse((*zconf)->CONF_FILE, (Zshare_Config_Parse_Cb)config_parse, new)){
		config_free(new);
		return;
	}
	config_defaults_fill(new);
	config_free(*zconf);
	*zconf = new;
}

int
config_parse(int line, const char *name, const char *value, zentific_config *zconf)
{
	int intval;
//booleans
#define SET_BOOLVAL(X) do { \
	if (!zshare_strcmp(name, #X)) { \
		if (!zshare_strcasecmp(value, "yes") || !zshare_strcmp(value, "1")) \
			zconf->X = EINA_TRUE; \
		else \
			zconf->X = EINA_FALSE; \
		return 1; \
	} \
} while (0)

		SET_BOOLVAL(SELF_HOST);
		SET_BOOLVAL(DAEMONIZE);
		SET_BOOLVAL(USE_SSL);
		SET_BOOLVAL(XML_DEBUG);
		SET_BOOLVAL(GDB_BT);

//numeric values
#define SET_INTVAL(X) do { \
	if ((!zshare_strcmp(name, #X)) && (sscanf(value, "%d", &intval) == 1)) {\
		zconf->X = intval; \
		return 1; \
	} \
} while (0)

		SET_INTVAL(PORT);
		SET_INTVAL(SESSION_TIMEOUT);
		SET_INTVAL(RETRY_TIMEOUT);
		SET_INTVAL(MAX_LOGIN_FAILURES);

//path data, strings, etc
#define SET_CFGVAL(X) do {\
	if (!zshare_strcmp(name, #X)) { \
		eina_stringshare_replace(&zconf->X, value); \
		return 1; \
	} \
} while (0)

		SET_CFGVAL(UTILS_PATH);
		SET_CFGVAL(VM_CONF_PATH);
		SET_CFGVAL(VAR_PATH);
		SET_CFGVAL(SSH_PATH);
		SET_CFGVAL(MODULES_PATH);
		SET_CFGVAL(DB_MODULES_PATH);
		SET_CFGVAL(DB_MODULE);
		SET_CFGVAL(LISTEN);
		SET_CFGVAL(KEY_PATH);
		SET_CFGVAL(SSL_PATH);
		SET_CFGVAL(WEB_ROOT);
		SET_CFGVAL(PID_FILE);
		SET_CFGVAL(LOG_FILE);
		SET_CFGVAL(LOG_PREFIX);

		if (!zshare_strcmp(name, "MY_UUID")) {
			if (!zshare_validate_uuid(value)){
			       	ERR("ERROR: invalid uuid on line [%d].", line);
			       	exit(1);
			}

			eina_stringshare_replace(&zconf->MY_UUID, value);
			return 1;
		}
		if (!zshare_strcmp(name, "LOG_LEVEL")) {

			if (!zshare_strcmp(value, "DEVEL") || (value[0] == '4'))
				zconf->LOG_LEVEL = ZSHARE_LOG_VERBOSITY_DEVEL;
			else if (!zshare_strcmp(value, "DEBUG") || (value[0] == '5'))
				zconf->LOG_LEVEL = ZSHARE_LOG_VERBOSITY_DEBUG;
			else if (!zshare_strcmp(value, "QUIETTEST") || (value[0] == '2'))
				zconf->LOG_LEVEL = ZSHARE_LOG_VERBOSITY_QUIETTEST;
			else if (!zshare_strcmp(value, "TEST") || (value[0] == '3'))
				zconf->LOG_LEVEL = ZSHARE_LOG_VERBOSITY_TEST;
			else if (!zshare_strcmp(value, "NONE") || (value[0] == '0'))
				zconf->LOG_LEVEL = ZSHARE_LOG_VERBOSITY_NONE;
			else
				zconf->LOG_LEVEL = ZSHARE_LOG_VERBOSITY_PRODUCTION;

			return 1;
		}
		if (!zshare_strcmp(name, "LOG_TYPE")) {
			zconf->LOG_TYPE = 0;
			if (strstr(value, "file")) zconf->LOG_TYPE |= ZSHARE_LOG_TYPE_FILE;
			if (strstr(value, "lsyslog")) zconf->LOG_TYPE |= ZSHARE_LOG_TYPE_LSYSLOG;
			if (strstr(value, "local")) zconf->LOG_TYPE |= ZSHARE_LOG_TYPE_LOCAL;
			if (!zconf->LOG_TYPE) {
				fprintf(stderr, "ERROR: invalid log type on line [%d].\n", line);
				exit(1);
			}
			return 1;
		}

	return 0;
}

void
config_print(zentific_config *zconf)
{
	if (!zconf) return;
#define INT_PRINTCFG(X) \
	DBG(S(X)" = %d",zconf->X);
#define STR_PRINTCFG(X) \
	DBG(S(X)" = %s",zconf->X);

	if (!zconf->DAEMONIZE  && (zconf->LOG_LEVEL > 0) && (zconf->LOG_LEVEL < 4)){
		INF("\n****** Debug output enabled.");
		DBG("zconf settings detected are as follows:");
		INT_PRINTCFG(PORT)
		INT_PRINTCFG(SELF_HOST)
		INT_PRINTCFG(DAEMONIZE)
		INT_PRINTCFG(IS_VM)
		INT_PRINTCFG(USE_SSL)
		INT_PRINTCFG(XML_DEBUG)
		INT_PRINTCFG(SESSION_TIMEOUT)
		STR_PRINTCFG(UTILS_PATH)
		STR_PRINTCFG(VAR_PATH)
		STR_PRINTCFG(SSH_PATH)
		STR_PRINTCFG(MODULES_PATH)
		STR_PRINTCFG(DB_MODULES_PATH)
		STR_PRINTCFG(DB_MODULE)
		STR_PRINTCFG(LISTEN)
		STR_PRINTCFG(MY_UUID)
		STR_PRINTCFG(SSL_PATH)
		STR_PRINTCFG(SSL_CERT)
		STR_PRINTCFG(KEY_PATH)
		STR_PRINTCFG(WEB_ROOT)
		STR_PRINTCFG(PID_FILE)
		STR_PRINTCFG(LOG_FILE)
		STR_PRINTCFG(LOG_PREFIX)
		STR_PRINTCFG(VM_CONF_PATH)
	}
}

void config_defaults_fill(zentific_config *zconf){
#define FILL(X, Y) if (!X) X = Y
	//set defaults
	FILL(zconf->PID_FILE, eina_stringshare_add(DEFAULT_VAR_PATH"/run/zentific/"DEFAULT_PID_FILE));
	FILL(zconf->WEB_ROOT, eina_stringshare_add(DEFAULT_WEB_ROOT));
	FILL(zconf->LISTEN, eina_stringshare_add(DEFAULT_LISTEN));
	FILL(zconf->CONF_FILE, eina_stringshare_add(DEFAULT_CONF_FILE));
	FILL(zconf->LOG_FILE, eina_stringshare_add(DEFAULT_LOG_FILE));
	FILL(zconf->LOG_PREFIX, eina_stringshare_add(DEFAULT_LOG_FILE));
	FILL(zconf->UTILS_PATH, eina_stringshare_add(DEFAULT_UTILS_PATH));
	FILL(zconf->VAR_PATH, eina_stringshare_add(DEFAULT_VAR_PATH));
	FILL(zconf->SSH_PATH, eina_stringshare_add(DEFAULT_SSH_PATH));
	FILL(zconf->DB_MODULES_PATH, eina_stringshare_add(DEFAULT_DB_MODULES_PATH));
	FILL(zconf->DB_MODULE, eina_stringshare_add(DEFAULT_DB_MODULE));
	FILL(zconf->MODULES_PATH, eina_stringshare_add(DEFAULT_MODULES_PATH));
	FILL(zconf->SSL_PATH, eina_stringshare_add(DEFAULT_SSL_PATH));
	FILL(zconf->SSL_CERT, eina_stringshare_add(DEFAULT_SSL_CERT));
	FILL(zconf->KEY_PATH, eina_stringshare_add(DEFAULT_KEY_PATH));
	FILL(zconf->VM_CONF_PATH, eina_stringshare_add(DEFAULT_VM_CONF_PATH));
	FILL(zconf->CREDSTORE_PUBKEY_PATH, eina_stringshare_add(DEFAULT_CREDSTORE_PUBKEY_PATH));
}
zentific_config * config_new(void){

	zentific_config *zconf = calloc(1, sizeof(zentific_config));

	zconf->LOG_LEVEL 	= ZSHARE_LOG_VERBOSITY_PRODUCTION;
	zconf->PORT 		= DEFAULT_PORT;
	zconf->SESSION_TIMEOUT	= DEFAULT_SESSION_TIMEOUT;
	zconf->RETRY_TIMEOUT	= DEFAULT_RETRY_TIMEOUT;
	zconf->MAX_LOGIN_FAILURES = DEFAULT_MAX_LOGIN_FAILURES;
	zconf->DAEMONIZE 	= EINA_TRUE;
	zconf->USE_SSL	 	= EINA_FALSE;
	zconf->SELF_HOST	= EINA_FALSE;
	zconf->IS_VM 		= EINA_FALSE;
	zconf->XML_DEBUG	= EINA_FALSE;

	//FIXME
	zconf->LOG_TYPE	= DEFAULT_LOG_TYPE;


	return zconf;
}

void config_free(zentific_config *zconf){

	if (!zconf) return;

	eina_stringshare_del(zconf->UTILS_PATH);
	eina_stringshare_del(zconf->VAR_PATH);
	eina_stringshare_del(zconf->SSH_PATH);
	eina_stringshare_del(zconf->MODULES_PATH);
	eina_stringshare_del(zconf->DB_MODULES_PATH);
	eina_stringshare_del(zconf->DB_MODULE);
	eina_stringshare_del(zconf->LISTEN);
	eina_stringshare_del(zconf->MY_UUID);
	eina_stringshare_del(zconf->SSL_PATH);
	eina_stringshare_del(zconf->SSL_CERT);
	eina_stringshare_del(zconf->KEY_PATH);
	eina_stringshare_del(zconf->WEB_ROOT);
	eina_stringshare_del(zconf->PID_FILE);
	eina_stringshare_del(zconf->LOG_FILE);
	eina_stringshare_del(zconf->CONF_FILE);
	eina_stringshare_del(zconf->LOG_PREFIX);
	eina_stringshare_del(zconf->VM_CONF_PATH);
	free(zconf);
}
