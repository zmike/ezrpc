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

/* API: platform data */

/* checks for module existence and (if platformid >= 0) correlation with a platform */
Eina_Bool isControlModuleForPlatform(DB *db, Zentific_Ctx *ctx, const char *modname, int platformid){
	QIAB(
		"SELECT id FROM `controlmodules` "
		"WHERE name='%s' "
		"AND platformid = '%d'",
		modname, platformid);
}

Eina_Bool isControlModuleForPlatformByMid(DB *db, Zentific_Ctx *ctx, int mid, int platformid){
	QIAB(
		"SELECT id FROM `controlmodules` "
		"WHERE id='%i' "
		"AND platformid = '%d'",
		mid, platformid);
}

Eina_Bool isControlModuleForPlatformById(DB *db, Zentific_Ctx *ctx, int mid, const char *platformname){
	QIAB(
		"SELECT id FROM `controlmodules` "
		"WHERE id='%i' "
		"AND name='%s'",
		mid, platformname);
}

Eina_Bool isControlModuleForPlatformByName(DB *db, Zentific_Ctx *ctx, const char *modname, const char *platformname){
	QIAB(
		"SELECT * FROM `controlmodules` "
		"RIGHT JOIN platforms ON controlmodules.platformid=platforms.id"
		"WHERE controlmodules.name='%s' "
		"AND platforms.name LIKE '%%%s%%'",
		modname, platformname);
}

Eina_Bool isControlModule(DB *db, Zentific_Ctx *ctx, int id){
	QIAB(
		"SELECT id FROM `controlmodules` "
		"WHERE id = '%d'",
		id);
}

Eina_Bool isControlModuleByName(DB *db, Zentific_Ctx *ctx, const char *name){
	QIAB(
		"SELECT id FROM `controlmodules` "
		"WHERE name='%s'",
		name);
}

Eina_Bool isPlatform(DB *db, Zentific_Ctx *ctx, int id){
	QIAB(
		"SELECT name FROM `platforms` WHERE id='%d'", id);
}

Eina_Bool isPlatformByName(DB *db, Zentific_Ctx *ctx, const char *platname){
	QIAB(
		"SELECT id FROM `platforms` WHERE name LIKE '%%%s%%'", platname);
}

Eina_Bool enableControlModule(DB *db, Zentific_Ctx *ctx, int id){
	QE(
		"UPDATE `controlmodules` SET enabled='1' WHERE id='%d'", id);
}

Eina_Bool disableControlModule(DB *db, Zentific_Ctx *ctx, int id){
	QE(
		"UPDATE `controlmodules` SET enabled='0' WHERE id='%d'", id);
}

Eina_Bool enablePlatform(DB *db, Zentific_Ctx *ctx, int id){
	QE(
		"UPDATE `platforms` SET enabled='1' WHERE id='%d'", id);
}

Eina_Bool disablePlatform(DB *db, Zentific_Ctx *ctx, int id){
	QE(
		"UPDATE `platforms` SET enabled='0' WHERE id='%d'", id);
}

Eina_Bool getPlatformId(DB *db, Zentific_Ctx *ctx, const char *name){
	QIA(
		"SELECT id FROM `platforms` where name='%s'",
		name);
}

Eina_Bool getPlatforms(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT id FROM `platforms`");
}

Eina_Bool getPlatformsFull(DB *db, Zentific_Ctx *ctx){
 	DB_VARS;
 	QUERY_NOESCAPE(Array_PLATFORM_esql,
		"SELECT * FROM `platforms`");
	ctx->free_func = (Ecore_Cb)Array_PLATFORM_free;
	QID_RETURN;
}


Eina_Bool getPlatform(DB *db, Zentific_Ctx *ctx, int id){
 	DB_VARS;
 	QUERY(PLATFORM_esql,
		"SELECT * FROM `platforms` WHERE `id`='%d'", id);
	ctx->free_func = (Ecore_Cb)PLATFORM_free;
	QID_RETURN;
}

Eina_Bool getPlatformByName(DB *db, Zentific_Ctx *ctx, const char *name){
	DB_VARS;
	QUERY(PLATFORM_esql,
		"SELECT * FROM `platforms` WHERE `name`='%s'", name);
	ctx->free_func = (Ecore_Cb)PLATFORM_free;
	QID_RETURN;
}

