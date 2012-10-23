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
#ifndef Z_RPC_H
#define Z_RPC_H

#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include "zentific.h"
#include "functions.h"

//sha1 hashing
#include "sha1.h"
#include "sha1wrap.h"

Eina_Bool _getOperatingSystems(Zentific_Ctx *ctx);
Eina_Bool _getLanguages(Zentific_Ctx *ctx);
Eina_Bool _getConfig(Zentific_Ctx *ctx, const char *key);
Eina_Bool _getConfigsFull(Zentific_Ctx *ctx);
Eina_Bool _setConfigs(Zentific_Ctx *ctx, Eina_List *configs);
Eina_Bool _setConfig(Zentific_Ctx *ctx, const char *key, const char *value);
Eina_Bool _getAllPlugins(Zentific_Ctx *ctx, int which);
Eina_Bool _getFrontendPlugins(Zentific_Ctx *ctx, int which);
Eina_Bool _modifyPlugin(Zentific_Ctx *ctx, const char *name, Eina_Bool enabled, Eina_Bool xmlrpc, Eina_Bool frontend, Eina_Bool scheduler, Eina_Bool poller);
Eina_Bool _isFrontendPluginEnabled(Zentific_Ctx *ctx, const char *name);
Eina_Bool _cleanDB(Zentific_Ctx *ctx);
Eina_Bool _getLatestRevision(Zentific_Ctx *ctx, const char *uuid, unsigned char type);
Eina_Bool _generateMAC(Zentific_Ctx *ctx);
#endif
