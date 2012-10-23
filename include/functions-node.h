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
#ifndef Z_NODE_H
#define Z_NODE_H

#include "ztypes.h"

Eina_Bool _getNODEstat(Zentific_Ctx *ctx, const char *uuid, const char *stat);
Eina_Bool _getNODEattribute(Zentific_Ctx *ctx, const char *uuid, const char *attribute);
Eina_Bool _isNODEowner(Zentific_Ctx *ctx, const char *uuid, int uid);
Eina_Bool _registerNode(Zentific_Ctx *ctx, NODE *node);
Eina_Bool _getNodeControlModule(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _isNodeEnabled(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _isNodeConfigured(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _isNodeEnabledAndConfigured(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _updateNodeStats(Zentific_Ctx *ctx, const char *uuid, NODE_STATS *stats);
Eina_Bool _getLatestNodeStat(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getNodeStats(Zentific_Ctx *ctx, const char *uuid, int start_time, int end_time, int max_events, int interval);
Eina_Bool _getAllNodes(Zentific_Ctx *ctx, int type, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getAllNodesByUID(Zentific_Ctx *ctx, int uid, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getAllUnconfiguredNodes(Zentific_Ctx *ctx, int offset, int recordcount);
Eina_Bool _getAllUnconfiguredNodesByUID(Zentific_Ctx *ctx, int uid, int offset, int recordcount);
Eina_Bool _isNODE(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _addNode(Zentific_Ctx *ctx, NODE *node, Eina_Bool enabled);
Eina_Bool _removeNode(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getNodePlatform(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _setNodeControlModuleByName(Zentific_Ctx *ctx, const char *uuid, const char *controlmodule);
Eina_Bool _setNodeControlModule(Zentific_Ctx *ctx, const char *uuid, int controlmodule);
Eina_Bool _setNodeStatus(Zentific_Ctx *ctx, const char *uuid, Eina_Bool enabled);
Eina_Bool _getNodeName(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getNode(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getAllNodesFullRangeSorted(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getAllNodesFull(Zentific_Ctx *ctx);
Eina_Bool _getNodeByHostname(Zentific_Ctx *ctx, const char *hostname);
Eina_Bool _getNodeAddress(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getNodeDefaultVncPass(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _supportsHvm(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getNODEprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid);
Eina_Bool _grantNODEprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int level);
Eina_Bool _revokeNODEprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid);
Eina_Bool _getNodeByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getNodeHostname(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getNodeCount(Zentific_Ctx *ctx);
#endif

