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
#include "functions-platform.h"
#include "zentific-defines.h"
#include "utility.h"
#include "ztypes.h"

Eina_Bool _getPlatforms(Zentific_Ctx *ctx){
	db->getPlatforms(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getControlModulesByPlatform(Zentific_Ctx *ctx, int platformid){

	/* make sure that modules having platform == ANY. that is,
	 *	platformid == -1, are added to the list returned for
	 *	platformid >= 0; that is, legit platforms != ANY
 	 */

	if (platformid < -1){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	/* fetch all control modules targeting platform == -1, because
	 *	they are considered to be valid for the requested
	 *	platformid as well. ( platform == -1 is essentially
	 *	a wildcard)
	 */
	db->getControlModulesByPlatform(db, ctx, platformid);
	CTX_RETURN;
}

static void
platform_get(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	PLATFORM *p = ctx->retval;

	switch (ctx->count){
		case 0:
			db->getPlatformConfig(db, ctx, p->id, NULL);
			CTX_VALUE_JOB(platform_get);
			break;
		case 1:
			if (ctx->error) break;
			p->config = CTX_PARAM_GET;
			goto out;
		default:
			break;
	}

	if (!ctx->error) return;
	if (ctx->error) pctx->error = ctx->error;
out:
	if (--pctx->count == 2){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

Eina_Bool _getPlatform(Zentific_Ctx *ctx, int id){
	PLATFORM *p;
	Zentific_Ctx *zctx;
	switch (ctx->count){
		case 0:
			if (id < -1){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				CTX_RETURN;
			}
			db->getPlatform(db, ctx, id);
			CTX_VALUE_RETURN;
			break;
		case 1:
			if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){
				ctx->error = AZY_ERR(PLATFORM_NOT_PRESENT);
				break;
			}
			if (ctx->error) break;
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->parent = ctx;
			zctx->retval = CTX_PARAM_GET;
			platform_get(zctx);
			ctx->count++;
			break;
		case 2:
			if (ctx->error){
				if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE))
					ctx->error = AZY_ERR(PLATFORM_NOT_PRESENT);
				PLATFORM_free(ctx->retval);
				break;
			}
			p = ctx->retval;
			ctx->retval = NULL;
			AZY_RETVAL(p);
			RESUME;
			break;
		default:
			ctx->error = AZY_ERR(PLATFORM_NOT_PRESENT);
	}
	CTX_RETURN;
}

Eina_Bool _getPlatformByName(Zentific_Ctx *ctx, const char *name){
	PLATFORM *p;
	Zentific_Ctx *zctx;
	switch (ctx->count){
		case 0:
			if ((!name) || (!name[0])){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				CTX_RETURN;
			}
			db->getPlatformByName(db, ctx, name);
			CTX_VALUE_RETURN;
			break;
		case 1:
			if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){
				ctx->error = AZY_ERR(PLATFORM_NOT_PRESENT);
				break;
			}
			if (ctx->error) break;
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->parent = ctx;
			zctx->retval = CTX_PARAM_GET;
			platform_get(zctx);
			ctx->count++;
			break;
		case 2:
			if (ctx->error){
				if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE))
					ctx->error = AZY_ERR(PLATFORM_NOT_PRESENT);
				PLATFORM_free(ctx->retval);
				break;
			}
			p = ctx->retval;
			ctx->retval = NULL;
			AZY_RETVAL(p);
			RESUME;
			break;
		default:
			ctx->error = AZY_ERR(PLATFORM_NOT_PRESENT);
	}
	CTX_RETURN;
}

Eina_Bool _addPlatform(Zentific_Ctx *ctx, PLATFORM *p){
	if (!ctx->count){
		_isPlatformByName(ctx, p->name);
		CTX_CB_ADD(cb_bool_rewind);
	} else if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE))
		db->addPlatform(db, ctx, p);
	else if (!ctx->error)
		ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
	CTX_RETURN;
}

Eina_Bool _modifyPlatform(Zentific_Ctx *ctx, int id, PLATFORM *p){
	if (!ctx->count){
		_isPlatformByName(ctx, p->name);
		CTX_CB_ADD(cb_bool_rewind);
	} else if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE))
		db->modifyPlatform(db, ctx, id, p);
	else if (!ctx->error)
		ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
	CTX_RETURN;
}

// see propsed FIXME in the db module code
Eina_Bool _setPlatformConfig(Zentific_Ctx *ctx, int id, Eina_List /*KEYVALUE*/ *config){
	db->setPlatformConfig(db, ctx, id, config);
	CTX_RETURN;
}

