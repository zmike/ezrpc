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
#include "zentific.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sha1.h"

static char* _sha1_to_string(const unsigned char* hashout)
{
  const char hextab[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  char* sha1 = calloc(1, 41);
  unsigned int i = 0;
  for (i=0; i<20; i++)
  {
    sha1[2*i] = hextab[(hashout[i] >> 4) & 0x0f];
    sha1[2*i+1] = hextab[hashout[i] & 0x0f];
  }
  return sha1;
}

char* sha1_file(const char* path)
{
  int fd = open(path, O_RDONLY);
  if (fd < 0)
  {
    ERR("can't open file (%s)", strerror(errno));
    return 0;
  }
  char* buffer = malloc(1024*512);
  ssize_t datalen;
  SHA_CTX2 ctx;
  unsigned char hashout[20];

  SHA1_Init2(&ctx);
  while ((datalen = read(fd, buffer, 1024*512)) > 0)
    SHA1_Update2(&ctx, buffer, datalen);
  SHA1_Final2(hashout, &ctx);

  close(fd);
  free(buffer);
  return _sha1_to_string(hashout);
}

char* sha1_buffer(const char* buf1, const char *buf2)
{
  SHA_CTX2 ctx;
  unsigned char hashout[20];
  char *buf;
  size_t len;

  len = asprintf(&buf, "%s%s", buf1, buf2);
  if (len < 1) return NULL;

  SHA1_Init2(&ctx);
  SHA1_Update2(&ctx, buf, len);
  SHA1_Final2(hashout, &ctx);
  free(buf);
  return _sha1_to_string(hashout);
}
