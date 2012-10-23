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

// -----------------------------------------------
/* API Functions: Storage  			   */
/* ----------------------------------------------- */

/* TODO determine whether handling of role in this case is even valid
 *
 */
Eina_Bool getAllStorage(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	if ((offset < 0) || (recordcount < 0))
		QIS(
			"SELECT id FROM `storage`");
	QISA(
		"SELECT id FROM `storage` LIMIT %d, %d",
		offset, recordcount);
}

Eina_Bool getAllStorageFull(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	DB_VARS;
	if ((offset < 0) || (recordcount < 0))
		QUERY_NOESCAPE(Array_STORAGE_esql,
			"SELECT * FROM `storage`");
	else
		QUERY(Array_STORAGE_esql,
			"SELECT * from storage LIMIT %d, %d",
			offset, recordcount);
	ctx->free_func = (Ecore_Cb)Array_STORAGE_free;
	QID_RETURN;
}

Eina_Bool getAllStorageFullSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	DB_VARS;
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QUERY(Array_STORAGE_esql,
		"SELECT * FROM `storage` "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
	ctx->free_func = (Ecore_Cb)Array_STORAGE_free;
	QID_RETURN;
}

Eina_Bool getAllStorageSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QISA(
		"SELECT id FROM `storage` "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
}

Eina_Bool getAllStorageByType(DB *db, Zentific_Ctx *ctx, const char *type, int offset, int recordcount){
	if ((offset < 0) || (recordcount < 0))
		QISA(
			"SELECT id FROM `storage` WHERE type='%s'",
			type);
	QISA(
		"SELECT id FROM `storage` WHERE type='%s' LIMIT %d, %d",
		type, offset, recordcount);
}

Eina_Bool getStorageHost(DB *db, Zentific_Ctx *ctx, int id){
	QSA(
		"SELECT host FROM storage WHERE id='%i'",
		id);
}

Eina_Bool getStorage(DB *db, Zentific_Ctx *ctx, int id){
	DB_VARS;
	QUERY(Array_STORAGE_esql,
		"SELECT * FROM `storage` WHERE id='%d'", id);
	ctx->free_func = (Ecore_Cb)Array_STORAGE_free;
	QID_RETURN;
}

Eina_Bool removeStorage(DB *db, Zentific_Ctx *ctx, int id){
	QE(
		"DELETE FROM `storage` WHERE id='%d'", id);
}

Eina_Bool getNodesByStorage(DB *db, Zentific_Ctx *ctx, int id){
	QSSA(
		"SELECT uuid from nodes RIGHT JOIN `storagenodes` ON storagenodes.node=nodes.id"
		" WHERE storagenodes.storage='%d'",
		id);
}

Eina_Bool getStorageByNode(DB *db, Zentific_Ctx *ctx, const char *uuid){
	DB_VARS;
	QUERY(Array_STORAGE_esql,
		"SELECT * FROM `storage` RIGHT JOIN storagenodes ON storagenodes.storage=storage.id"
		" RIGHT JOIN nodes ON nodes.id=storagenodes.node WHERE nodes.uuid='%s'",
		uuid);
	ctx->free_func = (Ecore_Cb)Array_STORAGE_free;
	QID_RETURN;
}

Eina_Bool defineStorage(DB *db, Zentific_Ctx *ctx, STORAGE *stor){
	QEJ(
		"INSERT INTO storage"
		" (name, type, path, host, mode, capacity, freespace, mid)"
		" VALUES "
		" ('%s', '%s', '%s', '%s', '%s', '%d', '%d', '%d')",
		stor->name, stor->type, stor->path, stor->host,
		stor->mode, stor->capacity, stor->freespace, stor->mid);
}

Eina_Bool modifyStorage(DB *db, Zentific_Ctx *ctx, int id, STORAGE *storage){
	QE(
		"UPDATE `storage`"
		" SET name='%s', type='%s', path='%s', capacity='%d',"
		" freespace='%d', mid='%d', mode='%s', host='%s'"
		" WHERE id='%d'",
		storage->name, storage->type, storage->path, storage->capacity,
		storage->freespace, storage->mid, storage->mode, storage->host,
		id);
}

Eina_Bool isStorageId(DB *db, Zentific_Ctx *ctx, int id){
	QIAB(
		"SELECT id FROM `storage`"
		" WHERE id='%d'",
		id);
}

Eina_Bool isStorage(DB *db, Zentific_Ctx *ctx, STORAGE *stor){
	QIAB(
		"SELECT id FROM `storage`"
		" WHERE name='%s'",
		stor->name);
}

Eina_Bool addDisk(DB *db, Zentific_Ctx *ctx, int storid, DISK *disk){
	QE(
		"INSERT INTO `diskconfig`"
		"  (disk, revision, type, mode, int_dev, ext_dev, mapped_dev, partition_type, num_sectors, start_sector, size_sector, block_size)"
		" VALUES ("
		"  (SELECT id FROM `disks` WHERE storid='%d' and path='%s'),"
		"  '%d',"
		"  '%s', '%s', '%s', '%s', '%s', '%s',"
		"  '%lf', '%lf', '%d', '%d'"
		" )",
		storid, disk->ext_dev,
		0,  /* this is the initial revision */
		disk->type, disk->mode, disk->int_dev,
		disk->ext_dev, disk->mapped_dev, disk->partition_type,
		disk->num_sectors, disk->start_sector,
		disk->size_sector, disk->block_size);
}

