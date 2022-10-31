/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright 2016-2020, Intel Corporation */

/*
 * tx.h -- internal definitions for transactions
 */

#ifndef DAV_INTERNAL_TX_H
#define DAV_INTERNAL_TX_H 1

#include <stdint.h>

#define TX_DEFAULT_RANGE_CACHE_SIZE (1 << 15)

struct ulog_entry_base;
struct pmem_ops;
/*
 * tx_create_wal_entry -- convert to WAL a single ulog UNDO entry
 */
int tx_create_wal_entry(struct ulog_entry_base *e, void *arg,
	const struct pmem_ops *p_ops);

#endif
