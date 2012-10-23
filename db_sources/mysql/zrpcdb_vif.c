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
/* API Functions: NetIf    			   */
/* ----------------------------------------------- */

Eina_Bool isNetIfByMAC(DB *db, Zentific_Ctx *ctx, const char *mac){
	QIAB(
		"SELECT id FROM `vifs` WHERE mac='%s'",
		mac);
}

Eina_Bool isNetIf(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac){
	QIAB(
		"SELECT id FROM `vifs` "
		"JOIN vifconfig ON vifs.id=vifconfig.vif "
		"WHERE vifconfig.mac='%s' "
		"AND uuid='%s'",
		mac, uuid);
}

Eina_Bool createNetIf(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac){
	QEE(
		"INSERT INTO `vifs` (uuid, mac) VALUES ('%s','%s')", uuid, mac);
}

Eina_Bool getNumVMvifs(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QIA(
		"SELECT COUNT(id) FROM `vifs` WHERE vifs.uuid='%s'",
		uuid);
}

Eina_Bool updateNetIfStats(DB *db, Zentific_Ctx *ctx, const char *uuid, NETIF *vif){
	QE(
		"INSERT INTO `vifstats` (vif, rxpackets, rxbytes, rxdrop, rxerr,"
		"	txpackets, txbytes, txdrop, txerr)"
		" VALUES ("
			"(SELECT id FROM `vifs`, `vifconfig` WHERE"
			" vifs.uuid='%s' AND"
			" vifs.id=vifconfig.vif AND"
			" vifconfig.mac='%s'"
			" ORDER BY vifconfig.revision DESC LIMIT 1),"
		"       '%lf', '%lf', '%lf', '%lf', '%lf', '%lf', '%lf', '%lf'"
		")",
		uuid, vif->mac,
		vif->rxpackets, vif->rxbytes, vif->rxdrop, vif->rxerr,
		vif->txpackets, vif->txbytes, vif->txdrop, vif->txerr);
}

Eina_Bool getVMvifByMac(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac){
	DB_VARS;
	QUERY(NETIF_esql,
/*
		"SELECT vifs.*, v_confs.*, v_stats.*"
			" FROM vifs"
		" RIGHT JOIN ("
			" SELECT * FROM vifconfig"
			" WHERE vifconfig.timestamp <= NOW()"
			" GROUP BY vifconfig.vif"
			" ORDER BY vifconfig.vif ASC, vifconfig.timestamp DESC"
		" ) AS v_confs"
		" ON v_confs.vif=vifs.id"
		" RIGHT JOIN ("
			" SELECT * FROM vifstats"
			" WHERE vifstats.timestamp <= NOW()"
			" GROUP BY vifstats.vif"
			" ORDER BY vifstats.vif ASC, vifstats.timestamp DESC"
		") AS v_stats"
		" ON v_stats.vif=vifs.id"
		" WHERE vifs.uuid='%s' AND v_confs.mac = '%s'",
		uuid, mac);
*/
		"SELECT * FROM vifconfig"
		" RIGHT JOIN vifs ON vifs.id=vifconfig.vif"
		" WHERE vifs.uuid='%s' AND vifconfig.mac`='%s'"
		" ORDER BY vifconfig.revision DESC LIMIT 1",
		uuid, mac);
	ctx->free_func = (Ecore_Cb)NETIF_free;
	QID_RETURN;
}

Eina_Bool getVMvifsByNetwork(DB *db, Zentific_Ctx *ctx, int nwid){
	DB_VARS;
	QUERY(Array_NETIF_esql,

		"SELECT * FROM vifconfig "
		"RIGHT JOIN vifs ON vifs.id=vifconfig.vif "
		"WHERE vifconfig.revision="
			"(SELECT revision FROM `vifconfig` "
			"RIGHT JOIN `vifs` ON vifconfig.vif=vifs.id "
			"ORDER BY vifconfig.timestamp DESC LIMIT 1) "
		"ORDER BY vifs.uuid ASC",
		nwid);
	ctx->free_func = (Ecore_Cb)Array_NETIF_free;
	QID_RETURN;
}

Esql_Query_Id getvmvifs(DB *db, Zentific_Ctx *ctx, const char *uuid, int rev){
	DB_VARS;
	if (rev < 0)
		QUERY(Array_NETIF_esql,
			"SELECT * FROM vifconfig "
			"RIGHT JOIN vifs ON vifs.id=vifconfig.vif "
			"WHERE vifs.uuid='%s' AND vifconfig.revision="
				"(SELECT revision FROM `vifconfig` "
				"RIGHT JOIN `vifs` ON vifconfig.vif=vifs.id "
				"WHERE vifs.uuid='%s' "
				"ORDER BY vifconfig.timestamp DESC LIMIT 1) "
			"ORDER BY vifconfig.revision DESC",
			uuid, uuid);
	else
		QUERY(Array_NETIF_esql,
			"SELECT * FROM vifconfig "
			"RIGHT JOIN vifs ON vifs.id=vifconfig.vif "
			"WHERE vifs.uuid='%s' AND vifconfig.revision='%i'",
			uuid, rev);
	ctx->free_func = (Ecore_Cb)Array_NETIF_free;
	return qid;
}

Eina_Bool getVMvifs(DB *db, Zentific_Ctx *ctx, const char *uuid){
	return !!getvmvifs(db, ctx, uuid, -1);
}

Eina_Bool updateNetIfConfig(DB *db, Zentific_Ctx *ctx, const char *uuid, int nextrevision, NETIF *vif){
		//FIXME creating new vifconfig entry based upon id of entry in vifs table in
		//      this way may be inappropriate, depending upon item removal, etc
	QE(
		"INSERT INTO `vifconfig`"
		" (vif, revision, type, mac, bridge, name, script, ip, netmask, gateway, broadcast, mtu)"
		" VALUES ("
		"       (SELECT id FROM `vifs` WHERE uuid='%s' AND mac='%s'),"
		"       '%d',"
		"	'%s', '%s', '%s', '%s', '%s',"
		"	'%s', '%s', '%s', '%s', '%d'"
		")",
		uuid, vif->mac,
		nextrevision,
		vif->type, vif->mac, vif->bridge, vif->name, vif->script,
		vif->ip, vif->netmask, vif->gateway, vif->broadcast, vif->mtu);
}

// age really isnt age. think timestamp < now - retentionlength
// 	age = (now - retentionlength)
Eina_Bool pruneNetIfStats(DB *db, Zentific_Ctx *ctx, const char * uuid, const char *mac){
	QE(
		"DELETE FROM `vifstats` "
		"JOIN vifs ON vifstats.vif=vifs.id "
		"WHERE timestamp < DATE_SUB(CURDATE(), "
		"INTERVAL IFNULL("
				"(SELECT value FROM `config` WHERE `key`='stat_retention'"
				" AND `subsystem`='zrpc' AND `mid`='-1'"
				" AND `uid`='-1' AND `gid`='-1' AND `vgid`='-1'"
				" AND `ngid`='-1' AND `vm`='-1' AND `node`='-1'"
				" AND `platform`='-1'), 31) HOUR) "
		"AND vifs.uuid='%s' AND vifconfig.mac='%s'",
		 uuid, mac, uuid, mac);
}

/* FIXME: correct? */
Eina_Bool isNetIfInUse(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac ) {
	QIAB(
		"SELECT nwid from vifs WHERE AND mac='%s' and uuid='%s'",
		mac, uuid);
}

