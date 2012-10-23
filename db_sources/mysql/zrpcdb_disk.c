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
/* API Functions: Disk    			   */
/* ----------------------------------------------- */

Eina_Bool isDisk(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *ext_dev){
	QIAB(
		"SELECT id FROM `disks` WHERE path='%s' AND uuid='%s'",
		ext_dev, uuid);
}

Eina_Bool createDisk(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *path, int storid){
	QEE(
		"INSERT INTO `disks` (uuid, path, storid) VALUES ('%s', '%s', '%d')",
		uuid, path, storid);
}


Eina_Bool getNumVMdisks(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QIA(
		"SELECT COUNT(id) FROM `disks` WHERE disks.uuid='%s'",
		uuid);
}

Eina_Bool getDisksByStorage(DB *db, Zentific_Ctx *ctx, int storid){
	DB_VARS;
	QUERY(Array_DISK_esql,
		"SELECT * FROM disks "
		"LEFT JOIN diskconfig ON disks.id=diskconfig.disk "
		"WHERE storid='%d' "
		"AND diskconfig.revision=(SELECT revision FROM diskconfig ORDER BY timestamp DESC LIMIT 1) "
		"GROUP BY diskconfig.disk",
		storid);
	ctx->free_func = (Ecore_Cb)Array_DISK_free;
	QID_RETURN;
}

Eina_Bool getVMdisksByStorage(DB *db, Zentific_Ctx *ctx, const char *uuid, int storid){
	DB_VARS;
	QUERY(Array_DISK_esql,
		"SELECT * FROM diskconfig "
		"RIGHT JOIN disks ON disks.id=diskconfig.disk "
		"WHERE disks.uuid='%s' AND diskconfig.revision= "
			"(SELECT revision FROM diskconfig "
			"RIGHT JOIN disks ON diskconfig.disk=disks.id "
			"WHERE disks.uuid='%s' "
			"ORDER BY diskconfig.timestamp DESC LIMIT 1) "
		"AND disks.storid='%i' "
		"ORDER BY diskconfig.timestamp DESC",
		storid, uuid, uuid);
	ctx->free_func = (Ecore_Cb)Array_DISK_free;
	QID_RETURN;
}

Esql_Query_Id getvmdisks(DB *db, Zentific_Ctx *ctx, const char *uuid, int rev){
	DB_VARS;
	if (rev < 0)
		QUERY(Array_DISK_esql,
			"SELECT * FROM diskconfig "
			"RIGHT JOIN disks ON disks.id=diskconfig.disk "
			"WHERE disks.uuid='%s' AND diskconfig.revision= "
				"(SELECT revision FROM diskconfig "
				"RIGHT JOIN disks ON diskconfig.disk=disks.id "
				"WHERE disks.uuid='%s' "
				"ORDER BY diskconfig.timestamp DESC LIMIT 1) "
			"ORDER BY diskconfig.timestamp DESC",
			uuid, uuid);
	else
		QUERY(Array_DISK_esql,
			"SELECT * FROM diskconfig "
			"RIGHT JOIN disks ON disks.id=diskconfig.disk "
			"WHERE disks.uuid='%s' AND diskconfig.revision='%i'",
			uuid, rev);
	ctx->free_func = (Ecore_Cb)Array_DISK_free;
	return qid;
}

Eina_Bool getVMdisks(DB *db, Zentific_Ctx *ctx, const char *uuid){
	return !!getvmdisks(db, ctx, uuid, -1);
}

Eina_Bool getVmDisk(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *path){
	DB_VARS;
	QUERY(Array_DISK_esql,
		"SELECT * FROM diskconfig"
		" RIGHT JOIN disks ON disks.id=diskconfig.disk"
		" WHERE disks.uuid='%s' AND diskconfig.ext_dev='%s'"
		" ORDER BY diskconfig.revision DESC",
		uuid, path);
	ctx->free_func = (Ecore_Cb)Array_DISK_free;
	QID_RETURN;
}

