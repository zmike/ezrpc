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
#include <inttypes.h>

/* ----------------------------------------------- */
/* API Functions: Network   			   */
/* ----------------------------------------------- */

/* TODO determine whether handling of role in this case is even valid
 *
 */
Eina_Bool getAllNetworksFull(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	DB_VARS;
	if ((offset < 0) || (recordcount < 0))
		QUERY_NOESCAPE(Array_NETWORK_esql,
			"SELECT * FROM `networks`");
	else
		QUERY(Array_NETWORK_esql,
			"SELECT * FROM `networks` LIMIT %d, %d",
			offset, recordcount);
	ctx->free_func = (Ecore_Cb)Array_NETWORK_free;
	QID_RETURN;
}

Eina_Bool getAllNetworksFullSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	DB_VARS;
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QUERY(Array_NETWORK_esql,
		"SELECT * FROM `networks` "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
	ctx->free_func = (Ecore_Cb)Array_NETWORK_free;
	QID_RETURN;
}

Eina_Bool getAllNetworksSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QISA(
		"SELECT id FROM `networks` "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
}

Eina_Bool getAllNetworks(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	DB_VARS;
	if ((offset < 0) || (recordcount < 0))
		QUERY_NOESCAPE(Array_int_esql,
			"SELECT id FROM `networks`");
	else
		QUERY(Array_NETWORK_esql,
			"SELECT id FROM `networks` LIMIT %d, %d",
			offset, recordcount);
	ctx->free_func = (Ecore_Cb)eina_list_free;
	QID_RETURN;
}

Eina_Bool getAllNetworksByType(DB *db, Zentific_Ctx *ctx, const char *type, int offset, int recordcount){
	DB_VARS;
	if ((offset < 0) || (recordcount < 0))
		QUERY(Array_int_esql,
			"SELECT id FROM `networks` WHERE type='%s'",
			type);
	else
		QUERY(Array_int_esql,
			"SELECT id FROM `networks` WHERE type='%s' LIMIT %d, %d",
			type, offset, recordcount);
	ctx->free_func = (Ecore_Cb)eina_list_free;
	QID_RETURN;
}

Eina_Bool getNetwork(DB *db, Zentific_Ctx *ctx, int id){
	DB_VARS;
	QUERY(NETWORK_esql,
		"SELECT * FROM `networks` WHERE id='%d'", id);
	ctx->free_func = (Ecore_Cb)NETWORK_free;
	QID_RETURN;
}

Eina_Bool getNetworkVlans(DB *db, Zentific_Ctx *ctx, int id){
	QSA(
		"SELECT vlans FROM `networks` WHERE id='%d'",
		id);
}

Eina_Bool removeNetwork(DB *db, Zentific_Ctx *ctx, int id){
	QE(
		"DELETE FROM `networks` WHERE id='%d'", id);
}

Eina_Bool getNodesByNetworkVlan(DB *db, Zentific_Ctx *ctx, int vlan){
	QSSA(
		"SELECT nodes.uuid FROM nodes"
		" RIGHT JOIN `networksnodes` ON networksnodes.node=nodes.id"
		" RIGHT JOIN `networks` ON networksnodes.network=networks.id"
		" WHERE networks.vlans REGEXP '(^|,)%d(,|$)'",
		vlan);
}

Eina_Bool getNodesByNetwork(DB *db, Zentific_Ctx *ctx, int id){
	QSSA(
		"SELECT uuid from nodes RIGHT JOIN `networksnodes` ON networksnodes.node=nodes.id"
		" WHERE networksnodes.network='%d'",
		id);
}

Eina_Bool getNetworksByNode(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	QUERY(Array_NETWORK_esql,
		"SELECT * FROM `networks` RIGHT JOIN networksnodes ON networksnodes.node=node.id WHERE nodes.uuid='%s'",
		uuid);
	ctx->free_func = (Ecore_Cb)Array_NETWORK_free;
	QID_RETURN;
}

