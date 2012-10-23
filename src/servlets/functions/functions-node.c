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
#include "functions-node.h"
#include "functions-jobs.h"
#include "functions-platform.h"
#include "functions-user.h"
#include <sys/time.h>

/* NOTE: this function cannot and should not be replaced
 * 	by NODE_eq, however tempting that may be.
 * 	NODE has 'enabled' and 'controlmodule' which are
 * 	values configured at the zrpc level.
 * 	they are sent as struct members by zpoll just to
 * 	make zrpc happy, but they are not knowable values
 * 	for zpoll.
 *
 * NOTE2: node->cpu_mhz in here may cause problems with
 * 	systems having CPU freq scaling enabled because
 * 	the value will flap back and forth..
 */
static int nodeconfig_eq(NODE *a, NODE *b){
	if (a == b)
		return 1;
	if ((!a) || (!b))
		return 0;
	if (a->kernel != b->kernel)
		return 0;
	if (a->address != b->address)
		return 0;
	if (a->os != b->os)
		return 0;
	if (a->architecture != b->architecture)
		return 0;
	if (a->hostname != b->hostname)
		return 0;
	if (a->domainname != b->domainname)
		return 0;
	if (a->platformver != b->platformver)
		return 0;
	if (a->platformbuild != b->platformbuild)
		return 0;
	if (a->platformvendor != b->platformvendor)
		return 0;
	if (a->platformfull != b->platformfull)
		return 0;
	if (a->platform != b->platform)
		return 0;
	if (a->default_vnc_pass != b->default_vnc_pass)
		return 0;
	if (a->num_cpus != b->num_cpus)
		return 0;
	if (a->cores_per_socket != b->cores_per_socket)
		return 0;
	if (a->threads_per_core != b->threads_per_core)
		return 0;
	if (a->num_cpu_nodes != b->num_cpu_nodes)
		return 0;
	if (a->sockets_per_node != b->sockets_per_node)
		return 0;
	if (a->cpu_mhz != b->cpu_mhz)
		return 0;
	if (a->total_phys_mem != b->total_phys_mem)
		return 0;
	if (a->hvm != b->hvm)
		return 0;
	if (a->capabilities != b->capabilities)
		return 0;

	return 1;
}

static void
nodeconfig_update(Zentific_Ctx *ctx, NODE *aNode){
	NODE *stored;

	switch (ctx->count){
		case 2:
			db->createNode(db, ctx, aNode);
			CTX_CB_ADD(cb_int_rewind);
		case 3:
			db->getNode(db, ctx, aNode->uuid);
			CTX_VALUE_RETURN;
			break;
		case 4:
			stored = CTX_PARAM_GET;
			if (!nodeconfig_eq(aNode, stored)) {
				INF("Node '%s' config changed, updating", aNode->uuid);
				db->updateNodeConfig(db, ctx, aNode);
				CTX_CB_ADD(cb_int_rewind);
			} else
				/* node is unchanged */
				INF("Node '%s' unchanged", aNode->uuid);
			NODE_free(stored);
		default:
			break;
	}
}

Eina_Bool _getNODEstat(Zentific_Ctx *ctx, const char *uuid, const char *stat){
	db->getNODEstat(db, ctx, uuid, stat);
	CTX_RETURN;
}

Eina_Bool _getNODEattribute(Zentific_Ctx *ctx, const char *uuid, const char *attribute){
	db->getNODEattribute(db, ctx, uuid, attribute);
	CTX_RETURN;
}

Eina_Bool _isNODEowner(Zentific_Ctx *ctx, const char *uuid, int uid){
	if (!ctx->user_type)
		db->isNode(db, ctx, uuid);
	else
	db->isNODEowner(db, ctx, uuid, uid); /* automatically returns UNKNOWN_RESOURCE if insufficient permissions */
	CTX_RETURN;
}

//FIXME make a refresh_database function
//	errmsg.h from /usr/include/mysql and
//	mysqld_error.h from the same dir
//	contain error codes yielded from mysql functions
//	*in particular, test for 2006 (server gone)

/* NODE functions */

