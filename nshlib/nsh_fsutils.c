/****************************************************************************
 * apps/nshlib/nsh_fsutils.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_catfile
 *
 * Description:
 *   Dump the contents of a file to the current NSH terminal.
 *
 * Input Paratemets:
 *   vtbl     - session vtbl
 *   cmd      - NSH command name to use in error reporting
 *   filepath - The full path to the file to be dumped
 *
 * Returned Value:
 *   Zero (OK) on success; -1 (ERROR) on failure.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_CATFILE
int nsh_catfile(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                FAR const char *filepath)
{
  FAR char *buffer;
  int fd;
  int ret = OK;

  /* Open the file for reading */

  fd = open(filepath, O_RDONLY);
  if (fd < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, cmd, "open", NSH_ERRNO);
      return ERROR;
    }

  buffer = (FAR char *)malloc(IOBUFFERSIZE);
  if(buffer == NULL)
    {
      (void)close(fd);
      nsh_output(vtbl, g_fmtcmdfailed, cmd, "malloc", NSH_ERRNO);
      return ERROR;
    }

  /* And just dump it byte for byte into stdout */

  for (;;)
    {
      int nbytesread = read(fd, buffer, IOBUFFERSIZE);

      /* Check for read errors */

      if (nbytesread < 0)
        {
          int errval = errno;

          /* EINTR is not an error (but will stop stop the cat) */

#ifndef CONFIG_DISABLE_SIGNALS
          if (errval == EINTR)
            {
              nsh_output(vtbl, g_fmtsignalrecvd, cmd);
            }
          else
#endif
            {
              nsh_output(vtbl, g_fmtcmdfailed, cmd, "read", NSH_ERRNO_OF(errval));
            }

          ret = ERROR;
          break;
        }

      /* Check for data successfully read */

      else if (nbytesread > 0)
        {
          int nbyteswritten = 0;

          while (nbyteswritten < nbytesread)
            {
              ssize_t n = nsh_write(vtbl, buffer, nbytesread);
              if (n < 0)
                {
                  int errval = errno;

                  /* EINTR is not an error (but will stop stop the cat) */

#ifndef CONFIG_DISABLE_SIGNALS
                  if (errval == EINTR)
                    {
                      nsh_output(vtbl, g_fmtsignalrecvd, cmd);
                    }
                  else
#endif
                    {
                      nsh_output(vtbl, g_fmtcmdfailed, cmd, "write", NSH_ERRNO);
                    }

                  ret = ERROR;
                  break;
                }
              else
                {
                  nbyteswritten += n;
                }
            }
        }

      /* Otherwise, it is the end of file */

      else
        {
          break;
        }
    }

   /* Make sure that the following NSH prompt appears on a new line.  If the
    * file ends in a newline, then this will print an extra blank line
    * before the prompt, but that is preferable to the case where there is
    * no newline and the NSH prompt appears on the same line as the cat'ed
    * file.
    */

   nsh_output(vtbl, "\n");

   /* Close the input file and return the result */

   (void)close(fd);
   free(buffer);
   return ret;
}
#endif

/****************************************************************************
 * Name: nsh_readfile
 *
 * Description:
 *   Read a small file into a buffer buffer.  An error occurs if the file
 *   will not fit into the buffer.
 *
 * Input Paramters:
 *   vtbl     - The console vtable
 *   filepath - The full path to the file to be read
 *   buffer   - The user-provided buffer into which the file is read.
 *   buflen   - The size of the user provided buffer
 *
 * Returned Value:
 *   Zero (OK) is returned on success; -1 (ERROR) is returned on any
 *   failure to read the fil into the buffer.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_READFILE
static int nsh_readfile(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                        FAR const char *filepath, FAR char *buffer,
                        size_t buflen)
{
  FAR char *bufptr;
  size_t remaining;
  ssize_t nread;
  ssize_t ntotal;
  int fd;
  int ret;

  /* Open the file */

  fd = open(filepath, O_RDONLY);
  if (fd < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, cmd, "open", NSH_ERRNO);
      return ERROR;
    }

  /* Read until we hit the end of the file, until we have exhausted the
   * buffer space, or until some irrecoverable error occurs
   */

  ntotal    = 0;          /* No bytes read yet */
  *buffer   = '\0';       /* NUL terminate the empty buffer */
  bufptr    = buffer;     /* Working pointer */
  remaining = buflen - 1; /* Reserve one byte for a NUL terminator */
  ret       = ERROR;      /* Assume failure */

  do
    {
      nread = read(fd, bufptr, remaining);
      if (nread < 0)
        {
          /* Read error */

          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          /* EINTR is not a read error.  It simply means that a signal was
           * received while waiting for the read to complete.
           */

          if (errcode != EINTR)
            {
              /* Fatal error */

              nsh_output(vtbl, g_fmtcmdfailed, cmd, "read", NSH_ERRNO);
              break;
            }
        }
      else if (nread == 0)
        {
          /* End of file */

          ret = OK;
          break;
        }
      else
        {
          /* Successful read.  Make sure that the buffer is null terminated */

          DEBUGASSERT(nread <= remaining);
          ntotal += nread;
          buffer[ntotal] = '\0';

          /* Bump up the read count and continuing reading to the end of
           * file.
           */

          bufptr    += nread;
          remaining -= nread;
        }
    }
  while (buflen > 0);

  /* Close the file and return. */

  close(fd);
  return ret;
}
#endif