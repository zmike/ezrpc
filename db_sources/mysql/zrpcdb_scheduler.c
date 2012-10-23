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
#include <inttypes.h>

/* API: job/task data */

Eina_Bool isJob(DB *db, Zentific_Ctx *ctx, int jobid){
	QIAB(
		"SELECT `id` FROM `jobs` WHERE `id`='%d'",
		 jobid);
}

Eina_Bool getJob(DB *db, Zentific_Ctx *ctx, int jobid, int caller_uid){
	DB_VARS;
	QUERY(JOB_esql,
		"SELECT * FROM jobs "
		"LEFT JOIN nodes ON jobs.target_node=nodes.uuid "
		"LEFT JOIN ownershipnode ON ownershipnode.node=nodes.id "
		"LEFT JOIN vms ON jobs.target_vm=vms.uuid "
		"LEFT JOIN ownershipvm ON ownershipvm.vm=vms.id "
		"LEFT JOIN users ON jobs.uid=users.uid "
		"WHERE status < 4 "
		"AND jobs.id='%i' "
		"AND (jobs.uid='%i' "
			"OR (ownershipvm.uid='%i' AND ownershipvm.privileges != 4) OR (ownershipnode.uid='%i' AND ownershipnode.privileges != 4) "
			"OR (users.uid='%i' AND users.type='0'))",
		jobid, caller_uid, caller_uid, caller_uid, caller_uid);
	ctx->free_func = (Ecore_Cb)JOB_free;
	QID_RETURN;
}

Eina_Bool getRecentJobs(DB *db, Zentific_Ctx *ctx, int age){
	QISA(
		"SELECT `id` FROM `jobs` WHERE "
		" `timestamp` >= DATE_SUB(NOW(), INTERVAL %d MINUTE)",
		age);
}

Eina_Bool getRecentJobsByUID(DB *db, Zentific_Ctx *ctx, int uid, int age){
	QISA(
		"SELECT `id` FROM `jobs` "
		"LEFT JOIN users ON jobs.uid=users.uid "
		"WHERE jobs.uid='%d' "
		"AND jobs.timestamp >= users.lastlogin "
		"AND `timestamp` >= DATE_SUB(NOW(), INTERVAL %d MINUTE)",
		uid, age);
}

Eina_Bool getJobUID(DB *db, Zentific_Ctx *ctx, int id){
	QIA(
		"SELECT uid FROM `jobs` WHERE id='%d'", id);

}

