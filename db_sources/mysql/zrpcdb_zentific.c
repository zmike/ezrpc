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
/* API Functions: Zentific   			   */
/* ----------------------------------------------- */

/* TODO determine whether handling of role in this case is even valid
 *
 */
Eina_Bool getConfigValue(DB *db, Zentific_Ctx *ctx, const char *key, const char *subsystem,
	int mid, int uid, int gid, int vgid, int ngid, int vm, int node, int platform, int role){

	if (!key || !subsystem || !key[0] || !subsystem[0])
		return EINA_FALSE;

	if (role < 0)
		QSA(
			"SELECT value FROM `config` WHERE `key`='%s'"
			" AND `subsystem`='%s' AND `mid`='%d'"
			" AND `uid`='%d' AND `gid`='%d' AND `vgid`='%d'"
			" AND `ngid`='%d' AND `vm`='%d' AND `node`='%d'"
			" AND `platform`='%d'",
			key, subsystem, mid, uid, gid,
			vgid, ngid, vm, node, platform);
	QSA(
		"SELECT value FROM `config` WHERE `key`='%s'"
		" AND `subsystem`='%s' AND `mid`='%d'"
		" AND `uid`='%d' AND `gid`='%d' AND `vgid`='%d'"
		" AND `ngid`='%d' AND `vm`='%d' AND `node`='%d'"
		" AND `platform`='%d' AND `role`='%d'",
		key, subsystem, mid, uid, gid,
		vgid, ngid, vm, node, platform, role);
}

Eina_Bool getOperatingSystems(DB *db, Zentific_Ctx *ctx){
	QSS(
		"SELECT name from `operatingsystems`");

}

Eina_Bool getConfigsFull(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_KEYVALUE_esql,
		"SELECT * from `config`");
	ctx->free_func = (Ecore_Cb)Array_KEYVALUE_free;
	QID_RETURN;
}

/* node: 0
 * vm: 1
 * disk: 2
 * vif: 3
 */
/* USED BY:
 * getVMdisks
 * addNetIf
 * updateNodeConfig
 */
Eina_Bool getLatestRevision(DB *db, Zentific_Ctx *ctx, const char *uuid, unsigned char type){
	const char *t;
	switch (type){
		case 0:
			t = "node";
			break;
		case 1:
			t = "vm";
			break;
		case 2:
			t = "disk";
			break;
		case 3:
			t = "vif";
			break;
		default:
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			return EINA_FALSE;
	}
	//FIXME cant assume rev index starts at 0?
	QIA(
		"SELECT revision FROM `%sconfig` "
		"RIGHT JOIN `%ss` ON %sconfig.%s=%ss.id "
		"WHERE %ss.uuid='%s' "
		"ORDER BY %sconfig.timestamp DESC LIMIT 1",
		t, t, t, t, t, t, uuid, t);
}

Eina_Bool getConfigs(DB *db, Zentific_Ctx *ctx, const char *subsystem,
	int mid, int uid, int gid, int vgid, int ngid, int vm,
	int node, int platform, int role){

	if (!subsystem || !subsystem[0]){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}

	DB_VARS;
	QUERY(Array_KEYVALUE_esql,
		"SELECT * FROM `config` WHERE "
		" AND `subsystem`='%s' AND `mid`='%d'"
		" AND `uid`='%d' AND `gid`='%d' AND `vgid`='%d'"
		" AND `ngid`='%d' AND `vm`='%d' AND `node`='%d'"
		" AND `platform`='%d' AND `role`='%d'",
		subsystem, mid, uid, gid,
		vgid, ngid, vm, node, platform, role);
	ctx->free_func = (Ecore_Cb)Array_KEYVALUE_free;
	QID_RETURN;

}

