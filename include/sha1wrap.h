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
/** @file sha1wrap.h SHA1 Wrapper API */

/** @addtogroup misc_api */
/*! @{ */

#ifndef __SHA1_WRAP_API_H__
#define __SHA1_WRAP_API_H__

/** Claculate sha1 of the file contents.
 *
 * @param path File path.
 * @return sha1 string on success.
 */
char* sha1_file(const char* path);

/** Claculate sha1 of the buffer contents.
 *
 * @param buf Buffer with data.
 * @param len Buffer length.
 * @return sha1 string on success.
 */
char* sha1_buffer(const char *, const char *);

#endif

/*! @} */
