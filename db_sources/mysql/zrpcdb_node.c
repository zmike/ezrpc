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
#include "zrpcdb.h"
/* ----------------------------------------------- */
/* API Functions: NODE   			   */
/* ----------------------------------------------- */

Eina_Bool getNODEstat(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *stat){
	QSA(
		"SELECT `%s` FROM `nodestats` "
		"LEFT JOIN `nodes` ON nodes.id=nodestats.node "
		"WHERE nodes.uuid='%s' "
		"ORDER BY nodestats.timestamp DESC LIMIT 1",
		 stat, uuid);
}

Eina_Bool getNODEattribute(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *attribute){
	QSA(
		"SELECT `%s` FROM `nodeconfig` "
		"LEFT JOIN `nodes` ON nodes.id=nodeconfig.node "
		"WHERE nodes.uuid='%s' "
		"ORDER BY nodeconfig.timestamp DESC LIMIT 1",
		 attribute, uuid);
}

//FIXME
Eina_Bool getLatestNodeStat(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	QUERY(NODE_STATS_esql,
		"SELECT * FROM nodes "
		"RIGHT JOIN nodestats ON nodes.id=nodestats.node "
		"WHERE nodestats.timestamp <= NOW() "
		"AND nodes.uuid='%s' "
		"ORDER BY nodestats.timestamp DESC LIMIT 1",
		uuid);
	ctx->free_func = (Ecore_Cb)NODE_STATS_free;
	QID_RETURN;
}

Eina_Bool getNodeAddress(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT address FROM `nodeconfig` "
		"JOIN nodes ON nodeconfig.node=nodes.id "
		"WHERE nodes.uuid='%s' LIMIT 1",
		uuid);
}

/* FIXME: handle interval, or do so in the function calling this one by calling getNodeStats{Week,Day,...}(..) */
Eina_Bool getNodeStats(DB *db, Zentific_Ctx *ctx, const char *uuid, int start_epoch, int end_epoch, int max_events, int interval __UNUSED__){
	DB_VARS;
	QUERY(Array_NODE_STATS_esql,
		"SELECT * FROM nodes "
		"RIGHT JOIN nodestats ON nodes.id=nodestats.node "
		"WHERE nodestats.timestamp <= NOW() "
		"AND nodes.uuid='%s' "
		"AND nodestats.timestamp >= FROM_UNIXTIME('%d') "
		"AND nodestats.timestamp <= FROM_UNIXTIME('%d') "
		"ORDER BY nodestats.timestamp DESC LIMIT 0, %d",
		uuid, start_epoch, end_epoch,
		((max_events < 1) || (max_events > 100)) ? 30 : max_events);
	ctx->free_func = (Ecore_Cb)Array_NODE_STATS_free;
	QID_RETURN;
}

Eina_Bool updateNodeStats(DB *db, Zentific_Ctx *ctx, const char *uuid, NODE_STATS *stats){
	/* FIXME: FIX THIS so the other params are used.
	 * 	for example, the node id param may be useful in this context
	 *	especially
	 */
	/* FIXME / TODO
	 * 	at the moment, this sql insert **implicitly**
	 * 	sets the timestamp as NOW() - i.e., when the query is
	 * 	itself executed and committed.
	 *
	 * 	It may be much more appropriate to extract the
	 * 	timestamp from the node_stats->timestamp variable
	 * 	which is an int (seconds; unixtime);
	 *
	 * 	IF query is processed in a delayed manner long
	 *	after the stats actually arrived, this may produce
	 *	incorrect or otherwise misleading results in calculations
	 *	or statistical analyses which are very time sensitive
	 *	and related to performance statistics.
	 *
	 *	Food for thought.
	 */
	QE(
		"IF("
			"SELECT value FROM `config` WHERE `key`='stat_retention'"
			" AND `subsystem`='zrpc' AND `mid`='-1'"
			" AND `uid`='-1' AND `gid`='-1' AND `vgid`='-1'"
			" AND `ngid`='-1' AND `vm`='-1' AND `node`='-1'"
			" AND `platform`='-1'"
			", "
			"DELETE FROM `nodestats` where nodestats.timestamp < DATE_SUB(CURDATE(), INTERVAL ("
				"SELECT value FROM `config` WHERE `key`='stat_retention'"
				" AND `subsystem`='zrpc' AND `mid`='-1'"
				" AND `uid`='-1' AND `gid`='-1' AND `vgid`='-1'"
				" AND `ngid`='-1' AND `vm`='-1' AND `node`='-1'"
				" AND `platform`='-1'"
				") HOUR) AND node="
					"(SELECT nodes.id FROM `nodes` WHERE nodes.uuid='%s')"
			", "
			"DELETE FROM `nodestats` where nodestats.timestamp < DATE_SUB(CURDATE(), INTERVAL 31"
				" HOUR) AND node="
					"(SELECT nodes.id FROM `nodes` WHERE nodes.uuid='%s')"
		");"
		"INSERT INTO `nodestats`"
		"       (node, "
		"       iowait, loadavg, uptime, cpupct,"
		"       intr, ctxts, num_procs, mem_free,"
		"       mem_total, mem_shared, mem_buffered, swap_total,"
		"       swap_free, total_phys_mem, free_phys_mem)"
		" VALUES ("
		"       (SELECT id FROM `nodes` WHERE uuid='%s' ORDER BY id DESC LIMIT 1),"
		"       '%lf', '%s', '%lf', '%lf',"
		"       '%d', '%d', '%d', '%d',"
		"       '%d', '%d', '%d', '%d', '%d',"
		"       '%lf', '%lf'"
		" );",
		uuid, uuid, uuid,
		stats->iowait, stats->loadavg, stats->uptime, stats->cpupct,
		stats->intr, stats->ctxts, stats->num_procs, stats->mem_free,
		stats->mem_total, stats->mem_shared, stats->mem_buffered, stats->swap_total,
		stats->swap_free,
		stats->total_phys_mem, stats->free_phys_mem);
}