/* called any time zpoll starts or the scheduler spots a new node */
Eina_Bool _registerNode(Zentific_Ctx *ctx, NODE *node) {
	NODE *cfg = NULL;

	switch (ctx->count){
		case 0:
			_isNODE(ctx, node->uuid);
			CTX_CB_ADD(cb_int_rewind);
			break;
		case 1:
			if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){
				/* we do not exist, so start an introduction
				 *	but do NOT enable the node, as an introduced
				 *	node in this context is unconfigured from a
				 *	control module standpoint
				 */
				INF("Node with uuid '%s'  recorded,"
					" set to disabled. Requires configuration.",
					node->uuid);
				node->enabled = EINA_FALSE;
				db->createNode(db, ctx, node);
				CTX_CB_ADD(cb_int_rewind);
				ctx->count++;
				/* the fuck if I'm unsetting this error */
				break;
			}
			ctx->error = 0;
			db->getNodeConfiguredState(db, ctx, node->uuid);
			ctx->count = 9;
			CTX_VALUE_RETURN;
		case 3:
			nodeconfig_update(ctx, node);
			break;
		case 4:
			nodeconfig_update(ctx, node);
			if (ctx->error) AZY_RETVAL(EINA_FALSE);
			else AZY_RETVAL(EINA_TRUE);
			break;
		case 10:
			cfg = CTX_PARAM_PEEK;
			if (cfg->address != node->address){
				WRN("Node='%s' host changed from old='%s' to new='%s'.",
					node->uuid, cfg->address, node->address);
				/*
				FIXME
				setNodeNeedsAttention
				requires an alert db table, getAlerts(), getAlert(),
					ALERT { int id, int nodeuuid, int vmuuid, int uid, int message, boolean acknowledged
				*/
				if (zshare_validate_ip(node->address)){
					db->setNodeConfiguredHost(db, ctx, node->uuid, node->address);
					CTX_CB_ADD(cb_int_rewind);
					break;
				}
				WRN("invalid IP, not updating configured host for node='%s'",
					node->uuid);
			}
		case 11:
			if (!cfg) cfg = CTX_PARAM_PEEK;
			if (ctx->error) {
				ERR("Failed to update address for node='%s'", node->uuid);
				ctx->error = 0;
			}
			if (!cfg->enabled){
				INF("Node '%s' is disabled.", node->uuid);
				AZY_RETVAL(EINA_FALSE);
				ctx->count = 3;
				REWIND;
				NODE_free(cfg);
				break;
			}
			_isControlModuleByName(ctx, cfg->controlmodule);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 12:
			cfg = CTX_PARAM_GET;
			if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){
				/* the node is enabled, yet its configured control module is invalid
				 *	e.g., the reported scheduler modules changed after inital setup
				 */
				ERR("Control module '%s' for node='%s' does not exist.",
					cfg->controlmodule, node->uuid);

				/* disable the node as a result */
				_setNodeStatus(ctx, node->uuid, EINA_FALSE);
				AZY_RETVAL(EINA_FALSE);
				CTX_CB_ADD(cb_int_rewind);
			}
			ctx->count = 2;
			NODE_free(cfg);
			break;
	}
	CTX_RETURN;
}

