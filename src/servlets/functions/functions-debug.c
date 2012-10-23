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
#include "functions.h"
#include "functions-debug.h"

#ifdef I_REALLY_KNOW_WHAT_I_AM_DOING
#ifdef THIS_IS_A_SECURITY_RISK_AND_I_AM_AWARE

int _crashme(void)
{
	char crash[2];
	sprintf(crash, "this is the crash string");
	return 0;
}

#endif /* THIS_IS_A_SECURITY_RISK_AND_I_AM_AWARE */
#endif /* I_REALLY_KNOW_WHAT_I_AM_DOING */

#ifndef I_REALLY_KNOW_WHAT_I_AM_DOING
#ifndef THIS_IS_A_SECURITY_RISK_AND_I_AM_AWARE

int _crashme(void)
{
	return 1;
}

int _nonexist(void)
{
	return 1;
}

#endif /* THIS_IS_A_SECURITY_RISK_AND_I_AM_AWARE */
#endif /* I_REALLY_KNOW_WHAT_I_AM_DOING */
