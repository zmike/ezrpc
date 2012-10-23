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
//system includes
#include "zentific.h"

#include <unistd.h>
#include <sys/types.h>
#include "functions-user.h"
#include "utility.h"
#include "db_sources.h"

//sha1 hashing
#include "sha1.h"
#include "sha1wrap.h"
#include "zentific-defines.h"
#include "zconf.h"

#ifndef _PASSWORD_LEN
#define _PASSWORD_LEN 80
#endif

char * getpass_x(const char *prompt);
const char *true = NULL;
zentific_config *config = NULL;
DB *db = NULL;
int zrpc_log_dom = -1;

Eina_Bool
error_handler(DB *db __UNUSED__, int type __UNUSED__, Zentific_Ctx *ctx){
	ERR("Password NOT successfully committed to database");
	ERR("Error: \"%s\"", eina_error_msg_get(ctx->error));

	ecore_main_loop_quit();
	return EINA_TRUE;
}

Eina_Bool
result_handler(DB *db __UNUSED__, int type __UNUSED__, Zentific_Ctx *ctx __UNUSED__){
	INF("Password successfully committed to database");
	ecore_main_loop_quit();
	return EINA_TRUE;
}

Eina_Bool _disconnectConsolesBySession(){return EINA_TRUE;}

int main(void) {
	char *passwd;
	char *passone = NULL;
	Zentific_Ctx ctx;

	eina_init();
	ecore_init();
	memset(&ctx, 0, sizeof(Zentific_Ctx));
	zrpc_log_dom = eina_log_domain_register("zrpcdb_init", EINA_COLOR_LIGHTBLUE);
	if (zrpc_log_dom < 0){
		EINA_LOG_ERR("Could not create log domain!");
		exit(1);
	}

	if (geteuid() != 0){
		ERR("Please run zrpcdb_init with elevated privileges");
		return 1;
	}

	config = config_new();
	if (!config) {
		fprintf(stderr, "Allocation error.\n");
		return 1;
	}
	true = eina_stringshare_add("true");
	config_defaults_fill(config);
	if (!zshare_config_parse(config->CONF_FILE, (Zshare_Config_Parse_Cb)config_parse, config))
		exit(1);
	eina_log_domain_level_set("zrpcdb_init", EINA_LOG_LEVEL_INFO);
	if (!db_init(NULL, (Ecore_Event_Handler_Cb)error_handler, (Ecore_Event_Handler_Cb)result_handler, config))
		exit(1);
	ecore_main_loop_begin();

	if (!db){
		ERR("Database object failure, cannot continue.");
		exit(1);
	}

	INF("\n\nZentific initialization:\nPlease choose a password for the 'admin' user.\n");
	do {
		if (passone){
			ERR("\nPasswords did not match, please try again.");
			free(passone);
		}
		/* ptr to static char */
		passwd = getpass_x("Enter a password: ");
		while (strlen(passwd) < MIN_PASSWD_LEN) {
			ERR("Password must be at least %d characters. Please try again.\n", MIN_PASSWD_LEN);
			passwd = getpass_x("Enter a password: ");
		}
		passone = strdup(passwd);
		passwd = getpass_x("Enter it again: ");
		while (strlen(passwd) < MIN_PASSWD_LEN) {
			ERR("Password must be at least %d characters. Please try again.\n", MIN_PASSWD_LEN);
			passwd = getpass_x("Enter it again: ");
		}
	} while (strcmp(passone, passwd));
	free(passone);
	INF("\nPasswords match. Now saving to database..");
	_setPasswordByName(&ctx, "admin", passwd);
	if (ctx.error){
		ERR("%s", eina_error_msg_get(ctx.error));
		exit(1);
	}
	ecore_main_loop_begin();

	return 0;
}