/* TODO lean on addDisk and this goes away (up to the layer above db)*/
Eina_Bool updateDiskConfig(DB *db, Zentific_Ctx *ctx, const char * uuid, int rev, DISK *disk){
	QE(
		"INSERT INTO `diskconfig`"
		"  (disk, revision, type, mode, int_dev, ext_dev, mapped_dev, partition_type, num_sectors, start_sector, size_sector, block_size, capacity)"
		" VALUES ("
		"  (SELECT id FROM `disks` WHERE uuid='%s' and path='%s'),"
		"  '%d',"
		"  '%s', '%s', '%s', '%s', '%s', '%s',"
		"  '%lf', '%lf', '%d', '%d', '%f'"
		" )",
		uuid, disk->ext_dev, rev, disk->type, disk->mode, disk->int_dev,
		disk->ext_dev, disk->mapped_dev, disk->partition_type,
		disk->num_sectors, disk->start_sector,
		disk->size_sector, disk->block_size, disk->capacity);
}

Eina_Bool updateDiskStats(DB *db, Zentific_Ctx *ctx, const char *uuid, DISK *disk){
	QE(
		"INSERT INTO `diskstats`"
			" (disk, ooreq, rdreq, wrreq)"
		" VALUES ("
			"( SELECT disks.id FROM `disks`, `diskconfig` WHERE"
			" disks.uuid='%s' AND"
			" disks.id=diskconfig.disk AND"
			" diskconfig.ext_dev = '%s'"
			" ORDER BY diskconfig.timestamp DESC LIMIT 1),"
		"       '%lf', '%lf', '%lf'"
		")",
		uuid,
		disk->ext_dev,
		disk->ooreq, disk->rdreq, disk->wrreq);
}
// age really isnt age. think timestamp < now - retentionlength
// 	age = (now - retentionlength)
Eina_Bool pruneDiskStats(DB *db, Zentific_Ctx *ctx, const char * uuid, const char *extdev){
	QE(
		"DELETE FROM diskstats "
		"JOIN disks ON diskstats.disk=disks.id "
		"JOIN diskconfig ON diskstats.disk=diskconfig.disk "
		"WHERE timestamp < DATE_SUB(CURDATE(), "
		"INTERVAL IFNULL("
			"(SELECT value FROM `config` WHERE `key`='stat_retention'"
			" AND `subsystem`='zrpc' AND `mid`='-1'"
			" AND `uid`='-1' AND `gid`='-1' AND `vgid`='-1'"
			" AND `ngid`='-1' AND `vm`='-1' AND `node`='-1'"
			" AND `platform`='-1'), 31) HOUR) "
		"AND disks.uuid='%s' "
		"AND diskconfig.ext_dev='%s' "
		"ORDER BY diskconfig.timestamp DESC LIMIT 1",
		 uuid, extdev);
}

Eina_Bool isDiskInUse(DB *db, Zentific_Ctx *ctx, int storid, const char *path ) {
	QIA(
		"SELECT storid FROM disks WHERE path='%s' AND storid='%d'",
		path, storid);
}

Eina_Bool getDiskByStorage(DB *db, Zentific_Ctx *ctx, int storid, const char *path){
	DB_VARS;
	/* TODO confirm that this is stringent enough to return only one row */
	QUERY(DISK_esql,
		"SELECT * FROM diskconfig"
		"RIGHT JOIN disks ON disks.id=diskconfig.disk "
		"WHERE disks.storid='%d' AND diskconfig.ext_dev='%s' "
		"AND diskconfig.revision=(SELECT revision FROM diskconfig ORDER BY timestamp DESC LIMIT 1) "
		"GROUP BY diskconfig.disk",
		storid, path);
	ctx->free_func = (Ecore_Cb)DISK_free;
	QID_RETURN;
}

Eina_Bool getVmByDisk(DB *db, Zentific_Ctx *ctx, int storid, const char *path){
	QSA(
		"SELECT disks.uuid FROM disks "
		"RIGHT JOIN diskconfig ON disks.id=diskconfig.disk "
		"WHERE disks.storid='%d' AND diskconfig.ext_dev='%s' "
		"AND diskconfig.revision=(SELECT revision FROM diskconfig ORDER BY timestamp DESC LIMIT 1) "
		"GROUP BY diskconfig.disk",
		storid, path);
}