Eina_Bool getConfig(DB *db, Zentific_Ctx *ctx, const char *key, const char *subsystem,
	int mid, int uid, int gid, int vgid, int ngid, int vm,
	int node, int platform, int role){

	if (!key || !subsystem || !key[0] || !subsystem[0]){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}

	DB_VARS;
	QUERY(KEYVALUE_esql,
		"SELECT * FROM `config` WHERE `key`='%s'"
		" AND `subsystem`='%s' AND `mid`='%d'"
		" AND `uid`='%d' AND `gid`='%d' AND `vgid`='%d'"
		" AND `ngid`='%d' AND `vm`='%d' AND `node`='%d'"
		" AND `platform`='%d' AND `role`='%d'",
		key, subsystem, mid, uid, gid,
		vgid, ngid, vm, node, platform, role);
	ctx->free_func = (Ecore_Cb)KEYVALUE_free;
	QID_RETURN;
}

Eina_Bool setBlobConfig(DB *db, Zentific_Ctx *ctx,
	const char *key, const unsigned char *value, const char *subsystem,
	int mid, int uid, int gid, int vgid, int ngid, int vm,
	int node, int platform, int role ){


	if (!key || !subsystem || !key[0] || !subsystem[0] || (!value)){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}

	QE("DELETE FROM `config` WHERE `key`='%s'"
		" AND `subsystem`='%s' AND `mid`='%d'"
		" AND `uid`='%d' AND `gid`='%d' AND `vgid`='%d'"
		" AND `ngid`='%d' AND `vm`='%d' AND `node`='%d'"
		" AND `platform`='%d' AND `role`='%d';"
		" INSERT INTO `config`"
		" (`key`, `valueBlob`, `subsystem`, `mid`,"
		"   `uid`, `gid`, `vgid`, `ngid`,"
		"   `vm`, `node`, `platform`, `role`)"
		" VALUES"
		" ('%s', '%s', '%s', '%d',"
		"  '%d', '%d', '%d', '%d',"
		"  '%d', '%d', '%d', '%d')",
		key, subsystem, mid, uid, gid,
		vgid, ngid, vm, node, platform, role,
		key, value, subsystem,
		mid, uid, gid, vgid,
		ngid, vm, node, platform, role);
}

//#warning implement config key->value deletion
Eina_Bool setConfig(DB *db, Zentific_Ctx *ctx, const char *key, const char *value, const char *subsystem,
	int mid, int uid, int gid, int vgid, int ngid, int vm,
	int node, int platform, int role ){


	if (!key || !subsystem || !key[0] || !subsystem[0] || (!value)){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}

	QE("DELETE FROM `config` WHERE `key`='%s'"
		" AND `subsystem`='%s' AND `mid`='%d'"
		" AND `uid`='%d' AND `gid`='%d' AND `vgid`='%d'"
		" AND `ngid`='%d' AND `vm`='%d' AND `node`='%d'"
		" AND `platform`='%d' AND `role`='%d';"
		" INSERT INTO `config`"
		" (`key`, `value`, `subsystem`, `mid`,"
		"   `uid`, `gid`, `vgid`, `ngid`,"
		"   `vm`, `node`, `platform`, `role`)"
		" VALUES"
		" ('%s', '%s', '%s', '%d',"
		"  '%d', '%d', '%d', '%d',"
		"  '%d', '%d', '%d', '%d')",
		key, subsystem, mid, uid, gid,
		vgid, ngid, vm, node, platform, role,
		key, value, subsystem,
		mid, uid, gid, vgid,
		ngid, vm, node, platform, role);
/*
	DBE("DELETE FROM `config` WHERE `key`='%s' "
		" AND `subsystem`='%s' AND `mid`='%d'"
		" AND `uid`='%d' AND `gid`='%d' AND `vgid`='%d'"
		" AND `ngid`='%d' AND `vm`='%d' AND `node`='%d'"
		" AND `platform`='%d' AND `role`='%d'",
		key, subsystem, mid, uid, gid,
		vgid, ngid, vm, node, platform, role);

	return DBE(
		"INSERT INTO `config`"
		" (`key`, `value`, `subsystem`, `mid`,"
		"   `uid`, `gid`, `vgid`, `ngid`,"
		"   `vm`, `node`, `platform`, `role`)"
		" VALUES"
		" ('%s', '%s', '%s', '%d',"
		"  '%d', '%d', '%d', '%d',"
		"  '%d', '%d', '%d', '%d')",
		key, value, subsystem,
		mid, uid, gid, vgid,
		ngid, vm, node, platform, role);
*/
}


