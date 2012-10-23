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
#include "zrpcdb.h"
Esql_Query_Id getvmdisks(DB *db, Zentific_Ctx *ctx, const char *uuid, int rev);
Esql_Query_Id getvmvifs(DB *db, Zentific_Ctx *ctx, const char *uuid, int rev);

static Eina_List *
zrpcdb_vm_keyvalue(Esql_Res *res){
  Eina_Iterator *it;
  Esql_Row *r;
  Eina_List *ret = NULL;
  KEYVALUE * tmp;
  EINA_SAFETY_ON_NULL_RETURN_VAL(res, NULL);

  if (!esql_res_rows_count(res)) return NULL;
  it = esql_res_row_iterator_new(res);
  EINA_ITERATOR_FOREACH(it, r)
    {
      Eina_Inlist *l;
      Esql_Cell *c;

      tmp = KEYVALUE_new();
      l = esql_row_cells_get(r);
      EINA_INLIST_FOREACH(l, c)
	{
	  if (!strcmp(c->colname, "uuid"))
	    tmp->key = eina_stringshare_add(c->value.string);
	  else if (!strcmp(c->colname, "avg"))
	    tmp->value = eina_stringshare_add(c->value.string);
	  tmp->type = eina_stringshare_add("double");
	}
      ret = eina_list_append(ret, tmp);
    }
  eina_iterator_free(it);
  return ret;
}

#define ESCAPE(X, ...) \
	_escape(__PRETTY_FUNCTION__, db, error, dbhandle, X, __VA_ARGS__)

#if 0
static char *_escape(const char *function, DB *db, Zentific_Ctx *ctx, MYSQL *dbhandle, const char *fmt, ...){
	va_list args;
	char *ret;

	va_start(args, fmt);
	ret = dbEscape(function, db, error, dbhandle, fmt, args);
	va_end(args);
	return ret;
}

static char *_print_vif(DB *db, Zentific_Ctx *ctx, MYSQL *dbhandle, ZVIF *vif){
	/* print in order of struct */
	return ESCAPE(
		"%i, %s, %i, %i, %s, %s, %s, %s, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
		vif->id, vif->name, vif->mtu, vif->bridge, vif->mac, vif->ip,
		vif->netmask, vif->broadcast, vif->rxbytes, vif->rxpackets,
		vif->rxerr, vif->rxdrop, vif->rfifo, vif->frame, vif->rcompressed,
		vif->multicast, vif->txbytes, vif->txpackets, vif->txerr,
		vif->txdrop, vif->tfifo, vif->collisions, vif->carrier, vif->tcompressed
	);
}

static char *_print_part(DB *db, Zentific_Ctx *ctx, MYSQL *dbhandle, ZPART *part){
	/* print in order of struct */
	return ESCAPE(
		"%s, %s, %s, %s, %s, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
		part->name, part->mode_opts, part->dev_location, part->mount_point,
		part->partition_type, part->f_bsize, part->f_frsize, part->f_block,
		part->f_bfree, part->f_bavail, part->f_files, part->f_ffree,
		part->f_favail, part->f_fsid, part->f_flag, part->f_namemax
	);
}

static char *_print_disk(DB *db, Zentific_Ctx *ctx, MYSQL *dbhandle, ZDISK *disk){
	/* print in order of struct */
	return ESCAPE(
		"%s, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
		disk->name, disk->read_ios, disk->read_merges, disk->read_sectors, disk->read_ticks, disk->write_ios, disk->write_merges, disk->write_sectors, disk->write_ticks, disk->in_flight, disk->io_ticks, disk->time_in_queue
	);
}

