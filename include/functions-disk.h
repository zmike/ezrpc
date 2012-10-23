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
#ifndef Z_DISK_H
#define Z_DISK_H

#include "zentific.h"
#include "functions.h"
#include "functions-zentific.h"

Eina_Bool _isDisk(Zentific_Ctx *ctx, const char *uuid, const char *ext_dev);
Eina_Bool _createDisk(Zentific_Ctx *ctx, const char *uuid, const char * path, int storid);
Eina_Bool _updateDiskStats(Zentific_Ctx *ctx, const char *uuid, DISK *disk);
Eina_Bool _getVmDisk(Zentific_Ctx *ctx, const char *uuid, const char *path);
Eina_Bool _getVMdisks(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getNumVMdisks(Zentific_Ctx *ctx, const char *uuid);
void disk_cb_updateconfig(Zentific_Ctx *ctx);
#endif
