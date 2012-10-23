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
#ifndef Z_SERVLETS_H
#define Z_SERVLETS_H

#include "zentific.h"
#include <Azy.h>
#include "ztypes.h"
#include "Common_Azy.h"

#ifdef ERROR
# undef ERROR
#endif
#define ERROR error_
#ifdef CTX_CB_ADD
# undef CTX_CB_ADD
#endif
#define CTX_CB_ADD(CB) do { \
	if ((!data_->ctx.dead) && (!data_->ctx.error)){ \
		DBG("CTX_CB_ADD "#CB); \
		data_->ctx.cbs = eina_list_append(data_->ctx.cbs, CB); \
	} \
} while (0)
#ifdef CTX_RETURN
# undef CTX_RETURN
#endif
#define CTX_RETURN do { \
	if (data_->ctx.ret_set) \
		azy_server_module_events_resume(module, EINA_TRUE); \
	else if (data_->ctx.error) \
		azy_err_faultcode_set(azy_server_module_content_get(module), data_->ctx.error); \
	return; \
} while (0)

#define PRE_CALL do { \
		extern Eina_Bool reloading_modules; \
		extern Eina_List *reload_modules_list; \
		DBG("ip: %s", azy_net_ip_get(azy_server_module_net_get(module))); \
		if (!data_->session) { \
			data_->session = SESSION_new(); \
			data_->session->uid = -1; \
			extern zentific_config *config; \
			data_->zconf = config; \
			memset(&data_->ctx, 0, sizeof(data_->ctx)); \
			data_->ctx.params = eina_hash_string_djb2_new(NULL); \
			data_->ctx.parent = module; \
		} \
		if (reloading_modules) { \
			reload_modules_list = eina_list_append(reload_modules_list, module); \
			return EINA_TRUE; \
		} \
		return session_pre_call(&data_->ctx, net); \
} while (0)

#define POST do { \
	DBG("(ctx=%p)", &data_->ctx); \
	SESSION_free(data_->session); \
	if (data_->ctx.cbs) eina_list_free(data_->ctx.cbs); \
	eina_hash_free(data_->ctx.params); \
	azy_server_module_events_resume(module, EINA_TRUE); \
	data_->session = NULL; \
	data_->zconf = NULL; \
	memset(&data_->ctx, 0, sizeof(Zentific_Ctx)); \
	return EINA_TRUE; \
} while (0)

#define CHECK_SUPER_PRIV do { \
		if (data_->ctx.user_type){ \
			azy_err_faultcode_set(error_, AZY_ERR(NOT_AUTHORIZED)); \
			azy_server_module_events_resume(module, EINA_FALSE); \
			return; \
		} \
	} while (0)

#define CHECK_ZPOLL_PRIV do { \
		Zentific_Ctx *ctx = &data_->ctx; \
		if ((ctx->user_type != USER_TYPE_SUPER_USER) && (ctx->user_type != USER_TYPE_NODE)){ \
			azy_err_faultcode_set(error_, AZY_ERR(NOT_AUTHORIZED)); \
			azy_server_module_events_resume(module, EINA_FALSE); \
			return; \
		} \
	} while (0)

#define CHECK_EXIST_AND_PRIVS(TYPE, FUNC) do { \
	Zentific_Ctx *ctx = &data_->ctx;\
	if ((!ctx->checked) && (!ctx->auth)) {\
		if (ctx->error) { \
			/* always fail with this to avoid leaking info */ \
			azy_err_faultcode_set(error_, AZY_ERR(NOT_AUTHORIZED));\
			azy_server_module_events_resume(module, EINA_FALSE);\
			return;\
		}\
		if (!ctx->user_type) \
			ctx->auth = ctx->checked = EINA_TRUE; \
		else { \
			if (!ctx->count) {\
				_is##TYPE(ctx, uuid);\
				CTX_CB_ADD(cb_bool_rewind);\
				return; \
			} else {\
				_is##TYPE##owner(ctx, uuid, data_->session->uid); \
				CTX_CB_ADD(cb_auth_rewind);\
				ctx->checked = EINA_TRUE;\
				return; \
			}\
		}\
	}\
	if (ctx->checked || ctx->auth) {\
		if (!ctx->auth) { \
			azy_err_faultcode_set(ERROR, AZY_ERR(NOT_AUTHORIZED)); \
			azy_server_module_events_resume(module, EINA_FALSE); \
		} else { \
			FUNC; \
		} \
	} \
} while (0)

#define CHECK_EXIST_AND_PRIVS_VM(FUNC) CHECK_EXIST_AND_PRIVS(VM, FUNC)
#define CHECK_EXIST_AND_PRIVS_NODE(FUNC) CHECK_EXIST_AND_PRIVS(NODE, FUNC)

#ifdef AZY_RETVAL
# undef AZY_RETVAL
#endif
#define AZY_RETVAL(RET) do { \
	Zentific_Ctx *ctx = &data_->ctx; \
	if ((!ctx->ret_set) && (!ctx->error) && (!azy_content_error_is_set(ERROR))) { \
		if (azy_content_retval_get(ERROR)) azy_value_unref(azy_content_retval_get(ERROR)); \
		Azy_Content_Retval_Cb cb = azy_content_retval_cb_get(ERROR); \
		azy_content_retval_set(ERROR, cb((void*)RET)); \
		ctx->ret = EINA_TRUE; \
		ctx->ret_set = EINA_TRUE; \
	} \
} while (0)

#endif