Eina_Bool getJobsByIds(DB *db, Zentific_Ctx *ctx, Eina_List *jids, int caller_uid){
	Eina_List *l;
	intptr_t *jid;
	Eina_Strbuf *jobs;
	DB_VARS;
	ctx->free_func = (Ecore_Cb)eina_list_free;
	jobs = eina_strbuf_new();
	EINA_LIST_FOREACH(jids, l, jid){
		if (l->next)
			eina_strbuf_append_printf(jobs, "jobs.id='%i' OR ", *jid);
		else
			eina_strbuf_append_printf(jobs, "jobs.id='%i'", *jid);
	}
	QUERY(Array_int_esql,
		"SELECT jobs.* FROM jobs "
		"LEFT JOIN nodes ON jobs.target_node=nodes.uuid "
		"LEFT JOIN ownershipnode ON ownershipnode.node=nodes.id "
		"LEFT JOIN vms ON jobs.target_vm=vms.uuid "
		"LEFT JOIN ownershipvm ON ownershipvm.vm=vms.id "
		"LEFT JOIN users ON jobs.uid=users.uid "
		"WHERE status < 4 "
		"AND (%s) "
		"AND (jobs.uid='%i' "
			"OR (ownershipvm.uid='%i' AND ownershipvm.privileges != 4) OR (ownershipnode.uid='%i' AND ownershipnode.privileges != 4) "
			"OR (users.uid='%i' AND users.type='0'))",
		eina_strbuf_string_get(jobs), caller_uid, caller_uid, caller_uid, caller_uid);
	eina_strbuf_free(jobs);
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

/* returns all jobs running, launching, queued, and yet to be fetched.
 *	(excludes jobs which have finished or errored out)
 */
Eina_Bool getJobs(DB *db, Zentific_Ctx *ctx){
	QISA(
		"SELECT id FROM `jobs` where status < '%d'",
		ZENSCHED_JOB_STATUS_COMPLETED);

}

/* returns all jobs running, launching, queued, and yet to be fetched.
 *	(excludes jobs which have finished or errored out)
 */
Eina_Bool getJobsByUID(DB *db, Zentific_Ctx *ctx, int uid){
	QISA(
		"SELECT jobs.id FROM jobs "
		"LEFT JOIN nodes ON jobs.target_node=nodes.uuid "
		"LEFT JOIN ownershipnode ON ownershipnode.node=nodes.id "
		"LEFT JOIN vms ON jobs.target_vm=vms.uuid "
		"LEFT JOIN ownershipvm ON ownershipvm.vm=vms.id "
		"LEFT JOIN users ON jobs.uid=users.uid "
		"WHERE status < %i "
		"AND (jobs.uid='%i' "
			"OR (ownershipvm.uid='%i' AND ownershipvm.privileges != 4) OR (ownershipnode.uid='%i' AND ownershipnode.privileges != 4) "
			"OR (users.uid='%i' AND users.type='0'))",
		ZENSCHED_JOB_STATUS_COMPLETED, uid, uid, uid, uid);
}

/* returns all jobs running, launching, queued, and yet to be fetched.
 *	(excludes jobs which have finished or errored out)
 */
Eina_Bool getJobsByUUID(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QISA(
		"SELECT id FROM `jobs` "
		"WHERE `status` < %i AND "
		"(`target_vm` = '%s' OR `target_node` = '%s' )",
		ZENSCHED_JOB_STATUS_COMPLETED, uuid, uuid);
}

Eina_Bool getJobsStatus(DB *db, Zentific_Ctx *ctx, Eina_List *jids, int caller_uid){
	Eina_List *l;
	intptr_t *jid;
	Eina_Strbuf *jobs;
	DB_VARS;
	ctx->free_func = (Ecore_Cb)eina_list_free;
	jobs = eina_strbuf_new();
	EINA_LIST_FOREACH(jids, l, jid){
		if (l->next)
			eina_strbuf_append_printf(jobs, "jobs.id='%i' OR ", *jid);
		else
			eina_strbuf_append_printf(jobs, "jobs.id='%i'", *jid);
	}
	QUERY(Array_int_esql,
		"SELECT jobs.status FROM jobs "
		"LEFT JOIN nodes ON jobs.target_node=nodes.uuid "
		"LEFT JOIN ownershipnode ON ownershipnode.node=nodes.id "
		"LEFT JOIN vms ON jobs.target_vm=vms.uuid "
		"LEFT JOIN ownershipvm ON ownershipvm.vm=vms.id "
		"LEFT JOIN users ON jobs.uid=users.uid "
		"WHERE (%s) "
		"AND (jobs.uid='%i' "
			"OR (ownershipvm.uid='%i' AND ownershipvm.privileges != 4) OR (ownershipnode.uid='%i' AND ownershipnode.privileges != 4) "
			"OR (users.uid='%i' AND users.type='0'))",
		eina_strbuf_string_get(jobs), caller_uid, caller_uid, caller_uid, caller_uid);
	eina_strbuf_free(jobs);
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

Eina_Bool getJobStatus(DB *db, Zentific_Ctx *ctx, int jobid, int caller_uid){
	QIA(
		"SELECT jobs.status FROM jobs "
		"LEFT JOIN nodes ON jobs.target_node=nodes.uuid "
		"LEFT JOIN ownershipnode ON ownershipnode.node=nodes.id "
		"LEFT JOIN vms ON jobs.target_vm=vms.uuid "
		"LEFT JOIN ownershipvm ON ownershipvm.vm=vms.id "
		"LEFT JOIN users ON jobs.uid=users.uid "
		"WHERE jobs.id='%i'"
		"AND (jobs.uid='%i' "
			"OR (ownershipvm.uid='%i' AND ownershipvm.privileges != 4) OR (ownershipnode.uid='%i' AND ownershipnode.privileges != 4) "
			"OR (users.uid='%i' AND users.type='0'))",
		jobid, caller_uid, caller_uid, caller_uid, caller_uid);
}

#if 0
original
Eina_Bool addJob(DB *db, Zentific_Ctx *ctx, JOB *job, int schedulerId){
	Eina_Strbuf *dependencies;
	Eina_List *l;
	intptr_t *depend;

	dependencies = eina_strbuf_new();
	EINA_LIST_FOREACH(job->dependencies, l, depend)
		if ((intptr_t)depend >= 0){
			if (l->next) eina_strbuf_append_printf(dependencies, "%"PRIiPTR",", (intptr_t)depend);
			else eina_strbuf_append_printf(dependencies, "%"PRIiPTR"", (intptr_t)depend);
		}
	DB_VARS;
	ctx->state = 1;
	QUERY_NOCONVERT(
		"INSERT INTO `jobs` "
		"(module_name, module_function, target_host, target_node, target_vm, input_string, extra, uid, status, dependencies, scheduler) "
		"VALUES "
		"('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d', '%s', '%i')",
		job->module_name, job->module_function, job->target_host,
		job->target_node, job->target_vm, job->input_string, job->extra,
		job->uid, ZENSCHED_JOB_STATUS_INSERTED, eina_strbuf_string_get(dependencies), schedulerId);

	eina_strbuf_free(dependencies);
	QID_CB_ADD(zrpcdb_cb_exec);
	QID_RETURN;
}
#endif
Eina_Bool addJob(DB *db, Zentific_Ctx *ctx, JOB *job){
	Eina_Strbuf *dependencies;
	Eina_List *l;
	intptr_t *depend;

	dependencies = eina_strbuf_new();
	EINA_LIST_FOREACH(job->dependencies, l, depend)
		if ((intptr_t)depend >= 0){
			if (l->next) eina_strbuf_append_printf(dependencies, "%"PRIiPTR",", (intptr_t)depend);
			else eina_strbuf_append_printf(dependencies, "%"PRIiPTR"", (intptr_t)depend);
		}
	DB_VARS;
	ctx->state = 1;
	QUERY_NOCONVERT(
		"INSERT INTO `jobs` "
		"(module_name, module_function, target_host, target_node, target_vm, input_string, extra, uid, status, dependencies, scheduler) "
		"VALUES "
		"('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d', '%s', "
			"(SELECT schedulers.id FROM schedulers "
			"RIGHT JOIN controlmodules on schedulers.id=controlmodules.scheduler "
			"WHERE controlmodules.name='%s' "
			"LIMIT 1)"
		")",
		job->module_name, job->module_function, job->target_host,
		job->target_node, job->target_vm, job->input_string, job->extra,
		job->uid, ZENSCHED_JOB_STATUS_INSERTED, eina_strbuf_string_get(dependencies), job->module_name);

	eina_strbuf_free(dependencies);
	QID_CB_ADD(zrpcdb_cb_exec);
	QID_RETURN;
}

Eina_Bool getSchedulerAddressByModuleName(DB *db, Zentific_Ctx *ctx, const char *modname){
	QSA(
		"SELECT schedulers.address FROM schedulers "
		"RIGHT JOIN controlmodules on schedulers.id=controlmodules.scheduler "
		"WHERE controlmodules.name='%s'",
		modname);
}

Eina_Bool getSchedulerAddressByJob(DB *db, Zentific_Ctx *ctx, int jobid){
	QSA(
		"SELECT schedulers.address FROM schedulers "
		"RIGHT JOIN controlmodules ON schedulers.id=controlmodules.scheduler "
		"RIGHT JOIN jobs ON jobs.module_name=controlmodules.name "
		"WHERE jobs.id='%d'",
		jobid);
}

Eina_Bool getSchedulerIdByModuleName(DB *db, Zentific_Ctx *ctx, const char *modname){
	QISA(
		"SELECT schedulers.id FROM schedulers"
		" RIGHT JOIN controlmodules on schedulers.id=controlmodules.scheduler"
		" WHERE controlmodules.name='%s'",
		modname);
}

Eina_Bool getSchedulerIdByJob(DB *db, Zentific_Ctx *ctx, int jobid){
	QISA(
		"SELECT scheduler FROM jobs WHERE id='%d'",
		jobid);

}

Eina_Bool getSchedulerIdByPlatform(DB *db, Zentific_Ctx *ctx, const char *modname){
	QISA(
		"SELECT schedulers.id FROM schedulers "
		"RIGHT JOIN controlmodules on schedulers.id=controlmodules.scheduler "
		"WHERE controlmodules.name='%s'",
		modname);
}

/* from zensched */
/**
 * Read 0 or more jobs from this source. If fetchAll = 1, then include jobs that have been
 * started already (but NOT jobs that have been completed or cancelled).
 *
 * Return the number of jobs added to the list.
 * FIXME almost certainly need schedulerId to be part of this query as a join w/ module/platform
 */
Eina_Bool fetchJobs(DB *db, Zentific_Ctx *ctx, Eina_Bool fetchAll) {
	DB_VARS;
	/* Pick the query based on what we are fetching */
	if (fetchAll)
		/* fetch all jobs that:
		 *	1) have not yet been fetched
		 *		or
		 *	2) are in-flight (without having completed or failed with some sort of error)
		 *
		 *	that includes currently executing tasks!
		 */
		QUERY(Array_JOB_esql,
			"SELECT * FROM `jobs` "
			"WHERE `status` >= '%d' AND `status` < '%d' "
			"ORDER BY `id` ASC",
			ZENSCHED_JOB_STATUS_INSERTED, ZENSCHED_JOB_STATUS_COMPLETED);
	else
		/* fetch all jobs that have not been handled at all yet */
		QUERY(Array_JOB_esql,
			"SELECT * FROM `jobs` WHERE `status`='%d'",
			ZENSCHED_JOB_STATUS_INSERTED);
	QID_RETURN;

}

/**
 * Update the database with the current stats of the job
 */
Eina_Bool updateJob(DB *db, Zentific_Ctx *ctx, Zensched_Job *job, int schedulerId){
	DB_VARS;
	/* Oh boy, some stupidity follows. Too tired to think of
	 * anything more clever. Probably dumb, but it should work
	 * just fine
	 */

	/* generate the full query */
	QE(
		"UPDATE `jobs` SET `status`='%d', `return_value`='%d', "
		"`nexttime`='%i', `croncount`='%i', "
		"`time_started`='%d', `pid`='%d', `time_finished`='%d', "
		"`output_string`='%s', scheduler='%d' "
		"WHERE `id`='%d'",
		job->status, job->return_value,
		(int)job->cron_nexttime, job->cron_count,
		(job->time_started > 0) ? job->time_started : -1,
		(job->pid > 0) ? job->pid : -1,
		(job->time_finished > 0) ? job->time_finished : -1,
		job->output_string, schedulerId, job->id);
	QID_RETURN;
}

Eina_Bool getModuleConfigByNode(DB *db, Zentific_Ctx *ctx, int mid, const char *uuid){
	DB_VARS;

	QUERY(Array_KEYVALUE_esql,
		"SELECT `config`.`key`, `config`.`value` FROM `config` "
		"RIGHT JOIN controlmodules ON config.mid=controlmodules.id "
		"RIGHT JOIN nodes ON nodes.id=config.node "
		"WHERE `config`.`valueBlob`='' AND `config`.`subsystem`='zensched' "
		"AND nodes.uuid='%s' AND config.mid='%d'",
		uuid, mid);
	QID_RETURN;
}

Eina_Bool getModuleConfigByVm(DB *db, Zentific_Ctx *ctx, int mid, const char *uuid){
	DB_VARS;

	QUERY(Array_KEYVALUE_esql,
		"SELECT `config`.`key`, `config`.`value` FROM `config` "
		"RIGHT JOIN controlmodules ON config.mid=controlmodules.id "
		"RIGHT JOIN vms ON vms.id=config.vm "
		"WHERE `config`.`valueBlob`='' AND `config`.`subsystem`='zensched' "
		"AND vms.uuid='%s' AND config.mid='%d'",
		uuid, mid);
	QID_RETURN;
}

/* TODO reasonable enough to have per user, platform config queries too, unless it's less work
 *	to combine all of these into one monster query and do some processing at the next level.
 *	that may ultimately be the best way, but for now this felt cleaner. (It's all relative..)
 */

/*
 *	Get any stored config information for the particular module
 */
Eina_Bool getModuleConfig(DB *db, Zentific_Ctx *ctx, const char *module_name){
	DB_VARS;

	QUERY(Array_KEYVALUE_esql,
		"SELECT `key`, `value` FROM `config` "
		"WHERE valueBlob='' AND `subsystem` = 'zensched' "
		"AND vm='-1' AND node='-1' AND uid='-1' AND vgid='-1' AND ngid='-1' "
		"AND gid='-1' AND platform='-1' AND role='-1' "
		"AND `mid` = (SELECT `id` FROM `controlmodules` WHERE `name`='%s')",
		module_name);
	QID_RETURN;
}

Eina_Bool getBlobConfig(DB *db, Zentific_Ctx *ctx, int platform, int module, const char *key){
#warning "sql for valueBlob is still broken"
	DB_VARS;
	if (key)
		QUERY(Array_KEYVALUE_esql,
			"SELECT `key`, `valueBlob` FROM `config`"
			" WHERE `platform`='%d' AND mid='%d' AND `key`='%s' AND `value`=''",
			platform, module, key);
	else
		QUERY(Array_KEYVALUE_esql,
			"SELECT `key`, `valueBlob` FROM `config`"
			" WHERE `platform`='%d' AND mid='%d' and `value`=''",
			platform, module);
	QID_RETURN;
}

Eina_Bool registerPlatform(DB *db, Zentific_Ctx *ctx, Zensched_Module_Info *modinfo, int schedulerId){

	QE(
		"INSERT INTO `platforms`"
		" (name, vendor, version, build, description, enabled, features, scheduler)"
		" VALUES ('%s', '%s', '%s', '%s', '%s', '1', '%d', '%d')"
		" ON DUPLICATE KEY UPDATE `vendor`='%s', `version`='%s',"
		" `build`='%s', `description`='%s', features='%d', scheduler='%d'",
		modinfo->platform, modinfo->platformvendor, modinfo->platformver,
		modinfo->platformbuild, modinfo->platformdescription, modinfo->platformfeatures,
		schedulerId,
		modinfo->platformvendor, modinfo->platformver,
		modinfo->platformbuild, modinfo->platformdescription, modinfo->platformfeatures,
		schedulerId);
}

Eina_Bool registerControlModule(DB *db, Zentific_Ctx *ctx, Zensched_Module_Info *modinfo, int platid, int schedulerId){
#warning "fix use of registerControlModule"
//	int platid = getPlatformID(db, error, modinfo->platform);

	if ((platid < 0) && zshare_strcmp(modinfo->platform, "ANY")){
		//platform does not exist
		ctx->error = AZY_ERR(PLATFORM_NOT_PRESENT);
		return EINA_FALSE;
	}

	/* NOTE: if platformid is recorded as -1 in this case, then the module
	 * 	 can optionally apply to any platform
	 *	 (e.g. via override/extension of other module defaults)
	 */
	QE(
		"INSERT INTO `controlmodules` "
		"(platformid, name, enabled, description, controlplatform, controlnetwork, controlstorage, scheduler) "
		"VALUES ('%d','%s', '1', '%s', '%d', '%d', '%d', '%d')",
#warning "modinfo->platform and platid should always match"
		(!zshare_strcmp(modinfo->platform, "ANY")) ? -1 : platid,
		modinfo->name,
		modinfo->description,
		!!(modinfo->type & ZENSCHED_MODULE_TYPE_PLATFORMCTRL),
		!!(modinfo->type & ZENSCHED_MODULE_TYPE_NETWORKCTRL),
		!!(modinfo->type & ZENSCHED_MODULE_TYPE_STORAGECTRL),
		schedulerId);

}

Eina_Bool registerScheduler(DB *db, Zentific_Ctx *ctx, const char *hostname, const char *address, int port){
	/* id=LAST_INSERT_ID(id) allows DBJ to be meaningful in this context when we're
	 *	updating a known scheduler
	 */

	QEJ(
		"INSERT INTO `schedulers` (hostname, address, port) "
		"VALUES ('%s', '%s', '%d') "
		"ON DUPLICATE KEY UPDATE `hostname`='%s', `address`='%s', "
		"port='%d', id=LAST_INSERT_ID(id)",
		hostname, address, port,
		hostname, address, port);

}

/*
 *	Get any platform id by name
 */
Eina_Bool getModuleID(DB *db, Zentific_Ctx *ctx, const char *module, int schedulerId){
	QIA(
		"SELECT `id` FROM `controlmodules` "
		"WHERE name='%s' AND AND scheduler='%d'",
		module, schedulerId);
}

/*
 *	Get any platform id by name
 */
Eina_Bool getPlatformID(DB *db, Zentific_Ctx *ctx, const char *platform){
	QIA(
		"SELECT `id` FROM `platforms` WHERE name='%s'",
		platform);

}

Eina_Bool isModule(DB *db, Zentific_Ctx *ctx, const char *modname, int schedulerId){
	QIAB(
		"SELECT id FROM `controlmodules` WHERE name='%s' and scheduler='%d'",
		modname, schedulerId);

}

Eina_Bool isPlatformNameByScheduler(DB *db, Zentific_Ctx *ctx, const char *platname, int schedulerId){
	QIAB(
		"SELECT id FROM `platforms` WHERE name='%s' and scheduler='%d'",
		platname, schedulerId);

}

/* FIXME: module config data is left hanging around.
 *	  don't truncate, but something like
 * 	delete from config where mid IN (SELECT * from controlmodules where schedulerId = blah)?
 */
Eina_Bool clearControlModules(DB *db, Zentific_Ctx *ctx, int schedulerId){

	QE(
		"DELETE FROM `controlmodules` WHERE scheduler='%d'",
		schedulerId);

}
