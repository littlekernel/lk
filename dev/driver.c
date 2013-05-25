/*
 * Copyright (c) 2012 Corey Tabaka
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

#include <dev/driver.h>
#include <assert.h>
#include <err.h>

extern struct device __devices[];
extern struct device __devices_end[];

status_t device_init_all(void)
{
	status_t res = NO_ERROR;

	struct device *dev = __devices;
	while (dev != __devices_end) {
		status_t code = device_init(dev);

		if (code < 0) {
			TRACEF("Driver init failed for driver \"%s\", device \"%s\", reason %d\n",
					dev->driver->type, dev->name, code);

			res = code;
		}

		dev++;
	}

	return res;
}

status_t device_fini_all(void)
{
	status_t res = NO_ERROR;

	struct device *dev = __devices;
	while (dev != __devices_end) {
		status_t code = device_fini(dev);

		if (code < 0) {
			TRACEF("Driver fini failed for driver \"%s\", device \"%s\", reason %d\n",
					dev->driver->type, dev->name, code);

			res = code;
		}

		dev++;
	}

	return res;
}

status_t device_init(struct device *dev)
{
	if (!dev)
		return ERR_INVALID_ARGS;

	DEBUG_ASSERT(dev->driver);
	
	const struct driver_ops *ops = dev->driver->ops;

	if (ops && ops->init)
		return ops->init(dev);
	else
		return ERR_NOT_SUPPORTED;
}

status_t device_fini(struct device *dev)
{
	if (!dev)
		return ERR_INVALID_ARGS;
	
	DEBUG_ASSERT(dev->driver);

	const struct driver_ops *ops = dev->driver->ops;

	if (ops && ops->fini)
		return ops->fini(dev);
	else
		return ERR_NOT_SUPPORTED;
}

status_t device_suspend(struct device *dev)
{
	if (!dev)
		return ERR_NOT_SUPPORTED;
	
	DEBUG_ASSERT(dev->driver);

	const struct driver_ops *ops = dev->driver->ops;

	if (ops && ops->suspend)
		return ops->suspend(dev);
	else
		return ERR_NOT_SUPPORTED;
}

status_t device_resume(struct device *dev)
{
	if (!dev)
		return ERR_NOT_SUPPORTED;
	
	DEBUG_ASSERT(dev->driver);

	const struct driver_ops *ops = dev->driver->ops;

	if (ops && ops->resume)
		return ops->resume(dev);
	else
		return ERR_NOT_SUPPORTED;
}