static char *_print_zsys(DB *db, Zentific_Ctx *ctx, MYSQL *dbhandle, ZSYS *zsys){
	char *ret = NULL;
	Eina_Strbuf *cpuflags, *modules;
	Eina_List *l;

	cpuflags = eina_strbuf_new();
	modules = eina_strbuf_new();

	for (l = zsys->cpuflags; l; l = l->next){
		g_string_append(cpuflags, l->data);
		if (l->next)
			g_string_append_c(cpuflags, ' ');
	}
	for (l = zsys->modules; l; l = l->next){
		g_string_append(modules, l->data);
		if (l->next)
			g_string_append_c(modules, ' ');
	}


	/* print in order of struct */
	ret = ESCAPE(
		"%s, %s, %s, %s, %s, %s, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %s, %s, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
		zsys->arch, zsys->ostype, zsys->os, zsys->version, modules->str,
		zsys->domainname, zsys->numcpus, zsys->userprocs, zsys->nicedprocs,
		zsys->sysprocs, zsys->idle, zsys->steal, zsys->guest, zsys->forks,
		zsys->runprocs, zsys->blockprocs, zsys->entropy, zsys->procs,
		zsys->ctxt, zsys->intr, cpuflags->str, zsys->model, zsys->mhz, zsys->iowait,
		zsys->load1, zsys->load5, zsys->load15, zsys->page_size, zsys->mem_free,
		zsys->mem_total, zsys->mem_shared, zsys->mem_buffered, zsys->swap_free,
		zsys->swap_total, zsys->mem_active, zsys->mem_inactive
	);

	eina_strbuf_free(cpuflags, EINA_TRUE);
	eina_strbuf_free(modules, EINA_TRUE);
	return ret;
}

static Eina_Bool _ztoolsUpdateSingle(DB *db, Zentific_Ctx *ctx, MYSQL *dbhandle, const char *uuid, VMSTATS *stats){
	Eina_Bool ret;
	Eina_List *l;
	char *zsys = NULL;

	if (!(zsys = _print_zsys(db, error, dbhandle, stats->system)))
		goto error;

	ret = DBE(
		"INSERT INTO `table` "
			"(uuid, table value names) "
		"VALUES "
			"(%s, %s)",
		uuid, zsys
		);
	/* handle !ret */
	ZFREE(zsys);

	for (l = stats->disks; l; l = l->next){
		char *disk;
		if (!(disk = _print_disk(db, error, dbhandle, l->data)))
			goto error;
		ret = DBE(
			"INSERT INTO `table` "
				"(uuid, table value names in order of struct) "
			"VALUES "
				"(%s, %s)",
			uuid, disk
			);
		ZFREE(disk);
		/* handle !ret */
	}



	for (l = stats->partitions; l; l = l->next){
		char *part;
		if (!(part = _print_part(db, error, dbhandle, l->data)))
			goto error;
		ret = DBE(
			"INSERT INTO `table` "
				"(uuid, table value names) "
			"VALUES "
				"(%s, %s)",
			uuid, part
			);
		ZFREE(part);
		/* handle !ret */
	}

	for (l = stats->vifs; l; l = l->next){
		char *vif;
		if (!(vif = _print_vif(db, error, dbhandle, l->data)))
			goto error;
		ret = DBE(
			"INSERT INTO `table` "
				"(uuid, table value names) "
			"VALUES "
				"(%s, %s)",
			uuid, vif
			);
		ZFREE(vif);
		/* handle !ret */
	}

	return EINA_TRUE;
error:
	return EINA_FALSE;
}
#endif
/* ----------------------------------------------- */
/* API Functions: VM    			   */
/* ----------------------------------------------- */


Eina_Bool isVM(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QIAB("SELECT * from `vms` where uuid='%s'", uuid);
}

/* all vm functions check isVM */
Eina_Bool isVmIgnored(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QBA("SELECT `ignored` FROM `vms` WHERE `uuid`='%s'", uuid);
}

Eina_Bool needsReboot(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QBA("SELECT needsreboot from `vms` where uuid='%s'", uuid);
}

Eina_Bool setVmNeedsReboot(DB *db, Zentific_Ctx *ctx, const char *uuid, Eina_Bool needreboot){
	QE(
		"UPDATE vms SET needsreboot='%d' WHERE uuid='%s'",
		(int)needreboot, uuid);
}

Eina_Bool getVMparentNode(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT nodes.uuid FROM nodes "
		"JOIN nodechildren ON nodes.id=nodechildren.node "
		"JOIN vms ON nodechildren.vm=vms.id "
		"WHERE vms.uuid='%s'",
		uuid);
}

