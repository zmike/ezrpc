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
#include "functions-vm.h"
#include "functions-node.h"
#include "functions-user.h"
#include "functions-session.h"
#include "functions-zentific.h"
#include "utility.h"

#if 0
static void
_updateVMparentNode(Zentific_Ctx *ctx, const char *uuid, const char *puuid){
	db->updateVMparentNode(db, ctx, uuid, puuid);
}

/* helper function to determine if a vm no longer has a parent node */
static Eina_Bool __isVMorphan(Zentific_Ctx *ctx, const char *uuid){
	const char *p;
	Eina_Error e;

	e = azy_content_error_code_get(ERROR);
	azy_content_error_reset(ERROR);

	p = db->getVMparentNode(db, ctx, uuid);
	azy_err_faultcode_set(ERROR, e);
	if (!p)
		return EINA_TRUE;

	eina_stringshare_del(p);
	return EINA_FALSE;
}

static Eina_Bool __adoptOrphanedVM(Zentific_Ctx *ctx, const char *uuid){

	if (!__isVMorphan(ctx, uuid))
		return EINA_FALSE;

	/* FIXME: we should probably have a mechanism for migrating orphaned vms
	 * to their new nodes if possible?
	 * CASE: vm x on node y. y gets deleted from db while x is off.
	 * db fails to delete x for whatever reason. x no longer has a puuid,
	 * thus lots of functions don't work for it, things happen, etc
	 * FOR NOW: deleting orphaned vms is the best solution to avoid db corruption/staleness
	 * as well as ui failures.
	 */
	WRN("Orphaned vm with uuid %s detected. Euthanizing...", uuid);
	db->removeVm(db, ctx, uuid);
}

/* compares all but uuid; for use with vms whose uuid changes */
static int
VM_similar(VM* a, VM* b){

	if (a == b)
		return 0;
	if ((!a) || (!b))
		return 1;
	if (zshare_strcmp(a->name, b->name))
		return 1;
	if (zshare_strcmp(a->puuid, b->puuid))
		return 1;
	if (zshare_strcmp(a->type, b->type))
		return 1;
	if (zshare_strcmp(a->os, b->os))
		return 1;
	if (a->id != b->id)
		return 1;
	if (zshare_strcmp(a->kernel, b->kernel))
		return 1;
	if (zshare_strcmp(a->ramdisk, b->ramdisk))
		return 1;
	if (zshare_strcmp(a->cmdline, b->cmdline))
		return 1;
	if (zshare_strcmp(a->on_reboot, b->on_reboot))
		return 1;
	if (zshare_strcmp(a->on_poweroff, b->on_poweroff))
		return 1;
	if (zshare_strcmp(a->on_crash, b->on_crash))
		return 1;
	if (a->mem != b->mem)
		return 1;
	if (a->maxmem != b->maxmem)
		return 1;
	if (a->vcpus != b->vcpus)
		return 1;
	if (a->cpupct != b->cpupct)
		return 1;
	if (a->cputime != b->cputime)
		return 1;
	if (a->uptime != b->uptime)
		return 1;
	if (a->vncport != b->vncport)
		return 1;
	if (zshare_strcmp(a->vncpasswd, b->vncpasswd))
		return 1;
	if (zshare_strcmp(a->state, b->state))
		return 1;
	if (!Array_DISK_eq(a->disks, b->disks))
		return 1;
	if (!Array_NETIF_eq(a->vifs, b->vifs))
		return 1;

	return 0;
}
#endif
static Eina_Bool
_handleVmStateChange(Zentific_Ctx *ctx, const char *uuid, const char *state){
//FIXME call state handler to watch for crashed shutdown paused migrating
//	when state change observed, fire off default job in response and
//	log the condition for that vm. do here or updatevmstats?
//FIXME the state handler needs to deal with migrating domains...which are
//	renamed VMNAME-migrating or similar. problem == potentially see multiple vms
//	with the same uuid on different nodes!!

	Eina_Bool ret = EINA_FALSE;

	if (!state)
		INF("VM %s is doing...stuff...maybe...", uuid);
	else if (*state == 'r'){
		INF("VM %s is running", uuid);
		//do nothing
	} else if (*state == 'b'){
		INF("VM %s is idle", uuid);
		//do nothing
	} else if (*state == 'p'){
		INF("VM %s is paused", uuid);
		//do nothing
	} else if (*state == 's'){
		INF("VM %s has shut down", uuid);
		//fetch configured behavior
		//invoke behavior
		ret = EINA_TRUE;
	} else if (*state == 'd'){
		INF("VM %s is dying", uuid);
		//fetch configured behavior
		//invoke behavior
		ret = EINA_TRUE;
	} else if (*state == 'c'){
		INF("VM %s has crashed", uuid);
		//fetch configured behavior
		//invoke behavior
		ret = EINA_TRUE;
	}

	//FIXME TODO revisit this
	db->setVmState(db, ctx, uuid, state);
	CTX_CB_ADD(NULL);
	db->updateVmTimestamp(db, ctx, uuid);
	CTX_CB_ADD(cb_bool_job);

	return ret;
}

