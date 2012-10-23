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
#ifndef Z_NETWORK_H
#define Z_NETWORK_H

#include "ztypes.h"

Eina_Bool _isNetwork(Zentific_Ctx *ctx, NETWORK *net);
Eina_Bool _isNetworkId(Zentific_Ctx *ctx, int id);
Eina_Bool _getNetwork(Zentific_Ctx *ctx, int id);
Eina_Bool _getAllNetworksByType(Zentific_Ctx *ctx, const char *type, int offset, int recordcount);
Eina_Bool _getAllNetworks(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getAllNetworksFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _defineNetwork(Zentific_Ctx *ctx, NETWORK *net);
Eina_Bool _getNetworksByNode(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _attachNetworkToNode(Zentific_Ctx *ctx, const char *uuid, int nwid, int caller_uid);
Eina_Bool _detachNetworkFromNode(Zentific_Ctx *ctx, const char *uuid, int nwid, int caller_uid);
Eina_Bool _removeNetwork(Zentific_Ctx *ctx, int id);
Eina_Bool _attachNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac, int nwid, int caller_uid);
Eina_Bool _detachNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac, int nwid, int caller_uid);
Eina_Bool _addNetIf(Zentific_Ctx *ctx, const char *uuid, int nwid, NETIF *iface, int caller_uid);
Eina_Bool _removeNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac, int caller_uid);
Eina_Bool _modifyNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac, NETIF *netif, int caller_uid);
Eina_Bool _setNetworkControlModule(Zentific_Ctx *ctx, int nwid, int mid);
Eina_Bool _getNetworkControlModule(Zentific_Ctx *ctx, int nwid);
Eina_Bool  _getNetworkControlModules(Zentific_Ctx *ctx);
Eina_Bool _getNetworkControlModulesFull(Zentific_Ctx *ctx);
Eina_Bool _detachAllNodesByNetwork(Zentific_Ctx *ctx, int nwid, int caller_uid);
Eina_Bool _detachAllNetIfsByNetwork(Zentific_Ctx *ctx, int nwid, int caller_uid);
Eina_Bool _getNodesByNetwork(Zentific_Ctx *ctx, int nwid);
Eina_Bool _modifyNetwork(Zentific_Ctx *ctx, int id, NETWORK *net, int caller_uid);
Eina_Bool _getNetworkIdByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getNodesByNetworkFull(Zentific_Ctx *ctx, int nwid);
Eina_Bool _getNetworkCount(Zentific_Ctx *ctx);
#endif
