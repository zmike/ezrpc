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

#include "functions.h"
#include "ztypes.h"


void cb_bool_job(Zentific_Ctx *ctx, Eina_Bool ret){
	MARK_CB;
	if (ctx->dead) return;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	ctx->count++;
	ecore_job_add(ctx->job, ctx);
}

void cb_int_job(Zentific_Ctx *ctx, int ret){
	const char *cb;
	MARK_CB;
	if (ctx->dead) return;
	cb = CTX_PARAM_GET;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	ctx->count++;
	if (ret) CTX_PARAM_ADD_FROM_VARIABLE(cb, (intptr_t*)ret);
	eina_stringshare_del(cb);
	ecore_job_add(ctx->job, ctx);
}

void cb_pointer_job(Zentific_Ctx *ctx, void *ret){
	const char *cb;
	if (ctx->dead) return;
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	cb = CTX_PARAM_GET;
	ctx->count++;
	if (ret) CTX_PARAM_ADD_FROM_VARIABLE(cb, ret);
	eina_stringshare_del(cb);
	ecore_job_add(ctx->job, ctx);
}

void cb_int_rewind(Zentific_Ctx *ctx, int ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	ctx->count++;
	REWIND;
	RESUME;
}

void cb_bool_rewind(Zentific_Ctx *ctx, Eina_Bool ret){
	MARK_CB;
	if (ctx->dead) return;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	ctx->count++;
	REWIND;
	azy_server_module_events_resume(ctx->parent, EINA_TRUE);
}

/* only used by priv check macros */
void cb_auth_rewind(Zentific_Ctx *ctx, Eina_Bool ret){
	MARK_CB;
	if (ctx->dead) return;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	ctx->count = 0;
	ctx->auth = (ret && (!ctx->error));
	REWIND;
	azy_server_module_events_resume(ctx->parent, EINA_TRUE);
}

void cb_pointer_return(Zentific_Ctx *ctx, void *ret){
	const char *cb;
	if (ctx->dead) return;
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	cb = CTX_PARAM_PEEK;
	ctx->count++;
	if (cb && ret) CTX_PARAM_ADD_FROM_VARIABLE(cb, ret);
	eina_stringshare_del(cb);
	REWIND;
	azy_server_module_events_resume(ctx->parent, EINA_TRUE);
}

void cb_int_return(Zentific_Ctx *ctx, int ret){
	const char *cb;
	if (ctx->dead) return;
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	cb = CTX_PARAM_PEEK;
	ctx->count++;
	if (cb) CTX_PARAM_ADD_FROM_VARIABLE(cb, (intptr_t*)ret);
	eina_stringshare_del(cb);
	REWIND;
	azy_server_module_events_resume(ctx->parent, EINA_TRUE);
}

void cb_job_target_node_addr(Zentific_Ctx *ctx, Eina_List *ret){
	JOB *job;
	const char *s;
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	job = CTX_PARAM_GET;
	if ((!ctx->error) && ret && (eina_list_count(ret) == 2)){
		ctx->count++;
		job->target_node = ret->data;
		job->target_host = ret->next->data;
		eina_list_free(ret);
		REWIND;
		RESUME;
		return;
	}
	JOB_free(job);
	EINA_LIST_FREE(ret, s)
		eina_stringshare_del(s);
	RESUME;
}

void cb_job_target_host(Zentific_Ctx *ctx, const char *ret){
	JOB *job;
	MARK_CB;
	DBG("(ctx=%p, ret=%s)", ctx, ret);
	job = CTX_PARAM_GET;
	if (!ctx->error) {
		ctx->count++;
		job->target_host = ret;
		REWIND;
		RESUME;
		return;
	}
	JOB_free(job);
	eina_stringshare_del(ret);
	RESUME;
}

void cb_job_module_name(Zentific_Ctx *ctx, const char *ret){
	JOB *job;
	MARK_CB;
	DBG("(ctx=%p, ret=%s)", ctx, ret);
	job = CTX_PARAM_GET;
	if (!ctx->error) {
		ctx->count++;
		if (ret && ret[0]) eina_stringshare_replace(&job->module_name, ret);
		else {
			ERR("network control module is invalid, fallback to module_name='parentJob'");
			eina_stringshare_replace(&job->module_name, "parentJob");
		}
		REWIND;
		RESUME;
		return;
	}
	JOB_free(job);
	eina_stringshare_del(ret);
	RESUME;
}