/* Convenience function for updateVmConfig. only required because
 * VM struct contains both config and stats data right now.
 * will be removed when that design flaw has been corrected
 */
static int
VM_config_cmp(VM *a, VM *b){

	if (a == b)
		return 0;
	if ((!a) || (!b))
		return 1;
	if (a->name != b->name)
		return 1;
	if (a->puuid != b->puuid)
		return 1;
	if (a->type != b->type)
		return 1;
	if (a->os != b->os)
		return 1;
	if (a->kernel != b->kernel)
		return 1;
	if (a->ramdisk != b->ramdisk)
		return 1;
	if (a->cmdline != b->cmdline)
		return 1;
	if (a->on_reboot != b->on_reboot)
		return 1;
	if (a->on_poweroff != b->on_poweroff)
		return 1;
	if (a->on_crash != b->on_crash)
		return 1;
	if (a->mem != b->mem)
		return 1;
	if (a->maxmem != b->maxmem)
		return 1;
	if (a->vcpus != b->vcpus)
		return 1;
	if (a->vncport != b->vncport)
		return 1;
	if (a->vncpasswd != b->vncpasswd)
		return 1;

	return 0;

}

static Eina_Bool _createVM(Zentific_Ctx *ctx, const char *uuid){
	db->createVM(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getTopNCpuConsumers(Zentific_Ctx *ctx, int intervalminutes, int n){
	db->getTopCpuConsumers(db, ctx, intervalminutes, n);
	CTX_RETURN;
}

Eina_Bool _getTopNBandwidthConsumers(Zentific_Ctx *ctx, int intervalminutes, int n){
	db->getTopBandwidthConsumers(db, ctx, intervalminutes, n);
	CTX_RETURN;
}

//for pushing configs out to nodes if no shared storage exists for the
//	purpose (and/or there is no remote service or concept of file
//	based configs
//
//NOTE uuid = NULL translates as "push all configs to node"
Eina_Bool _configPush(Zentific_Ctx *ctx, const char *uuid __UNUSED__){
#if 0
	Eina_Bool ret 	= EINA_FALSE;
	const char *push 	= NULL;
	VM *vm		= NULL;


	if(uuid)
		vm = _getVM(ctx, uuid);

	// export config to nodes if config-push is enabled.
	// 	if disabled then nodes probably have some sort of
	// 	shared storage set up to the zentific server where
	// 	configs are located centrally

	push = _getConfig(ctx, "config_push");

	/* stringshare */
	if (zshare_strcasecmp(push, "yes") == 0){
		//insert scp $localvmconfig.conf nodeaddr:$configpath
		Eina_List *nodes;
		NODE *n;

		nodes = _getNodes(ctx);

		EINA_LIST_FREE(nodes, n){
			const char *address;

			address = _getNodeAddress(ctx, n->uuid);

			//FIXME BROKEN
			//configPush was removed, just use generic addJob to do this
			//db->configPush(db, ctx, address, nodes->data, uuid);
			//

			eina_stringshare_del(address);
			NODE_free(n);
		}
		ret = EINA_TRUE;
	}

	eina_stringshare_del(push);

	if(vm)
		VM_free(vm);
#endif
	ctx->error = AZY_ERR(NOT_IMPLEMENTED);
	CTX_RETURN;
}

//	FIXME
//	maybe _hasVMaccess?
Eina_Bool _isVMowner(Zentific_Ctx *ctx, const char *uuid, int uid){
	db->isVMowner(db, ctx, uuid, uid);
	CTX_RETURN;
}

Eina_Bool _getVMprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid){
	db->getPrivilege(db, ctx, uuid, uid, gid, 1);
	CTX_RETURN;
}

Eina_Bool _grantVMprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int level){
	db->grantPrivilege(db, ctx, uuid, uid, gid, level, 1);
	CTX_RETURN;
}

Eina_Bool _revokeVMprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid){
	db->revokePrivilege(db, ctx, uuid, uid, gid, 1);
	CTX_RETURN;
}

