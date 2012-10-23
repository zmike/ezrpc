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
#ifndef Z_VIF_H
#define Z_VIF_H

#include "zentific.h"
#include "functions.h"
#include "functions-zentific.h"

Eina_Bool _isNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac);
Eina_Bool _createNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac);
Eina_Bool _getNumVMvifs(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getNetIfByMAC(Zentific_Ctx *ctx, const char *uuid, const char *mac);
Eina_Bool _updateNetIfStats(Zentific_Ctx *ctx, const char *uuid, NETIF *vif);
Eina_Bool _getVMvifs(Zentific_Ctx *ctx, const char *uuid);
int NETIF_mac_cmp(NETIF *a, NETIF *b);
int NETIF_config_cmp(NETIF *a, NETIF *b);
void vif_cb_updateconfig(Zentific_Ctx *ctx);
#endif
