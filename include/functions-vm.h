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
#ifndef Z_VM_H
#define Z_VM_H

#include "zentific.h"
#include "functions.h"
#include "functions-zentific.h"
#include "functions-vif.h"
#include "functions-jobs.h"
#include "functions-disk.h"
#include "functions-platform.h"

Eina_Bool _getTopNCpuConsumers(Zentific_Ctx *ctx, int intervalminutes, int n);
Eina_Bool _getTopNBandwidthConsumers(Zentific_Ctx *ctx, int intervalminutes, int n);
Eina_Bool _configPush(Zentific_Ctx *ctx, const char *uuid __UNUSED__);
Eina_Bool _isVMowner(Zentific_Ctx *ctx, const char *uuid, int uid);
Eina_Bool _getVMprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid);
Eina_Bool _grantVMprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int level);
Eina_Bool _revokeVMprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid);
Eina_Bool _getAllUsersByVm(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getAllUserGroupsByVm(Zentific_Ctx *ctx __UNUSED__, const char *uuid __UNUSED__);
Eina_Bool _isVM(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _setVmNeedsReboot(Zentific_Ctx *ctx, const char *uuid, Eina_Bool needreboot);
Eina_Bool _needsReboot(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getVMparentNode(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getVMparentAddress(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _saveVmNotes(Zentific_Ctx *ctx, const char *uuid, const char *notes);
Eina_Bool _getVmNotes(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getVMstat(Zentific_Ctx *ctx, const char *uuid, const char *stat);
Eina_Bool _getVMattribute(Zentific_Ctx *ctx, const char *uuid, const char *attribute);
Eina_Bool _saveVMrevision(Zentific_Ctx *ctx, const char *uuid, int uid, const char *name, const char *description);
Eina_Bool _getAllVmsByNode(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getVmsByNetwork(Zentific_Ctx *ctx, int nwid);
Eina_Bool _getVmsByNetworkFull(Zentific_Ctx *ctx, int nwid);
Eina_Bool _getAllVmsFullByNode(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getVMs(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getVMsUID(Zentific_Ctx *ctx, int uid, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getVM(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getVMsFullUID(Zentific_Ctx *ctx, int uid, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getVMsFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getVmByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getVmByRev(Zentific_Ctx *ctx, const char *uuid, int revision);
Eina_Bool _updateVMs(Zentific_Ctx *ctx, const char *uuid, Eina_List *vms);
Eina_Bool _isVmIgnored(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _removeVm(Zentific_Ctx *ctx, const char *uuid, Eina_Bool setInactive, Eina_Bool removeDisks __UNUSED__);
Eina_Bool _addVmToNode(Zentific_Ctx *ctx, const char *puuid, VM *vm, int caller_uid);
Eina_Bool vmAction(Zentific_Ctx *ctx, const char *uuid, int uid, const char *module_function, const char *input_string, const char *extra);
Eina_Bool _poweronVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid, Eina_Bool paused);
Eina_Bool _poweroffVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid);
Eina_Bool _shutdownVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid);
Eina_Bool _rebootVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid);
Eina_Bool _pauseVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid);
Eina_Bool _disableStateHandlingVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid);
Eina_Bool _enableStateHandlingVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid);
Eina_Bool _unpauseVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid);
Eina_Bool _suspendVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid, const char *destination);
Eina_Bool _snapshotVM(Zentific_Ctx *ctx, const char *uuid __UNUSED__, int caller_uid __UNUSED__, int storid __UNUSED__, Eina_Bool memoryOnly __UNUSED__);
Eina_Bool _restoreVM(Zentific_Ctx *ctx, const char *uuid __UNUSED__, int caller_uid __UNUSED__, const char *source __UNUSED__);
Eina_Bool _migrateVM(Zentific_Ctx *ctx, const char *uuid, int caller_uid, const char *destination);
Eina_Bool _saveVM(Zentific_Ctx *ctx, VM *vm, int uid __UNUSED__, Eina_Bool astemplate);
Eina_Bool _getAllTemplates(Zentific_Ctx *ctx);
Eina_Bool _getAllTemplatesFull(Zentific_Ctx *ctx);
Eina_Bool _getTemplate(Zentific_Ctx *ctx, int id);
Eina_Bool _getTemplateByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getTemplateConfigByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getTemplateConfig(Zentific_Ctx *ctx, int id);
Eina_Bool _ztoolsUpdate(Zentific_Ctx *ctx, const char *uuid, Eina_List *stats);
Eina_Bool _hasGuestTools(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _addTemplate(Zentific_Ctx *ctx, TEMPLATE *template);
Eina_Bool _modifyTemplate(Zentific_Ctx *ctx, int id, TEMPLATE *template);
Eina_Bool _getVmCount(Zentific_Ctx *ctx);
#endif