Eina_Bool addPlatform(DB *db, Zentific_Ctx *ctx, PLATFORM *p){
	QE(
		"INSERT INTO `platforms`"
		" ( name, version, vendor, build, description, mechanism, enabled, features)"
		" VALUES"
		" ('%s', '%s', '%s', '%s', '%s', '%s', '%d', '%d')",
		p->name, p->version, p->vendor,
		p->build, p->description, p->mechanism, !!p->enabled, p->features);
}

/* for compartmentalization, maybe this function should look more like modifyPlugin in its signature */
Eina_Bool modifyPlatform(DB *db, Zentific_Ctx *ctx, int id, PLATFORM *p){
	QE(
		"UPDATE `platforms`"
		" SET"
		" `name`='%s', `version`='%s', `vendor`='%s',"
		" `build`='%s', `description`='%s', `mechanism`='%s',"
		" `enabled`='%d', features='%d'"
		" WHERE `id`='%d'",
		p->name, p->version, p->vendor,
		p->build, p->description, p->mechanism,
		(int)p->enabled, p->features, id);
}

/* FIXME should probably just make this a one shot and loop in the caller */
#if 0
Eina_Bool setPlatformConfig(DB *db, Zentific_Ctx *ctx, int id, Eina_List /*KEYVALUE*/ *config){
	DBE(
		"DELETE FROM `config` WHERE `subsystem`='zensched' AND `platform`='%d'", id);

	for(;config && config->data && !(azy_content_error_is_set(error); config=config->next){

		KEYVALUE *kv = config->data;

		DBE(
			"INSERT INTO `config` (`platform`, `subsystem`, `key`, `value`)"
			" VALUES ('%d', 'zensched', '%s', '%s')",
			id, kv->key, kv->value);
			//" ON DUPLICATE KEY UPDATE `value`='%s'",
			//id, kv->key, kv->value, kv->value);

	}


	return (azy_content_error_is_set(error) ? EINA_FALSE : EINA_TRUE;
}
#endif
Eina_Bool getPlatformConfig(DB *db, Zentific_Ctx *ctx, int platformid, const char *key){
	DB_VARS;
	if(key && *key){
		QUERY(Array_KEYVALUE_esql,
			"SELECT * FROM `config` WHERE `platform`='%d' AND `key`='%s'",
			platformid, key);
	} else {
		QUERY(Array_KEYVALUE_esql,
			"SELECT * FROM `config` WHERE `platform`='%d'", platformid);
	}
	ctx->free_func = (Ecore_Cb)Array_KEYVALUE_free;
	QID_RETURN;

}

/* FIXME TODO convert to new getConfig/setConfig function and remove this */
Eina_Bool getControlModuleConfig(DB *db, Zentific_Ctx *ctx, int id, const char *key){
	DB_VARS;
	if(key){
		QUERY(Array_KEYVALUE_esql,
			"SELECT * FROM `config` WHERE `subsystem` = 'zensched'"
			" AND `mid` = '%d' AND `key`='%s'", id, key);
	} else {
		QUERY(Array_KEYVALUE_esql,
		"SELECT * FROM `config` WHERE `subsystem` = 'zensched'"
		" AND `mid` = '%d'", id);
	}
	ctx->free_func = (Ecore_Cb)Array_KEYVALUE_free;
	QID_RETURN;
}

Eina_Bool getControlModuleId(DB *db, Zentific_Ctx *ctx, const char *name){
	QIA(
		"SELECT id FROM `controlmodules` where name='%s'",
		name);
}

Eina_Bool getControlModuleName(DB *db, Zentific_Ctx *ctx, int id){
	QSA(
		"SELECT name FROM `controlmodules` where id='%d'",
		id);
}

Eina_Bool getControlModuleNameByNetwork(DB *db, Zentific_Ctx *ctx, int nwid){
	QSA(
		"SELECT name FROM `controlmodules` "
		"LEFT JOIN networks ON networks.mid=controlmodules.id "
		"WHERE networks.id='%d'",
		nwid);
}

Eina_Bool getControlModuleNameByStorage(DB *db, Zentific_Ctx *ctx, int id){
	QSA(
		"SELECT name FROM `controlmodules` "
		"LEFT JOIN storage ON storage.mid=controlmodules.id "
		"WHERE storage.id='%d'",
		id);
}

Eina_Bool getControlModules(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT id FROM `controlmodules`");
}

Eina_Bool getStorageControlModulesFull(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_PLUGIN_esql,
		"SELECT * FROM `controlmodules` WHERE controlstorage='1'");
	ctx->free_func = (Ecore_Cb)Array_PLUGIN_free;
	QID_RETURN;
}

Eina_Bool getNetworkControlModulesFull(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_PLUGIN_esql,
		"SELECT * FROM `controlmodules` WHERE controlnetwork='1'");
	ctx->free_func = (Ecore_Cb)Array_PLUGIN_free;
	QID_RETURN;
}

