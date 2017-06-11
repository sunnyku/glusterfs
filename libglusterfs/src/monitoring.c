/*
  Copyright (c) 2017 Red Hat, Inc. <http://www.redhat.com>
  This file is part of GlusterFS.

  This file is licensed to you under your choice of the GNU Lesser
  General Public License, version 3 or any later version (LGPLv3 or
  later), or the GNU General Public License, version 2 (GPLv2), in all
  cases as published by the Free Software Foundation.
*/

#include "monitoring.h"
#include "xlator.h"
#include "syscall.h"

#include <stdlib.h>

void
dump_memory_accounting (int fd)
{
#if MEMORY_ACCOUNTING_STATS
        char     msg[256] = {0,};
        int      i        = 0;
        uint64_t count    = 0;

        uint64_t tcalloc = GF_ATOMIC_GET (gf_memory_stat_counts.total_calloc);
        uint64_t tmalloc = GF_ATOMIC_GET (gf_memory_stat_counts.total_malloc);
        uint64_t tfree   = GF_ATOMIC_GET (gf_memory_stat_counts.total_free);

        memset (msg, 0, sizeof (msg));
        snprintf (msg, sizeof(msg), "memory.total.calloc %lu\n", tcalloc);
        sys_write (fd, msg, strlen (msg));

        memset (msg, 0, sizeof (msg));
        snprintf (msg, sizeof(msg), "memory.total.malloc %lu\n", tmalloc);
        sys_write (fd, msg, strlen (msg));

        memset (msg, 0, sizeof (msg));
        snprintf (msg, sizeof(msg), "memory.total.realloc %lu\n",
                  GF_ATOMIC_GET (gf_memory_stat_counts.total_realloc));
        sys_write (fd, msg, strlen (msg));

        memset (msg, 0, sizeof (msg));
        snprintf (msg, sizeof(msg), "memory.total.free %lu\n", tfree);
        sys_write (fd, msg, strlen (msg));

        memset (msg, 0, sizeof (msg));
        snprintf (msg, sizeof(msg), "memory.total.in-use %lu\n",
                  ((tcalloc + tmalloc) - tfree));
        sys_write (fd, msg, strlen (msg));

        for (i = 0; i < GF_BLK_MAX_VALUE; i++) {
                count = GF_ATOMIC_GET (gf_memory_stat_counts.blk_size[i]);
                memset (msg, 0, sizeof (msg));
                snprintf (msg, sizeof(msg), "memory.total.blk_size[%d] %lu\n",
                          i, count);
                sys_write (fd, msg, strlen (msg));
        }

        fsync (fd);
#endif
}


static void
update_latency_and_count (xlator_t *xl, int index, int fd)
{
        uint64_t fop;
        uint64_t cbk;
        char msg[1024] = {0,};
        glusterfs_graph_t *graph = NULL;

        graph = xl->graph;

        fop = GF_ATOMIC_GET (xl->metrics[index].fop);
        cbk = GF_ATOMIC_GET (xl->metrics[index].cbk);
        if (fop) {
                snprintf (msg, sizeof(msg), "%s.%d.%s.count %lu\n",
                          xl->name, (graph) ? graph->id:0, gf_fop_list[index], fop);
                sys_write (fd, msg, strlen (msg));
        }
        if (cbk) {
                memset (msg, 0, sizeof (msg));
                snprintf (msg, sizeof(msg),
                          "%s.%d.%s.fail_count %lu\n",
                          xl->name, (graph) ? graph->id:0, gf_fop_list[index],
                          cbk);
                sys_write (fd, msg, strlen (msg));
        }
        if (xl->latencies[index].mean != 0.0) {
                memset (msg, 0, sizeof (msg));
                snprintf (msg, sizeof(msg), "%s.%d.%s.latency %lf\n",
                          xl->name, (graph) ? graph->id:0, gf_fop_list[index],
                          xl->latencies[index].mean);
                sys_write (fd, msg, strlen (msg));
        }
}

static void
dump_metrics (glusterfs_ctx_t *ctx, int fd)
{
        xlator_t *xl = NULL;
        int fop = 0;

        xl = ctx->active->top;

        /* Let every file have information on which process dumped info */
        sys_write (fd, ctx->cmdlinestr, strlen (ctx->cmdlinestr));
        sys_write (fd, "\n", 1);

        /* Dump memory accounting */
        dump_memory_accounting (fd);

        while (xl) {
                for (fop = 0; fop < GF_FOP_MAXVALUE; fop++) {
                        update_latency_and_count (xl, fop, fd);
                }
                xl = xl->next;
        }

        return;
}

void
gf_monitor_metrics (int sig, glusterfs_ctx_t *ctx)
{
        int fd = 0;
        char filepath[128] = {0,};

        strncat (filepath, "/tmp/glusterfs.XXXXXX",
                 strlen ("/tmp/glusterfs.XXXXXX"));

        fd = mkstemp (filepath);
        if (fd < 0) {
                gf_log ("signal", GF_LOG_ERROR,
                        "failed to open tmp file %s (%s)",
                        filepath, strerror (errno));
                return;
        }

        dump_metrics (ctx, fd);

        sys_close (fd);

        return;
}