Eina_Bool _setSecureControlModuleConfig(Zentific_Ctx *ctx, int mid, KEYVALUE *modconf){

	unsigned char *encrypted 	= NULL;
	char *b64encrypted		= NULL;
	int size 			= 0;

	extern zentific_config *config;

	if(!modconf || !config ){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}

	if(!modconf->key || !modconf->value ){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}

	/* FIXME: use thread */
	if(!(encrypted = _encryptString(config->CREDSTORE_PUBKEY_PATH, modconf->value, &size))){
		ERR("error encrypting configuration data");
		ctx->error = AZY_ERR(SERVER_FAILURE);
		CTX_RETURN;
	}

	b64encrypted = azy_base64_encode((char*)encrypted, (double)size);

	db->setBlobConfig(db, ctx, modconf->key, (unsigned char*)b64encrypted, "zensched", mid,
		-1 /*uid*/, -1 /*gid*/, -1 /*vgid*/, -1 /*ngid*/, -1 /*vm*/,
		-1 /*node*/, -1 /*platform*/, -1 /*role*/ );

	free(encrypted);
	free(b64encrypted);

	CTX_RETURN;
}


Eina_Bool _getPlatformConfig(Zentific_Ctx *ctx, int id){
	db->getPlatformConfig(db, ctx, id, NULL);
	CTX_RETURN;
}


Eina_Bool _getPlatformsFull(Zentific_Ctx *ctx){
	PLATFORM *p;
	Eina_List *l, *ps;
	Zentific_Ctx *zctx;

	switch (ctx->count){
		case 0:
			db->getPlatformsFull(db, ctx);
			CTX_VALUE_RETURN;
			break;
		case 1:
			if (ctx->error) break;
			ctx->retval = ps = CTX_PARAM_GET;
			ctx->count += eina_list_count(ps);
			EINA_LIST_FOREACH(ps, l, p){
					zctx = calloc(1, sizeof(Zentific_Ctx));
					zctx->parent = ctx;
					zctx->retval = p;
					platform_get(zctx);
			}
			ctx->count++;
			break;
		case 2:
			if (ctx->error)
				/* FIXME: ??? */
				ctx->error = 0;
			ps = ctx->retval;
			AZY_RETVAL(ps);
			RESUME;
		default:
			break;
	}
	CTX_RETURN;
}

Eina_Bool _getControlModulesFull(Zentific_Ctx *ctx){
	db->getControlModulesFull(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getControlModules(Zentific_Ctx *ctx){
	db->getControlModules(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getControlModuleNameByNetwork(Zentific_Ctx *ctx, int nwid){
	db->getControlModuleNameByNetwork(db, ctx, nwid);
	CTX_RETURN;
}

Eina_Bool _getControlModule(Zentific_Ctx *ctx, int id){
	if (id < -1)
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->getControlModule(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _getControlModuleByName(Zentific_Ctx *ctx, const char *name){
	if ((!name) || (!name[0]))
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->getControlModuleByName(db, ctx, name);
	CTX_RETURN;
}

/* TODO rename _isPlatformId and bring a real isPlatform w/
 * 	 a PLATFORM being taken as a parameter
 */
Eina_Bool _isPlatform(Zentific_Ctx *ctx, int id){
	if (id < -1)
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->isPlatform(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _isPlatformByName(Zentific_Ctx *ctx, const char *name){
	if ((!name) || (!name[0]))
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->isPlatformByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _isControlModule(Zentific_Ctx *ctx, int id){
	if (id < 0)
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->isControlModule(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _isControlModuleByName(Zentific_Ctx *ctx, const char *name){
	if ((!name) || (!name[0]))
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->isControlModuleByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _isControlModuleForPlatform(Zentific_Ctx *ctx, const char *name, int platformid){
	if ((!name) || (!name[0]) || (platformid < -1))
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->isControlModuleForPlatform(db, ctx, name, platformid);
	CTX_RETURN;
}

Eina_Bool _isControlModuleForPlatformByName(Zentific_Ctx *ctx, const char *name, const char *platformname){
	if ((!name) || (!name[0]) || (!platformname) || (!platformname[0]))
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->isControlModuleForPlatformByName(db, ctx, name, platformname);
	CTX_RETURN;
}

Eina_Bool _getControlModuleConfig(Zentific_Ctx *ctx, int id){
	db->getControlModuleConfig(db, ctx, id, NULL);
	CTX_RETURN;
}

Eina_Bool _setControlModuleConfig(Zentific_Ctx *ctx, int mid, Eina_List /*KEYVALUE*/ *config){
	db->setControlModuleConfig(db, ctx, mid, config);
	CTX_RETURN;
}

Eina_Bool _enablePlatform(Zentific_Ctx *ctx, int id){
	db->enablePlatform(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _disablePlatform(Zentific_Ctx *ctx, int id){
	db->disablePlatform(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _enableControlModule(Zentific_Ctx *ctx, int mid){
	db->enableControlModule(db, ctx, mid);
	CTX_RETURN;
}

Eina_Bool _disableControlModule(Zentific_Ctx *ctx, int mid){
	db->disableControlModule(db, ctx, mid);
	CTX_RETURN;
}

Eina_Bool _getPlatformIdByVm(Zentific_Ctx *ctx, const char *uuid){
	db->getPlatformIdByVm(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getPlatformFeatures(Zentific_Ctx *ctx, int platform){
	db->getPlatformFeatures(db, ctx, platform);
	CTX_RETURN;
}