Eina_Bool updateNodeConfig(DB *db, Zentific_Ctx *ctx, NODE *node){
	QE(
		"INSERT INTO `nodeconfig`"
		"(node, revision, kernel, os, architecture, hostname, address, default_vnc_pass, "
		"domainname, platformver, platform, num_cpus, "
		"cores_per_socket, threads_per_core, num_cpu_nodes, sockets_per_node, "
		"cpu_mhz, total_phys_mem, hvm, "
		"platformfull, platformvendor, platformbuild, capabilities)"
		"VALUES ("
		"(SELECT id FROM `nodes` WHERE uuid='%s' ORDER BY id DESC LIMIT 1), "
		"((SELECT revision FROM `nodeconfig` "
			"RIGHT JOIN `nodes` ON nodeconfig.node=nodes.id "
			"WHERE nodes.uuid='node' "
			"ORDER BY nodeconfig.timestamp DESC LIMIT 1) + 1), "
		"'%s', '%s', '%s', '%s', '%s', '%s', "
		"'%s', '%s', '%s', '%d', "
		"'%d', '%d', '%d', '%d', "
		"'%lf', '%lf', '%d', "
		"'%s', '%s', '%s', '%s'"
		")",
		node->uuid,
		node->kernel, node->os, node->architecture, node->hostname, node->address, node->default_vnc_pass,
		node->domainname, node->platformver, node->platform, node->num_cpus,
		node->cores_per_socket, node->threads_per_core, node->num_cpu_nodes, node->sockets_per_node,
		node->cpu_mhz, node->total_phys_mem, node->hvm,
		node->platformfull, node->platformvendor, node->platformbuild, node->capabilities);
}

Eina_Bool getNodeControlModule(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT controlmodule FROM nodes WHERE uuid='%s'",
		uuid);
}

Eina_Bool setNodeConfiguredHost(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *host){
	QE(
		"UPDATE `nodes` SET `configuredhost`='%s' WHERE `uuid`='%s'", host, uuid);
}

Eina_Bool getNodeConfiguredHost(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT `configuredhost` FROM `nodes` WHERE `uuid`='%s'",
		uuid);
}

Eina_Bool createNode(DB *db, Zentific_Ctx *ctx, NODE *node){
	QE(
		"INSERT INTO `nodes` (`uuid`, `configuredhost`, `enabled`) "
		"VALUES ('%s', '%s', '%i')",
		node->uuid, node->address, !!node->enabled);
}

Eina_Bool setNodeStatus(DB *db, Zentific_Ctx *ctx, const char *uuid, Eina_Bool enabled){
	QE(
		"UPDATE `nodes` SET enabled='%d' WHERE uuid='%s'",
		!!enabled, uuid);
}

Eina_Bool setNodeControlModuleByName(DB *db, Zentific_Ctx *ctx, const char *nodeuuid, const char *controlmodule){
	QE(
		"UPDATE `nodes` SET `controlmodule` = '%s' WHERE uuid='%s'",
		controlmodule, nodeuuid);
}

