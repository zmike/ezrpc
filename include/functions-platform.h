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
#ifndef Z_PLATFORM_H
#define Z_PLATFORM_H

#include "zentific.h"
#include "functions.h"

Eina_Bool _getPlatforms(Zentific_Ctx *ctx);
Eina_Bool _getControlModulesByPlatform(Zentific_Ctx *ctx, int platformid);
Eina_Bool _getPlatform(Zentific_Ctx *ctx, int id);
Eina_Bool _getPlatformByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _addPlatform(Zentific_Ctx *ctx, PLATFORM *p);
Eina_Bool _modifyPlatform(Zentific_Ctx *ctx, int id, PLATFORM *p);
Eina_Bool _setPlatformConfig(Zentific_Ctx *ctx, int id, Eina_List /*KEYVALUE*/ *config);
Eina_Bool _setSecureControlModuleConfig(Zentific_Ctx *ctx, int mid, KEYVALUE *modconf);
Eina_Bool _getPlatformConfig(Zentific_Ctx *ctx, int id);
Eina_Bool _getPlatformsFull(Zentific_Ctx *ctx);
Eina_Bool _getControlModulesFull(Zentific_Ctx *ctx);
Eina_Bool _getControlModules(Zentific_Ctx *ctx);
Eina_Bool _getControlModuleNameByNetwork(Zentific_Ctx *ctx, int nwid);
Eina_Bool _getControlModule(Zentific_Ctx *ctx, int id);
Eina_Bool _getControlModuleByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _isPlatform(Zentific_Ctx *ctx, int id);
Eina_Bool _isPlatformByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _isControlModule(Zentific_Ctx *ctx, int id);
Eina_Bool _isControlModuleByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _isControlModuleForPlatform(Zentific_Ctx *ctx, const char *name, int platformid);
Eina_Bool _isControlModuleForPlatformByName(Zentific_Ctx *ctx, const char *name, const char *platformname);
Eina_Bool _getControlModuleConfig(Zentific_Ctx *ctx, int id);
Eina_Bool _setControlModuleConfig(Zentific_Ctx *ctx, int mid, Eina_List /*KEYVALUE*/ *config);
Eina_Bool _enablePlatform(Zentific_Ctx *ctx, int id);
Eina_Bool _disablePlatform(Zentific_Ctx *ctx, int id);
Eina_Bool _enableControlModule(Zentific_Ctx *ctx, int mid);
Eina_Bool _disableControlModule(Zentific_Ctx *ctx, int mid);
Eina_Bool _getPlatformIdByVm(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getPlatformFeatures(Zentific_Ctx *ctx, int platform);
#endif
