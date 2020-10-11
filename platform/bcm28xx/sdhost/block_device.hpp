/*=============================================================================
Copyright (C) 2016-2017 Authors of rpi-open-firmware
All rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

FILE DESCRIPTION
Block device.

=============================================================================*/

#include <lib/bio.h>

struct BlockDevice : bdev_t {
	unsigned int block_size;

	template <typename T>
	inline bool real_read_block(bnum_t sector, T* dest_buffer) {
		return real_read_block(sector, reinterpret_cast<uint32_t*>(dest_buffer));
	}

	inline unsigned int get_block_size() {
		return block_size;
	}

	bool real_read_block(bnum_t sector, uint32_t* buf);

	/* called to stop the block device */
	void stop() {}
};
