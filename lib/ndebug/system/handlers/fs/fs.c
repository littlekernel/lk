/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <lib/ndebug/system/handlers/fs/fs.h>

#include <compiler.h>
#include <debug.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include <lib/fs.h>
#include <lib/ndebug/system/cmdhdlr.h>
#include <lib/ndebug/system/mux.h>

static status_t reply(status_t status)
{
	uint32_t response = (uint32_t)(-status);
	if (response > 255) {
		return ERR_TOO_BIG;
	}

	response |= 0x52455400;  // RET<status>

	return ndebug_write_sys((uint8_t *)(&response), sizeof(response), 
							NDEBUG_SYS_CHANNEL_COMMAND, 1000);
}


static status_t putfile_handler(char *path, size_t length)
{
	status_t rc;
	filehandle *handle = NULL;
	const size_t file_len = length;

	rc = fs_create_file(path, &handle, length);
	if (rc != NO_ERROR) {
		reply(rc);
		return rc;
	}


	// TODO(gkalsi): Use the bcache here? Cache into block sized chunks.
	uint8_t *cache = malloc(length);
	uint8_t *cursor = cache;
	if (!cache) {
		reply(ERR_NO_MEMORY);
		fs_close_file(handle);
		return ERR_NO_MEMORY;
	}

	reply(NO_ERROR);

	while (length) {
		uint8_t *buf;
		ssize_t read = ndebug_read_sys(&buf, NDEBUG_SYS_CHANNEL_COMMAND, 1000);

		if (read < 0) {
			rc = read;
			goto finish;
		}

		if (read > (ssize_t)length) {
			read = length;
		}
		memcpy(cursor, buf, read);

		cursor += read;
		length -= read;
	}
		
	ssize_t written = fs_write_file(handle, cache, 0, file_len);
	if (written < 0) {
		rc = written;
		goto finish;
	}

	reply(NO_ERROR);
finish:
	free(cache);
	fs_close_file(handle);
	return rc;
}

static status_t getfile_handler(char *path)
{
	PANIC_UNIMPLEMENTED;
}

static status_t remfile_handler(char *path)
{
	PANIC_UNIMPLEMENTED;
}

static status_t exists_handler(char *path)
{
	PANIC_UNIMPLEMENTED;
}

static status_t cmdhdlr_fs_handler(uint8_t *data, const size_t len)
{
	cmdhdlr_fs_header_t header;

	if (len < sizeof(header)) {
		reply(ERR_INVALID_ARGS);
		return ERR_INVALID_ARGS;
	}

	memcpy((void *)&header, data, sizeof(header));
	data += sizeof(header);

	if (header.path_len > FS_MAX_PATH_LEN) {
		reply(ERR_TOO_BIG);
		return ERR_TOO_BIG;
	}

	reply(NO_ERROR);

	static char path[FS_MAX_PATH_LEN + 1];
	char *cursor = path;
	memset(path, 0, sizeof(path));
	while (header.path_len) {
		uint8_t *buf;
		ssize_t read = ndebug_read_sys(&buf, NDEBUG_SYS_CHANNEL_COMMAND, 1000);
		if (read < 0) {
			return read;
		}
		read = MIN(header.path_len, read);
		memcpy(cursor, buf, read);
		cursor += read;
		header.path_len -= read;
	}

	switch (header.opcode) {
		case OPCODE_PUTFILE:
			return putfile_handler(path, header.data_len);
			break;
		case OPCODE_GETFILE:
			reply(ERR_NOT_IMPLEMENTED);
			break;
		case OPCODE_REMFILE:
			reply(ERR_NOT_IMPLEMENTED);
			break;
		case OPCODE_EXISTS:
			reply(ERR_NOT_IMPLEMENTED);
			break;
	}
	return ERR_NOT_SUPPORTED;
}

void cmdhdlr_fs_init(void)
{
	cmdhdlr_register_handler(CMDHDLR_FS_OPCODE, cmdhdlr_fs_handler);
}