Eina_Bool defineNetwork(DB *db, Zentific_Ctx *ctx, NETWORK *net){
	intptr_t *vlan;
	Eina_List *l;
	Eina_Strbuf *vlans;

	vlans = eina_strbuf_new();

	EINA_LIST_FOREACH(net->vlans, l, vlan){
		if (*vlan >= 0){
			eina_strbuf_append_printf(vlans, "%"PRIdPTR, *vlan);
			if (l->next)eina_strbuf_append(vlans, ",");
		}
	}
	DB_VARS;
	ctx->state = 1;
	QUERY_NOCONVERT(
		"INSERT INTO networks"
		" (name, type, vlans, network, netmask, gateway, interface, mac, mid, host)"
		" VALUES"
		" ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%d', '%s')",
		net->name, net->type, (eina_strbuf_length_get(vlans)) ? eina_strbuf_string_get(vlans) : "", net->network,
		net->netmask, net->gateway, net->interface, net->mac, net->mid, net->host);

	eina_strbuf_free(vlans);
	QID_CB_ADD(zrpcdb_cb_exec_exists);

	QID_RETURN;
}

Eina_Bool modifyNetwork(DB *db, Zentific_Ctx *ctx, int id, NETWORK *net){
	intptr_t *vlan;
	Eina_List *l;
	Eina_Strbuf *vlans;

	vlans = eina_strbuf_new();

	EINA_LIST_FOREACH(net->vlans, l, vlan){
		if (*vlan >= 0){
			eina_strbuf_append_printf(vlans, "%"PRIdPTR, *vlan);
			if (l->next)eina_strbuf_append(vlans, ",");
		}
	}
	DB_VARS;
	ctx->state = 1;
	QUERY_NOCONVERT(
		"UPDATE `networks`"
		" SET name='%s', type='%s', vlans='%s', network='%s', mid='%d',"
		" netmask='%s', gateway='%s', interface='%s', mac='%s', host='%s'"
		" WHERE id='%d'",
		net->name, net->type, (eina_strbuf_length_get(vlans)) ? eina_strbuf_string_get(vlans) : "",
		net->network, net->mid, net->netmask, net->gateway, net->interface, net->mac, net->host,
		id);

	eina_strbuf_free(vlans);
	QID_CB_ADD(zrpcdb_cb_exec_exists);

	QID_RETURN;
}

Eina_Bool isNetwork(DB *db, Zentific_Ctx *ctx, NETWORK *net){
	QIAB(
		"SELECT id FROM `networks`"
		" WHERE name='%s' OR mac='%s'",
		net->name, net->mac);
}

Eina_Bool isNetworkId(DB *db, Zentific_Ctx *ctx, int id){
	QIAB(
		"SELECT id FROM `networks`"
		" WHERE id='%d'",
		id);
}

/* TODO (probable FIXME, really) : combine w/ createNetIF
 * use to refactor updatevifconfig
 *
 */
Eina_Bool addNetIf(DB *db, Zentific_Ctx *ctx, const char *uuid, NETIF *vif){

	QEE(
	       "INSERT INTO `vifconfig`"
		" 	(vif, revision, type, mac, bridge, name, script, label, ip, netmask, gateway, broadcast)"
		" VALUES ("
		"       (SELECT id FROM `vifs` WHERE uuid='%s' AND mac='%s'),"
		"       (("
				"SELECT revision FROM `vifconfig`"
				" RIGHT JOIN `vifs` ON vifconfig.vif=vifs.id"
				" WHERE vifs.uuid='%s'"
				" ORDER BY vifconfig.timestamp DESC LIMIT 1",
			") + 1),"
		"	'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s'"
		")",
		uuid, vif->mac, uuid,
		vif->type, vif->mac, vif->bridge, vif->name,
		vif->script, vif->label, vif->ip,
		vif->netmask, vif->gateway, vif->broadcast);
}

Eina_Bool attachNetIf(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac, int nwid){
	QE(
		"UPDATE vifs SET nwid='%d' WHERE mac='%s AND uuid='%s'",
		nwid, mac, uuid);
}

