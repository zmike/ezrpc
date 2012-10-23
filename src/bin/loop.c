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
#include "ztypes.h"
#include "functions.h"

Eina_Bool
error_handler(DB *db __UNUSED__, int type __UNUSED__, Zentific_Ctx *ctx){
	DBG("(ctx=%p)", ctx);
	if (ctx->dead)
		WRN("Ignoring error event for already dead client");
	else {
		ERR("Sending error to client: \"%s\"", eina_error_msg_get(ctx->error));

		azy_err_faultcode_set(ERROR, ctx->error);
		eina_list_free(ctx->cbs);
		ctx->cbs = NULL;
		eina_hash_free_buckets(ctx->params);
		azy_server_module_events_resume(ctx->parent, EINA_FALSE);
		ctx->dead = EINA_TRUE;
	}
	return EINA_TRUE;
}

Eina_Bool
result_handler(DB *db __UNUSED__, int type __UNUSED__, Zentific_Ctx *ctx){
	DBG("(ctx=%p)", ctx);
	/* content may be dead here so don't use it */
	if (ctx->dead)
		WRN("Ignoring result event for already dead client");
	else {
		Azy_Content *content = ERROR;
		if (content && (!ctx->ret_set)) {
			if (ctx->error) azy_err_faultcode_set(content, ctx->error);
			Azy_Content_Retval_Cb cb = azy_content_retval_cb_get(content);
			if (!cb) /* pre */ {
				azy_server_module_events_resume(ctx->parent, !ctx->error);
				return EINA_TRUE;
			}
			if (!ctx->error) azy_content_retval_set(content, cb(ctx->retval));
			if (ctx->free_func && ctx->retval) ctx->free_func(ctx->retval);
		}
		azy_server_module_events_resume(ctx->parent, EINA_TRUE);
		ctx->dead = EINA_TRUE;
	}
	return EINA_TRUE;
}

static void
loop_cb_dead(Zentific_Ctx *ctx, Eina_Bool ret __UNUSED__){
	free(ctx);
}

static void
loop_cb_bool_rewind(Zentific_Ctx *ctx, Eina_Bool ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	ctx->count++;
	REWIND;
	azy_server_module_events_resume(ctx->parent, EINA_TRUE);
}

Eina_Bool
console_handler(Zentific_Ctx *ctx, int t __UNUSED__, Ecore_Exe_Event_Del *ev){
	Zentific_Ctx *pctx;
	extern DB *db;
	const char *tag;
	char *type, *uuid, *session;
	tag = ecore_exe_tag_get(ev->exe);
	if (!tag){
		ERR("DEAD CONSOLE IN DB!");
		free(ctx);
		return EINA_TRUE;
	}
	uuid = strchr(tag, ':');
	session = strchr(uuid + 35, ':');
	type = strndupa(tag, uuid - tag);
	uuid = strndupa(uuid + 1, 36);
	session = strdupa(session + 1);
	if (!ctx->dead){
		/* immediate exit, something broke */
		pctx = ctx->parent;
		pctx->count--;
		ERR("Console broke! Type: %s; VM: %s; Session: %s;", type, uuid, session);
		db->releaseConsoleLock(db, pctx, type, uuid, session);
		pctx->cbs = eina_list_append(pctx->cbs, loop_cb_bool_rewind);
		pctx->error = AZY_ERR(CONSOLE_DAEMON_NOT_FOUND);
		ecore_timer_del(ctx->retval);
		free(ctx);
	} else {
		/* daemon exited sometime later, silently remove lock from db */
		ERR("Console exited with status %i.  Type: %s; VM: %s; Session: %s;", ev->exit_code, type, uuid, session);
		db->releaseConsoleLock(db, ctx, type, uuid, session);
		ctx->cbs = eina_list_append(ctx->cbs, loop_cb_dead);
	}
	return EINA_TRUE;
}
