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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 */
#include "functions.h"
#include "functions-storage.h"
#include "functions-node.h"
#include "functions-jobs.h"

/* Convenience function for updateDiskConfig. only required because
 * VM struct contains both config and stats data right now.
 * will be removed when that design flaw has been corrected
 */
static int
DISK_config_cmp(DISK *a, DISK *b){

	if (a == b)
		return 0;
	if ((!a) || (!b))
		return 1;
	if (a->int_dev != b->int_dev)
		return 1;
	if (a->ext_dev !=b->ext_dev)
		return 1;
	if (a->mode !=b->mode)
		return 1;
	if (a->type !=b->type)
		return 1;
	if (a->partition_type !=b->partition_type)
		return 1;
	if (a->mapped_dev !=b->mapped_dev)
		return 1;
	if (a->num_sectors != b->num_sectors)
		return 1;
	if (a->size_sector != b->size_sector)
		return 1;
	if (a->start_sector != b->start_sector)
		return 1;
	if (a->capacity != b->capacity)
		return 1;
	if (a->block_size != b->block_size)
		return 1;

	return 0;
}


static JOB *
storage_job_init(int uid, const char *module_function, const char *input_string, const char *extra){
	JOB *ret;

	ret =  JOB_new();

	ret->uid		= uid;
	ret->module_function	= eina_stringshare_add(module_function ? module_function : "");
	ret->input_string	= input_string;
	ret->extra		= eina_stringshare_add(extra ? extra : "");
	return ret;
}


static void
storage_job_init_node(Zentific_Ctx *ctx, const char *cb, const char *uuid, int uid, const char *module_function, const char *input_string, const char *extra){
	JOB *ret;

	ret = storage_job_init(uid, module_function, input_string, extra);
	ret->target_vm = eina_stringshare_add("");
	ret->target_node = eina_stringshare_add(uuid);
	_getNodeHostname(ctx, uuid);
	CTX_CB_ADD(cb_job_target_node_addr);
	CTX_PARAM_ADD(cb_job_target_node_addr, ret);
	CTX_PARAM_ADD_FROM_VARIABLE(cb, ret);
}

