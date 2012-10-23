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
#include "functions-disk.h"
#include "functions-vm.h"

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

/* convenience function.
 *
 * returns 0 for equivalent DISKs by comparing paths
 *
 */
static int
DISK_path_cmp(DISK *a, DISK *b){
	return (a->ext_dev != b->ext_dev);
}

Eina_Bool _isDisk(Zentific_Ctx *ctx, const char *uuid, const char *ext_dev){
	db->isDisk(db, ctx, uuid, ext_dev);
	CTX_RETURN;
}

Eina_Bool _createDisk(Zentific_Ctx *ctx, const char *uuid, const char * path, int storid){
	db->createDisk(db, ctx, uuid, path, storid);
	CTX_RETURN;
}

Eina_Bool _updateDiskStats(Zentific_Ctx *ctx, const char *uuid, DISK *disk){
	db->updateDiskStats(db, ctx, uuid, disk);
	CTX_RETURN;
}

Eina_Bool _getVmDisk(Zentific_Ctx *ctx, const char *uuid, const char *path){
	db->getVmDisk(db, ctx, uuid, path);
	CTX_RETURN;
}

Eina_Bool _getVMdisks(Zentific_Ctx *ctx, const char *uuid){
	db->getVMdisks(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getNumVMdisks(Zentific_Ctx *ctx, const char *uuid){
	db->getNumVMdisks(db, ctx, uuid);
	CTX_RETURN;
}


static void
disks_updateconfig(Zentific_Ctx *ctx){
	Zentific_Ctx *pctx = ctx->parent;
	VM *vm = ctx->retval;
	Eina_List *l;
	DISK *v;
	Ecore_Cb cb = ctx->cb;
	intptr_t *rev;
	switch (ctx->count){
		case 0:
			db->getLatestRevision(db, ctx, vm->uuid, 2); /* 2 is disk */
			CTX_INT_JOB(disks_updateconfig);
			break;
		case 1:
			rev = CTX_PARAM_GET;
			EINA_LIST_FOREACH(vm->disks, l, v){
				if (!pctx->ret){
					/* stats set if this is false */
					if (pctx->checked) {
						/* existing vm */
						db->pruneDiskStats(db, ctx, vm->uuid, v->ext_dev);
						CTX_CB_ADD(NULL);
					}
					db->updateDiskStats(db, ctx, vm->uuid, v);
					CTX_CB_ADD(NULL);
				}
				db->updateDiskConfig(db, ctx, vm->uuid, *rev + 1, v);
				CTX_BOOL_JOB(disks_updateconfig);
			}
			break;
		default:
			if (ctx->count == 1 + eina_list_count(vm->disks))
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

void disk_cb_updateconfig(Zentific_Ctx *ctx){
	VM *vm = ctx->retval;
	Eina_List *l, *disks;
	DISK *disk;
	Ecore_Cb cb = ctx->cb;
	Zentific_Ctx *pctx = ctx->parent;
	int create = 0;

	switch (ctx->count){
		case 0:
			disks = CTX_PARAM_GET;
			if (!disks){
				if (!vm->disks) goto out;
				else
					ctx->auth = ctx->checked = pctx->auth = EINA_TRUE;
				break;
			} else if (eina_list_count(disks) != eina_list_count(vm->disks))
				ctx->auth = ctx->checked = pctx->auth = EINA_TRUE;
			else {

				/* if a stored Disk is not found in the input list, we need to update */
				EINA_LIST_FOREACH(disks, l, disk) {
					/* find disk by path ( == ext_dev) */
					if (!eina_list_search_unsorted(vm->disks, (Eina_Compare_Cb)DISK_path_cmp, disk)){
						ctx->auth = ctx->checked = pctx->auth = EINA_TRUE;
						break;
					}
				}
				/* if an input Disk is not found in the stored list, we need to update */
				EINA_LIST_FOREACH(vm->disks, l, disk) {
					DISK *x;
					/* find disk by path ( == ext_dev) */
					x = eina_list_search_unsorted(disks, (Eina_Compare_Cb)DISK_path_cmp, disk);
					if (!x){
						/* TODO FIXME: attempt to determine backing storage rather than setting = -1 */
						db->createDisk(db, ctx, vm->uuid, disk->ext_dev, -1);
						CTX_BOOL_JOB(disk_cb_updateconfig);
						create++;
					} else if (DISK_config_cmp(x, disk))
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
			EINA_LIST_FREE(disks, disk)
				DISK_free(disk);
		default:
			if (ctx->count == 1000){
				ctx->auth = EINA_TRUE;
				ctx->count = 0;
			}
			if (ctx->auth)
				disks_updateconfig(ctx);
			break;
	}
	if (!ctx->error) return;
out:
	eina_hash_free(ctx->params);
	free(ctx);
	if (--pctx->count == 7)
		ecore_job_add(cb, pctx);
}