Eina_Bool enableDisk(DB *db, Zentific_Ctx *ctx, int storid, const char *path){
	QE(
		"UPDATE disks SET enabled='1' WHERE storid='%d' AND path='%s",
		storid, path);
}

Eina_Bool disableDisk(DB *db, Zentific_Ctx *ctx, int storid, const char *path){
	QE(
		"UPDATE disks SET enabled='0' WHERE storid='%d' AND path='%s'",
		storid, path);
}

Eina_Bool attachDisk(DB *db, Zentific_Ctx *ctx, int storid, const char *path){
	QE(
		"UPDATE disks SET attached='1' WHERE storid='%d' AND path='%s",
		storid, path);
}

Eina_Bool detachDisk(DB *db, Zentific_Ctx *ctx, int storid, const char *path){
	QE(
		"UPDATE disks SET attached='0' WHERE storid='%d' AND path='%s'",
		storid, path);
}

Eina_Bool modifyDisk(DB *db, Zentific_Ctx *ctx, int storid, const char *path, DISK *disk){
	QE(
		"INSERT INTO `diskconfig`"
		"  (disk, revision, type, mode, int_dev, ext_dev, mapped_dev, partition_type, num_sectors, start_sector, size_sector, block_size)"
		" VALUES ("
		"  (SELECT id FROM `disks` WHERE storid='%d' and path='%s'),"
		"  ((SELECT revision FROM diskconfig WHERE storid='%d' AND path='%s' ORDER BY timestamp DESC LIMIT 1)+1),"
		"  '%s', '%s', '%s', '%s', '%s', '%s',"
		"  '%lf', '%lf', '%d', '%d'"
		" )",
		storid, path,
		storid, path,
		disk->type, disk->mode, disk->int_dev,
		disk->ext_dev, disk->mapped_dev, disk->partition_type,
		disk->num_sectors, disk->start_sector,
		disk->size_sector, disk->block_size);

}

Eina_Bool removeDisk(DB *db, Zentific_Ctx *ctx, int storid, const char *path){
	QE(
		"DELETE diskstats.*, diskconfig.*, disks.* "
		"JOIN disks ON diskstats.disk=disks.id "
		"JOIN diskconfig ON diskconfig.disk=disks.id "
		"WHERE disks.path='%s' AND storid='%i'",
		path, storid);
}

Eina_Bool detachAllDisksByStorage(DB *db, Zentific_Ctx *ctx, int storid){
	QE(
		"UPDATE disks SET attached='0' WHERE storid='%d'",
		storid);

}

Eina_Bool getStorageControlModule(DB *db, Zentific_Ctx *ctx, int storid){
	QIA(
		"SELECT mid FROM storage WHERE id='%d'",
		storid);
}

Eina_Bool setStorageControlModule(DB *db, Zentific_Ctx *ctx, int storid, int mid){
	QE(
		"UPDATE storage SET mid='%d' WHERE id='%d'",
		mid, storid);
}

Eina_Bool detachStorageFromNode(DB *db, Zentific_Ctx *ctx, const char *uuid, int storid){
	QE(
		"DELETE FROM storagenodes "
		"JOIN nodes ON storagenodes.node=nodes.id "
		"WHERE nodes.uuid='%s' "
		"AND storage='%d'",
		uuid, storid);
}

Eina_Bool attachStorageToNode(DB *db, Zentific_Ctx *ctx, const char *uuid, int storid){
	QE(
		"INSERT INTO storagenodes (node, storage)"
		" VALUES ((SELECT id FROM nodes WHERE uuid='%s'), '%d')",
		uuid, storid);
}

Eina_Bool getVmsByStorage(DB *db, Zentific_Ctx *ctx, int storid){
	QSSA(
		"SELECT uuid FROM disks "
		"WHERE storid='%d' "
		"GROUP BY uuid",
		storid);
}

Eina_Bool getStorageByVm(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QISA(
		"SELECT storid FROM disks "
		"WHERE uuid='%s' "
		"GROUP BY storid",
		uuid);

}

/* FIXME: correct? */
Eina_Bool isStorageInUse(DB *db, Zentific_Ctx *ctx, int storid) {
	QIAB(
		"SELECT storid from storagenodes WHERE storage='%d' LIMIT 1",
		storid);
}

Eina_Bool getStorageByDisk(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *path){
	QIA(
		"SELECT storid FROM disks WHERE uuid='%s' AND path='%s'",
		uuid, path);
}

Eina_Bool getStorageIdByName(DB *db, Zentific_Ctx *ctx, const char *name){
	QIA(
		"SELECT id FROM storage WHERE name='%s'",
		name);
}

Eina_Bool getAllStorageTypes(DB *db, Zentific_Ctx *ctx){
	QSS("SELECT type FROM storage");
}

Eina_Bool isDiskByStorage(DB *db, Zentific_Ctx *ctx, int storid, const char *path) {
	QIAB(
		"SELECT id FROM disks"
		" WHERE storid='%d' AND path='%s'",
		storid, path);
}

Eina_Bool getStorageCount(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT COUNT(id) FROM `storage`");
}
