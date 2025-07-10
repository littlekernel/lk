/*
 * Copyright (c) 2015 Carlos Pizano-Uribe  cpu@chromium.org
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

// Ports are named, opaque objects and come in three flavors, the
// write-side, the read-side and a port group which is a collection
// of read-side ports.

#define PORT_NAME_LEN 12

typedef void *port_t;

// A Port packet is wide enough to carry two full words of data
#define PORT_PACKET_LEN (sizeof(void *) * 2)
typedef struct {
    char value[PORT_PACKET_LEN];
} port_packet_t;

typedef struct {
    void *ctx;
    port_packet_t packet;
} port_result_t;

typedef enum {
    PORT_MODE_BROADCAST   = 0,
    PORT_MODE_UNICAST     = 1,
    PORT_MODE_BIG_BUFFER  = 2,
} port_mode_t;

// Inits the port subsystem
void port_init(void);

// Make a named write-side port. broadcast ports can be opened by any
// number of read-clients. |name| can be up to PORT_NAME_LEN chars. If
// the write port exists it is returned even if the |mode| does not match.
status_t port_create(const char *name, port_mode_t mode, port_t *port);

// Make a read-side port. Only non-destroyed existing write ports can
// be opened with this api. Unicast ports can only be opened once. For
// broadcast ports, each call if successful returns a new port.
status_t port_open(const char *name, void *ctx, port_t *port);

// Creates a read-side port group which behaves just like a regular
// read-side port. A given port can only be assoicated with one port group.
status_t port_group(port_t *ports, size_t count, port_t *group);

// Adds a read-side port to an existing port group.
status_t port_group_add(port_t group, port_t port);

// Removes a read-side port to an existing port group.
status_t port_group_remove(port_t group, port_t port);

// Write to a port |count| packets, non-blocking, all or none atomic success.
status_t port_write(port_t port, const port_packet_t *pk, size_t count);

// Read one packet from the port or port group, blocking. The |result| contains
// the port that the message was read from. If |timeout| is zero the call
// does not block.
status_t port_read(port_t port, lk_time_t timeout, port_result_t *result);

// Destroy the write-side port, flush queued packets and release all resources,
// all calls will now fail on that port. Only a closed port can be destroyed.
status_t port_destroy(port_t port);

// Close the read-side port or the write side port. A closed write side port
// can be opened and the pending packets read. closing a port group does not
// close the included ports.
status_t port_close(port_t port);

__END_CDECLS
