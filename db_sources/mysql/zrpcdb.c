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
#include "zrpcdb_common.h"
#include <Ecore.h>

/* NOTE: for future reference, mysql_free_result CAN take a null res gracefully */

/* TODO clean up any functions that do not return boolean, ghashtable, or gslist
		- do not do type conversions from string to whatever at this layer

*/

static Ecore_Event_Handler *zrpc_res, *zrpc_err;
static int init_count;
int ZRPCDB_EVENT_ERROR = 0;
int ZRPCDB_EVENT_RESULT = 0;
int zrpcdb_log_dom = -1;
Eina_Hash *type_cbs = NULL;
Eina_Hash *db_cbs = NULL;
Eina_Hash *db_cb_params = NULL;

void
fake_free(void *x __UNUSED__, void *y __UNUSED__){}

void
event_cb(Esql_Res *res, Zentific_Ctx *ctx){
	const char *query;
	Z_Ctx_Cb end_cb;
	Zrpcdb_Cb cb;
	Esql_Query_Id qid;
	long long int insert_id;
	DB *db;
	Eina_Bool state;

	qid = esql_res_query_id_get(res);
	state = ctx->state;
	ctx->state = EINA_FALSE;
	if (esql_res_error_get(res)){
		ERR("Query %lu: \"%s\" - %s", qid, esql_res_query_get(res), esql_res_error_get(res));
		eina_hash_del_by_key(db_cbs, &qid);
		ctx->error = AZY_ERR(DB_QUERY);
		ecore_event_add(ZRPCDB_EVENT_ERROR, ctx, fake_free, NULL);
		return;
	}
	query = esql_res_query_get(res);
	cb = eina_hash_find(type_cbs, &qid);
	db = esql_data_get(esql_res_esql_get(res));
	if (cb) { /* all SELECT calls have cbs */
		DBG("%i rows returned", esql_res_rows_count(res));
		if (state && (esql_res_rows_count(res) > 1)){
			ctx->error = AZY_ERR(DB_MULTI);
			eina_hash_del_by_key(db_cbs, &qid);
			ctx->state = state;
			ecore_event_add(ZRPCDB_EVENT_ERROR, ctx, fake_free, NULL);
			return;
		}
		end_cb = eina_hash_find(db_cbs, &qid); /* another db call queued */
		if (end_cb) {
			eina_hash_del_by_key(db_cbs, &qid);
			end_cb(res, ctx, cb(res));
			return;
		}
		if (ctx->cbs){
			end_cb = ctx->cbs->data;
			end_cb(ctx, cb(res));
		} else {
			ctx->state = state;
			ctx->retval = cb(res);
			ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
		}
		return;
	}
	end_cb = eina_hash_find(db_cbs, &qid); /* another db call queued */
	insert_id = esql_res_id(res);
	if (esql_res_rows_affected(res))
		INF("Query \"%s\" inserted with id %lli", esql_res_query_get(res), insert_id);
	else
		ERR("Query \"%s\" affected 0 rows", esql_res_query_get(res));
	if (end_cb) {
		eina_hash_del_by_key(db_cbs, &qid);
		if (state) end_cb(res, ctx, insert_id);
		else end_cb(res, ctx);
		return;
	}
	if (ctx->cbs) {
		end_cb = ctx->cbs->data;
		if (state) end_cb(ctx, insert_id);
		else end_cb(ctx);
	} else {
		ctx->state = state;
		ctx->retval = (intptr_t*)(int)insert_id;
		ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
	}
}

/* FIXME: won't work for reload */
static void
init_cb(Esql *e, Zrpcdb_Connect_Cb cb /* exit function */){
	DB *d;

	d = esql_data_get(e);
	if (esql_error_get(e))
		ERR("DB initialization failed! %s", esql_error_get(e));
	else
		INF("Completed initializing source 'zrpcdb' with %i connections", d->config->conns);

	cb(esql_data_get(e), !esql_error_get(e));
	/* if we get here, then we're all connected. */
}
/**
 * Prepare the library for use through initialization of any required connections and / or
 * variables
 *
 * @return boolean EINA_TRUE for success, EINA_FALSE on error.
 */