Eina_Bool updateVMparentNode(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *puuid){
	QE(
		"REPLACE INTO `nodechildren` (vm, node) VALUES ("
		" (SELECT id FROM `vms` WHERE uuid='%s' LIMIT 1),"
		" (SELECT id FROM `nodes` WHERE uuid='%s' LIMIT 1)"
		" )",
		uuid, puuid);
}

Eina_Bool createVM(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QEE(
		"INSERT INTO `vms` (uuid, ignored, template) VALUES('%s', '0', '0')", uuid);
}

Eina_Bool createVmTemplate(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QEE(
		"INSERT INTO `vms` (uuid, ignored, template) VALUES('%s', '0', '1')", uuid);
}

Eina_Bool saveVMrevision(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid, const char *name, const char *description) {
	QE(
		"INSERT INTO `revisions` (vif, disk, vm, uuid, uid, name, description)"
		" VALUES("
			"("
				"SELECT vifs.id FROM `vifs`, `vifconfig` WHERE"
				"vifs.uuid='%s' AND"
				"vifs.id=vifconfig.vif AND"
				"vifconfig.mac = '%s'"
				"ORDER BY vifconfig.revision DESC LIMIT 1"
			"),("
				"SELECT disks.id FROM `disks`, `diskconfig` WHERE"
				"disks.uuid='%s' AND"
				"disks.id=diskconfig.disk AND"
				"diskconfig.mac = '%s'"
				"ORDER BY diskconfig.revision DESC LIMIT 1"
			"), ("
				"SELECT vms.id FROM `vms`, `vmconfig` WHERE"
				"vms.uuid='%s' AND"
				"vms.id=vmconfig.vm AND"
				"vmconfig.mac = '%s'"
				"ORDER BY vmconfig.revision DESC LIMIT 1"
			"),"
		"'%s', %d, '%s', '%s')",
		uuid, uuid, uuid,
		uuid, uid, name, description);
}

// functions to get VM from db and return a VM type
Esql_Query_Id getvms(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	/*FIXME: an issue is exposed here: vms with changing uuids are still returned
	 * even though they do not and never again will exist
	 *
	 * NOTE : this query is fine. this will be most properly fixed via the implementation of
	 *	a VM collision detection at time of intake
	 */
	DB_VARS;
	if ((offset < 0) || (recordcount < 0))
		QUERY_NOESCAPE(Array_string_esql,
			"SELECT uuid FROM `vms` WHERE template=0");
	else
		QUERY(Array_string_esql,
			"SELECT uuid FROM `vms` WHERE template=0 LIMIT %d, %d",
			offset, recordcount);
	ctx->free_func = (Ecore_Cb)Array_string_free;
	return qid;
}

/* WARNING: keep sortfield at a reasonable length in the caller */
Eina_Bool getVMsSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
	 snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QISA(
		"SELECT vms.uuid FROM "
			"(SELECT vm, revision, MAX(revision) AS m FROM vmconfig GROUP BY vm) AS xx "
		"INNER JOIN vmconfig AS t ON t.vm=xx.vm AND t.revision = xx.m "
		"JOIN vms ON vms.id=t.vm "
		"WHERE vms.template=0 "
		"GROUP BY t.vm "
		"ORDER BY t.`%s` %s %s",
		 sortfield,
		 (desc) ? "DESC" : "",
		 limit);

}

