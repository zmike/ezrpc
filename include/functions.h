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
#ifndef Z_FUNCTIONS_H
#define Z_FUNCTIONS_H

#include "zentific.h"
#include <Azy.h>
#include "ztypes.h"
#include <errno.h>
#include "Common_Azy.h"
#include "functions_cb.h"

//ERROR_RESET() in zentific.h

extern DB *db;
typedef struct Server_Module {
	SESSION *session;
	zentific_config *zconf;
	Zentific_Ctx ctx;
} Server_Module;
#define AZY_ERR(X) AZY_RPC_ERROR_##X
#define AZY_ERRCODE(X) AZY_RPC_ERROR_##X##_code
#define server_module_data_get(Azy_Server_Module) (Server_Module*)azy_server_module_data_get((Azy_Server_Module))
#ifdef ERROR
# undef ERROR
#endif
#define ERROR azy_server_module_content_get(ctx->parent)
#ifdef SES
# undef SES
#endif
#define SES (server_module_data_get(ctx->parent))->session
#ifdef CFG
# undef CFG
#endif
#define CFG (server_module_data_get(ctx->parent))->zconf
#ifdef CTX_CB_ADD
# undef CTX_CB_ADD
#endif
/** adds a callback to the end of the chain, also suspends for safety */
#define CTX_CB_ADD(CB) do { \
	if ((!ctx->dead) && (!ctx->error)){ \
		DBG("CTX_CB_ADD "#CB); \
		ctx->cbs = eina_list_append(ctx->cbs, CB); \
	} \
} while (0)
/** return value of previous callback to current function for CTX_PARAM_GET */
#define CTX_VALUE_RETURN do { \
	CTX_CB_ADD(cb_pointer_return); \
	CTX_PARAM_ADD(cb_pointer_return, eina_stringshare_add(__func__)); \
} while (0)
#define CTX_INT_RETURN do { \
	CTX_CB_ADD(cb_int_return); \
	CTX_PARAM_ADD(cb_int_return, eina_stringshare_add(__func__)); \
} while (0)
#define CTX_VALUE_SEND(CB) do { \
	CTX_CB_ADD(cb_pointer_return); \
	CTX_PARAM_ADD(cb_pointer_return, eina_stringshare_add(#CB)); \
} while (0)
#define CTX_JOB_SEND(CB) do { \
	CTX_CB_ADD(cb_pointer_job); \
	CTX_PARAM_ADD(cb_pointer_job, eina_stringshare_add(#CB)); \
} while (0)
#define CTX_BOOL_JOB(FUNC) do { \
	CTX_CB_ADD(cb_bool_job); \
	ctx->job = (Ecore_Cb)FUNC; \
} while (0)
#define CTX_INT_JOB(FUNC) do { \
	CTX_CB_ADD(cb_int_job); \
	ctx->job = (Ecore_Cb)FUNC; \
	CTX_PARAM_ADD(cb_int_job, eina_stringshare_add(#FUNC)); \
} while (0)
#define CTX_VALUE_JOB(FUNC) do { \
	CTX_CB_ADD(cb_pointer_job); \
	ctx->job = (Ecore_Cb)FUNC; \
	CTX_PARAM_ADD(cb_pointer_job, eina_stringshare_add(#FUNC)); \
} while (0)

#ifdef CTX_RETURN
# undef CTX_RETURN
#endif
#define CTX_RETURN do { \
	if (ctx->error) { \
		azy_err_faultcode_set(ERROR, ctx->error); \
		azy_server_module_events_resume(ctx->parent, EINA_FALSE); \
		return EINA_FALSE; \
	} else if (ctx->ret_set) { \
		azy_server_module_events_resume(ctx->parent, EINA_TRUE); \
		return EINA_TRUE; \
	} \
	return EINA_TRUE; \
} while (0)

#define _(X) S(X)
#define CTX_PARAM_ADD(CB, P) eina_hash_set(ctx->params, #CB, P)
#define CTX_PARAM_ADD_FROM_VARIABLE(CB, P) eina_hash_set(ctx->params, CB, P)
#define CTX_PARAM_GET \
	eina_hash_find(ctx->params, __func__); \
	eina_hash_del_by_key(ctx->params, __func__)
#define CTX_PARAM_PEEK eina_hash_find(ctx->params, __func__)

#ifdef MARK_CB
# undef MARK_CB
#endif
/** remove a callback from the chain */
#define MARK_CB DBG("MARK_CB %p (%s)", ctx->cbs->data, __func__); \
	ctx->cbs = eina_list_remove_list(ctx->cbs, ctx->cbs)

#ifdef NEXT_CB
# undef NEXT_CB
#endif
/** calls the next chained cb with passed params
 * or resumes if chain is empty using either pre-set value from RET()
 * or true/false based on whether an error has been set
 */
#define NEXT_CB(...) do { \
	DBG("NEXT_CB"); \
	if (ctx->dead) \
		DBG("Client is dead, doing nothing"); \
	else { \
		Azy_Content *errorcontent = ERROR; \
		if (ctx->cbs){ \
			DBG("Taking cb %p", ctx->cbs->data); \
			((Z_Ctx_Cb)(ctx->cbs->data))(ctx, ##__VA_ARGS__); \
		} else if (ctx->error || azy_content_error_is_set(errorcontent)) \
			RESUME; \
		else if (ctx->ret_set && azy_content_retval_get(errorcontent)) { \
			DBG("Resumed with pre-set value"); \
			azy_server_module_events_resume(ctx->parent, ctx->ret); \
		} else \
			RESUME; \
	} \
} while (0)
/** suspends and rewinds call execution to run the current directive again
 * @code
 * method_doX(Azy_Server_Module *error, const char *param){
 * 	if (!SESSION->somevalue){
 *		functionThatSetsValueFromDB(error);
 * 		REWIND;
 * 		return EINA_TRUE;
 * 	}
 * 	printf("somevalue = %s\n", SESSION->somevalue);
 * }
 */
#define REWIND do { \
	azy_server_module_events_rewind(ctx->parent); \
} while (0)
/** suspends execution to allow callbacks */
#define SUSPEND azy_server_module_events_suspend(ctx->parent)
/** resumes execution */
#define RESUME do { \
	Eina_Bool ret = EINA_TRUE; \
	if (!ctx->dead) { \
		Azy_Content *errorcontent = ERROR; \
		DBG("Resuming from value based on error"); \
		if (errorcontent) {\
			if (ctx->error && (!azy_content_error_is_set(errorcontent))) \
				azy_err_faultcode_set(errorcontent, ctx->error); \
			ret = !azy_content_error_is_set(errorcontent); \
		} \
		azy_server_module_events_resume(ctx->parent, ret); \
	} \
} while (0)

#ifdef AZY_RETVAL
# undef AZY_RETVAL
#endif
#define AZY_RETVAL(RET) do { \
	Azy_Content *errorcontent = ERROR; \
	if ((!ctx->ret_set) && (!ctx->error) && (!azy_content_error_is_set(errorcontent))) { \
		Azy_Content_Retval_Cb cb = azy_content_retval_cb_get(errorcontent); \
		azy_content_retval_set(errorcontent, cb((void*)RET)); \
		ctx->ret = EINA_TRUE; \
		ctx->ret_set = EINA_TRUE; \
	} \
} while (0)

#ifdef AZY_RET
#undef AZY_RET
#endif
/** set a return value (true/false) to be used for the current directive */
#define AZY_RET(X) do { \
	ctx->ret = X; \
	ctx->ret_set = EINA_TRUE; \
} while (0)

#define STRTO_ERRNO_CHECK(X) do { \
		if (errno){ \
			WRN("Failed to convert '%s' to number: %s", X, strerror(errno)); \
		} \
} while (0)

#endif
