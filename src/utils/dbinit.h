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
//system includes
#include "zentific.h"
#include <sys/wait.h>
#include <getopt.h>
#include <dirent.h>

//for kicking the scheduler
#include  <signal.h>
#include  <sys/ipc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

//gets us openssl BIO structures so
// we can pull socket to get remote IP
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

//string manipulation
//misc utilities
#include "utility.h"
#include "db_sources.h"

//sha1 hashing
#include "sha1.h"
#include "sha1wrap.h"
#include "zentific-defines.h"
#include "zconf.h"
#include "Common.h"

#ifndef _PATH_TTY
#define _PATH_TTY "/dev/tty"
#endif

#ifndef _PASSWORD_LEN
#define _PASSWORD_LEN 80
#endif

#define SIZE 65536

char * getpass_x(const char *prompt);