Eina_Bool getVMs(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	Esql_Query_Id qid;

	qid = getvms(db, ctx, offset, recordcount);
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

/* WARNING: keep sortfield at a reasonable length in the caller */
Eina_Bool getVMsByUIDSorted(DB *db, Zentific_Ctx *ctx, int uid, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
	 snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QISA(
		"SELECT vms.uuid FROM `vms` "
		"RIGHT JOIN vmconfig ON vmconfig.vm=vms.id "
		"JOIN ownershipvm ON ownershipvm.vm=vms.id "
		"WHERE vms.template=0 AND ownershipvm.uid='%d' "
		"ORDER BY vmconfig.revision, `vmconfig`.`%s` %s %s",
		uid, sortfield,
		(desc) ? "DESC" : "",
		limit);

}

Eina_Bool getVMsByUID(DB *db, Zentific_Ctx *ctx, int uid, int offset, int recordcount){
	if ((offset < 0) || (recordcount < 0))
		QSSA(
			"SELECT uuid FROM vms JOIN ownershipvm ON ownershipvm.vm=vms.id"
			" WHERE ownershipvm.uid='%d'",
			uid);
	QSSA(
		"SELECT uuid FROM vms JOIN ownershipvm ON ownershipvm.vm=vms.id"
		" WHERE ownershipvm.uid='%d' LIMIT %d, %d",
		uid, offset, recordcount);
}

// age really isnt age. think timestamp < now - retentionlength
// 	age = (now - retentionlength)
Eina_Bool pruneVmStats(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QE(
		"USING `vmstats` "
		"JOIN vms ON vmstats.vm=vms.id "
		"WHERE vmstats.timestamp < DATE_SUB(CURDATE(), "
		"INTERVAL IFNULL("
			"(SELECT value FROM `config` WHERE `key`='stat_retention'"
			" AND `subsystem`='zrpc' AND `mid`='-1'"
			" AND `uid`='-1' AND `gid`='-1' AND `vgid`='-1'"
			" AND `ngid`='-1' AND `vm`='-1' AND `node`='-1'"
			" AND `platform`='-1'), 31) HOUR) "
		"AND vms.uuid='%s'",
		uuid, uuid);
}

Eina_Bool setVmState(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *state){
	QE(
		"UPDATE vms SET state='%s', timestamp=NOW() WHERE uuid='%s'",
		state, uuid);

}

Eina_Bool updateVmTimestamp(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QE(
		"UPDATE vms SET timestamp=NOW() WHERE uuid='%s'", uuid);

}

Eina_Bool updateVmStats(DB *db, Zentific_Ctx *ctx, VM *vm){

	/* FIX THIS so the other params are used.
	 * 	for example, the node id param may be useful in this context
	 *	especially
	 */
	QE(
		"INSERT INTO `vmstats` (vm, state, cputime, cpupct, "
			" domid, uptime)"
		" VALUES ("
			"(SELECT vms.id FROM `vms` WHERE vms.uuid='%s'),"
		"       '%s', '%lf', '%lf', '%d', '%lf'"
		");",
		vm->uuid, vm->state, vm->cputime, vm->cpupct, vm->id, vm->uptime);
}

Eina_Bool getTopCpuConsumers(DB *db, Zentific_Ctx *ctx, int intervalminutes, int number){
	DB_VARS;
	QUERY(zrpcdb_vm_keyvalue,
		"SELECT AVG(cpupct) AS `avg`, uuid from vmstats "
		"RIGHT JOIN vms ON vmstats.vm=vms.id "
		"WHERE timestamp > DATE_SUB(NOW(), INTERVAL %d MINUTE) "
		"GROUP BY `vm` "
		"ORDER BY `avg` DESC LIMIT %d",
		intervalminutes, number);
	QID_RETURN;
}

Eina_Bool getTopBandwidthConsumers(DB *db, Zentific_Ctx *ctx, int intervalminutes, int number){
	DB_VARS;
	QUERY(zrpcdb_vm_keyvalue,
		"SELECT SUM(txbytes+rxbytes)/1024/1024 AS avg, vms.uuid from vifstats "
		"RIGHT JOIN vifs ON vifstats.vif=vifs.id "
		"RIGHT JOIN vms ON vifs.uuid=vms.uuid "
		"WHERE timestamp > DATE_SUB(NOW(), INTERVAL %d MINUTE) "
		"GROUP BY vifs.id ORDER BY bw DESC LIMIT %d",
		intervalminutes, number);
	QID_RETURN;
}

Eina_Bool ignoreVm(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QE("UPDATE `vms` SET ignored=1 WHERE uuid='%s'", uuid);
}

Eina_Bool removeVm(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QE(
		"DELETE FROM vms, vmstats, vmconfig, revisions "
		"LEFT JOIN vmstats ON vms.id=vmstats.vm "
		"LEFT JOIN vmconfig ON vms.id=vmconfig.vm "
		"LEFT JOIN revisions ON vms.id=revisions.vm "
		"WHERE uuid='%s'",
		uuid);
}

static void
zrpcdb_vm_vifs(Esql_Res *res, Zentific_Ctx *ctx, Eina_List *vifs){
	VM *v;
	Esql_Query_Id qid;

	qid = esql_res_query_id_get(res);
	v = QID_PARAM_GET;
	DBG("Got vifs for VM: %s", v->uuid);
	v->vifs = vifs;
	ctx->free_func = (Ecore_Cb)VM_free;
	if (!v->vifs) v->vifs = (void*)EINA_TRUE;
	if (v->disks) {
		if (v->disks == (void*)EINA_TRUE) v->disks = NULL;
		if (ctx->cbs) {
			Z_Ctx_Cb cb;
			cb = ctx->cbs->data;
			if (!cb) {
				ctx->cbs = eina_list_remove_list(ctx->cbs, ctx->cbs);
				return;
			}
			cb(ctx, v);
			return;
		}
		ctx->retval = v;
		ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
	}
}

static void
zrpcdb_vm_disks(Esql_Res *res, Zentific_Ctx *ctx, Eina_List *disks){
	VM *v;
	Esql_Query_Id qid;

	qid = esql_res_query_id_get(res);
	v = QID_PARAM_GET;
	DBG("Got disks for VM: %s", v->uuid);
	v->disks = disks;
	ctx->free_func = (Ecore_Cb)VM_free;
	if (!v->disks) v->disks = (void*)EINA_TRUE;
	if (v->vifs) {
		if (v->vifs == (void*)EINA_TRUE) v->vifs = NULL;
		if (ctx->cbs) {
			Z_Ctx_Cb cb;
			cb = ctx->cbs->data;
			if (!cb) {
				ctx->cbs = eina_list_remove_list(ctx->cbs, ctx->cbs);
				return;
			}
			cb(ctx, v);
			return;
		}
		ctx->retval = v;
		ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
	}
}

/* FIXME: probably needs a little work */
static void
zrpcdb_vm_stats(Esql_Res *res, Zentific_Ctx *ctx, VM *v){
	Esql_Query_Id qid;
	int rev;
	DB *db;
	if (!esql_res_rows_count(res)) /* id not found */
		/* NOT an error event...yet */
		ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
	if (ctx->error) {
		if (ctx->cbs) {
			Z_Ctx_Cb cb;
			cb = ctx->cbs->data;
			if (!cb) {
				ctx->cbs = eina_list_remove_list(ctx->cbs, ctx->cbs);
				return;
			}
			DBG("Query returned no results:\"%s\"", esql_res_query_get(res));
			cb(ctx, NULL);
			return;
		}
		ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
		return;
	}
	DBG("Getting stats for VM: %s", v->uuid);
	db = esql_data_get(esql_res_esql_get(res));
	qid = esql_res_query_id_get(res);
	rev = (intptr_t)QID_PARAM_GET;
	qid = getvmdisks(db, ctx, v->uuid, rev);
	QID_CB_ADD(zrpcdb_vm_disks);
	QID_PARAM_ADD(v);
	qid = getvmvifs(db, ctx, v->uuid, rev);
	QID_CB_ADD(zrpcdb_vm_vifs);
	QID_PARAM_ADD(v);
}

Eina_Bool getVM(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	QUERY(VM_esql,
		"SELECT vms.*, vmconfig.*, nodes.uuid AS puuid FROM vms "
		"RIGHT JOIN vmconfig ON vmconfig.vm=vms.id "
			"AND vmconfig.revision="
			"(SELECT revision FROM vmconfig LEFT JOIN vms ON vmconfig.vm=vms.id WHERE vms.uuid='%s' ORDER BY REVISION DESC LIMIT 1) "
		"LEFT JOIN nodechildren ON vms.id=nodechildren.vm "
		"LEFT JOIN nodes ON nodechildren.node=nodes.id "
		"WHERE vms.uuid='%s' "
		"LIMIT 1",
		uuid, uuid);
	ctx->free_func = (Ecore_Cb)VM_free;
	QID_CB_ADD(zrpcdb_vm_stats);
	QID_PARAM_ADD((intptr_t*)-1);
	QID_RETURN;
}

Eina_Bool getVmByRev(DB *db, Zentific_Ctx *ctx, const char *uuid, int rev){
	DB_VARS;
	QUERY(VM_esql,
		"SELECT * FROM vms RIGHT JOIN vmconfig ON vmconfig.vm=vms.id"
		" WHERE vms.uuid='%s' AND vmconfig.revision='%d'"
		" ORDER BY vmconfig.revision DESC LIMIT 1",
		uuid, rev);
	ctx->free_func = (Ecore_Cb)VM_free;
	QID_CB_ADD(zrpcdb_vm_stats);
	QID_PARAM_ADD((intptr_t*)rev);
	QID_RETURN;
}

#if 0
static void
zrpcdb_vm_getallfull(Esql_Res *res, Zentific_Ctx *ctx, Eina_List *vms){
	Esql_Query_Id qid;
	DB *db;
	const char *uuid;

	if (!esql_res_rows_count(res)) /* id not found */
		/* NOT an error event...yet */
		ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
	if (ctx->error) {
		if (ctx->cbs) {
			Z_Ctx_Cb cb;
			cb = ctx->cbs->data;
			if (!cb) {
				ctx->cbs = eina_list_remove_list(ctx->cbs, ctx->cbs);
				return;
			}
			DBG("Query returned no results:\"%s\"", esql_res_query_get(res));
			cb(ctx, NULL);
			return;
		}
		ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
		return;
	}
	EINA_LIST_FREE(vms, uuid){
		getVM(db, ctx, uuid);
		eina_stringshare_del(uuid);
	}
	db = esql_data_get(esql_res_esql_get(res));
	qid = getvmdisks(db, ctx, v->uuid);
	QID_CB_ADD(zrpcdb_vm_disks);
	QID_PARAM_ADD(v);
	qid = getvmvifs(db, ctx, v->uuid);
	QID_CB_ADD(zrpcdb_vm_vifs);
	QID_PARAM_ADD(v);
}

Eina_Bool getVMsFull(DB *db, Zentific_Ctx *ctx){
/* FIXME: this is too slow!!!
	DB_VARS;
	QUERY_NOESCAPE(Array_VM_esql,
		"SELECT vms.*, vmconfig.*, nodes.uuid AS puuid FROM vms "
		"RIGHT JOIN vmconfig ON vmconfig.vm=vms.id "
		"LEFT JOIN nodechildren ON vms.id=nodechildren.vm "
		"LEFT JOIN nodes ON nodechildren.node=nodes.id "
		"WHERE template='0' "
		"GROUP BY vms.uuid",
		);
	ctx->free_func = (Ecore_Cb)Array_VM_free;
	QID_RETURN;
*/
	Esql_Query_Id qid;
	qid = getvms(db, ctx);
	QID_CB_ADD(zrpcdb_vm_getallfull);
	QID_RETURN;
}
#endif
Eina_Bool updateVmConfig(DB *db, Zentific_Ctx *ctx, VM *input){
	QE(
		"INSERT INTO `vmconfig`"
		"  (vm, revision, name, type, os,"
		"       kernel, ramdisk, cmdline, on_reboot,"
		"       on_poweroff, on_crash, vncpasswd,"
		"       mem, maxmem, vcpus, vncport)"
		" VALUES ("
		"       (SELECT id FROM `vms` WHERE uuid='%s' ORDER BY id DESC LIMIT 1),"
			"((SELECT revision FROM vmconfig "
			"RIGHT JOIN vms ON vmconfig.vm=vms.id "
			"WHERE vms.uuid='%s' "
			"ORDER BY vmconfig.timestamp DESC LIMIT 1) + 1),"
		"       '%s', '%s', '%s', '%s', '%s',"
		"       '%s', '%s', '%s', '%s', '%s',"
		"       '%d', '%d', '%d', '%d'"
		" )",
		input->uuid, input->uuid,
		input->name, input->type, input->os, input->kernel, input->ramdisk,
		input->cmdline, input->on_reboot, input->on_poweroff,
		input->on_crash, input->vncpasswd,
		input->mem, input->maxmem, input->vcpus, input->vncport);
}

Eina_Bool getAllUsersByVm(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QISA(
		"SELECT ownershipvm.uid FROM ownershipvm"
		" RIGHT JOIN vms ON vms.id=ownershipvm.vm"
		" WHERE vms.uuid='%s' "
		"GROUP BY ownershipvm.uid",
		uuid);
}

Eina_Bool getVMstat(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *stat){
	QSA(
		"SELECT `%s` FROM `vmstats` LEFT JOIN `vms` ON vms.id=vmstats.vm"
		" WHERE vms.uuid='%s' ORDER BY vmstats.timestamp DESC LIMIT 1",
		 stat, uuid);
}

Eina_Bool getVMattribute(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *attribute){
	QSA(
		"SELECT `%s` FROM `vmconfig` LEFT JOIN `vms` ON vms.id=vmconfig.vm"
		" WHERE vms.uuid='%s' ORDER BY vmconfig.timestamp DESC LIMIT 1",
		 attribute, uuid);
}

Eina_Bool saveVmNotes(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *notes){
	QE(
		"REPLACE INTO `vmnotes` (vm, notes) VALUES ("
		" (SELECT `id` FROM `vms` WHERE uuid='%s'),"
		" '%s')",
		 uuid, notes);
}


Eina_Bool getVmNotes(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT `notes` FROM `vmnotes` RIGHT JOIN `vms` ON vmnotes.vm=vms.id"
		" WHERE vms.uuid='%s'",
		uuid);
}


