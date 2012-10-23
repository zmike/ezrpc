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

#ifndef DEFAULT_H
#define DEFAULT_H

#include "zentific.h"
#include <Esskyuehl.h>
#include "Common_Esskyuehl.h"
#include "ztypes.h"

extern int zrpcdb_log_dom;
#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(zrpcdb_log_dom, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(zrpcdb_log_dom, __VA_ARGS__)
#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(zrpcdb_log_dom, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(zrpcdb_log_dom, __VA_ARGS__)

#ifdef AZY_ERR
# undef AZY_ERR
#endif
#define AZY_ERR(X) AZY_RPC_ERROR_##X

struct _DB_config {
	const char *username;
	const char *password;
	const char *database;
	const char *backend; //host:port, or path, etc
	Eina_Log_Level loglevel;
	int conns;
	void *other;
};
extern Eina_Hash *type_cbs;
extern Eina_Hash *db_cbs;
extern Eina_Hash *db_cb_params;
void event_cb(Esql_Res *res, Zentific_Ctx *ctx);
typedef void *(*Zrpcdb_Cb)();
void fake_free(void *x, void *y);
#include "zrpcdb_common.h"
/** sets up variables used in macros */
#define DB_VARS Esql_Query_Id qid = 0
/** makes a query calling @p CB callback to convert #Esql_Res into desired return type */
#define QUERY(CB, fmt, args ...) do { \
	qid = esql_query_args(db->e, ctx, fmt, ##args); \
	if (!qid) { \
		ERR("Could not create query!"); \
		ctx->error = AZY_ERR(DB_FAIL); \
		return EINA_FALSE; \
	} \
	esql_query_callback_set(qid, (Esql_Query_Cb)event_cb); \
	eina_hash_add(type_cbs, &qid, CB); \
} while (0)
/** same as QUERY, but does not use va_args so no escape */
#define QUERY_NOESCAPE(CB, STR) do { \
	qid = esql_query(db->e, ctx, STR); \
	if (!qid) { \
		ERR("Could not create query!"); \
		ctx->error = AZY_ERR(DB_FAIL); \
		return EINA_FALSE; \
	} \
	esql_query_callback_set(qid, (Esql_Query_Cb)event_cb); \
	eina_hash_add(type_cbs, &qid, CB); \
} while (0)
/** return success based on success of query init */
#define QID_RETURN return !!qid
/** makes an execute-type query */
#define QUERY_NOCONVERT(fmt, args ...) do { \
	qid = esql_query_args(db->e, ctx, fmt, ##args); \
	if (!qid) { \
		ERR("Could not create query!"); \
		ctx->error = AZY_ERR(DB_FAIL); \
		return EINA_FALSE; \
	} \
	esql_query_callback_set(qid, (Esql_Query_Cb)event_cb); \
} while (0)

/** simple string query with no args, single string return */
#define QS(STR) do { \
	DB_VARS; \
	ctx->state = 1; \
	ctx->free_func = (Ecore_Cb)eina_stringshare_del; \
	QUERY_NOESCAPE(esql_res_to_string, STR); \
	QID_CB_ADD(zrpcdb_cb_ptr); \
	QID_RETURN; \
} while (0)
/** string query with args, single string return */
#define QSA(STR, args ...) do { \
	DB_VARS; \
	ctx->state = 1; \
	ctx->free_func = (Ecore_Cb)eina_stringshare_del; \
	QUERY(esql_res_to_string, STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_ptr); \
	QID_RETURN; \
} while (0)

/** simple string query with no args, multiple string return */
#define QSS(STR) do { \
	DB_VARS; \
	ctx->free_func = (Ecore_Cb)Array_string_free; \
	QUERY_NOESCAPE(Array_string_esql, STR); \
	QID_CB_ADD(zrpcdb_cb_ptr); \
	QID_RETURN; \
} while (0)
/** string query with args, multiple string return */
#define QSSA(STR, args ...) do { \
	DB_VARS; \
	ctx->free_func = (Ecore_Cb)Array_string_free; \
	QUERY(Array_string_esql, STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_ptr); \
	QID_RETURN; \
} while (0)

