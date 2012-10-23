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

#ifndef ZENTIFIC_HEADER
#define ZENTIFIC_HEADER

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#elif defined __GNUC__
#define alloca __builtin_alloca
#elif defined _AIX
#define alloca __alloca
#else
#include <stddef.h>
void *alloca (size_t);
#endif
//#include <stdlib.h>
//#include <stdio.h>
#include <string.h>
#include <Eina.h>
#include <Zshare.h>

extern int zrpc_log_dom;

#define ERR(...) EINA_LOG_DOM_ERR(zrpc_log_dom, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(zrpc_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(zrpc_log_dom, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(zrpc_log_dom, __VA_ARGS__)

#define S(X) #X

#endif