Eina_Bool getVMparentAddress(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT `configuredhost` FROM `nodes` "
		"LEFT JOIN nodechildren ON nodes.id=nodechildren.node "
		"LEFT JOIN vms ON nodechildren.vm=vms.id "
		"WHERE vms.uuid='%s' LIMIT 1",
		uuid);
}


/* ----------------------------------------------- */
/* API Functions: VM  				   */
/*  (continued, action functions) 		   */
/* ----------------------------------------------- */

// functions to get VM from db and return a VM type
Eina_Bool getVmLogs(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	QUERY(Array_LOG_esql,
		"SELECT timestamp, uid, severity, message FROM `vmlogs` "
		"LEFT JOIN vms ON vmlogs.vm=vms.id "
		"WHERE vms.uuid='%s' "
		"ORDER BY timestamp DESC",
		uuid);
	ctx->free_func = (Ecore_Cb)Array_LOG_free;
	QID_RETURN;
}

Eina_Bool getTemplateByName(DB *db, Zentific_Ctx *ctx, const char *name){
	DB_VARS;
	QUERY(TEMPLATE_esql,
		"SELECT * FROM `vmtemplates` WHERE name='%s'",
		name);
	ctx->free_func = (Ecore_Cb)TEMPLATE_free;
	QID_RETURN;
}