Eina_Bool _getAllUsersByVm(Zentific_Ctx *ctx, const char *uuid){
	db->getAllUsersByVm(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getAllUserGroupsByVm(Zentific_Ctx *ctx __UNUSED__, const char *uuid __UNUSED__){
//warning fixme, not implemented
//	db->getAllUserGroupsByVm(db, ctx, uuid);
	ctx->error = AZY_ERR(NOT_IMPLEMENTED);
	CTX_RETURN;
}

Eina_Bool _isVM(Zentific_Ctx *ctx, const char *uuid){
	if (!zshare_validate_uuid(uuid))
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->isVM(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _setVmNeedsReboot(Zentific_Ctx *ctx, const char *uuid, Eina_Bool needreboot){
	db->setVmNeedsReboot(db, ctx, uuid, needreboot);
	CTX_RETURN;
}

Eina_Bool _needsReboot(Zentific_Ctx *ctx, const char *uuid){
	db->needsReboot(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getVMparentNode(Zentific_Ctx *ctx, const char *uuid){
	db->getVMparentNode(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getVMparentAddress(Zentific_Ctx *ctx, const char *uuid){
	db->getVMparentAddress(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _saveVmNotes(Zentific_Ctx *ctx, const char *uuid, const char *notes){
	db->saveVmNotes(db, ctx, uuid, notes);
	CTX_RETURN;
}

Eina_Bool _getVmNotes(Zentific_Ctx *ctx, const char *uuid){
	db->getVmNotes(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getVMstat(Zentific_Ctx *ctx, const char *uuid, const char *stat){
	db->getVMstat(db, ctx, uuid, stat);
	CTX_RETURN;
}

Eina_Bool _getVMattribute(Zentific_Ctx *ctx, const char *uuid, const char *attribute){
	db->getVMattribute(db, ctx, uuid, attribute);
	CTX_RETURN;
}

Eina_Bool _saveVMrevision(Zentific_Ctx *ctx, const char *uuid, int uid, const char *name, const char *description){
	db->saveVMrevision(db, ctx, uuid, uid, name, description);
	CTX_RETURN;
}

Eina_Bool _getAllVmsByNode(Zentific_Ctx *ctx, const char *uuid){
	db->getAllVmsByNode(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getVmsByNetwork(Zentific_Ctx *ctx, int nwid){
	db->getVmsByNetwork(db, ctx, nwid);
	CTX_RETURN;
}

static void
vm_cb_getvm(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	VM *vm;

	DBG("(ctx=%p, count=%i, pcount=%i)", ctx, ctx->count, pctx->count);
	switch (ctx->count){
		case 0:
			db->getVM(db, ctx, ctx->data);
			CTX_VALUE_JOB(vm_cb_getvm);
			break;
		case 1:
			vm = CTX_PARAM_GET;
			if ((!vm) || ctx->error){
				if (vm) VM_free(vm);
				break;
			}
			pctx->retval = eina_list_append(pctx->retval, vm);
			goto out;
		default:
			break;
	}
	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	eina_stringshare_del(ctx->data);
	free(ctx);
	if (--pctx->count == 2){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

static void
vm_cb_getallvmsfull(Zentific_Ctx *ctx){
	Eina_List *uuids, *ret;
	const char *u;
	Zentific_Ctx *zctx;

	DBG("(ctx=%p, count=%i)", ctx, ctx->count);
	switch (ctx->count){
		case 1:
			uuids = CTX_PARAM_GET;
			if (ctx->error){
				EINA_LIST_FREE(uuids, u)
					eina_stringshare_del(u);
				break;
			} else if (!uuids) {
				AZY_RETVAL(NULL);
				RESUME;
				break;
			}
			ctx->count += eina_list_count(uuids);
			EINA_LIST_FREE(uuids, u){
				zctx = calloc(1, sizeof(Zentific_Ctx));
				zctx->params = eina_hash_string_djb2_new(NULL);
				zctx->parent = ctx;
				zctx->data = (void*)u;
				ecore_job_add((Ecore_Cb)vm_cb_getvm, zctx);
			}
			ctx->count++;
			break;
		case 2:
			ret = ctx->retval;
			ctx->retval = NULL;
			AZY_RETVAL(ret);
			Array_VM_free(ret);
		default:
			break;
	}
}

Eina_Bool _getVmsByNetworkFull(Zentific_Ctx *ctx, int nwid) {
	if (!ctx->count){
		_getVmsByNetwork(ctx, nwid);
		CTX_VALUE_SEND(vm_cb_getallvmsfull);
	} else
		vm_cb_getallvmsfull(ctx);
	CTX_RETURN;
}

Eina_Bool _getAllVmsFullByNode(Zentific_Ctx *ctx, const char *uuid){
	if (!ctx->count){
		db->getAllVmsByNode(db, ctx, uuid);
		CTX_VALUE_SEND(vm_cb_getallvmsfull);
	} else
		vm_cb_getallvmsfull(ctx);
	CTX_RETURN;
}

// functions to get VM from db and return a VM type
Eina_Bool _getVMs(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	/* keep sortfield at a reasonable length */
	if(sortfield){
		if (eina_strlen_bounded(sortfield, 26) > 25)
			ctx->error = AZY_ERR(INVALID_PARAMETER);
		else
			db->getVMsSorted(db, ctx, sortfield, desc, offset, recordcount);
	} else
		db->getVMs(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getVMsUID(Zentific_Ctx *ctx, int uid, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	/* keep sortfield at a reasonable length */
	if(sortfield){
		if (eina_strlen_bounded(sortfield, 26) > 25)
			ctx->error = AZY_ERR(INVALID_PARAMETER);
		else
			db->getVMsByUIDSorted(db, ctx, uid, sortfield, desc, offset, recordcount);
	} else
		db->getVMsByUID(db, ctx, uid, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getVM(Zentific_Ctx *ctx, const char *uuid){
	db->getVM(db, ctx, uuid);
	CTX_RETURN;
	/* __adoptOrphanedVM(ctx, uuid); */
}

Eina_Bool _getVMsFullUID(Zentific_Ctx *ctx, int uid, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (!ctx->count){
		_getVMsUID(ctx, uid, sortfield, desc, offset, recordcount);
		CTX_VALUE_SEND(vm_cb_getallvmsfull);
	} else
		vm_cb_getallvmsfull(ctx);
	CTX_RETURN;
}

Eina_Bool _getVMsFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (!ctx->count){
		_getVMs(ctx, sortfield, desc, offset, recordcount);
		CTX_VALUE_SEND(vm_cb_getallvmsfull);
	} else
		vm_cb_getallvmsfull(ctx);
	CTX_RETURN;
}


Eina_Bool _getVmByName(Zentific_Ctx *ctx, const char *name){
	db->getVmByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _getVmByRev(Zentific_Ctx *ctx, const char *uuid, int revision){
	if (revision < 0)
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->getVmByRev(db, ctx, uuid, revision);
	CTX_RETURN;
}


void
vm_cb_update(Zentific_Ctx *ctx){
	VM *stored = NULL, *vm = ctx->retval;
	Zentific_Ctx *zctx, *pctx = ctx->parent;
	intptr_t *data;

	switch (ctx->count){
		case 0:
			if (!zshare_validate_uuid(vm->uuid)) {
				//FIXME throw error? log definitely
				ERR("%s is not a valid uuid! Ignoring update", vm->uuid);
				goto out;
			}
			db->createVM(db, ctx, vm->uuid);
			CTX_BOOL_JOB(vm_cb_update);
			break;
		case 1:
			if (ctx->error == AZY_ERR(ENTITY_ALREADY_EXISTS)){
				ctx->count--;
				ctx->error = 0;
				ctx->checked = EINA_TRUE;
				_handleVmStateChange(ctx, vm->uuid, vm->state);
				break;
			}
			//skip all of this if the machine is migrating;
			//	we'll catch it after things have finished
			//FIXME note, having a continue here could  cause failures if the vm was *just*
			//	observed for the first time and has no config
			//	---make the getVM stuff more resilient
			if (strstr(vm->name, "-migrating")){
				INF("Ignoring migrating vm '%s'", vm->name);
				goto out;
			}
			db->updateVMparentNode(db, ctx, vm->uuid, vm->puuid);
			CTX_BOOL_JOB(vm_cb_update);
			break;
		case 2:
			ctx->error = 0;
			if (ctx->checked){
				db->isVmIgnored(db, ctx, vm->uuid);
				CTX_BOOL_JOB(vm_cb_update);
				break;
			}
		case 3:
			if (ctx->checked){
				data = CTX_PARAM_GET;
			/* vm is ignored */
		//continue if state change observed/acted upon,
		//	because sometimes data inconsistency arises
		//	on the edges of existence
		//FIXME what if _handleVmStateChange requires the information
		//	newly configured JUST prior to state change?
		//FIXME note, having a continue here causes failures if the vm was *just*
		//	observed for the first time and has no config
		//if (statehandled)
		//	continue;
				if (data) goto out;
				db->getVM(db, ctx, vm->uuid);
				CTX_VALUE_JOB(vm_cb_update);
				break;
			} else
				ctx->count++;
		case 4:
			if (ctx->checked) stored = CTX_PARAM_GET;
			if (VM_config_cmp(vm, stored)){
				db->updateVmConfig(db, ctx, vm);
				ctx->auth = EINA_TRUE;
				CTX_BOOL_JOB(vm_cb_update);
			} else
				ctx->count++;
			if (stored) VM_free(stored);
			if (ctx->count != 5) break;
		case 5:
			ctx->error = 0;
			if (!ctx->ret){
				if (ctx->checked){
					db->pruneVmStats(db, ctx, vm->uuid);
					CTX_CB_ADD(NULL);
				}
				db->updateVmStats(db, ctx, vm);
				CTX_BOOL_JOB(vm_cb_update);
				break;
			}
		case 6:
			ctx->error = 0;
			if (vm->disks){
				zctx = calloc(1, sizeof(Zentific_Ctx));
				zctx->params = eina_hash_string_djb2_new(NULL);
				zctx->parent = ctx;
				zctx->retval = vm;
				zctx->cb = (Ecore_Cb)vm_cb_update;
				if (ctx->checked) db->getVMdisks(db, zctx, vm->uuid);
				zctx->cbs = eina_list_append(zctx->cbs, cb_pointer_job);
				eina_hash_add(zctx->params, "cb_pointer_job", disk_cb_updateconfig);
				ctx->count++;
			}
			if (vm->vifs){
				zctx = calloc(1, sizeof(Zentific_Ctx));
				zctx->params = eina_hash_string_djb2_new(NULL);
				zctx->parent = ctx;
				zctx->retval = vm;
				zctx->cb = (Ecore_Cb)vm_cb_update;
				if (ctx->checked) db->getVMvifs(db, zctx, vm->uuid);
				zctx->cbs = eina_list_append(zctx->cbs, cb_pointer_job);
				eina_hash_add(zctx->params, "cb_pointer_job", vif_cb_updateconfig);
				ctx->count++;
			}
			ctx->count++;
			if (ctx->count != 7) break;
		case 7:
			ctx->error = 0;
			if (ctx->auth){
				if (ctx->ret)
					db->saveVMrevision(db, ctx, vm->uuid, (server_module_data_get(pctx->parent))->session->uid, "zrpc", "Saved vm.");
				else
					db->saveVMrevision(db, ctx, vm->uuid, -1, "zrpc",
						"Automatically generated from zpoll data received by zrpc daemon.");
				CTX_BOOL_JOB(vm_cb_update);
				break;
			}
		default:
			goto out;
	}
	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	free(ctx);
	if (--pctx->count == 5){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

//FIXME change name please? done. but consistent
//	capitalization would be nice too
Eina_Bool _updateVMs(Zentific_Ctx *ctx, const char *uuid, Eina_List *vms){
 	VM *vm;
 	const char *mod;
 	Zentific_Ctx **vctx;
 	unsigned int i;

	switch (ctx->count) {
		case 0:
			db->getNodeControlModule(db, ctx, uuid);
			CTX_VALUE_RETURN;
			break;
		case 1:
			if (ctx->error) break;
			mod = CTX_PARAM_GET;
			if ((!mod) || (!mod[0])){
				ctx->error = AZY_ERR(NODE_UNCONFIGURED);
				break;
			}
			eina_stringshare_del(mod);
			/* we use the divide+conquer method here, splitting ctxes and running lots of queries */
			vctx = calloc(eina_list_count(vms), sizeof(Zentific_Ctx*));
			for (i = 0; i < eina_list_count(vms); i++){
				vctx[i] = calloc(1, sizeof(Zentific_Ctx));
				vctx[i]->params = eina_hash_string_djb2_new(NULL);
				vctx[i]->parent = ctx;
				vctx[i]->retval = vm = eina_list_nth(vms, i);
				eina_stringshare_replace(&vm->puuid, uuid);
				ecore_job_add((Ecore_Cb)vm_cb_update, vctx[i]);
			}
			ctx->count += eina_list_count(vms) + 4; /* will stop on 5 */
			break;
		default:
			AZY_RETVAL(EINA_TRUE);
			RESUME;
			break;
/*
	new vm : uuid, owned by admin, member of group 'unassigned'
	needs to update ownership and groupmembership tables
	name, type, mem, maxmem, uuid, type, vnc, vncport, vncpasswd, vcpus, kernel, ramdisk
	FIXME need to be collected: pae, vnc, sdl, sound, boot, device_model, pvgrub, studomain,
		backend, extra, root, usbdevice, keymap, vscsi, cpuid, usb, localtime,
		rtc_timeoffset, serial, nographic, stdvga, oncrash, onshutdown, onreboot,
		cpus (esp important, limits upon what physical cpus the vcpu can execute), fda
	FIXME somewhat deprecated options, but may be worth checking:
		dhcp, ip, hostname, cdrom, mac, name, bridge, ip, backend, script, type, model
	FIXME add model to poller collection

	NOTE: mem/maxmem/vcpus/vncport duplicated here and in vmconfig because if temporary change occurs during runtime
 */


	}

	CTX_RETURN;
}

Eina_Bool _isVmIgnored(Zentific_Ctx *ctx, const char *uuid){
	db->isVmIgnored(db, ctx, uuid);
	CTX_RETURN;
}

// TODO #warning implement remove-vm job, ram grow/shrink job, split saveVM into modifyVm and saveVmTemplateConfig
Eina_Bool _removeVm(Zentific_Ctx *ctx, const char *uuid, Eina_Bool setInactive, Eina_Bool removeDisks __UNUSED__){
	//mark as inactive? or remove db data? or remove disks only and mark inactive?
	if (setInactive)
		db->ignoreVm(db, ctx, uuid);
	else
		db->removeVm(db, ctx, uuid);
	CTX_RETURN;
#if 0
	if (removeDisks && setInactive){
		/* FIXME set remove disks */
	}
#endif
}

// #warning fix me: addVm with automatic node determination (partly here already)
//JOB * addVmToAutoNode(char *puuid,
//FIXME access privileges
Eina_Bool _addVmToNode(Zentific_Ctx *ctx, const char *puuid, VM *vm, int caller_uid){
	const char *mod;
	NODE_STATS *stats;
	Zentific_Ctx *vctx;
	JOB *job;
	switch (ctx->count){
		case 0:
			if(!puuid || !vm || caller_uid < 0 ){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				CTX_RETURN;
			}

			if(!zshare_validate_uuid(vm->uuid)){
				eina_stringshare_replace(&vm->uuid, azy_uuid_new());
				INF("new vm lacks uuid; generated uuid='%s'", vm->uuid);
			}
			eina_stringshare_replace(&vm->puuid, puuid);
			_isVM(ctx, vm->uuid);
			CTX_INT_RETURN;
			break;
		case 1:
			if (!ctx->error){
				ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
				break;
			}
			ctx->error = 0;
			_getNodeControlModule(ctx, puuid);
			CTX_VALUE_RETURN;
			break;
		case 2:
			if (ctx->error) break;
			mod = CTX_PARAM_GET;
			if ((!mod) || (!mod[0])){
				ctx->error = AZY_ERR(NODE_UNCONFIGURED);
				break;
			}
			ctx->retval = job = JOB_new();
			job->module_name = mod;
			_getLatestNodeStat(ctx, puuid);
			CTX_VALUE_RETURN;
			break;
		case 3:
			stats = CTX_PARAM_GET;
			if (!stats) {
				ctx->error = AZY_ERR(DB_FAIL);
				break;
			} else if (ctx->error) {
				NODE_STATS_free(stats);
				break;
			}
			/* TODO FIXME
			 *
			 *	Verify that the user/role requesting a new VM
			 *		is permitted to request a VM, amount
			 *		of storage/ram, etc, has access privileges
			 *
			 */


			/* Confirmation that sufficent resources exist to support the requested VM occurs below.
			 */
			if ((stats->total_phys_mem <= vm->mem) || (stats->free_phys_mem <= vm->mem)){
				WRN("node='%s' having total_phys_mem='%f' and free_phys_mem='%f'"
					" cannot accomodate new vm='%s' with mem='%d'",
					puuid, stats->total_phys_mem, stats->free_phys_mem, vm->uuid, vm->mem);
					ctx->error = AZY_ERR(INSUFFICIENT_RESOURCES);
					NODE_STATS_free(stats);
					break;
			}
			NODE_STATS_free(stats);
	/*
	 * initial db setup..assumes we can tell the virt platform the uuid we want
	 *	rather than asking for one from it..
	 *
	 *	in that scenario, we can probably use this uuid temporarily and then update
	 *	the vm's reference as needed. bridge later to be crossed, but hopefully not
	 */

			_createVM(ctx, vm->uuid);
			CTX_INT_RETURN;
			break;
		case 4:
			if (ctx->error) break;
			vctx = calloc(1, sizeof(Zentific_Ctx));
			vctx->params = eina_hash_string_djb2_new(NULL);
			vctx->parent = ctx;
			vctx->retval = vm;
			vctx->ret = EINA_TRUE;
			ecore_job_add((Ecore_Cb)vm_cb_update, vctx);
			ctx->count += 2; /* stop on 5 */
			break;
		case 5:
			job = ctx->retval;
			job->target_node	= eina_stringshare_ref(vm->puuid);
			job->target_vm		= eina_stringshare_ref(vm->uuid);
			job->module_function	= eina_stringshare_add("addVm");
			job->input_string	= eina_stringshare_printf("uuid=%s,puuid=%s", vm->uuid, puuid);
			job->extra		= eina_stringshare_add("");
			_getVMparentAddress(ctx, vm->uuid);
			CTX_CB_ADD(cb_job_target_host);
			CTX_PARAM_ADD(cb_job_target_host, job);
			break;
		case 6:
			job_add(ctx, ctx->retval, SES->uid);
			JOB_free(ctx->retval);
		default:
			break;

	}

	CTX_RETURN;
}

/* VM control functions */

/* FIXME
 *
 *	MUST have representation in code of virt-platform-specific
 *		job choices. look up uuid. is it a node? is it a domU?
 *		a disk? etc. upon what platform? check the platform
 *		table for action->job mapping and perform
 *		job insertion on those grounds
 *
 */
Eina_Bool vmAction(Zentific_Ctx *ctx, const char *uuid, int uid, const char *module_function, const char *input_string, const char *extra){
/* used by storage_cb_vmjob in storage.c */
/*
#warning revisit this
tighten this function, it's still very rough around the edges

 find node
 use node unknown error
 find platform control method
 make sure it exists
 use platfform uknown error
 make sure scheduler is running
 add scheduler-not-running error
 add scheduler-could-not-be-contacted error (kill(0) to check if pid is present
 use vm unknown error
 lookup control method module
 make sure it exists
 throw control module not present error
 add job with correct platform, control module, node, vm, job type
 log EVERYTHING (that doesnt hit the jobs table)
		 user, node uuid, vm uuid, job, platform, job platform module, job name

*/
	JOB *job = NULL;
	switch (ctx->count){
		case 0:
			ctx->retval = job = JOB_new();
			job->uid = uid;
			job->target_vm		= eina_stringshare_ref(uuid);
			job->module_function	= eina_stringshare_add(module_function);
			job->input_string	= eina_stringshare_add(input_string);
			job->extra		= eina_stringshare_add(extra);
			db->getVMparentNodeAndAddress(db, ctx, uuid);
			CTX_CB_ADD(cb_job_target_node_addr);
			CTX_PARAM_ADD(cb_job_target_node_addr, job);
			break;
		case 1:
			if (ctx->error) break;
			job = ctx->retval;
			_getNodeControlModule(ctx, job->target_node);
			CTX_CB_ADD(cb_job_module_name);
			CTX_PARAM_ADD(cb_job_module_name, job);
			break;
		case 2:
//FIXME for 'start vm on host X' we need node adddress stuff HERE rather than in the db->action
//	function
			job_add(ctx, ctx->retval, SES->uid);
			JOB_free(ctx->retval);
		default:
			break;
	}
	if (ctx->error){
		WRN("Action '%s' for vm '%s' could not be initiated; job creation failed: '%s'",
			module_function, uuid, eina_error_msg_get(ctx->error));
		ctx->error = AZY_ERR(JOB_CREATION_FAILED);
	}
	CTX_RETURN;
}

Eina_Bool _poweronVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid, Eina_Bool paused){

	if(paused)
		return vmAction(ctx, uuid, caller_uid, "powerOnPaused", "", "");

	return vmAction(ctx, uuid, caller_uid, "powerOn", "", "");
}

Eina_Bool _poweroffVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid){
	return vmAction(ctx, uuid, caller_uid, "powerOff", "", "");
}

Eina_Bool _shutdownVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid){
	return vmAction(ctx, uuid, caller_uid, "shutdown", "", "");
}

Eina_Bool _rebootVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid){
	return vmAction(ctx, uuid, caller_uid, "reboot", "", "");
}

Eina_Bool _pauseVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid){
	return vmAction(ctx, uuid, caller_uid, "pause", "", "");
}

Eina_Bool _disableStateHandlingVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid){
	return vmAction(ctx, uuid, caller_uid, "disablestatehandling", "", "");
}

Eina_Bool _enableStateHandlingVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid){
	return vmAction(ctx, uuid, caller_uid, "enablestatehandling", "", "");
}

Eina_Bool _unpauseVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid){
	return vmAction(ctx, uuid, caller_uid, "unpause", "", "");
}

Eina_Bool _suspendVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid, const char *destination){

	return vmAction(ctx, uuid, caller_uid, "suspend",
			((destination) ? destination : ""),
			"");
}

Eina_Bool _snapshotVM(Zentific_Ctx *ctx, const char *uuid __UNUSED__, int caller_uid __UNUSED__, int storid __UNUSED__, Eina_Bool memoryOnly __UNUSED__){

	/* TODO FIXME
 	 * must check caller's ability, platform's ability to make snapshots.
	 *	'snapshot destination' should be pre-configured
	 *	need to check storage backend's free space, etc
	 */

	ctx->error = AZY_ERR(NOT_IMPLEMENTED);
	CTX_RETURN;
#if 0
	return vmAction(ctx, uuid, caller_uid, "snaphot",
			"/path/to/snapshot/destination",
			((memoryOnly) ? "memoryOnly" : ""));
#endif
}

Eina_Bool _restoreVM(Zentific_Ctx *ctx, const char *uuid __UNUSED__, int caller_uid __UNUSED__, const char *source __UNUSED__){

	/* TODO FIXME
 	 * must implement complementary 'list snapshots, get snapshot' functions
	 */
	ctx->error = AZY_ERR(NOT_IMPLEMENTED);
	CTX_RETURN;
#if 0
	return vmAction(ctx, uuid, caller_uid, "restore",
			((source) ? source : ""),
			"");
#endif
}

/* TODO default to live migration or allow a choice?
 * 	To support all platforms, might want to change
 * 	 terminology to: hot, warm, cold.
 *
 * 	 "cold" would be for infrastructures not capable of standard migration,
 *	 i.e., shared storage etc.
 *
 *	 cold would imply:
 *	 	1) shut down or memory snapshot vm
 *	 	2) copy disk (and if exists, snapshot) to destination
 *	 	3) restore/boot vm
 *
 */
Eina_Bool _migrateVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid, const char *destination){
	return vmAction(ctx, uuid, caller_uid, "migrate",
			((destination) ? destination : ""),
		 	"");
}

//#warning fixme: set reboot flag and/or issue job to modify vm. change return type to JOB *
Eina_Bool _saveVM(Zentific_Ctx *ctx, VM *vm, int uid __UNUSED__, Eina_Bool astemplate){
	TEMPLATE *template = NULL;
	Zentific_Ctx *vctx;
	intptr_t *i;

	switch (ctx->count){
		if(astemplate){
		case 0:
			ctx->retval = template = TEMPLATE_new();
			template->name 		= eina_stringshare_ref(vm->name);
			template->description	= eina_stringshare_add("Templated generated via saveVM");

			// TODO: consider whether to keep or drop template->type
			template->type		= eina_stringshare_ref(vm->type);

			//FIXME change to vm->architecture as soon as possible
			template->architecture	= eina_stringshare_add("64");
			template->uuid		= azy_uuid_new();

			_getPlatformIdByVm(ctx, vm->uuid);
			CTX_INT_RETURN;
			break;
		case 1:
			i = CTX_PARAM_GET;
			template = ctx->retval;
			template->platform = i ? *i : 0;
			db->createVmTemplate(db, ctx, template->uuid);
			CTX_INT_RETURN;
			break;
		case 2:
			_addTemplate(ctx, template);
			ctx->count++;
			CTX_INT_RETURN;
			eina_stringshare_replace(&vm->uuid, template->uuid);
			TEMPLATE_free(template);
		}
		case 4:
			vctx = calloc(1, sizeof(Zentific_Ctx));
			vctx->params = eina_hash_string_djb2_new(NULL);
			vctx->parent = ctx;
			vctx->retval = vm;
			vctx->ret = EINA_TRUE;
			ecore_job_add((Ecore_Cb)vm_cb_update, vctx);
			ctx += 2;
			break;
		case 5:
			AZY_RETVAL(!ctx->error);
			RESUME;
		default:
			break;
	}

	//TODO warning handle vm->needsreboot or update-on-the-fly
	//FIXME set 'needs reboot' or fire a job
	CTX_RETURN;
}

Eina_Bool _getAllTemplates(Zentific_Ctx *ctx){
	db->getAllTemplates(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getAllTemplatesFull(Zentific_Ctx *ctx){
	db->getAllTemplatesFull(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getTemplate(Zentific_Ctx *ctx, int id){
	db->getTemplate(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _getTemplateByName(Zentific_Ctx *ctx, const char *name){
	db->getTemplateByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _getTemplateConfigByName(Zentific_Ctx *ctx, const char *name){
	db->getTemplateConfigByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _getTemplateConfig(Zentific_Ctx *ctx, int id){
	db->getTemplateConfig(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _ztoolsUpdate(Zentific_Ctx *ctx, const char *uuid, Eina_List *stats){
	if (!ctx->state) _isNodeEnabledAndConfigured(ctx, uuid);
	else db->ztoolsUpdate(db, ctx, uuid, stats);
	CTX_RETURN;
}

Eina_Bool _hasGuestTools(Zentific_Ctx *ctx, const char *uuid){
	db->hasGuestTools(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _addTemplate(Zentific_Ctx *ctx, TEMPLATE *template){
	db->addTemplate(db, ctx, template);
	CTX_RETURN;
}

Eina_Bool _modifyTemplate(Zentific_Ctx *ctx, int id, TEMPLATE *template){
	db->modifyTemplate(db, ctx, id, template);
	CTX_RETURN;
}

Eina_Bool _getVmCount(Zentific_Ctx *ctx){
	if (!ctx->user_type)
		db->getVmCount(db, ctx);
	else
		db->getVmCountByUID(db, ctx, SES->uid);
	CTX_RETURN;
}
