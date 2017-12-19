/*
 * Circular buffer implementation.
 * Copyright (C) 2017 Cumulus Networks
 * Quentin Young
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; see the file COPYING; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <zebra.h>

#include "ringbuf.h"
#include "memory.h"

DEFINE_MTYPE_STATIC(LIB, RINGBUFFER, "Ring buffer")

struct ringbuf *ringbuf_new(size_t size)
{
	struct ringbuf *buf = XCALLOC(MTYPE_RINGBUFFER, sizeof(struct ringbuf));
	buf->data = XCALLOC(MTYPE_RINGBUFFER, size);
	buf->size = size;
	buf->empty = true;
	return buf;
}

void ringbuf_del(struct ringbuf *buf)
{
	XFREE(MTYPE_RINGBUFFER, buf->data);
	XFREE(MTYPE_RINGBUFFER, buf);
}

size_t ringbuf_remain(struct ringbuf *buf)
{
	ssize_t diff = buf->end - buf->start;
	diff += ((diff == 0) && !buf->empty) ? buf->size : 0;
	diff += (diff < 0) ? buf->size : 0;
	return (size_t)diff;
}

size_t ringbuf_space(struct ringbuf *buf)
{
	return buf->size - ringbuf_remain(buf);
}

size_t ringbuf_put(struct ringbuf *buf, const void *data, size_t size)
{
	const uint8_t *dp = data;
	size_t space = ringbuf_space(buf);
	size_t copysize = MIN(size, space);
	size_t tocopy = copysize;
	if (tocopy > buf->size - buf->end) {
		size_t ts = buf->size - buf->end;
		memcpy(buf->data + buf->end, dp, ts);
		buf->end = 0;
		tocopy -= ts;
		dp += ts;
	}
	memcpy(buf->data + buf->end, dp, tocopy);
	buf->end += tocopy;
	buf->empty = (buf->start == buf->end) && (buf->empty && !copysize);
	return copysize;
}

size_t ringbuf_get(struct ringbuf *buf, void *data, size_t size)
{
	uint8_t *dp = data;
	size_t remain = ringbuf_remain(buf);
	size_t copysize = MIN(remain, size);
	size_t tocopy = copysize;
	if (tocopy > buf->size - buf->start) {
		size_t ts = buf->size - buf->start;
		memcpy(dp, buf->data + buf->start, ts);
		buf->start = 0;
		tocopy -= ts;
		dp += ts;
	}
	memcpy(dp, buf->data + buf->start, tocopy);
	buf->start = buf->start + tocopy;
	buf->empty = (buf->start == buf->end) && (buf->empty || copysize);
	return copysize;
}

void ringbuf_reset(struct ringbuf *buf)
{
	buf->start = buf->end = 0;
	buf->empty = true;
}

void ringbuf_wipe(struct ringbuf *buf)
{
	memset(buf->data, 0x00, buf->size);
	ringbuf_reset(buf);
	buf->empty = true;
}