Eina_Bool getTemplate(DB *db, Zentific_Ctx *ctx, int id){
	DB_VARS;
	QUERY(TEMPLATE_esql,
		"SELECT * FROM `vmtemplates` WHERE id='%d'",
		id);
	ctx->free_func = (Ecore_Cb)TEMPLATE_free;
	QID_RETURN;
}

Eina_Bool getAllTemplates(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT id FROM `vmtemplates`");
}

Eina_Bool getAllTemplatesFull(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_TEMPLATE_esql,
		"SELECT * FROM `vmtemplates`");
	ctx->free_func = (Ecore_Cb)Array_TEMPLATE_free;
	QID_RETURN;
}

Eina_Bool getTemplateConfig(DB *db, Zentific_Ctx *ctx, int id){
	DB_VARS;
	QUERY(VM_esql,
		"SELECT * FROM vms"
		" RIGHT JOIN vmtemplates ON vmtemplates.uuid=vms.uuid"
		" RIGHT JOIN vmconfig ON vms.id=vmconfig.vm"
		" WHERE vmtemplates.id='%d'",
		id);
	ctx->free_func = (Ecore_Cb)VM_free;
	QID_RETURN;
}

Eina_Bool getTemplateConfigByName(DB *db, Zentific_Ctx *ctx, const char *name){
	DB_VARS;
	QUERY(VM_esql,
		"SELECT * FROM vms"
		" RIGHT JOIN vmtemplates ON vmtemplates.uuid=vms.uuid"
		" RIGHT JOIN vmconfig ON vms.id=vmconfig.vm"
		" WHERE vmtemplates.name='%s'",
		name);
	ctx->free_func = (Ecore_Cb)VM_free;
	QID_RETURN;
}