Eina_Bool isFrontendPluginEnabled(DB *db, Zentific_Ctx *ctx, const char * name ){
	QBA("SELECT enabled FROM `plugins` WHERE frontend='1' and name='%s'", name);
}

Eina_Bool getAllPlugins(DB *db, Zentific_Ctx *ctx, int which ){
	DB_VARS;
	switch (which) {
		case -1:
			QUERY(Array_PLUGIN_esql,
				"SELECT name, description, xmlrpc, frontend, scheduler, poller, enabled, minpriv"
				" FROM `plugins`");
			break;
		case 0:
			QUERY(Array_PLUGIN_esql,
				"SELECT name, description, xmlrpc, frontend, scheduler, poller, enabled, minpriv"
				" FROM `plugins` WHERE enabled=0");
			break;
		default:
			QUERY(Array_PLUGIN_esql,
				"SELECT name, description, xmlrpc, frontend, scheduler, poller, enabled, minpriv"
				" FROM `plugins` WHERE enabled=1");
	}
	ctx->free_func = (Ecore_Cb)Array_PLUGIN_free;
	QID_RETURN;
}

Eina_Bool getFrontendPlugins(DB *db, Zentific_Ctx *ctx, int which ){
	switch (which) {
		case -1:
		{
			QSS(
				"SELECT name FROM `plugins` WHERE frontend=1");
		}
		case 0:
		{
			QSS(
				"SELECT name FROM `plugins` WHERE frontend=1"
				" AND enabled=0");
		}
		default:
		{
			QSS(
				"SELECT name FROM `plugins` WHERE frontend=1"
				" AND enabled=1");
		}
	}
}

Eina_Bool modifyPlugin(DB *db, Zentific_Ctx *ctx, const char *name, Eina_Bool enabled, Eina_Bool xmlrpc, Eina_Bool frontend, Eina_Bool scheduler, Eina_Bool poller){
	QE(
		"UPDATE `plugins`"
		" SET enabled='%c', xmlrpc='%c', frontend='%c', scheduler='%c', poller='%c'"
		" WHERE name='%s'",
		enabled, xmlrpc,  frontend,  scheduler,  poller, name);
}

//removes stats and config data for the specified bits (and associated info)
Eina_Bool cleanDB(DB *db, Zentific_Ctx *ctx){
#define Q(STR) esql_query(db->e, ctx, STR)

		Q("TRUNCATE TABLE vmstats");
		Q("TRUNCATE TABLE vms");
		Q("TRUNCATE TABLE vmnotes");
		Q("TRUNCATE TABLE vmconsolelocks");
		Q("TRUNCATE TABLE vmconfig");
		Q("TRUNCATE TABLE vmstats");
		Q("TRUNCATE TABLE vifstats");
		Q("TRUNCATE TABLE vifs");
		Q("TRUNCATE TABLE vifconfig");
		Q("TRUNCATE TABLE sessions");
		Q("TRUNCATE TABLE revisions");
		Q("TRUNCATE TABLE nodestats");
		Q("TRUNCATE TABLE nodes");
		Q("TRUNCATE TABLE nodechildren");
		Q("TRUNCATE TABLE nodeconfig");
		Q("TRUNCATE TABLE nodestats");
		Q("TRUNCATE TABLE diskstats");
		Q("TRUNCATE TABLE disks");
		Q("TRUNCATE TABLE diskconfig;");
		return EINA_TRUE;
}