Eina_Bool detachNetIf(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac, int nwid){
	QE(
		"UPDATE vifs SET nwid='-1' WHERE nwid='%d' AND mac='%s' and uuid='%s'",
		nwid, mac, uuid);
}

Eina_Bool modifyNetIf(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac, NETIF *vif){

	QE(
		"INSERT INTO `vifconfig`"
		" 	(vif, revision, type, mac, bridge, name, script, label, ip, netmask, gateway, broadcast)"
		" VALUES ("
		"       (SELECT id FROM `vifs` WHERE uuid='%s' AND mac='%s'),"
		"       (("
				"SELECT revision FROM `vifconfig`"
				" RIGHT JOIN `vifs` ON vifconfig.vif=vifs.id"
				" WHERE vifs.uuid='%s'"
				" ORDER BY vifconfig.timestamp DESC LIMIT 1",
			") + 1),"

		"	'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s'"
		")",
		uuid, mac, uuid,
		vif->type, vif->mac, vif->bridge, vif->name,
		vif->script, vif->label, vif->ip,
		vif->netmask, vif->gateway, vif->broadcast);

}

Eina_Bool removeNetIf(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac){
	QE(
		"DELETE vifstats.*, vifconfig.*, vifs.* "
		"JOIN vifs ON vifstats.vif=vifs.id "
		"JOIN vifconfig ON vifconfig.vif=vifs.id "
		"WHERE vifs.mac='%s' AND uuid='%s'",
		mac, uuid);
}

Eina_Bool detachAllNetIfsByNetwork(DB *db, Zentific_Ctx *ctx, int nwid){
	QE(
		"UPDATE vifs SET nwid='-1' WHERE nwid='%d'",
		nwid);

}

Eina_Bool getNetworkControlModule(DB *db, Zentific_Ctx *ctx, int nwid){
	QIA(
		"SELECT mid FROM networks WHERE id='%d'",
		nwid);
}

Eina_Bool setNetworkControlModule(DB *db, Zentific_Ctx *ctx, int nwid, int mid){
	QE(
		"UPDATE networks SET mid='%d' WHERE id='%d'",
		mid, nwid);
}

Eina_Bool detachNetworkFromNode(DB *db, Zentific_Ctx *ctx, const char *uuid, int nwid){
	QE(
		"DELETE FROM networksnodes "
		"USING networksnodes JOIN nodes ON networksnodes.node=nodes.id "
		"WHERE nodes.uuid='%s' "
		"AND networksnodes.network='%d'",
		uuid, nwid);
}

Eina_Bool attachNetworkToNode(DB *db, Zentific_Ctx *ctx, const char *uuid, int nwid){
	QE(
		"INSERT INTO networksnodes (node, network) "
		"VALUES ((SELECT id FROM nodes WHERE uuid='%s'), '%d')",
		uuid, nwid);
}

Eina_Bool getVmsByNetwork(DB *db, Zentific_Ctx *ctx, int nwid){
	QSSA(
		"SELECT uuid FROM vifs "
		"WHERE nwid='%d' "
		"GROUP BY uuid",
		nwid);
}

Eina_Bool getNetworksByVm(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QISA(
		"SELECT nwid FROM vifs "
		"WHERE uuid='%s' "
		"GROUP BY nwid",
		uuid);
}

Eina_Bool isNetworkInUse(DB *db, Zentific_Ctx *ctx, int nwid) {
	QIAB(
		"SELECT network from networksnodes WHERE network='%d' LIMIT 1",
		nwid);
}

Eina_Bool getNetworkByNetIf(DB * db, Zentific_Ctx *ctx, const char * uuid, const char *mac){
	QIA(
		"SELECT nwid FROM vifs WHERE uuid='%s' AND mac='%s'",
		uuid, mac);

}

Eina_Bool getNetworkIdByName( DB *db, Zentific_Ctx *ctx, const char *name){
	QIA(
		"SELECT id FROM networks WHERE name='%s'",
		name);
}

Eina_Bool getNetworkCount(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT COUNT(id) FROM `networks`");
}
