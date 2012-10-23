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
#include "functions-vif.h"
#include "functions-vm.h"

//uses mac because (though not expressed in DB schema due to
//      revisioning desire) it is 1:1 unique to vif and vm
Eina_Bool _isNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac){
	db->isNetIf(db, ctx, uuid, mac);
	CTX_RETURN;
}

Eina_Bool _createNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac){
	db->createNetIf(db, ctx, uuid, mac);
	CTX_RETURN;
}

Eina_Bool _getNumVMvifs(Zentific_Ctx *ctx, const char *uuid){
	db->getNumVMvifs(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getNetIfByMAC(Zentific_Ctx *ctx, const char *uuid, const char *mac){
	db->getVMvifByMac(db, ctx, uuid, mac);
	CTX_RETURN;
}

Eina_Bool _updateNetIfStats(Zentific_Ctx *ctx, const char *uuid, NETIF *vif){
	db->updateNetIfStats(db, ctx, uuid, vif);
	CTX_RETURN;
}

Eina_Bool _getVMvifs(Zentific_Ctx *ctx, const char *uuid){
	db->getVMvifs(db, ctx, uuid);
	CTX_RETURN;
}

/* convenience function.
 *
 * returns 0 for equivalent NETIFs by comparing MACs
 *
 */
int NETIF_mac_cmp(NETIF *a, NETIF *b){

	return (a->mac != b->mac);
}

/* Convenience function for updateNetIfConfig. only required because
 * VM struct contains both config and stats data right now.
 * will be removed when that design flaw has been corrected
 */
int NETIF_config_cmp(NETIF *a, NETIF *b){

	if (a == b)
		return 0;
	if ((!a) || (!b))
		return 1;
	if (a->name == b->name)
		return 1;
	if (a->netmask == b->netmask)
		return 1;
	if (a->ip == b->ip)
		return 1;
	if (a->type == b->type)
		return 1;
	if (a->script == b->script)
		return 1;
	if (a->bridge == b->bridge)
		return 1;
	if (a->mac == b->mac)
		return 1;
	if (a->gateway == b->gateway)
		return 1;
	if (a->broadcast == b->broadcast)
		return 1;
	if(a->mtu != b->mtu)
		return 1;

	return 0;
}

static void
vifs_updateconfig(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	VM *vm = ctx->retval;
	Eina_List *l;
	NETIF *v;
	Ecore_Cb cb = ctx->cb;
	intptr_t *rev;
	switch (ctx->count){
		case 0:
			db->getLatestRevision(db, ctx, vm->uuid, 3); /* 3 is vif */
			CTX_INT_JOB(vifs_updateconfig);
			break;
		case 1:
			rev = CTX_PARAM_GET;
			EINA_LIST_FOREACH(vm->vifs, l, v){
				if (!pctx->ret){
					/* stats set if this is false */
					if (pctx->checked) {
						/* existing vm */
						db->pruneNetIfStats(db, ctx, vm->uuid, v->mac);
						CTX_CB_ADD(NULL);
					}
					db->updateNetIfStats(db, ctx, vm->uuid, v);
					CTX_CB_ADD(NULL);
				}
				db->updateNetIfConfig(db, ctx, vm->uuid, *rev + 1, v);
				CTX_BOOL_JOB(vifs_updateconfig);
			}
			break;
		default:
			if (ctx->count == 1 + eina_list_count(vm->vifs))
				goto out;
			break;
	}
	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	free(ctx);
	if (--pctx->count == 7)
		ecore_job_add(cb, pctx);
}

void vif_cb_updateconfig(Zentific_Ctx *ctx){
	VM *vm = ctx->retval;
	Eina_List *l, *vifs;
	NETIF *v;
	Ecore_Cb cb = ctx->cb;
	Zentific_Ctx *pctx = ctx->parent;
	int create = 0;

	switch (ctx->count){
		case 0:
			vifs = CTX_PARAM_GET;
			if (!vifs){
				if (!vm->vifs) goto out;
				else
					ctx->auth = ctx->checked = pctx->auth = EINA_TRUE;
				break;
			} else if (eina_list_count(vifs) != eina_list_count(vm->vifs))
				ctx->auth = ctx->checked = pctx->auth = EINA_TRUE;
			else {

				/* if a stored NetIf is not found in the input list, we need to update */
				EINA_LIST_FOREACH(vifs, l, v) {
					/* find netif by mac */
					if (!eina_list_search_unsorted(vm->vifs, (Eina_Compare_Cb)NETIF_mac_cmp, v)){
						ctx->auth = ctx->checked = pctx->auth = EINA_TRUE;
						break;
					}
				}
				/* if an input NetIf is not found in the stored list, we need to update */
				EINA_LIST_FOREACH(vm->vifs, l, v) {
					NETIF *x;
					/* find netif by mac */
					x = eina_list_search_unsorted(vifs, (Eina_Compare_Cb)NETIF_mac_cmp, v);
					if (!x){
						db->createNetIf(db, ctx, vm->uuid, v->mac);
						CTX_BOOL_JOB(vif_cb_updateconfig);
						create++;
					} else if (NETIF_config_cmp(x, v))
						ctx->checked = EINA_TRUE;
				}
				if (ctx->checked){
					pctx->auth = EINA_TRUE;
					if (!create) {
						ctx->auth = EINA_TRUE;
						break;
					}
				}
				ctx->count = 1000 - create;
			}
			EINA_LIST_FREE(vifs, v)
				NETIF_free(v);
		default:
			if (ctx->count == 1000){
				ctx->auth = EINA_TRUE;
				ctx->count = 0;
			}
			if (ctx->auth)
				vifs_updateconfig(ctx);
			break;
	}
	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	free(ctx);
	if (--pctx->count == 7)
		ecore_job_add(cb, pctx);
}
