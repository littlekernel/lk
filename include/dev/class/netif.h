/*
 * Copyright (c) 2013 Corey Tabaka
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
#ifndef __DEV_CLASS_NETIF_H
#define __DEV_CLASS_NETIF_H

#include <list.h>
#include <compiler.h>
#include <dev/driver.h>

struct netstack_state;
struct pbuf;

/* netif interface */
struct netif_ops {
	struct driver_ops std;

	status_t (*set_state)(struct device *dev, struct netstack_state *state);
	ssize_t (*get_hwaddr)(struct device *dev, void *buf, size_t max_len);
	ssize_t (*get_mtu)(struct device *dev);

	status_t (*set_status)(struct device *dev, bool up);
	status_t (*output)(struct device *dev, struct pbuf *p);
	status_t (*mcast_filter)(struct device *dev, const uint8_t *mac, int action);
};

__BEGIN_CDECLS

/* netif API */
status_t class_netif_set_state(struct device *dev, struct netstack_state *state);
ssize_t class_netif_get_hwaddr(struct device *dev, void *buf, size_t max_len);
ssize_t class_netif_get_mtu(struct device *dev);
status_t class_netif_set_status(struct device *dev, bool up);
status_t class_netif_output(struct device *dev, struct pbuf *p);
status_t class_netif_mcast_filter(struct device *dev, const uint8_t *mac, int action);

status_t class_netif_add(struct device *dev);

/* network stack API - called by drivers */
status_t class_netstack_input(struct device *dev, struct netstack_state *state, struct pbuf *p);

status_t class_netstack_wait_for_network(lk_time_t timeout);

__END_CDECLS

#endif

