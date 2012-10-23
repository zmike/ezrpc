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

//returns list of vm uuids which have console locks associated
//	with the provided session id
Eina_Bool getConsoleLockVmsBySession(DB *db, Zentific_Ctx *ctx, const char *session_id){
	QSSA(
		"SELECT vms.uuid FROM vms, sessions, vmconsolelocks "
		"WHERE vmconsolelocks.session=sessions.id "
		"AND vms.id=vmconsolelocks.vm "
		"AND sessions.session_id = '%s'",
		session_id);
}


//returns sessionid
Eina_Bool getConsoleType(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *session_id){
	QSA(
		"SELECT type FROM vmconsolelocks "
		"JOIN sessions ON vmconsolelocks.session=sessions.id "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"WHERE sessions.session_id='%s' AND vms.uuid='%s'",
		session_id, uuid);
}


//returns sessionid
Eina_Bool getConsoleLockSession(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *type){
	QSA(
		"SELECT session_id FROM vmconsolelocks "
		"JOIN sessions ON vmconsolelocks.session=sessions.id "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"WHERE vms.uuid='%s' AND vmconsolelocks.type='%s'",
		uuid, type);
}

Eina_Bool hasConsoleLock(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *type){
	QBA(
		"SELECT vm FROM vmconsolelocks "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"WHERE vms.uuid='%s' AND vmconsolelocks.type='%s'",
		uuid, type);
}

/* NOTE:
 *
 * it may seem redundant to track vm, session,
 * clientip (as if it werent already there from the session),
 * and nodehost (as if it werent already discernable from the vm)
 * BUT such things can change (e.g. client maintains same session
 * cookie but changes IP, or vm migrates from one node to another)
 * and we need to be able to accurately tear down the console session,
 * related daemons, and firewall rules. so: store a bit more than may
 * at first seem necessary
 *
 */
Eina_Bool acquireConsoleLock(DB *db, Zentific_Ctx *ctx, const char *uuid, SESSION *session, const char *type, const char *nodehost){
	QE(
		"INSERT INTO `vmconsolelocks` (vm, session, client, type, nodehost)"
		" VALUES ("
		"	(SELECT `id` FROM `vms` WHERE uuid='%s'),"
		"	(SELECT `id` FROM `sessions` WHERE session_id='%s'),"
		"	'%s',"
		"	'%s',"
		"	'%s'"
		" )",
		uuid, session->session_id, session->client_host, type, nodehost);
}

Eina_Bool releaseConsoleLock(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *sessionid, const char *type){
	QE(
		"USING vmconsolelocks "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"JOIN sessions ON vmconsolelocks.session=sessions.session_id "
		"WHERE vms.uuid='%s' "
		"AND sessions.session_id='%s' AND "
		"vmconsolelocks.type='%s'",
		uuid, sessionid, type);
}

Eina_Bool saveGraphicalConsolePort(DB *db, Zentific_Ctx *ctx, const char *uuid, int tunnelport, int externalport){
	QE(
		"UPDATE vmconsolelocks "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"SET externalport='%d', tunnelport='%d' "
		"WHERE vms.uuid='%s' "
		"AND type='vnc'",
		externalport, tunnelport, uuid);
}

Eina_Bool getGraphicalConsolePort(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QIA(
		"SELECT externalport FROM vmconsolelocks "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"WHERE vms.uuid='%s' "
		"AND type='vnc'",
		uuid);
}

Eina_Bool getConsoleNodeHost(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *type){
	QSA(
		"SELECT nodehost FROM vmconsolelocks "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"WHERE vms.uuid='%s' "
		"AND type='%s'",
		uuid, type);
}

Eina_Bool getConsoleClientHost(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *type){

	QSA(
		"SELECT host FROM vmconsolelocks "
		"JOIN sessions ON vmconsolelocks.session=sessions.id "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"WHERE vms.uuid='%s' AND vmconsolelocks.type='%s'",
		uuid, type);
}

Eina_Bool getGraphicalConsoleTunnelPort(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QISA(
		"SELECT tunnelport FROM vmconsolelocks "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"WHERE vms.uuid='%s' "
		"AND type='vnc'",
		uuid);
}

Eina_Bool saveTextConsolePort(DB *db, Zentific_Ctx *ctx, const char *uuid,  int externalport){
	QE(
		"UPDATE vmconsolelocks "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"SET externalport='%d' "
		"WHERE vms.uuid='%s' "
		"AND type='text'",
		externalport, uuid);
}

Eina_Bool getTextConsolePort(DB *db, Zentific_Ctx *ctx, const char *uuid){
	QISA(
		"SELECT externalport FROM vmconsolelocks "
		"JOIN vms ON vmconsolelocks.vm=vms.id "
		"WHERE vms.uuid='%s' "
		"AND type='text'",
		uuid);
}