Eina_Bool init(DB *db, Zrpcdb_Connect_Cb cb, Ecore_Event_Handler_Cb error_cb, Ecore_Event_Handler_Cb result_cb, zentific_config *conf){
	Esql *e;

	if ((!db) || (!cb) || (!conf))
		return EINA_FALSE;

	eina_init();
	zrpcdb_log_dom = eina_log_domain_register("zrpcdb", EINA_COLOR_ORANGE);
	if (zrpcdb_log_dom < 0) {
		fprintf(stderr, "Could not init zrpcdb log domain\n");
		goto err;
	}
	INF("Initializing source 'zrpcdb'");

	if (!db_config_parse(db, conf->DB_MODULES_PATH)){
		ERR("Could not parse config file");
		goto log;
	}

	/* make sure that we have all config options */
	if ((!db->config->backend) || (!db->config->username) ||
		(!db->config->password) || (!db->config->database)) {

		ERR("Missing options in db->config file. Requires HOSTNAME, USERNAME, PASSWORD, and DATABASE fields.");
		goto cfg;
	}
	/* FIXME: better value here */
	esql_init();
	if (db->config->conns < 5){
		WRN("DB init: DB pool size must be at least 5. Resizing to 5.");
		db->config->conns = 5;
	}
	e = esql_pool_new(db->config->conns, ESQL_TYPE_MYSQL);
	EINA_SAFETY_ON_NULL_GOTO(e, esql);
	esql_data_set(e, db);
	db->pool_size = db->config->conns;
	esql_connect_callback_set(e, (Esql_Connect_Cb)init_cb, cb);
	esql_database_set(e, db->config->database);
	if (!esql_connect(e, db->config->backend, db->config->username, db->config->password)){
		ERR("Could not begin database connection!");
		goto conn;
	}
	db->e = e;
	if (!init_count++){
		ZRPCDB_EVENT_ERROR = ecore_event_type_new();
		zrpc_err = ecore_event_handler_add(ZRPCDB_EVENT_ERROR, error_cb, db);
		ZRPCDB_EVENT_RESULT = ecore_event_type_new();
		zrpc_res = ecore_event_handler_add(ZRPCDB_EVENT_RESULT, result_cb, db);
		type_cbs = eina_hash_int64_new(NULL);
		db_cbs = eina_hash_int64_new(NULL);
		db_cb_params = eina_hash_int64_new(NULL);
	}
//	eina_log_domain_level_set("zrpcdb", db->config->loglevel);
	return EINA_TRUE;
conn:
	esql_free(e);
esql:
	esql_shutdown();
cfg:
	db_config_free(db->config);
log:
	eina_log_domain_unregister(zrpcdb_log_dom);
	zrpcdb_log_dom = -1;
err:
	eina_shutdown();
	return EINA_FALSE;
}

/**
 * Close anything opened by this module
 */
Eina_Bool uninit(DB *db) {
	EINA_SAFETY_ON_NULL_RETURN_VAL(db, EINA_FALSE);
	INF("Uninitializing source 'zrpcdb'");

	if (db->config)
		db_config_free(db->config);

	if (db->e){
		INF("Connection pool teardown for 'zrpcdb'");
		esql_free(db->e);
		INF("Uninitialization of source 'zrpcdb' complete");
	}
	if (--init_count == 0){
		ecore_event_handler_del(zrpc_res);
		zrpc_res = NULL;
		ecore_event_handler_del(zrpc_err);
		zrpc_err = NULL;
	}
	if (type_cbs) eina_hash_free(type_cbs);
	type_cbs = NULL;
	if (db_cbs) eina_hash_free(db_cbs);
	db_cbs = NULL;
	if (db_cb_params) eina_hash_free(db_cb_params);
	db_cb_params = NULL;

	esql_shutdown();
	eina_shutdown();
	return EINA_TRUE;
}
