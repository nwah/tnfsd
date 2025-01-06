//go:build ignore
/* The MIT License
 *
 * Copyright (c) 2010 Dylan Smith
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * The main()
 *
 * */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "datagram.h"
#include "event.h"
#include "session.h"
#include "directory.h"
#include "errortable.h"
#include "chroot.h"
#include "log.h"
#include "auth.h"

/* declare the main() - it won't be used elsewhere so I'll not bother
 * with putting it in a .h file */
int main(int argc, char **argv);

int main(int argc, char **argv)
{
    int opt;
#ifdef ENABLE_CHROOT
    char *uvalue = NULL;
    char *gvalue = NULL;
#endif
    bool read_only = false;
    char *pvalue = NULL;

    if(argc >= 2)
    {
        #ifdef ENABLE_CHROOT
        while((opt = getopt(argc, argv, "ru:g:p:")) != -1)
        #else
        while((opt = getopt(argc, argv, "rp:")) != -1)
        #endif
        {
            switch(opt)
            {

                case 'p':
                    pvalue = optarg;
                    break;
                case 'r':
                    read_only = true;
                    break;
                #ifdef ENABLE_CHROOT
                case 'u':
                    uvalue = optarg;
                    break;
                case 'g':
                    gvalue = optarg;
                    break;
                #endif
                case ':':
                    LOG("option needs a value\n");
                    break;
                case '?':
                    LOG("unknown option: %c\n", optopt);
                    break;
            }
        }
    }
    else
    {
    #ifdef ENABLE_CHROOT
    LOG("Usage: tnfsd <root dir> [-u <username> -g <group> -p <port>] -r\n");
    #else
    LOG("Usage: tnfsd <root dir> [-p <port>] -r\n");
    #endif
    exit(-1);
    }

    #ifdef ENABLE_CHROOT
    if (uvalue || gvalue)
    {
        /* chroot into the specified directory and drop privs */
        if (uvalue == NULL)
        {
            LOG("chroot username required\n");
            exit(-1);
        } else if (gvalue == NULL)
        {
            LOG("chroot group required\n");
            exit(-1);
        }
        chroot_tnfs(uvalue, gvalue, argv[optind]);
        if (tnfs_setroot("/") < 0)
        {
            LOG("Unable to chdir to /...\n");
            exit(-1);
        }
    }
    else if (tnfs_setroot(argv[optind]) < 0)
    {
    #else
    if(tnfs_setroot(argv[optind]) < 0)
    {
    #endif
		LOG("Invalid root directory\n");
		exit(-1);
	}

    #ifdef ENABLE_CHROOT
    warn_if_root();
    #endif

    int port = TNFSD_PORT;

    if (pvalue)
    {
        port = atoi(pvalue);
        if (port < 1 || port > 65535)
        {
            LOG("Invalid port\n");
            exit(-1);
        }
    }

	const char *version = "24.0522.1";

	LOG("Starting tnfsd version %s on port %d using root directory \"%s\"\n", version, port, argv[optind]);
    if (read_only)
    {
        LOG("The server runs in read-only mode. TNFS clients can only list and download files.\n");
    }
    else
    {
        LOG("The server runs in read-write mode. TNFS clients can upload and modify files. Use -r to enable read-only mode.\n");
    }

	tnfs_init();              /* initialize structures etc. */
	tnfs_init_errtable();     /* initialize error lookup table */
	tnfs_event_init();        /* initialize event system */
	tnfs_sockinit(port);      /* initialize communications */
	auth_init(read_only);     /* initialize authentication */
	tnfs_mainloop();          /* run */

	return 0;
}