/** simple string query with no args, single bool return */
#define QB(STR) do { \
	DB_VARS; \
	ctx->state = 1; \
	ctx->free_func = NULL; \
	QUERY_NOESCAPE(esql_res_to_lli, STR); \
	QID_CB_ADD(zrpcdb_cb_int); \
	QID_RETURN; \
} while (0)
/** string query with args, single bool return */
#define QBA(STR, args ...) do { \
	DB_VARS; \
	ctx->state = 1; \
	ctx->free_func = NULL; \
	QUERY(esql_res_to_lli, STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_int); \
	QID_RETURN; \
} while (0)

/** simple string query with no args, single int return */
#define QI(STR) do { \
	DB_VARS; \
	ctx->state = 1; \
	ctx->free_func = NULL; \
	QUERY_NOESCAPE(esql_res_to_lli, STR); \
	QID_CB_ADD(zrpcdb_cb_int); \
	QID_RETURN; \
} while (0)
/** string query with args, single int return */
#define QIA(STR, args ...) do { \
	DB_VARS; \
	ctx->state = 1; \
	ctx->free_func = NULL; \
	QUERY(esql_res_to_lli, STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_int); \
	QID_RETURN; \
} while (0)
/** string query with args, single int return */
#define QIAB(STR, args ...) do { \
	DB_VARS; \
	ctx->state = 1; \
	ctx->free_func = NULL; \
	QUERY(esql_res_to_lli, STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_bool); \
	QID_RETURN; \
} while (0)

/** simple string query with no args, multiple int return */
#define QIS(STR) do { \
	DB_VARS; \
	ctx->free_func = (Ecore_Cb)eina_list_free; \
	QUERY_NOESCAPE(Array_int_esql, STR); \
	QID_CB_ADD(zrpcdb_cb_ptr); \
	QID_RETURN; \
} while (0)
/** string query with args, single int return */
#define QISA(STR, args ...) do { \
	DB_VARS; \
	ctx->free_func = (Ecore_Cb)eina_list_free; \
	QUERY(Array_int_esql, STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_ptr); \
	QID_RETURN; \
} while (0)

/** string query with args, executes changes returns UNKNOWN_RESOURCE on error */
#define QE(STR, args ...) do { \
	DB_VARS; \
	ctx->free_func = NULL; \
	QUERY_NOCONVERT(STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_exec); \
	QID_RETURN; \
} while (0)

/** string query with args, executes changes returns ENTITY_ALREADY_EXISTS on error */
#define QEE(STR, args ...) do { \
	DB_VARS; \
	ctx->free_func = NULL; \
	QUERY_NOCONVERT(STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_exec_exists); \
	QID_RETURN; \
} while (0)

/** string query with args, executes changes */
#define QEJ(STR, args ...) do { \
	DB_VARS; \
	ctx->state = 1; \
	ctx->free_func = NULL; \
	QUERY_NOCONVERT(STR, ##args); \
	QID_CB_ADD(zrpcdb_cb_exec); \
	QID_RETURN; \
} while (0)

#ifdef QID_CB_ADD
# undef QID_CB_ADD
#endif
/** add a callback to be called at db-level after the current query */
#define QID_CB_ADD(CB) do { \
	DBG("QID_CB_ADD "#CB); \
	if (qid) eina_hash_set(db_cbs, &qid, CB); \
} while (0)
#define QID_PARAM_ADD(PARAM) do { \
	if (qid) eina_hash_set(db_cb_params, &qid, PARAM); \
} while (0)
#define QID_PARAM_GET eina_hash_find(db_cb_params, &qid); \
	eina_hash_del_by_key(db_cb_params, &qid)

extern int ZRPCDB_EVENT_RESULT;
extern int ZRPCDB_EVENT_ERROR;
#include "zrpcdb.x"
#endif // end default
