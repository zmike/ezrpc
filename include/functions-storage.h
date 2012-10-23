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
#ifndef Z_STORAGE_H
#define Z_STORAGE_H

#include "ztypes.h"

Eina_Bool _isStorage(Zentific_Ctx *ctx, STORAGE *stor);
Eina_Bool _isStorageId(Zentific_Ctx *ctx, int id);
Eina_Bool _getStorage(Zentific_Ctx *ctx, int id);
Eina_Bool _getAllStorageByType(Zentific_Ctx *ctx, const char *type, int offset, int recordcount);
Eina_Bool _getAllStorage(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getAllStorageFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _defineStorage(Zentific_Ctx *ctx, STORAGE *stor);
Eina_Bool _modifyStorage(Zentific_Ctx *ctx, int id, STORAGE *stor, int caller_uid);
Eina_Bool _getStorageControlModulesFull(Zentific_Ctx *ctx);
Eina_Bool _getStorageControlModules(Zentific_Ctx *ctx);
Eina_Bool _setStorageControlModule(Zentific_Ctx *ctx, int storid, int mid);
Eina_Bool _getStorageControlModule(Zentific_Ctx *ctx, int storid);
Eina_Bool _getStorageIdByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getStorageByNode(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _removeStorage(Zentific_Ctx *ctx, int id);
Eina_Bool _attachDiskToStorage(Zentific_Ctx *ctx, int storid, const char * path, int caller_uid);
Eina_Bool _detachDiskFromStorage(Zentific_Ctx *ctx, int storid, const char *path, int caller_uid);
Eina_Bool _addDiskToStorage(Zentific_Ctx *ctx, int storid, DISK *disk, int caller_uid);
Eina_Bool _removeDiskFromStorage(Zentific_Ctx *ctx, int storid, const char * path, int caller_uid);
Eina_Bool _modifyDiskByStorage(Zentific_Ctx *ctx, int storid, const char *path, DISK *disk, int caller_uid);
Eina_Bool _detachAllNodesByStorage(Zentific_Ctx *ctx, int storid, int caller_uid);
Eina_Bool _detachAllDisksByStorage(Zentific_Ctx *ctx, int storid, int caller_uid);
Eina_Bool _attachStorageToNode(Zentific_Ctx *ctx, const char * uuid, int storid, int caller_uid);
Eina_Bool _detachStorageFromNode(Zentific_Ctx *ctx, const char * uuid, int storid, int caller_uid);
Eina_Bool _getNodesByStorage(Zentific_Ctx *ctx, int storid);
Eina_Bool _getVmsByStorage(Zentific_Ctx *ctx, int storid);
Eina_Bool _defineDisk(Zentific_Ctx *ctx, int storid, DISK * disk, int caller_uid __UNUSED__);
Eina_Bool _enableDiskByStorage(Zentific_Ctx *ctx, int storid, const char * path, int caller_uid);
Eina_Bool _disableDiskByStorage(Zentific_Ctx *ctx, int storid, const char * path, int caller_uid);
Eina_Bool _getAllStorageTypes(Zentific_Ctx *ctx);
Eina_Bool _getDisksByStorage(Zentific_Ctx *ctx, int storid);
Eina_Bool _getDiskByStorage(Zentific_Ctx *ctx, int storid, const char *path);
Eina_Bool _getVmByDisk(Zentific_Ctx *ctx, int storid, const char * path);
Eina_Bool _isDiskByStorage(Zentific_Ctx *ctx, int storid, const char *path);
Eina_Bool _getStorageCount(Zentific_Ctx *ctx);
#endif