Eina_Bool _getNodeControlModule(Zentific_Ctx *ctx, const char *uuid){
	db->getNodeControlModule(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _isNodeEnabled(Zentific_Ctx *ctx, const char *uuid){
	db->isNodeEnabled(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _isNodeConfigured(Zentific_Ctx *ctx, const char *uuid){
	db->getNodeControlModule(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _isNodeEnabledAndConfigured(Zentific_Ctx *ctx, const char *uuid){
	db->isNodeEnabledAndConfigured(db, ctx, uuid);
	CTX_RETURN;
}

/* NOTE not intended to be called from API */
Eina_Bool _updateNodeStats(Zentific_Ctx *ctx, const char *uuid, NODE_STATS *stats){
	if (!ctx->count) {
		db->isNodeEnabledAndConfigured(db, ctx, uuid);
		CTX_CB_ADD(cb_bool_rewind);
		CTX_RETURN;
	}
	else if (!ctx->error) db->updateNodeStats(db, ctx, uuid, stats);
	CTX_RETURN;
}


//FIXME in updateNodeStats, address, hostname, domainname are never updated
//	is is ESSENTIAL that they are if changes are made to a node
Eina_Bool _getLatestNodeStat(Zentific_Ctx *ctx, const char *uuid){
	db->getLatestNodeStat(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getNodeStats(Zentific_Ctx *ctx, const char *uuid, int start_time, int end_time, int max_events, int interval){
	if ((start_time < 0) || (end_time <= 0) || (end_time <= start_time) || (max_events <= 0) || (interval <= 0))
		ctx->error = AZY_ERR(INVALID_PARAMETER);
	else db->getNodeStats(db, ctx, uuid, start_time, end_time, max_events, interval);
	CTX_RETURN;
}


/* get all nodes disabled and enabled*/
Eina_Bool _getAllNodes(Zentific_Ctx *ctx, int type, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield && sortfield[0])
		db->getAllNodesSorted(db, ctx, type, sortfield, desc, offset, recordcount);
	else
		db->getAllNodes(db, ctx, type, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllNodesByUID(Zentific_Ctx *ctx, int uid, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield && sortfield[0])
		db->getAllNodesByUIDSorted(db, ctx, -1, sortfield, desc, uid, offset, recordcount);
	else
		db->getAllNodesByUID(db, ctx, -1, uid, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllUnconfiguredNodes(Zentific_Ctx *ctx, int offset, int recordcount){
	db->getAllUnconfiguredNodes(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllUnconfiguredNodesByUID(Zentific_Ctx *ctx, int uid, int offset, int recordcount){
	db->getAllUnconfiguredNodesByUID(db, ctx, uid, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _isNODE(Zentific_Ctx *ctx, const char *uuid){
	if (!zshare_validate_uuid(uuid)){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	db->isNode(db, ctx, uuid);
	CTX_RETURN;
}

/* called to pre-configure a node before it has been observed via zpoll or otherwise contacted */
Eina_Bool _addNode(Zentific_Ctx *ctx, NODE *node, Eina_Bool enabled) {

	switch (ctx->count){
		case 0:
			if (!node || !node->platform){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				CTX_RETURN;
			}
			_isNODE(ctx, node->uuid);
			CTX_CB_ADD(cb_int_rewind);
			break;
		case 1:
			if (ctx->error != AZY_ERR(UNKNOWN_RESOURCE)){
				ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
				ERR("Node with uuid '%s' is already known. Aborting.", node->uuid);
				break;
			}
			ctx->error = 0;
			INF("no node found, proceeding");
			db->isPlatformByName(db, ctx, node->platform);
			CTX_CB_ADD(cb_int_rewind);
			break;
		case 2:
			if (ctx->error != AZY_ERR(UNKNOWN_RESOURCE)){
				WRN("Platform '%s' not found; adding node, but ignoring input and forcing it to be disabled",
					node->platform);
				enabled = EINA_FALSE;
			} else
				INF("Platform '%s' found, now creating Node", node->platform);
			ctx->error = 0;
			node->enabled = enabled;
		default:
			if (ctx->error) break;
			nodeconfig_update(ctx, node);
	}
	CTX_RETURN;
}

/* FIXME upon node removal what shall we do? */
Eina_Bool _removeNode(Zentific_Ctx *ctx, const char *uuid){
	db->removeNode(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getNodePlatform(Zentific_Ctx *ctx, const char *uuid){
	db->getNodePlatform(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _setNodeControlModuleByName(Zentific_Ctx *ctx, const char *uuid, const char *controlmodule){
	const char *platform = NULL;

	switch (ctx->count){
		case 0:
			/* case of controlmodule == ''
			 *	NODE: this action implicitly disables
			 *		the node
			 *	FIXME: set 'needs attention' flag for node
			 */
			if (!controlmodule || !controlmodule[0]){
				_setNodeStatus(ctx, uuid, EINA_FALSE);
				db->setNodeControlModuleByName(db, ctx, uuid, controlmodule);
				CTX_CB_ADD(cb_int_rewind);
				break;
			}
			db->getNodePlatform(db, ctx, uuid);
			CTX_VALUE_RETURN;
			break;
		case 1:
			if (!controlmodule || !controlmodule[0]){
				if (!ctx->error) AZY_RETVAL(EINA_TRUE);
				break;
			}
			platform = CTX_PARAM_GET;
			if (!platform) {
				AZY_RETVAL(EINA_FALSE);
				break;
			}
			_isControlModuleForPlatformByName(ctx, controlmodule, platform);
			eina_stringshare_del(platform);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (!ctx->error)
				//check it's a module and associated with that platform
				//FIXME handle node config revision just like vms too
				db->setNodeControlModuleByName(db, ctx, uuid, controlmodule);
			else if (ctx->count == 2) {
				//FIXME: this line should go away once modules exist
				_isControlModuleForPlatform(ctx, controlmodule, -1);
				CTX_CB_ADD(cb_bool_rewind);
			} else
				ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
	}
	CTX_RETURN;
}

//#warning make sure to set a scheduled job for periodic node polling by control module if the admin specifies it

//TODO merge with the unnecessarily duplicated setNodeControlModuleByName
Eina_Bool _setNodeControlModule(Zentific_Ctx *ctx, const char *uuid, int controlmodule){
	const char *platform = NULL;

	switch (ctx->count){
		case 0:
			/* case of controlmodule == ''
			 *	NODE: this action implicitly disables
			 *		the node
			 *	FIXME: set 'needs attention' flag for node
			 */
			if (!controlmodule){
				_setNodeStatus(ctx, uuid, EINA_FALSE);
				db->setNodeControlModuleByName(db, ctx, uuid, "");
				CTX_CB_ADD(cb_int_rewind);
				break;
			}
			db->getNodePlatform(db, ctx, uuid);
			CTX_VALUE_RETURN;
			break;
		case 1:
			if (!controlmodule){
				if (!ctx->error) AZY_RETVAL(EINA_TRUE);
				break;
			}
			platform = CTX_PARAM_GET;
			if (!platform) {
				AZY_RETVAL(EINA_FALSE);
				break;
			}
			db->isControlModuleForPlatformById(db, ctx, controlmodule, platform);
			eina_stringshare_del(platform);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (!ctx->error)
				//check it's a module and associated with that platform
				//FIXME handle node config revision just like vms too
				db->setNodeControlModule(db, ctx, uuid, controlmodule);
			else if (ctx->count == 2) {
				//FIXME: this line should go away once modules exist
				db->isControlModuleForPlatformByMid(db, ctx, controlmodule, -1);
				CTX_CB_ADD(cb_bool_rewind);
			} else
				ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
	}
	CTX_RETURN;
}

Eina_Bool _setNodeStatus(Zentific_Ctx *ctx, const char *uuid, Eina_Bool enabled){
	db->setNodeStatus(db, ctx, uuid, enabled);
	CTX_RETURN;
}

Eina_Bool _getNodeName(Zentific_Ctx *ctx, const char *uuid){
	db->getNodeName(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getNode(Zentific_Ctx *ctx, const char *uuid){
	db->getNode(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getAllNodesFullRangeSorted(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	db->getAllNodesFullRangeSorted(db, ctx, sortfield, desc, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllNodesFull(Zentific_Ctx *ctx){
	db->getAllNodesFull(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getNodeByHostname(Zentific_Ctx *ctx, const char *hostname){
	db->getNodeByHostname(db, ctx, hostname);
	CTX_RETURN;
}

Eina_Bool _getNodeAddress(Zentific_Ctx *ctx, const char *uuid){
	db->getNodeAddress(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getNodeDefaultVncPass(Zentific_Ctx *ctx, const char *uuid){
	db->getNodeDefaultVNCpass(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _supportsHvm(Zentific_Ctx *ctx, const char *uuid){
	db->getNodeHVM(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getNODEprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid){
	db->getPrivilege(db, ctx, uuid, uid, gid, 0);
	CTX_RETURN;
}

Eina_Bool _grantNODEprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int level){
	db->grantPrivilege(db, ctx, uuid, uid, gid, level, 0);
	CTX_RETURN;
}

Eina_Bool _revokeNODEprivilege(Zentific_Ctx *ctx, const char *uuid, int uid, int gid){
	db->revokePrivilege(db, ctx, uuid, uid, gid, 0);
	CTX_RETURN;
}

Eina_Bool _getNodeByName(Zentific_Ctx *ctx, const char *name){
	db->getNodeByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _getNodeHostname(Zentific_Ctx *ctx, const char *uuid){
	db->getNodeConfiguredHost(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _getNodeCount(Zentific_Ctx *ctx){
	if (!ctx->user_type)
		db->getNodeCount(db, ctx);
	else
		db->getNodeCountByUID(db, ctx, SES->uid);
	CTX_RETURN;
}
