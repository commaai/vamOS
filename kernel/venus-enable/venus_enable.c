// SPDX-License-Identifier: GPL-2.0
/* Enable the existing comma tizi Venus node without replacing the boot image. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "venus_overlay.h"

static int overlay_id;

static int __init venus_enable_init(void)
{
	struct device_node *node;
	struct platform_device *device;
	int ret;

	if (!of_machine_is_compatible("comma,tizi"))
		return -ENODEV;

	node = of_find_node_by_path("/soc@0/video-codec@aa00000");
	if (!node)
		return -ENODEV;
	device = of_find_device_by_node(node);
	ret = device || of_device_is_available(node) ? -EBUSY : 0;
	if (device)
		put_device(&device->dev);
	of_node_put(node);
	if (ret)
		return ret;

	return of_overlay_fdt_apply(venus_overlay, sizeof(venus_overlay), &overlay_id, NULL);
}

static void __exit venus_enable_exit(void)
{
	int ret = of_overlay_remove(&overlay_id);

	if (ret)
		pr_err("venus-enable: overlay removal failed: %d\n", ret);
}

module_init(venus_enable_init);
module_exit(venus_enable_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("comma tizi Venus enablement for existing device trees");
