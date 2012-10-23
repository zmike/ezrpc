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
#include "zrpcdb.h"
#include "zentific-defines.h"
void db_config_free(DB_config *config) {
	eina_stringshare_del(config->backend);
	eina_stringshare_del(config->username);
	eina_stringshare_del(config->password);
	eina_stringshare_del(config->database);
	free(config);
}

static int
db_config_parse_cb(int line __UNUSED__, const char *name, const char *value, DB *db)
{
	if 	(!zshare_strcmp(name, "HOSTNAME")){
		if (strchr(value, ':'))
			db->config->backend = eina_stringshare_add(value);
		else
			db->config->backend = eina_stringshare_printf("%s:"ESQL_DEFAULT_PORT_MYSQL, value);
	}
	else if (!zshare_strcmp(name, "USERNAME"))
		db->config->username = eina_stringshare_add(value);
	else if (!zshare_strcmp(name, "PASSWORD"))
		db->config->password = eina_stringshare_add(value);
	else if (!zshare_strcmp(name, "DATABASE"))
		db->config->database = eina_stringshare_add(value);
	else if (!zshare_strcmp(name, "CONNECTIONS"))
		sscanf(value, "%d", &db->config->conns);
	else if (!zshare_strcmp(name, "LOG_LEVEL")) {

		if (!zshare_strcmp(value, "DEVEL") || (value[0] == '4'))
			db->config->loglevel = ZSHARE_LOG_VERBOSITY_DEVEL;
		else if (!zshare_strcmp(value, "DEBUG") || (value[0] == '5'))
			db->config->loglevel = ZSHARE_LOG_VERBOSITY_DEBUG;
		else if (!zshare_strcmp(value, "QUIETTEST") || (value[0] == '2'))
			db->config->loglevel = ZSHARE_LOG_VERBOSITY_QUIETTEST;
		else if (!zshare_strcmp(value, "TEST") || (value[0] == '3'))
			db->config->loglevel = ZSHARE_LOG_VERBOSITY_TEST;
		else if (!zshare_strcmp(value, "NONE") || (value[0] == '0'))
			db->config->loglevel = ZSHARE_LOG_VERBOSITY_NONE;
		else
			db->config->loglevel = ZSHARE_LOG_VERBOSITY_PRODUCTION;

		return 1;
	}
	else
		return 0;

	return 1;
}

Eina_Bool db_config_parse(DB *db, const char *db_modules_path) {
	char config_path[4096];
	Eina_Bool ret;

	db->config = calloc(1, sizeof(DB_config));
	if (!db->config) {
		ERR("Cannot allocate memory for new database config object.");
		return EINA_FALSE;
	}
	snprintf(config_path, sizeof(config_path), "%s/zrpcdb.conf", db_modules_path);

	ret = zshare_config_parse(config_path, (Zshare_Config_Parse_Cb)db_config_parse_cb, db);
	if (!ret){
		ERR("%s", eina_error_msg_get(AZY_RPC_ERROR_DB_CONFIG));
		db_config_free(db->config);
		db->config = NULL;
		return EINA_FALSE;
	}
	if ((!db->config->conns) || (db->config->conns < 3))
		db->config->conns = DEFAULT_CONNS;

	return ret;
}

