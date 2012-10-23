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
/* API Functions: Auth    			   */
/* ----------------------------------------------- */

Eina_Bool getAuthByName(DB *db, Zentific_Ctx *ctx, const char *name) {
	QSSA(
		"select salt, hash from users where username='%s'",
		name);
}

Eina_Bool getAuth(DB *db, Zentific_Ctx *ctx, int uid) {
	DB_VARS;
	QUERY(Array_string_esql,
		"select salt, hash from users where uid='%d'",
		uid);
	ctx->free_func = (Ecore_Cb)Array_string_free;
	QID_RETURN;
}
