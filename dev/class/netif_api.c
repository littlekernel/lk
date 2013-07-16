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

#include <err.h>
#include <dev/class/netif.h>

status_t class_netif_set_state(struct device *dev, struct netstack_state *state)
{
	struct netif_ops *ops = device_get_driver_ops(dev, struct netif_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->set_state)
		return ops->set_state(dev, state);
	else
		return ERR_NOT_SUPPORTED;
}

ssize_t class_netif_get_hwaddr(struct device *dev, void *buf, size_t max_len)
{
	struct netif_ops *ops = device_get_driver_ops(dev, struct netif_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->get_hwaddr)
		return ops->get_hwaddr(dev, buf, max_len);
	else
		return ERR_NOT_SUPPORTED;
}

ssize_t class_netif_get_mtu(struct device *dev)
{
	struct netif_ops *ops = device_get_driver_ops(dev, struct netif_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->get_mtu)
		return ops->get_mtu(dev);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_netif_set_status(struct device *dev, bool up)
{
	struct netif_ops *ops = device_get_driver_ops(dev, struct netif_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->set_status)
		return ops->set_status(dev, up);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_netif_output(struct device *dev, struct pbuf *p)
{
	struct netif_ops *ops = device_get_driver_ops(dev, struct netif_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->output)
		return ops->output(dev, p);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_netif_mcast_filter(struct device *dev, const uint8_t *mac, int action)
{
	struct netif_ops *ops = device_get_driver_ops(dev, struct netif_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->mcast_filter)
		return ops->mcast_filter(dev, mac, action);
	else
		return ERR_NOT_SUPPORTED;
}

