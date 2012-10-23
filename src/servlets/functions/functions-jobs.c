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
#include "functions-jobs.h"
#include "functions-vm.h"
#include "functions-node.h"
#include "zentific-defines.h"
#include <Ecore_Con.h>

static Eina_Bool jobs_cb_connect_timer(Ecore_Con_Server *svr);
static Eina_Bool jobs_cb_connect(void *data, int type, Ecore_Con_Event_Server_Add *ev);
static Eina_Bool jobs_cb_data(void *data, int type, Ecore_Con_Event_Server_Data *ev);
static Eina_Bool jobs_cb_disconnect(void *data, int type, Ecore_Con_Event_Server_Del *ev);
static void jobs_cb_address(Zentific_Ctx *ctx, const char *ret);
static void jobs_kickScheduler(Zentific_Ctx *ctx, JOB *job, const char *address);
static void jobs_fill(Zentific_Ctx *ctx, JOB *ret);

static Eina_Bool
jobs_cb_connect_timer(Ecore_Con_Server *svr){
	Zentific_Ctx *ctx;

	ctx = ecore_con_server_data_get(svr);
	ctx->error = AZY_ERR(SCHEDULER_CONNECT_ERROR);
	ecore_con_server_del(svr);
	ctx->data = NULL;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
jobs_cb_connect(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Add *ev){
	Zentific_Ctx *ctx;
	JOB *job;
	Eina_Bool cancel = EINA_FALSE;

	ctx = ecore_con_server_data_get(ev->server);
	job = CTX_PARAM_GET;

	if (!zshare_strcmp(job->module_function, "cancel_job")) cancel = EINA_TRUE;

	if (cancel)
		ecore_con_server_send(ev->server, ZENSCHED_PROTO_CANCELJOBMSG, sizeof(ZENSCHED_PROTO_CANCELJOBMSG) - 1);
	else
		ecore_con_server_send(ev->server, ZENSCHED_PROTO_NEWJOBMSG, sizeof(ZENSCHED_PROTO_NEWJOBMSG) - 1);
	ecore_timer_interval_set(ctx->data, 3.0);
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
jobs_cb_data(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Data *ev){
	Zentific_Ctx *ctx;

	ctx = ecore_con_server_data_get(ev->server);
	if (strncmp(ev->data, ZENSCHED_PROTO_RESPONSE, ev->size))
		/* malformed response */
		ctx->error = AZY_ERR(SCHEDULER_CONNECT_ERROR);
	ecore_con_server_del(ev->server);
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
jobs_cb_disconnect(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Server_Del *ev){
	Zentific_Ctx *ctx;
	JOB *job;

	ctx = ecore_con_server_data_get(ev->server);
	job = CTX_PARAM_GET;
	if (!ctx->error) AZY_RETVAL(job);
	if (ctx->data) ecore_timer_del(ctx->data);
	JOB_free(job);
	RESUME;
	return ECORE_CALLBACK_CANCEL;
}

static void
jobs_cb_address(Zentific_Ctx *ctx, const char *ret){
	JOB *job;
	MARK_CB;
	DBG("(ctx=%p, ret=%s)", ctx, ret);
	job = CTX_PARAM_GET;
	if (ctx->error){
		JOB_free(job);
		ctx->error = AZY_ERR(SCHEDULER_NOT_FOUND);
		RESUME;
		return;
	}
	jobs_kickScheduler(ctx, job, ret);
	eina_stringshare_del(ret);
}

static void
jobs_kickScheduler(Zentific_Ctx *ctx, JOB *job, const char *address){
	Ecore_Con_Server *svr;

	if (!address){
		/* we're completely abstracted from the original method
		 * here so we have to start getting creative
		 */
		db->getSchedulerAddressByJob(db, ctx, job->id);
		CTX_CB_ADD(jobs_cb_address);
		CTX_PARAM_ADD(jobs_cb_address, job);
		if (ctx->error) RESUME;
		return;
	}

	svr = ecore_con_server_connect(ECORE_CON_REMOTE_NODELAY, address, DEFAULT_ZENSCHED_PORT, ctx);
	ecore_event_handler_add(ECORE_CON_EVENT_SERVER_ADD, (Ecore_Event_Handler_Cb)jobs_cb_connect, NULL);
	ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DEL, (Ecore_Event_Handler_Cb)jobs_cb_disconnect, NULL);
	ecore_event_handler_add(ECORE_CON_EVENT_SERVER_DATA, (Ecore_Event_Handler_Cb)jobs_cb_data, NULL);
	/* FIXME: handle error events after 1.1 release */
	ctx->data = ecore_timer_add(3.0, (Ecore_Task_Cb)jobs_cb_connect_timer, svr);
	CTX_PARAM_ADD(jobs_cb_disconnect, job);
	CTX_PARAM_ADD(jobs_cb_connect, job);
}

/** error becomes NOT_AUTHORIZED */
static void
ptr_job_cb(Zentific_Ctx *ctx, void *ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	if ((!ret) || ctx->error){
		if (!ret) ctx->error = AZY_ERR(NOT_AUTHORIZED);
		if (ctx->free_func && ret) ctx->free_func(ret);
		RESUME;
	}
	AZY_RETVAL(ret);
	if (ctx->free_func) ctx->free_func(ret);
	RESUME;
}

/** error becomes NOT_AUTHORIZED */
static void
ptr_int_cb(Zentific_Ctx *ctx, int ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	if ((!ret) || ctx->error){
		if (!ret) ctx->error = AZY_ERR(NOT_AUTHORIZED);
		RESUME;
	}
	AZY_RET(ret);
	RESUME;
}

static void
jobs_cb_resource(Zentific_Ctx *ctx, const char *ret){
	JOB *job;
	MARK_CB;
	DBG("(ctx=%p, ret=%s)", ctx, ret);
	job = CTX_PARAM_GET;
	if (ctx->error){
		JOB_free(job);
		eina_stringshare_del(ret);
		RESUME;
		return;
	}
	job->resource = ret;
	ctx->count += 100;
	jobs_fill(ctx, job);
}

static void
jobs_fill(Zentific_Ctx *ctx, JOB *ret){
	intptr_t *kick;

	if (ctx->count < 100){
		if (ctx->user_type) /* not superuser */
			eina_stringshare_replace(&ret->target_host, "");

		/* NOTE/TODO: this is a quick hack to expose some details about the resource
		 *  	being manipulated.
		 */
		/* TODO: examine impact if 'parentJob' is job->module_name
		 *       ALL schedulers will see 'parentJob' -- job fetch should take note of this
		 *	      or we're going to have a bug for all chained jobs
		 */
		if (zshare_validate_uuid(ret->target_vm) && zshare_validate_uuid(ret->target_node)){
			_getVMattribute(ctx, ret->target_vm, "name");
			CTX_CB_ADD(jobs_cb_resource);
			CTX_PARAM_ADD(jobs_cb_resource, ret);
		} else if (zshare_validate_uuid(ret->target_node) && (!ret->target_vm || !ret->target_vm[0])){
			_getNODEattribute(ctx, ret->target_node, "hostname");
			CTX_CB_ADD(jobs_cb_resource);
			CTX_PARAM_ADD(jobs_cb_resource, ret);
		}
		return;
	}
	kick = CTX_PARAM_GET;
	if (kick){
		ctx->count -= 100;
		jobs_kickScheduler(ctx, ret, NULL);
		return;
	}
	AZY_RETVAL(ret);
	RESUME;
}

static void
jobs_cb_add(Zentific_Ctx *ctx, int ret){
	intptr_t *uid;
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	uid = CTX_PARAM_GET;
	if (ctx->error || (!uid)){
		ctx->error = AZY_ERR(JOB_CREATION_FAILED);
		RESUME;
		return;
	}
	/* queue up a scheduler kick. using ctx is fine because the struct is never read from */
	CTX_PARAM_ADD(jobs_fill, ctx);
	_getJob(ctx, ret, *uid);
}

static void
jobs_cb_get(Zentific_Ctx *ctx, JOB *ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	if (ctx->error || (!ret)){
		/* not possible that job is not found, so must be not authorized */
		ctx->error = AZY_ERR(NOT_AUTHORIZED);
		RESUME;
		return;
	}
	jobs_fill(ctx, ret);
}

Eina_Bool _getJob(Zentific_Ctx *ctx, int id, int caller_uid){
	if ((id < 0) || (caller_uid < 0)){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}

	db->getJob(db, ctx, id, caller_uid);
	CTX_CB_ADD(jobs_cb_get);
	CTX_RETURN;
}

/*	returns all in-flight jobs
 *	completed/failed jobs are not returned
 */
Eina_Bool _getJobs(Zentific_Ctx *ctx){
	db->getJobs(db, ctx);
	CTX_RETURN;
}

/*	returns all in-flight jobs related to the
 *	requested UID; completed/failed jobs are not returned
 */
Eina_Bool _getJobsByUID(Zentific_Ctx *ctx, int uid){
	/* TODO: decide whether to change directionso here:
	 *	augment with jobs pertaining to resources owned by $uid as well? (now)
	 *	 or
	 *	return just jobs initiated by $uid?
	 */
	if (uid < 0)
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else db->getJobsByUID(db, ctx, uid);
	CTX_RETURN;
}

/*	returns all in-flight jobs related to the
 *	requested UUID; completed/failed jobs are not returned
 */
Eina_Bool _getJobsByUUID(Zentific_Ctx *ctx, const char * uuid){
	db->getJobsByUUID(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getRecentJobs(Zentific_Ctx *ctx){
	//param 3 = age in minutes.
	db->getRecentJobs(db, ctx, 30);
	CTX_CB_ADD(ptr_job_cb);
	CTX_RETURN;
}

Eina_Bool _getRecentJobsByUID(Zentific_Ctx *ctx, int uid){
	db->getRecentJobsByUID(db, ctx, uid, 30);
	CTX_CB_ADD(ptr_job_cb);
	CTX_RETURN;
}

Eina_Bool _getJobsByJIDs(Zentific_Ctx *ctx, Eina_List *jobids, int caller_uid){
	intptr_t *jid;
	Eina_List *l;

	if (caller_uid < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	EINA_LIST_FOREACH(jobids, l, jid)
		if (*jid < 0){
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}
	db->getJobsByIds(db, ctx, jobids, caller_uid);
	CTX_CB_ADD(ptr_job_cb);
	CTX_RETURN;
}

Eina_Bool _getJobsStatus(Zentific_Ctx *ctx, Eina_List *jobids, int caller_uid){
	intptr_t *jid;
	Eina_List *l;

	if (caller_uid < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	EINA_LIST_FOREACH(jobids, l, jid)
		if (*jid < 0){
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}
	db->getJobsStatus(db, ctx, jobids, caller_uid);
	CTX_CB_ADD(ptr_job_cb);
	CTX_RETURN;
}

/* TODO handle user group associations, etc to determine whether
 * 	caller_uid has ability to access job status
 * 	e.g. if job uid != caller_uid but (caller_uid is in group
 * 		of caller_uid and job gid == gid shared between caller_uid and job uid), then
 * 		share status
 * 	and similar variations..
 */
Eina_Bool _getJobStatus(Zentific_Ctx *ctx, int jobid, int caller_uid){
	if ((jobid < 0) || (caller_uid < 0))
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else {
		db->getJobStatus(db, ctx, jobid, caller_uid);
		CTX_CB_ADD(ptr_int_cb);
	}
	CTX_RETURN;
}

Eina_Bool _isJob(Zentific_Ctx *ctx, int id){
	if (id < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	db->isJob(db, ctx, id);
	CTX_RETURN;
}

static void
jobs_cancel(Zentific_Ctx *ctx, const char *uuid, int uid, int caller_uid, Eina_Bool user){
	Eina_List *jobs;
	intptr_t *jid;

	if (!ctx->count){
		if (user) _getJobsByUID(ctx, uid);
		else _getJobsByUUID(ctx, uuid);
		CTX_VALUE_RETURN;
		return;
	}

	if (ctx->error){
		if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){
			/* no jobs */
			ctx->error = 0;
			AZY_RETVAL(EINA_TRUE);
		}
		RESUME;
	}
	jobs = CTX_PARAM_GET;
	EINA_LIST_FREE(jobs, jid)
		_cancelJob(ctx, (intptr_t)jid, caller_uid);
}

Eina_Bool _cancelJobsByUUID(Zentific_Ctx *ctx, const char *uuid, int caller_uid){
	jobs_cancel(ctx, uuid, 0, caller_uid, EINA_FALSE);
	CTX_RETURN;
}

Eina_Bool _cancelJobsByUID(Zentific_Ctx *ctx, int uid, int caller_uid){
	jobs_cancel(ctx, NULL, uid, caller_uid, EINA_TRUE);
	CTX_RETURN;
}

/* TODO warning do caller_uid checks (verify caller can cancel target job)
 *	i.e. caller initiated the job, owns the target resource, and can
 *		alter the target resource (read only users should probably
 *		not be able to cancel).
 *		possily allow is a member of the group that initiated the job
 *		or is a member of a group that owns the resource
 */
Eina_Bool _cancelJob(Zentific_Ctx *ctx, int jobid, int caller_uid){
	JOB *canceljob;

	canceljob = JOB_new();
	canceljob->input_string = eina_stringshare_printf("%i", jobid);
	canceljob->module_name = eina_stringshare_add("zensched_internal");
	canceljob->module_function = eina_stringshare_add("cancel_job");

	job_add(ctx, canceljob, caller_uid);
	CTX_RETURN;
}

//#warning be sure to implement addScheduledJob
Eina_Bool _addJob(Zentific_Ctx *ctx, JOB *job, int caller_uid){
	if (caller_uid < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}

	job->uid = caller_uid;
	db->addJob(db, ctx, job);
	CTX_RETURN;
}

/* returns the actual job */
Eina_Bool job_add(Zentific_Ctx *ctx, JOB *job, int caller_uid){
	if (caller_uid < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}

	job->uid = caller_uid;
	db->addJob(db, ctx, job);
	CTX_CB_ADD(jobs_cb_add);
	CTX_PARAM_ADD(jobs_cb_add, (intptr_t*)caller_uid);
	CTX_RETURN;
}