Eina_Bool modifyTemplate(DB *db, Zentific_Ctx *ctx, int id, TEMPLATE *template){
	QE(
		"UPDATE `vmtemplates`"
		" SET name='%s', architecture='%s', description='%s', type='%s', uuid='%s', platform='%d'"
		" WHERE id='%d'",
		template->name, template->architecture, template->description,
		template->type, template->uuid, template->platform,
		id);
}

Eina_Bool addTemplate(DB *db, Zentific_Ctx *ctx, TEMPLATE *template){
	QEJ(
		"INSERT INTO `vmtemplates`"
		" (name, architecture, description, type, uuid, platform)"
		" VALUES"
		" ('%s', '%s', '%s', '%s', '%s', '%d')",
		template->name, template->architecture, template->description,
		template->type, template->uuid, template->platform);
}

Eina_Bool hasGuestTools(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QIAB(
		"SELECT zentifictools FROM `vms` WHERE uuid='%s'",
		uuid);
}

Eina_Bool getAllVmsByNode(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSSA(
		"SELECT vms.uuid FROM vms RIGHT JOIN nodechildren ON nodechildren.vm=vms.id "
		"RIGHT JOIN nodes ON nodechildren.node=nodes.id "
		"WHERE nodes.uuid='%s' AND vms.template=0 "
		"GROUP BY vms.uuid",
		uuid);
}

