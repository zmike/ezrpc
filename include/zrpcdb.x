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

#define ZRPCDB_HANDLE_RET(ERROR, RET, RETERR) do {\
	Z_Ctx_Cb cb; \
	if (!esql_res_rows_count(res)) /* id not found */ \
		/* NOT an error event...yet */ \
		if (!ctx->error) ctx->error = AZY_ERR(ERROR); \
	if (ctx->cbs) { \
		cb = ctx->cbs->data; \
		if (!cb) { \
			ctx->cbs = eina_list_remove_list(ctx->cbs, ctx->cbs); \
			return; \
		} \
		if (ctx->error){ /* id not found */ \
			DBG("Query returned no results: \"%s\"", esql_res_query_get(res)); \
			cb(ctx, RETERR); \
			return; \
		} \
		cb(ctx, RET); \
		return; \
	} \
} while (0)

static inline void
zrpcdb_cb_exec(Esql_Res *res, Zentific_Ctx *ctx){
	Z_Ctx_Cb cb;

	if (!esql_res_rows_affected(res))
		/* NOT an error event...yet */
		if (!ctx->error) ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
	if (ctx->cbs) {
		cb = ctx->cbs->data;
		if (!cb) {
			ctx->cbs = eina_list_remove_list(ctx->cbs, ctx->cbs);
			return;
		}
		if (ctx->error){ /* id not found */
			cb(ctx, EINA_FALSE);
			return;
		}
		cb(ctx, EINA_TRUE);
		return;
	}
	ctx->retval = (intptr_t*)!ctx->error;
	ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
}

/* simple pass through conversion function */
static inline void *
DUMMY_esql(Esql_Res *res){
	return res;
}

static inline void
zrpcdb_cb_ptr(Esql_Res *res, Zentific_Ctx *ctx, void *data){
	ZRPCDB_HANDLE_RET(UNKNOWN_RESOURCE, data, NULL);
	ctx->retval = data;
	ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
}

static inline void
zrpcdb_cb_int(Esql_Res *res, Zentific_Ctx *ctx, int ret){
	ZRPCDB_HANDLE_RET(UNKNOWN_RESOURCE, ret, -1);
	ctx->retval = (intptr_t*)&ret;
	ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
}

static inline void
zrpcdb_cb_exec_exists(Esql_Res *res, Zentific_Ctx *ctx){
	ZRPCDB_HANDLE_RET(ENTITY_ALREADY_EXISTS, EINA_TRUE, EINA_FALSE);
	ctx->retval = (intptr_t*)!ctx->error;
	ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
}

static inline void
zrpcdb_cb_bool(Esql_Res *res, Zentific_Ctx *ctx, void *data __UNUSED__){
	ZRPCDB_HANDLE_RET(UNKNOWN_RESOURCE, EINA_TRUE, EINA_FALSE);
	ctx->retval = (intptr_t*)!ctx->error;
	ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
}
