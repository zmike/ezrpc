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
#ifndef Z_JOBS_H
#define Z_JOBS_H

#include "zentific.h"
#include "functions.h"

Eina_Bool _getJob(Zentific_Ctx *ctx, int id, int caller_uid);
Eina_Bool _getJobs(Zentific_Ctx *ctx);
Eina_Bool _getJobsByUID(Zentific_Ctx *ctx, int uid);
Eina_Bool _getJobsByUUID(Zentific_Ctx *ctx, const char * uuid);
Eina_Bool _getRecentJobs(Zentific_Ctx *ctx);
Eina_Bool _getRecentJobsByUID(Zentific_Ctx *ctx, int uid);
Eina_Bool _getJobsByJIDs(Zentific_Ctx *ctx, Eina_List *jobids, int caller_uid);
Eina_Bool _getJobsStatus(Zentific_Ctx *ctx, Eina_List *jobids, int caller_uid);
Eina_Bool _getJobStatus(Zentific_Ctx *ctx, int jobid, int caller_uid);
Eina_Bool _isJob(Zentific_Ctx *ctx, int id);
Eina_Bool _cancelJobsByUUID(Zentific_Ctx *ctx, const char *uuid, int caller_uid);
Eina_Bool _cancelJobsByUID(Zentific_Ctx *ctx, int uid, int caller_uid);
Eina_Bool _cancelJob(Zentific_Ctx *ctx, int jobid, int caller_uid);
Eina_Bool _addJob(Zentific_Ctx *ctx, JOB *job, int caller_uid);
Eina_Bool job_add(Zentific_Ctx *ctx, JOB *job, int caller_uid);
#endif