Eina_Bool getAllVmsByNodeByUID(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid){
	QSSA(
		"SELECT vms.uuid FROM vms RIGHT JOIN nodechildren ON nodechildren.vm=vms.id "
		"RIGHT JOIN nodes ON nodechildren.node=nodes.id "
		"RIGHT JOIN ownershipvm ON ownershipvm.vm=vms.id "
		"WHERE nodes.uuid='%s' AND vms.template='0' AND ownershipvm.uid='%d' ",
		"GROUP BY vms.uuid",
		uuid, uid);
}

Eina_Bool getVmByName(DB *db, Zentific_Ctx *ctx, const char *name){
	QSA(
		"SELECT uuid FROM vms RIGHT JOIN vmconfig ON vmconfig.vm=vms.id "
		"WHERE vmconfig.name='%s' ORDER BY vmconfig.revision DESC LIMIT 1",
		name);

}

Eina_Bool getVMparentNodeAndAddress(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSSA(
		"SELECT nodes.uuid, configuredhost FROM nodes "
		"JOIN nodechildren ON nodes.id=nodechildren.node "
		"JOIN vms ON nodechildren.vm=vms.id "
		"WHERE vms.uuid='%s' "
		"ORDER BY vms.uuid ASC",
		uuid);
}

Eina_Bool ztoolsUpdate(DB *db __UNUSED__, Zentific_Ctx *ctx __UNUSED__, const char *uuid __UNUSED__, Eina_List *info __UNUSED__){
#if 0
Eina_Bool ztoolsUpdate(DB *db, Zentific_Ctx *ctx, const char *uuid, Eina_List *info){
	MYSQL *dbhandle;
	if (!(dbhandle = get_handle(db, error)) || (azy_content_error_is_set(error))
		return EINA_FALSE;

	for (; info; info = info->next)
		_ztoolsUpdateSingle(db, error, dbhandle, uuid, info->data);
	release_handle(db, error, dbhandle);
#endif
	return EINA_TRUE;
}

Eina_Bool getVmCount(DB *db, Zentific_Ctx *ctx){
	QI(
		"SELECT COUNT(uuid) FROM `vms` WHERE template=0");
}

Eina_Bool getVmCountByUID(DB *db, Zentific_Ctx *ctx, int uid){
	QIA(
		"SELECT COUNT(uuid) FROM `vms` "
		"JOIN ownershipvm ON ownershipvm.vm=vms.id "
		"WHERE ownershipvm.uid='%d' AND template=0",
		uid);
}