Eina_Bool setNodeControlModule(DB *db, Zentific_Ctx *ctx, const char *nodeuuid, int controlmodule){
	QE(
		"UPDATE `nodes` "
		"JOIN controlmodules ON nodes.controlmodule=controlmodules.name "
		"SET nodes.controlmodule=controlmodules.name "
		"WHERE controlmodules.id='%i' "
		"AND uuid='%s'",
		controlmodule, nodeuuid);
}

Eina_Bool removeNode(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QE(
		"DELETE FROM `nodes` WHERE `uuid`='%s'", uuid);
}

Eina_Bool isNodeEnabled(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QBA(
		"SELECT enabled from `nodes` where uuid='%s'", uuid);
}

static void
zrpcdb_node_cb_conf(Esql_Res *res, Zentific_Ctx *ctx, int id __UNUSED__){
	Z_Ctx_Cb end_cb;
	if (!esql_res_rows_count(res)){
		ctx->error = AZY_ERR(NODE_UNCONFIGURED);
		ecore_event_add(ZRPCDB_EVENT_ERROR, ctx, fake_free, NULL);
		return;
	}
	if (ctx->cbs){
		end_cb = ctx->cbs->data;
		ctx->state = EINA_TRUE;
		end_cb(ctx, (void*)EINA_TRUE);
	} else {
		ctx->retval = (void*)EINA_TRUE;
		ecore_event_add(ZRPCDB_EVENT_RESULT, ctx, fake_free, NULL);
	}
}

static void
zrpcdb_node_cb_enc(Esql_Res *res, Zentific_Ctx *ctx, Eina_Bool enabled){
	DB_VARS;

	if (!esql_res_rows_count(res)){
		ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
		ecore_event_add(ZRPCDB_EVENT_ERROR, ctx, fake_free, NULL);
		return;
	}
	if (!enabled) {
		ctx->error = AZY_ERR(NODE_DISABLED);
		ecore_event_add(ZRPCDB_EVENT_ERROR, ctx, fake_free, NULL);
		return;
	}
	ctx->state = EINA_TRUE;
	qid = esql_query_args(esql_res_esql_get(res), ctx,
		"SELECT id FROM `nodes` "
		"WHERE uuid='%s' "
		"AND controlmodule != ''",
		(char*)ctx->retval);
	if (!qid) {
		ERR("Could not create query!");
		ctx->error = AZY_ERR(DB_FAIL);
		ecore_event_add(ZRPCDB_EVENT_ERROR, ctx, fake_free, NULL);
		return;
	}
	esql_query_callback_set(qid, (Esql_Query_Cb)event_cb);
	eina_hash_add(type_cbs, &qid, esql_res_to_lli);
	QID_CB_ADD(zrpcdb_node_cb_conf);
}

Eina_Bool isNodeEnabledAndConfigured(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	ctx->state = EINA_TRUE;
	QUERY(esql_res_to_lli,
		"SELECT enabled from `nodes` where uuid='%s'", uuid);
	QID_CB_ADD(zrpcdb_node_cb_enc);
	ctx->retval = (void*)uuid;
	QID_RETURN;
}

Eina_Bool getAllUnconfiguredNodes(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	if ((offset < 0) || (recordcount < 0))
		QSSA(
			"SELECT uuid FROM `nodes` WHERE controlmodule = ''");
	QSSA(
		"SELECT uuid FROM `nodes` WHERE controlmodule = '' LIMIT %d, %d",
		offset, recordcount);
}

Eina_Bool getAllUnconfiguredNodesByUID(DB *db, Zentific_Ctx *ctx, int uid, int offset, int recordcount){
	if ((offset < 0) || (recordcount < 0))
		QSSA(
			"SELECT uuid FROM `nodes` "
			"JOIN ownershipnode ON ownershipnode.node=nodes.id "
			"WHERE ownershipnode.uid='%d' AND controlmodule = ''",
			uid);
	QSSA(
		"SELECT uuid FROM `nodes` "
		"JOIN ownershipnode ON ownershipnode.node=nodes.id "
		"WHERE ownershipnode.uid='%d' AND controlmodule = '' "
		"LIMIT %d, %d",
		uid, offset, recordcount);
}

/* FIXME: should this be exposed? */
Eina_Bool getNodeConfiguredState(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	QUERY(NODE_esql,
		"SELECT enabled, controlmodule, configuredhost AS address FROM `nodes` "
		"WHERE uuid='%s'",
		uuid);
	ctx->free_func = (Ecore_Cb)NODE_free;
	QID_RETURN;
}

Eina_Bool isNode(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QBA(
		"SELECT * from `nodes` where uuid='%s'", uuid);
}

