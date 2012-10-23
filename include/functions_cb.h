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

#ifndef FUNCTIONS_CB_H
#define FUNCTIONS_CB_H

#include "zentific.h"
#include "ztypes.h"

void cb_int_rewind(Zentific_Ctx *ctx, int ret);
void cb_pointer_job(Zentific_Ctx *ctx, void *ret);
void cb_bool_job(Zentific_Ctx *ctx, Eina_Bool ret);
void cb_int_job(Zentific_Ctx *ctx, int ret);
void cb_bool_rewind(Zentific_Ctx *ctx, Eina_Bool ret);
void cb_auth_rewind(Zentific_Ctx *ctx, Eina_Bool ret);
void cb_pointer_return(Zentific_Ctx *ctx, void *ret);
void cb_int_return(Zentific_Ctx *ctx, int ret);
void cb_job_target_node_addr(Zentific_Ctx *ctx, Eina_List *ret);
void cb_job_target_host(Zentific_Ctx *ctx, const char *ret);
void cb_job_module_name(Zentific_Ctx *ctx, const char *ret);
#endif
