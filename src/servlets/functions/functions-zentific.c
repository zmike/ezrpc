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
#include "functions-zentific.h"
#include "zentific-defines.h"
#include "ztypes.h"


   /*  .getConfig(key) - returns a string for the config value for the given config option. QUESTION: should some config vars be globally readable and others require higher privileges?
    * .setconfig(key, value) - sets a config entry for the given key to the given value
    * .getFrontendPlugins() - returns an array of plugin names.
    * .enableFrontendPlugin(string name) - returns bool
    * .disableFrontendPlugin(string name) - returns bool
*/

/* Eina_List * */
Eina_Bool _getOperatingSystems(Zentific_Ctx *ctx){
	db->getOperatingSystems(db, ctx);
	CTX_RETURN;
}

/* Eina_List * */
Eina_Bool _getLanguages(Zentific_Ctx *ctx){
	db->getLanguages(db, ctx);
	CTX_RETURN;
}

/* FIXME : this should probably have the role/gid/etc stuff exposed high up in
 *	the API level (if not directly to the caller of the API)
 */
Eina_Bool _getConfig(Zentific_Ctx *ctx, const char *key){
	db->getConfigValue(db, ctx, key, "zrpc",
	-1, -1, -1, -1, -1, -1,
	-1, -1, -1 );
	CTX_RETURN;
}

/* Eina_List * */
Eina_Bool _getConfigsFull(Zentific_Ctx *ctx){
	db->getConfigsFull(db, ctx);
	CTX_RETURN;
}

Eina_Bool _setConfigs(Zentific_Ctx *ctx, Eina_List *configs){
	KEYVALUE *kv;
	Eina_List *l;

	// there must be a better pattern to use in functions of this type.
	// 	if any iteration fails, should we bail THERE or try the rest anyway?

	EINA_LIST_FOREACH(configs, l, kv){
		if ((!kv->key) || (!kv->value)) continue;
		_setConfig(ctx, kv->key, kv->value);
		CTX_CB_ADD(NULL);

		if(ctx->error) {
			ERR("%s", eina_error_msg_get(ctx->error));
			ctx->error = 0;
		}
	}

	CTX_RETURN;
}

/* FIXME : this should probably have the role/gid/etc stuff exposed high up in
 *	the API level (if not directly to the caller of the API)
 */
Eina_Bool _setConfig(Zentific_Ctx *ctx, const char *key, const char *value){

/*
Eina_Bool setConfig(DB *db, const char *key, const char *value, const char *subsystem,
	int mid, int uid, int gid, int vgid, int ngid, int vm,
	int node, int platform, int role ){
*/

	db->setConfig(db, ctx, key, value, "zrpc",
		-1, -1, -1, -1, -1, -1,
		-1, -1, -1 );
	CTX_RETURN;

}

/* which:
 * 	-1) all
 * 	 0) disabled
 * 	 1) enabled
 */
/* Eina_List * */
Eina_Bool _getAllPlugins(Zentific_Ctx *ctx, int which){
	db->getAllPlugins(db, ctx, which);
	CTX_RETURN;
}

/* Eina_List * */
Eina_Bool _getFrontendPlugins(Zentific_Ctx *ctx, int which){
	db->getFrontendPlugins(db, ctx, which);
	CTX_RETURN;
}

Eina_Bool _modifyPlugin(Zentific_Ctx *ctx, const char *name, Eina_Bool enabled, Eina_Bool xmlrpc, Eina_Bool frontend, Eina_Bool scheduler, Eina_Bool poller){
	db->modifyPlugin(db, ctx, name,   enabled,  xmlrpc,  frontend,  scheduler,  poller);
	CTX_RETURN;
}

Eina_Bool _isFrontendPluginEnabled(Zentific_Ctx *ctx, const char *name){
	db->isFrontendPluginEnabled(db, ctx, name);
	CTX_RETURN;
}


Eina_Bool _cleanDB(Zentific_Ctx *ctx){
	db->cleanDB(db, ctx);
	CTX_RETURN;
}

//FIXME should this be getLatestVMvifRevision /vm/disk ?
//      or is one funct consistent. mood tonight is one function :)
/* node: 0
 * vm: 1
 * disk: 2
 * vif: 3
 */
/* int */
Eina_Bool _getLatestRevision(Zentific_Ctx *ctx, const char *uuid, unsigned char type){
	db->getLatestRevision(db, ctx, uuid, type);
	CTX_RETURN;
}

/* FIXME OR TODO
 *
 * improve randomness here by:
 *	-initstate (default w/o calling is 128, not bad, but..)
 *	-consider drand48 et al. see random(3) for note on threading
 *		(that manpage was somewhat unclear. it's probably fine
 *		 to use just random() as long as it's re-entrant)
 *
 */


static void
zentific_generatemac_cb(Zentific_Ctx *ctx, Eina_Bool ret){
	const char *mac;
	MARK_CB;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	mac = CTX_PARAM_GET;
	if (!ret) {
		if (!ctx->cbs) AZY_RETVAL((void*)mac);
		NEXT_CB(mac);
		eina_stringshare_del(mac);
		return;
	}
	eina_stringshare_del(mac);
	if (!_generateMAC(ctx)){
		ERR("Error in recursion!");
		NEXT_CB(NULL);
		return;
	}
	/* move caller to end of cb chain */
	ctx->cbs = eina_list_demote_list(ctx->cbs, ctx->cbs);
}

Eina_Bool _generateMAC(Zentific_Ctx *ctx){
	unsigned char mac[6] = {0};
	char ret[24];
	struct timeval tv;

	memset(&tv, 0, sizeof(struct timeval));
	gettimeofday(&tv, NULL);
	srandom(tv.tv_usec);

 	/* NOTE: using modulus here is not a problem.
	 *	the standard wisdom that higher-order
	 *	bits have better entropy is no longer
	 *	true in current versions of libc
	 *	see rand(3)
	 */
	mac[0] = random() % 256;
	mac[1] = random() % 256;
	mac[2] = random() % 256;
	mac[3] = random() % 256;
	mac[4] = random() % 256;
	mac[5] = random() % 256;

	snprintf(ret, sizeof(ret), "00:16:3e:%2.2x:%2.2x:%2.2x", mac[0], mac[1], mac[2]);

	db->isNetIfByMAC(db, ctx, ret);
	CTX_CB_ADD(zentific_generatemac_cb);
	CTX_PARAM_ADD(zentific_generatemac_cb, eina_stringshare_add(ret));
	CTX_RETURN;
}

/* NOTES
 *
 * if we use on_crash/on_shutdown/on_restart to control vms, then under no circumstances should we be unaware of state
 * HOWEVER if the vm dies for other reasons (e.g. qemu-dm segfaults) this fact may not be reflected in one of these
 * states.
 * so, LAST_UPDATE timestamp of the vm would then be NOW-K where if K>C (C=some expiry threshold), we can say "state unknown"
 * FIXME ^^^ this logic should go in get_vm/get_vm_state and/or perhaps in the scheduler, which can watch for such things in
 * its ticking thread
 *
 */