Eina_Bool getNodePlatform(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT platform FROM `nodeconfig` "
		"JOIN nodes ON nodeconfig.node=nodes.id "
		"WHERE nodes.uuid='%s' "
		"ORDER BY revision DESC LIMIT 1",
		uuid);
}

Eina_Bool getNode(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	QUERY(NODE_esql,
		"SELECT * from nodeconfig "
		"LEFT JOIN nodes ON nodeconfig.node=nodes.id "
		"WHERE nodes.uuid='%s'"
		"ORDER BY nodeconfig.timestamp DESC LIMIT 1",
		uuid);
	ctx->free_func = (Ecore_Cb)NODE_free;
	QID_RETURN;
}


Eina_Bool getAllNodesFullRangeSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	DB_VARS;
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QUERY(Array_NODE_esql,
		"SELECT * FROM `nodeconfig` "
		"LEFT JOIN nodes ON nodeconfig.node=nodes.id "
		"GROUP BY nodes.uuid "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
	ctx->free_func = (Ecore_Cb)Array_NODE_free;
	QID_RETURN;
}

Eina_Bool getAllNodesFull(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_NODE_esql,
		"SELECT * from nodeconfig "
		"LEFT JOIN nodes ON nodeconfig.node=nodes.id "
		"GROUP BY nodes.uuid");
	ctx->free_func = (Ecore_Cb)Array_NODE_free;
	QID_RETURN;
}

/* yeah I know this looks dumb */
Eina_Bool getNodeNameAndUuid(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSSA(
		"SELECT uuid, hostname from nodeconfig "
		"JOIN nodes ON nodeconfig.node=nodes.id "
		"WHERE nodes.uuid='%s' "
		"ORDER BY `timestamp` DESC LIMIT 1",
		uuid);
}

Eina_Bool getNodeName(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT `hostname` from nodeconfig "
		"JOIN nodes ON nodeconfig.node=nodes.id "
		"WHERE nodes.uuid='%s' "
		"ORDER BY `timestamp` DESC LIMIT 1",
		uuid);
}

Eina_Bool getNodeByName(DB *db, Zentific_Ctx *ctx, const char *name){
	QSA(
		"SELECT uuid FROM nodes "
		"RIGHT JOIN nodeconfig ON nodeconfig.node=nodes.id "
		"WHERE nodeconfig.name='%s' "
		"ORDER BY nodeconfig.timestamp DESC LIMIT 1",
		name);

}

/* questionable multi-function function.
 * which = 1 	---> all enabled
 * 	   0 	---> all disabled
 *	  other ---> all
 */
Eina_Bool getAllNodes(DB *db, Zentific_Ctx *ctx, int which, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	if ((which == 0) || (which == 1))
		QSSA(
			"SELECT uuid FROM `nodes` WHERE enabled=%i%s",
			which, limit);
	QSSA(
		"SELECT uuid FROM `nodes`%s",
		limit);
}

Eina_Bool getAllNodesByUID(DB *db, Zentific_Ctx *ctx, int which, int uid, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	if ((which == 0) || (which == 1))
		QSSA(
			"SELECT uuid FROM `nodes` "
			"JOIN ownershipnode ON ownershipnode.node=nodes.id "
			"WHERE ownershipnode.uid='%d' AND enabled=%i%s",
			uid, which, limit);
	QSSA(
		"SELECT uuid FROM `nodes` ",
			"JOIN ownershipnode ON ownershipnode.node=nodes.id "
			"WHERE ownershipnode.uid='%d'%s",
			uid, limit);
}