static void
storage_cb_nodejob(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	JOB *node, *job = ctx->retval;
	const char *name;
	intptr_t *jid;

	switch (ctx->count){
		case 0:
			db->getNodeAddress(db, ctx, job->target_node);
			CTX_VALUE_JOB(storage_cb_nodejob);
			break;
		case 1:
			job->target_host = CTX_PARAM_GET;
			db->getNodeControlModule(db, ctx, job->target_node);
			CTX_VALUE_JOB(storage_cb_nodejob);
		case 2:
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			node = JOB_new();
			node->uid = (server_module_data_get(pctx->parent))->session->uid;
			node->target_node = eina_stringshare_ref(job->target_node);
			node->target_vm = eina_stringshare_add("");
			node->module_function = eina_stringshare_ref(job->module_function);
			node->input_string = eina_stringshare_ref(job->input_string);
			node->extra = eina_stringshare_ref(job->extra);
			node->target_host = eina_stringshare_ref(job->target_host);
			node->module_name = eina_stringshare_add(name[0] ? name : "parentJob");
			db->addJob(db, ctx, node);
			CTX_INT_JOB(storage_cb_nodejob);
			JOB_free(node);
			break;
		default:
			jid = CTX_PARAM_GET;
			if (ctx->error) break;
			job->dependencies = eina_list_append(job->dependencies, jid);
			goto out;
	}


	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	free(ctx);
	if (--pctx->count == 4){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

static void
storage_cb_setup_storage(Zentific_Ctx *ctx, const char *action, const char *uuid, int storid, int caller_uid){
	STORAGE *stor;
	JOB *ret;
	Zentific_Ctx *zctx;
	const char *name;

	switch (ctx->count){
		case 0:
			if (ctx->error) break;
			_isNODE(ctx, uuid);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			_getStorage(ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 2:
			ctx->retval = stor = CTX_PARAM_GET;
			if (ctx->error){
				if (stor) STORAGE_free(stor);
				break;
			}
			if (ctx->error) break;
			if (!zshare_strcmp(action, "attachStorage"))
				db->attachStorageToNode(db, ctx, uuid, storid);
			else if (!zshare_strcmp(action, "detachStorage"))
				db->detachStorageFromNode(db, ctx, uuid, storid);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 3:
			if (ctx->error){
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				STORAGE_free(ctx->retval);
				break;
			}
			stor = ctx->retval;
			ctx->retval = NULL;
			if (!zshare_strcmp(action, "attachStorage"))
				storage_job_init_node(ctx, __func__, uuid, caller_uid, action,
					eina_stringshare_printf("path=%s::host=%s::name=%s::type=%s::mode=%s",
					stor->path, stor->host, stor->name, stor->type, stor->mode), "");
			else if (!zshare_strcmp(action, "detachStorage"))
				storage_job_init_node(ctx, __func__, uuid, caller_uid, action,
					eina_stringshare_printf("path=%s::host=%s::name=%s::type=%s::mode=%s",
						stor->path, stor->host, stor->name, stor->type, stor->mode), "");

			STORAGE_free(stor);
			break;
		case 4:
			if (ctx->retval && uuid[0]){
				job_add(ctx, ctx->retval, caller_uid);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				break;
			}
			ctx->retval = CTX_PARAM_GET;
			db->getControlModuleNameByStorage(db, ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 5:
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ret = ctx->retval;
			ret->module_name = name;
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->params = eina_hash_string_djb2_new(NULL);
			zctx->parent = ctx;
			zctx->retval = ret;
			ecore_job_add((Ecore_Cb)storage_cb_nodejob, zctx);
		default:
			break;
	}
}

Eina_Bool _isStorage(Zentific_Ctx *ctx, STORAGE *stor){
	db->isStorage(db, ctx, stor);
	CTX_RETURN;
}

Eina_Bool _isStorageId(Zentific_Ctx *ctx, int id){
	db->isStorageId(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _getStorage(Zentific_Ctx *ctx, int id){
	db->getStorage(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _getAllStorageByType(Zentific_Ctx *ctx, const char *type, int offset, int recordcount){
	db->getAllStorageByType(db, ctx, type, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllStorage(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield)
		db->getAllStorageSorted(db, ctx, sortfield, desc, offset, recordcount);
	else
		db->getAllStorage(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllStorageFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield)
		db->getAllStorageFullSorted(db, ctx, sortfield, desc, offset, recordcount);
	else
		db->getAllStorageFull(db, ctx, offset, recordcount);
	CTX_RETURN;
}

//#warning implement addStorage
static void
storage_cb_jid_collect(Zentific_Ctx *ctx, int ret){
	Eina_List *jids;
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	jids = ctx->retval;
	if (ctx->count == 10){
		/* list init */
		jids = eina_list_append(NULL, (intptr_t*)&ret);
	}
	jids = eina_list_append(jids, (intptr_t*)&ret);
	if (ctx->count == eina_list_count(jids) + 10){
		ctx->count = 6;
		ctx->retval = jids;
		REWIND;
		RESUME;
	}

}

Eina_Bool _defineStorage(Zentific_Ctx *ctx, STORAGE *stor){
	switch (ctx->count){
		case 0:
			_isStorage(ctx, stor);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (ctx->error){
				ctx->error = 0;
				// ignore whatever was passed in for a control module
				stor->mid = -1;
				db->defineStorage(db, ctx, stor);
				break;
			}
			ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
			WRN("Storage '%s' (%s) already exists",
				stor->name, stor->path);
			break;
	}
	CTX_RETURN;
}

static void
storage_cb_vmjob(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	JOB *vm, *job = ctx->retval;
	const char *name;
	intptr_t *jid;
	Eina_List *ret;
	switch (ctx->count){
		case 0:
			ctx->retval = vm = JOB_new();
			vm->uid = (server_module_data_get(pctx->parent))->session->uid;
			vm->target_vm		= eina_stringshare_ref(job->target_vm);
			vm->module_function	= eina_stringshare_ref(job->module_function);
			vm->input_string	= eina_stringshare_add("");
			vm->extra		= eina_stringshare_add("");
			db->getVMparentNodeAndAddress(db, ctx, vm->target_vm);
			CTX_VALUE_JOB(storage_cb_vmjob);
			break;
		case 1:
			ret = CTX_PARAM_GET;
			if ((!ret) || (eina_list_count(ret) != 2) || ctx->error) {
				EINA_LIST_FREE(ret, name)
					eina_stringshare_del(name);
				JOB_free(ctx->retval);
				break;
			}
			vm = ctx->retval;
			vm->target_node = ret->data;
			vm->target_host = ret->next->data;
			eina_list_free(ret);
			_getNodeControlModule(ctx, vm->target_node);
			CTX_VALUE_JOB(storage_cb_vmjob);
			break;
		case 2:
//FIXME for 'start vm on host X' we need node adddress stuff HERE rather than in the db->action
//	function
			name = CTX_PARAM_GET;
			if (ctx->error) {
				eina_stringshare_del(name);
				JOB_free(ctx->retval);
				break;
			}
			vm = ctx->retval;
			vm->module_name = name;
			db->addJob(db, ctx, ctx->retval);
			JOB_free(ctx->retval);
			CTX_INT_JOB(storage_cb_vmjob);
		default:
			jid = CTX_PARAM_GET;
			if (ctx->error) break;
			job->dependencies = eina_list_append(job->dependencies, jid);
			goto out;
	}
	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	free(ctx);
	if (--pctx->count == 4){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

Eina_Bool _modifyStorage(Zentific_Ctx *ctx, int id, STORAGE *stor, int caller_uid){
	int i = 0;
	JOB *ret, *job;
	const char *uuid, *name;
	STORAGE *existing;
	Eina_List *nodes;
	Zentific_Ctx **zctx;
	switch (ctx->count){
		case 0:
			_getStorage(ctx, id);
			CTX_VALUE_RETURN;
			break;
		case 1:
			existing = CTX_PARAM_GET;
			if (ctx->error || (!existing)){
				if (existing) STORAGE_free(existing);
				break;
			}
			if(STORAGE_eq(stor, existing)){
				ctx->error = AZY_ERR(NO_CHANGE_PERFORMED);
				STORAGE_free(existing);
				break;
			}
			STORAGE_free(existing);
			_getNodesByStorage(ctx, id);
			CTX_VALUE_RETURN;
			break;
		case 2:
			nodes = CTX_PARAM_GET;
			ctx->count = 4 + eina_list_count(nodes);
			zctx = calloc(eina_list_count(nodes), sizeof(Zentific_Ctx));
			EINA_LIST_FREE(nodes, uuid){

				ret = storage_job_init(caller_uid, "modifyNodeStorage",
					eina_stringshare_printf("path=%s::host=%s::mode=%s::name=%s::type=%s::capacity=%d::freespace=%d",
					stor->path, stor->host, stor->mode, stor->name,
					stor->type, stor->capacity, stor->freespace), "");
				ret->target_vm = eina_stringshare_add("");
				ret->target_node = uuid;
				zctx[i]->params = eina_hash_string_djb2_new(NULL);
				zctx[i]->parent = ctx;
				zctx[i]->retval = ret;
				ecore_job_add((Ecore_Cb)storage_cb_nodejob, zctx[i]);
				ctx->retval = eina_list_append(ctx->retval, ret);
				i++;
			}
			break;
		/* NOT A MISTAKE */
		case 4:
			db->getControlModuleNameByStorage(db, ctx, id);
			CTX_VALUE_RETURN;
		case 5:
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				EINA_LIST_FREE(ctx->retval, job)
					JOB_free(job);
				ctx->retval = NULL;
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ctx->count = 10 + eina_list_count(ctx->retval);
			EINA_LIST_FREE(ctx->retval, job){
				if (name[0])
					eina_stringshare_replace(&job->module_name, name);
				else
					job->module_name = eina_stringshare_add("parentJob");
				_addJob(ctx, job, caller_uid);
				CTX_CB_ADD(storage_cb_jid_collect);
				JOB_free(job);
			}
			eina_stringshare_del(name);
			ctx->retval = NULL;
			break;
		case 6:
			db->modifyStorage(db, ctx, id, stor);
			CTX_CB_ADD(cb_int_rewind);
			break;
		case 7:
			ret = JOB_new();
			ret->dependencies = ctx->retval;
			ctx->retval = NULL;
			ret->module_name = eina_stringshare_add("parentJob");
			ret->module_function = eina_stringshare_add("modifyStorage");
			ret->uid = caller_uid;
			job_add(ctx, ret, caller_uid);
			JOB_free(ret);
			break;
		default:
			ERR("something crazy happened here");
	}
	CTX_RETURN;
}

Eina_Bool _getStorageControlModulesFull(Zentific_Ctx *ctx) {
	db->getStorageControlModulesFull(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getStorageControlModules(Zentific_Ctx *ctx){
	/*just list of ints, return directly */
	db->getStorageControlModules(db, ctx);
	CTX_RETURN;
}

Eina_Bool _setStorageControlModule(Zentific_Ctx *ctx, int storid, int mid){
	db->setStorageControlModule(db, ctx, storid, mid);
	CTX_RETURN;
}

Eina_Bool _getStorageControlModule(Zentific_Ctx *ctx, int storid){
	db->getStorageControlModule(db, ctx, storid);
	CTX_RETURN;
}

/* TODO warning
 *
 *	race condition possible here on functions returning JOB
 *		db->attachStorageToNode
 *		do job attachStorageToNode #takes a while to run
 *		client requests data relevant to this relationship (assumine it exists or doesnt exist), job not finished
 *		possible boom and/or unexpected result
 *	so
 *
 * 	for db/job interaction, the most reliable
 * 	mechanism would be set job, scheduler executes job, and
 * 	job itself inserts storage<=>node relationship
 *
 * 	OR
 *
 * 	set job with a specified on-complete job scheduler executes job,
 */
Eina_Bool _getStorageIdByName(Zentific_Ctx *ctx, const char *name){
	db->getStorageIdByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _getStorageByNode(Zentific_Ctx *ctx, const char *uuid){
	db->getStorageByNode(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _removeStorage(Zentific_Ctx *ctx, int id){
	switch (ctx->count){
		case 0:
			/* keep this check here, seems inappropriate for refactoring */
			db->isStorageInUse(db, ctx, id);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (!ctx->error){
				ctx->error = AZY_ERR(RESOURCE_IN_USE);
				WRN("cannot remove storage '%d' because it is currently utilized by one or more resources",
					id);
				break;
			}
			db->removeStorage(db, ctx, id);
	}
	CTX_RETURN;
}

static void
storage_cb_setup_disk(Zentific_Ctx *ctx, const char *action, int storid, const char * path, int caller_uid){
	STORAGE *stor;
	DISK *disk;
	JOB *ret;

	switch (ctx->count){
		case 0:
			if(!path || storid < 0){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			_getStorage(ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 1:
			stor = CTX_PARAM_GET;
			if (ctx->error){
				if (stor) STORAGE_free(stor);
				break;
			}
			ctx->retval = stor;
			_getDiskByStorage(ctx, storid, path);
			CTX_VALUE_RETURN;
			break;
		case 2:
			disk = CTX_PARAM_GET;
			stor = ctx->retval;
			if (ctx->error){
				STORAGE_free(stor);
				if (disk) DISK_free(disk);
				break;
			}
			ctx->retval = ret = storage_job_init(caller_uid, action, NULL, "");
			if ((!zshare_strcmp(action, "attachDisk")) || (!zshare_strcmp(action, "enableDisk")) || (!zshare_strcmp(action, "disableDisk"))){
				ret->input_string = eina_stringshare_printf("diskpath=%s::storagepath=%s::storid=%d::shost=%s::mode=%s::internal=%s",
					disk->ext_dev, stor->path, storid, stor->host, disk->mode, disk->int_dev);
				if (stor->host && zshare_validate_ip(stor->host))
					eina_stringshare_replace(&ret->target_host, stor->host);
			} else if (!zshare_strcmp(action, "detachDisk"))
				ret->input_string = eina_stringshare_printf("diskpath=%s::storagepath=%s::storid=%d::shost=%s::mode=%s",
					disk->ext_dev, stor->path, storid, stor->host, disk->mode);
			STORAGE_free(stor);
			DISK_free(disk);
		default:
			break;
	}
}


Eina_Bool _attachDiskToStorage(Zentific_Ctx *ctx, int storid, const char * path, int caller_uid){
	switch (ctx->count){
		case 0:
		case 1:
		case 2:
			storage_cb_setup_disk(ctx, "attachDisk", storid, path, caller_uid);
			if (ctx->count != 2) break;
			db->getControlModuleNameByStorage(db, ctx, storid);
			CTX_CB_ADD(cb_job_module_name);
			break;
		case 3:
			db->attachDisk(db, ctx, storid, path);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (ctx->error){
				ERR("could not record attachment of disk='%s' to stor='%d'", path, storid);
				JOB_free(ctx->retval);
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			job_add(ctx, ctx->retval, caller_uid);
			JOB_free(ctx->retval);
			break;
	}
	CTX_RETURN;
}

Eina_Bool _detachDiskFromStorage(Zentific_Ctx *ctx, int storid, const char *path, int caller_uid){
	JOB *ret;
	const char *uuid;
	Zentific_Ctx *zctx;

	switch (ctx->count){
		case 0:
		case 1:
		case 2:
			storage_cb_setup_disk(ctx, "detachDisk", storid, path, caller_uid);
			if (ctx->count != 2) break;
			_getVmByDisk(ctx, storid, path);
			CTX_VALUE_RETURN;
			break;
		case 3:
			uuid = CTX_PARAM_GET;
			if (ctx->error){
				eina_stringshare_del(uuid);
				JOB_free(ctx->retval);
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ret = ctx->retval;
			ret->target_vm = uuid;
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->parent = ctx;
			zctx->retval = ret;
			zctx->params = eina_hash_string_djb2_new(NULL);
			ecore_job_add((Ecore_Cb)storage_cb_vmjob, zctx);
			ctx->count = 5;
			break;
		case 4:
			db->detachDisk(db, ctx, storid, path);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			job_add(ctx, ctx->retval, caller_uid);
			JOB_free(ctx->retval);
			ctx->retval = NULL;
			break;
	}
	CTX_RETURN;
}

Eina_Bool _addDiskToStorage(Zentific_Ctx *ctx, int storid, DISK *disk, int caller_uid){
	JOB * ret;
	const char *name;

	switch (ctx->count){
		case 0:
			if(!disk){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			_isDiskByStorage(ctx, storid, disk->ext_dev);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			if (!ctx->error){
				ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
				break;
			}
			ctx->error = 0;
			db->getStorageHost(db, ctx, storid);
			CTX_VALUE_RETURN;
		case 2:
			name = CTX_PARAM_GET;
			ctx->retval = ret = storage_job_init(caller_uid, "addDisk",
				eina_stringshare_printf("path=%s::internal=%s::mode=%s::capacity=%f::size_sector=%d::block_size=%d::partition_type=%s::type=%s::storid=%d",
					disk->ext_dev, disk->int_dev, disk->mode, disk->capacity,
					disk->size_sector, disk->block_size, disk->partition_type, disk->type, storid), "");
			if (zshare_validate_ip(name))
				eina_stringshare_replace(&ret->target_host, name);
			db->createDisk(db, ctx, "unassigned", disk->ext_dev, storid);
			CTX_CB_ADD(cb_bool_rewind);
			eina_stringshare_del(name);
			break;
		case 3:
			ctx->error = 0;
			db->addDisk(db, ctx, storid, disk);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 4:
			db->getControlModuleNameByStorage(db, ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 5:
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ret = ctx->retval;
			ret->module_name = name;
			job_add(ctx, ret, caller_uid);
			JOB_free(ret);
		default:
			break;
	}
	CTX_RETURN;
}

Eina_Bool _removeDiskFromStorage(Zentific_Ctx *ctx, int storid, const char * path, int caller_uid){
	JOB *ret;
	DISK *disk;
	STORAGE *stor;
	intptr_t *use;
	const char *name;

	switch (ctx->count){
		case 0:
			if(!path || storid < 0){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			db->isDiskInUse(db, ctx, storid, path);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			if (ctx->error) break; /* UNKNOWN_RESOURCE */
			use = CTX_PARAM_GET;
			if (use != (intptr_t*)-1){
				WRN("cannot remove disk='%s' from storid='%d' because it is currently in use by one or more resources",
					path, storid);
				ctx->error = AZY_ERR(RESOURCE_IN_USE);
				break;
			}
			_getStorage(ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 2:
			ctx->retval = CTX_PARAM_GET;
			if (ctx->error){
				if (ctx->retval) STORAGE_free(ctx->retval);
				break;
			}
			_getDiskByStorage(ctx, storid, path);
			CTX_VALUE_RETURN;
			break;
		case 3:
			stor = ctx->retval;
			disk = CTX_PARAM_GET;
			if (ctx->error){
				STORAGE_free(stor);
				if (disk) DISK_free(disk);
				break;
			}
			ctx->retval = ret = storage_job_init(caller_uid, "removeDisk",
				eina_stringshare_printf("diskpath=%s::storagepath=%s::storid=%d::shost=%s::mode=%s",
					disk->ext_dev, stor->path, storid, stor->host, disk->mode), "");
			if (stor->host && zshare_validate_ip(stor->host))
				eina_stringshare_replace(&ret->target_host, stor->host);
			STORAGE_free(stor);
			DISK_free(disk);
			db->removeDisk(db, ctx, storid, path);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 4:
			db->getControlModuleNameByStorage(db, ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 5:
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ret = ctx->retval;
			ret->module_name = name;
			job_add(ctx, ret, caller_uid);
			JOB_free(ret);
		default:
			break;
	}
	CTX_RETURN;
}

Eina_Bool _modifyDiskByStorage(Zentific_Ctx *ctx, int storid, const char *path, DISK *disk, int caller_uid){
	DISK *stored;
	JOB *ret;
	const char *name;

	switch (ctx->count){
		case 0:
			if(!path || storid < 0){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			_getDiskByStorage(ctx, storid, path);
			CTX_VALUE_RETURN;
			break;
		case 1:
			stored = CTX_PARAM_GET;
			if (ctx->error){
				if (stored) DISK_free(stored);
				break;
			}
			if (!DISK_config_cmp(disk, stored)){
				ctx->error = AZY_ERR(NO_CHANGE_PERFORMED);
				DISK_free(stored);
				break;
			}
			DISK_free(stored);
			ctx->retval = ret = storage_job_init(caller_uid, "modifyDisk",
				eina_stringshare_printf("path=%s::newpath=%s::label=%s::mapped_dev=%s"
					"int_dev=%s::type=%s:::partition_type=%s::mode=%s",
					path, disk->ext_dev, disk->label, disk->mapped_dev,
					disk->int_dev, disk->type, disk->partition_type, disk->mode), "");
			db->modifyDisk(db, ctx, storid, path, disk);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 2:
			db->getControlModuleNameByStorage(db, ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 3:
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ret = ctx->retval;
			ret->module_name = name;
			job_add(ctx, ret, caller_uid);
			JOB_free(ret);
		default:
			break;
	}
	CTX_RETURN;
}

static void
storage_cb_detachdisks(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	JOB *ret, *job = pctx->retval;
	intptr_t *id;
	const char *name, *uuid = ctx->data;
	STORAGE *stor;
	Zentific_Ctx *zctx;

	switch (ctx->count){
		case 0:
			db->isNode(db, ctx, uuid);
			CTX_BOOL_JOB(storage_cb_detachdisks);
			break;
		case 1:
			id = CTX_PARAM_GET;
			db->getStorage(db, ctx, id ? *id : 0);
			CTX_VALUE_JOB(storage_cb_detachdisks);
			break;
		case 2:
			ctx->retval = stor = CTX_PARAM_GET;
			if (ctx->error){
				if (stor) STORAGE_free(stor);
				break;
			}
			if (ctx->error) break;
			db->detachStorageFromNode(db, ctx, uuid, stor->id);
			CTX_BOOL_JOB(storage_cb_detachdisks);
			break;
		case 3:
			if (ctx->error){
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				STORAGE_free(ctx->retval);
				break;
			}
			stor = ctx->retval;
			ctx->retval = NULL;
			ctx->retval = ret = storage_job_init(job->uid, "detachDisk",
				eina_stringshare_printf("path=%s::host=%s::name=%s::type=%s::mode=%s",
					stor->path, stor->host, stor->name, stor->type, stor->mode), "");
			ret->target_vm = eina_stringshare_add("");
			ret->target_node = uuid;
			db->getNodeAddress(db, ctx, uuid);
			CTX_VALUE_JOB(storage_cb_detachdisks);
			ctx->data = stor;
			break;
		case 4:
			ret = ctx->retval;
			if (ret->module_name){
				db->addJob(db, ctx, ctx->retval);
				CTX_INT_JOB(storage_cb_detachdisks);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				ctx->count = 5;
				break;
			}
			name = CTX_PARAM_GET;
			ret->target_host = name;
			stor = ctx->data;
			db->getControlModuleNameByStorage(db, ctx, stor->id);
			CTX_VALUE_JOB(storage_cb_detachdisks);
			STORAGE_free(stor);
			ctx->data = NULL;
			break;
		case 5:
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ret = ctx->retval;
			ret->module_name = name;
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->params = eina_hash_string_djb2_new(NULL);
			zctx->parent = ctx;
			zctx->retval = ret;
			ecore_job_add((Ecore_Cb)storage_cb_nodejob, zctx);
		case 6:
			id = CTX_PARAM_GET;
			job->dependencies = eina_list_append(job->dependencies, id);
			goto out;
		default:
			break;
	}

	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	free(ctx);
	if (--pctx->count == 2){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

Eina_Bool _detachAllNodesByStorage(Zentific_Ctx *ctx, int storid, int caller_uid){
	const char *uuid;
	JOB *ret;
	Eina_List *nodes;
	Zentific_Ctx **zctx;
	int i = 0;

	switch (ctx->count){
		case 0:
			if(storid < 0){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			/* if no nodes are connected to the storage, this is a NO-OP */
			_getNodesByStorage(ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 1:
			nodes = CTX_PARAM_GET;
			if (!nodes){
				ctx->error = AZY_ERR(NO_CHANGE_PERFORMED);
				break;
			}
			ctx->retval = ret = JOB_new();

			ret->module_name = eina_stringshare_add("parentJob");
			ret->module_function = eina_stringshare_add("detachAllNodesByStorage");
			ret->uid = caller_uid;
			ctx->count = 2 + eina_list_count(nodes);
			zctx = calloc(eina_list_count(nodes), sizeof(Zentific_Ctx));
			EINA_LIST_FREE(nodes, uuid){
				zctx[i]->params = eina_hash_string_djb2_new(NULL);
				zctx[i]->parent = ctx;
				zctx[i]->data = (void*)uuid;
				eina_hash_add(zctx[i]->params, "storage_cb_detachdisks", &storid);
				ecore_job_add((Ecore_Cb)storage_cb_detachdisks, zctx[i]);
				i++;
			}
			break;
		case 2:
			job_add(ctx, ctx->retval, caller_uid);
			JOB_free(ctx->retval);
			ctx->retval = NULL;
		default:
			break;
	}
	CTX_RETURN;
}

static void
storage_cb_detachdisksvm(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	JOB *ret, *job = pctx->retval;
	intptr_t *id;
	DISK *disk = ctx->data;
	const char *name;
	STORAGE *stor;
	Zentific_Ctx *zctx;

	switch (ctx->count){
		case 0:
			db->getVmByDisk(db, ctx, (intptr_t)pctx->data, disk->ext_dev);
			CTX_VALUE_JOB(storage_cb_detachdisksvm);
			break;
		case 1:
			ctx->retval = CTX_PARAM_GET;
			db->getStorage(db, ctx, (intptr_t)pctx->data);
			CTX_VALUE_JOB(storage_cb_detachdisksvm);
			break;
		case 2:
			stor = CTX_PARAM_GET;
			ret = storage_job_init(job->uid, "detachDisk",
				eina_stringshare_printf("diskpath=%s::storagepath=%s::mode=%s::host=%s::type=%s",
					disk->ext_dev, stor->path, disk->mode, stor->host, disk->type), "");
			ret->target_vm = ctx->retval;
			db->getVMparentNodeAndAddress(db, ctx, ret->target_vm);
			CTX_VALUE_JOB(storage_cb_detachdisksvm);
			ctx->retval = ret;
			STORAGE_free(stor);
			break;
		case 3:
			ret = ctx->retval;
			Eina_List *n;
			n = CTX_PARAM_GET;
			if ((!n) || (eina_list_count(n) != 2)){
				JOB_free(ret);
				EINA_LIST_FREE(n, name)
					eina_stringshare_del(name);
				goto out;
			}
			ret->target_node = n->data;
			ret->target_host = n->next->data;
			eina_list_free(n);
			db->getControlModuleNameByStorage(db, ctx, (int)((intptr_t*)pctx->data));
			CTX_VALUE_JOB(storage_cb_detachdisksvm);
			break;
		case 4:
			ret = ctx->retval;
			if (ret->module_name){
				db->addJob(db, ctx, ctx->retval);
				CTX_INT_JOB(storage_cb_detachdisksvm);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				break;
			}
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				JOB_free(ctx->retval);
				ctx->retval = NULL;
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ret = ctx->retval;
			ret->module_name = name;
			zctx = calloc(1, sizeof(Zentific_Ctx));
			zctx->params = eina_hash_string_djb2_new(NULL);
			zctx->parent = ctx;
			zctx->retval = ret;
			ecore_job_add((Ecore_Cb)storage_cb_vmjob, zctx);
		case 6:
			id = CTX_PARAM_GET;
			job->dependencies = eina_list_append(job->dependencies, id);
			goto out;
		default:
			break;
	}

	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	DISK_free(ctx->data);
	free(ctx);
	if (--pctx->count == 2){
		azy_server_module_events_rewind(pctx->parent);
		azy_server_module_events_resume(pctx->parent, EINA_TRUE);
	}
}

Eina_Bool _detachAllDisksByStorage(Zentific_Ctx *ctx, int storid, int caller_uid){
	JOB *ret;
	DISK *disk;
	Eina_List * disks;
	Zentific_Ctx **zctx;
	int i = 0;

	switch (ctx->count){
		case 0:
			if(storid < 0){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			/* if no disks are connected to the storage, this is a NO-OP */
			db->getDisksByStorage(db, ctx, storid);
			CTX_VALUE_RETURN;
			break;
		case 1:
			disks = CTX_PARAM_GET;
			if (!disks){
				ctx->error = AZY_ERR(NO_CHANGE_PERFORMED);
				break;
			}
			ctx->retval = ret = JOB_new();
			ctx->data = (intptr_t*)storid;
			ret->module_name = eina_stringshare_add("parentJob");
			ret->module_function = eina_stringshare_add("detachAllDisksByStorage");
			ret->uid = caller_uid;
			ctx->count = 2 + eina_list_count(disks);
			zctx = calloc(eina_list_count(disks), sizeof(Zentific_Ctx));
			EINA_LIST_FREE(disks, disk){
				zctx[i]->params = eina_hash_string_djb2_new(NULL);
				zctx[i]->parent = ctx;
				zctx[i]->data = disk;
				ecore_job_add((Ecore_Cb)storage_cb_detachdisksvm, zctx[i]);
				i++;
			}
			break;
		case 2:
			db->detachAllDisksByStorage(db, ctx, storid);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 3:
			job_add(ctx, ctx->retval, caller_uid);
			JOB_free(ctx->retval);
			ctx->retval = NULL;
		default:
			break;
	}
	CTX_RETURN;
}

Eina_Bool _attachStorageToNode(Zentific_Ctx *ctx, const char * uuid, int storid, int caller_uid){
	storage_cb_setup_storage(ctx, "attachStorage", uuid, storid, caller_uid);
	CTX_RETURN;
}

Eina_Bool _detachStorageFromNode(Zentific_Ctx *ctx, const char * uuid, int storid, int caller_uid){
	storage_cb_setup_storage(ctx, "detachStorage", uuid, storid, caller_uid);
	CTX_RETURN;
}

Eina_Bool _getNodesByStorage(Zentific_Ctx *ctx, int storid){
	db->getNodesByStorage(db, ctx, storid);
	CTX_RETURN;
}

Eina_Bool _getVmsByStorage(Zentific_Ctx *ctx, int storid){
	db->getVmsByStorage(db, ctx, storid);
	CTX_RETURN;
}

Eina_Bool _defineDisk(Zentific_Ctx *ctx, int storid, DISK * disk, int caller_uid __UNUSED__){
	switch (ctx->count){
		case 0:
			if(!disk || storid < 0){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			_isDiskByStorage(ctx, storid, disk->ext_dev);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			if (ctx->error) {
				WRN("Disk '%s' already exists on storid='%d'", disk->ext_dev, storid);
				break;
			}
			db->createDisk(db, ctx, "unassigned", disk->ext_dev, storid);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 2:
			db->addDisk(db, ctx, storid, disk);
			break;
	}
	CTX_RETURN;
}

Eina_Bool _enableDiskByStorage(Zentific_Ctx *ctx, int storid, const char * path, int caller_uid){
	switch (ctx->count){
		case 0:
		case 1:
		case 2:
			storage_cb_setup_disk(ctx, "enableDisk", storid, path, caller_uid);
			if (ctx->count != 2) break;
			db->getControlModuleNameByStorage(db, ctx, storid);
			CTX_CB_ADD(cb_job_module_name);
			break;
		case 3:
			db->enableDisk(db, ctx, storid, path);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (ctx->error){
				ERR("could not record enabling of disk='%s' to stor='%d'", path, storid);
				JOB_free(ctx->retval);
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			job_add(ctx, ctx->retval, caller_uid);
			JOB_free(ctx->retval);
			break;
	}
	CTX_RETURN;
}

Eina_Bool _disableDiskByStorage(Zentific_Ctx *ctx, int storid, const char * path, int caller_uid){
	switch (ctx->count){
		case 0:
		case 1:
		case 2:
			storage_cb_setup_disk(ctx, "disableDisk", storid, path, caller_uid);
			if (ctx->count != 2) break;
			db->getControlModuleNameByStorage(db, ctx, storid);
			CTX_CB_ADD(cb_job_module_name);
			break;
		case 3:
			db->disableDisk(db, ctx, storid, path);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (ctx->error){
				ERR("could not record disabling of disk='%s' to stor='%d'", path, storid);
				JOB_free(ctx->retval);
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			job_add(ctx, ctx->retval, caller_uid);
			JOB_free(ctx->retval);
			break;
	}
	CTX_RETURN;
}

Eina_Bool _getAllStorageTypes(Zentific_Ctx *ctx){
	db->getAllStorageTypes(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getDisksByStorage(Zentific_Ctx *ctx, int storid){
	db->getDisksByStorage(db, ctx, storid);
	CTX_RETURN;
}

Eina_Bool _getDiskByStorage(Zentific_Ctx *ctx, int storid, const char *path){
	if(storid < 0 || !path || !path[0] )
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else
		db->getDiskByStorage(db, ctx, storid, path);
	CTX_RETURN;
}

Eina_Bool _getVmByDisk(Zentific_Ctx *ctx, int storid, const char * path){
	db->getVmByDisk(db, ctx, storid, path);
	CTX_RETURN;
}

Eina_Bool _isDiskByStorage(Zentific_Ctx *ctx, int storid, const char *path){
	db->isDiskByStorage(db, ctx, storid, path);
	CTX_RETURN;
}

Eina_Bool _getStorageCount(Zentific_Ctx *ctx){
	db->getStorageCount(db, ctx);
	CTX_RETURN;
}