Eina_Bool getPlatformControlModulesFull(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_PLUGIN_esql,
		"SELECT * FROM `controlmodules` WHERE controlplatform='1'");
	ctx->free_func = (Ecore_Cb)Array_PLUGIN_free;
	QID_RETURN;
}

Eina_Bool getPlatformControlModules(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT id FROM `controlmodules` WHERE controlplatform='1'");
}

Eina_Bool getStorageControlModules(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT id FROM `controlmodules` WHERE controlstorage='1'");
}

Eina_Bool getNetworkControlModules(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT id FROM `controlmodules` WHERE controlnetwork='1'");
}

Eina_Bool getControlModulesByPlatform(DB *db, Zentific_Ctx *ctx, int id){
	QISA(
		"SELECT id FROM `controlmodules` WHERE (platformid='%d' OR platformid='-1')",
		id);
}

Eina_Bool getControlModulesFull(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_PLUGIN_esql,
		"SELECT * FROM `controlmodules`");
	ctx->free_func = (Ecore_Cb)Array_PLUGIN_free;
	QID_RETURN;
}


Eina_Bool getControlModule(DB *db, Zentific_Ctx *ctx, int id){
	DB_VARS;
	QUERY(PLUGIN_esql,
		"SELECT * FROM `controlmodules` WHERE `id`='%d'",
		id);
	ctx->free_func = (Ecore_Cb)PLUGIN_free;
	QID_RETURN;
}

Eina_Bool getControlModuleByName(DB *db, Zentific_Ctx *ctx, const char *name){
	DB_VARS;
	QUERY(PLUGIN_esql,
		"SELECT * FROM `controlmodules` WHERE `name`='%s'", name);
	ctx->free_func = (Ecore_Cb)Array_PLUGIN_free;
	QID_RETURN;
}

/* consider refactoring */
#if 0
Eina_Bool setControlModuleConfig(DB *db, Zentific_Ctx *ctx, int id, Eina_List /*KEYVALUE*/ *config){
	DBE(
		"DELETE FROM `config` WHERE `subsystem`='zensched' AND `mid`='%d'", id);

	for(;config && config->data && !(azy_content_error_is_set(error); config=config->next){

		KEYVALUE *kv = (KEYVALUE*)config->data;

		DBE(
			"INSERT INTO `config` (`mid`, `subsystem`, `key`, `value`)"
			" VALUES ('%d', 'zensched', '%s', '%s')",
			id, kv->key, kv->value);

	}

	return (azy_content_error_is_set(error) ? EINA_FALSE : EINA_TRUE;
}
#endif

Eina_Bool getPlatformIdByVm(DB *db, Zentific_Ctx *ctx, const char *uuid) {

	/* There must be a better way.. */
	QIA(
		"SELECT platforms.id FROM platforms"
		" RIGHT JOIN controlmodules ON platforms.id=controlmodules.platformid"
		" RIGHT JOIN nodes ON nodes.controlmodule=controlmodules.name"
		/*
		 * join with a generated table like this because we desire to keep a record
		 *  in nodechildren rather than a replace into for vm-node association
		 */
		" RIGHT JOIN (SELECT vm, node FROM nodechildren"
		"	RIGHT JOIN vms ON vms.id=nodechildren.vm"
		"	WHERE vms.uuid='%s'"
		"	ORDER by nodechildren.timestamp DESC LIMIT 1)"
		" AS nc ON nc.node=nodes.id"
		" RIGHT JOIN vms ON nc.vm=vms.id"
		" WHERE vms.uuid='%s'",
		uuid, uuid);

}

Eina_Bool getPlatformFeatures(DB *db, Zentific_Ctx *ctx, int platform){
	QIA(
		"SELECT features FROM platforms WHERE id='%d'",
		platform);
}