Eina_Bool getAllNodesSorted(DB *db, Zentific_Ctx *ctx, int which, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	if ((which == 0) || (which == 1))
		QSSA(
			"SELECT nodes.uuid FROM "
			"(SELECT node, revision, MAX(revision) AS m FROM nodeconfig GROUP BY node) AS xx "
			"INNER JOIN nodeconfig AS t ON t.node=xx.node AND t.revision = xx.m "
			"JOIN nodes ON nodes.id=t.node "
			"WHERE nodes.enabled='%i' "
			"GROUP BY t.node "
			"ORDER BY t.`%s` %s %s",
			which,
			sortfield,
			(desc) ? "DESC": "",
			limit);
	QSSA(
		"SELECT nodes.uuid FROM "
		"(SELECT node, revision, MAX(revision) AS m FROM nodeconfig GROUP BY node) AS xx "
		"INNER JOIN nodeconfig AS t ON t.node=xx.node AND t.revision = xx.m "
		"JOIN nodes ON nodes.id=t.node "
		"GROUP BY t.node "
		"ORDER BY t.`%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);

}

Eina_Bool getAllNodesByUIDSorted(DB *db, Zentific_Ctx *ctx, int which, const char *sortfield, Eina_Bool desc, int uid, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	if ((which == 0) || (which == 1))
		QSSA(
			"SELECT nodes.uuid FROM "
			"(SELECT node, revision, MAX(revision) AS m FROM nodeconfig GROUP BY node) AS xx "
			"INNER JOIN nodeconfig AS t ON t.node=xx.node AND t.revision = xx.m "
			"JOIN nodes ON nodes.id=t.node "
			"JOIN ownershipnode ON ownershipnode.node=nodes.id "
			"WHERE ownershipnode.uid='%d' AND nodes.enabled='%i' "
			"GROUP BY t.node "
			"ORDER BY t.`%s` %s %s",
			uid, which,
			sortfield,
			(desc) ? "DESC" : "",
			limit);

	QSSA(
		"SELECT nodes.uuid FROM "
		"(SELECT node, revision, MAX(revision) AS m FROM nodeconfig GROUP BY node) AS xx "
		"INNER JOIN nodeconfig AS t ON t.node=xx.node AND t.revision = xx.m "
		"JOIN nodes ON nodes.id=t.node "
		"JOIN ownershipnode ON ownershipnode.node=nodes.id "
		"WHERE ownershipnode.uid='%d' "
		"GROUP BY t.node "
		"ORDER BY t.`%s` %s %s",
		uid, sortfield,
		(desc) ? "DESC" : "",
		limit);

}


Eina_Bool getNodeByHostname(DB *db, Zentific_Ctx *ctx, const char *hostname){
	DB_VARS;
	QUERY(NODE_esql,
		"SELECT * FROM nodes "
		"RIGHT JOIN nodeconfig ON nodes.id=nodeconfig.node "
		"WHERE nodeconfig.timestamp <= NOW() "
		"AND nodeconfig.hostname='%s' "
		"ORDER BY nodeconfig.timestamp DESC LIMIT 1",
		hostname);
	ctx->free_func = (Ecore_Cb)NODE_free;
	QID_RETURN;
}

// age really isnt age. think timestamp < now - retentionlength
// 	age = (now - retentionlength)
Eina_Bool pruneNodeStats(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QE(
		"USING `nodestats` "
		"JOIN nodes ON nodestats.node=nodes.id "
		"WHERE timestamp < DATE_SUB(CURDATE(), "
		"WHERE timestamp < DATE_SUB(CURDATE(), "
		"INTERVAL IFNULL("
			"(SELECT value FROM `config` WHERE `key`='stat_retention'"
			" AND `subsystem`='zrpc' AND `mid`='-1'"
			" AND `uid`='-1' AND `gid`='-1' AND `vgid`='-1'"
			" AND `ngid`='-1' AND `vm`='-1' AND `node`='-1'"
			" AND `platform`='-1'), 31) HOUR) "
		"AND nodes.uuid='%s'",
		 uuid);
}

// functions to get VM from db and return a VM type
Eina_Bool getNodeLogs(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	QUERY(Array_LOG_esql,
		"SELECT timestamp, uid, severity, message FROM `nodelogs` "
		"JOIN nodes ON nodelogs.node=nodes.id "
		"WHERE nodes.uuid='%s' "
		"ORDER BY timestamp DESC",
		uuid);
	ctx->free_func = (Ecore_Cb)Array_LOG_free;
	QID_RETURN;
}

Eina_Bool getNodeDefaultVNCpass(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QSA(
		"SELECT default_vnc_pass FROM `nodeconfig` "
		"JOIN nodes ON nodeconfig.node=nodes.id "
		"WHERE nodes.uuid='%s' LIMIT 1",
		uuid);
}

Eina_Bool getNodeHVM(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QBA(
		"SELECT hvm FROM `nodeconfig` "
		"JOIN nodes ON nodeconfig.node=nodes.id "
		"WHERE nodes.uuid='%s' LIMIT 1",
		uuid);
}

Eina_Bool getNodeCount(DB *db, Zentific_Ctx *ctx){
	QI(
		"SELECT COUNT(uuid) FROM `nodes`");
}

Eina_Bool getNodeCountByUID(DB *db, Zentific_Ctx *ctx, int uid){
	QIA(
		"SELECT COUNT(uuid) FROM `nodes` "
		"JOIN ownershipnode ON ownershipvm.node=nodes.id "
		"WHERE ownershipnode.uid='%d'",
		uid);
}
