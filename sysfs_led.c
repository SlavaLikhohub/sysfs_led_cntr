// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/gpio.h>

MODULE_AUTHOR("Viaheslav Lykhohub <viacheslav.lykhohub@globallogic.com>");
MODULE_DESCRIPTION("Control of onboard LED using sysfs");
MODULE_LICENSE("GPL");

#define GPIO_NUMBER(port, bit) (32 * (port) + (bit))

/* On-board LESs
 *  0: D2	GPIO1_21	heartbeat
 *  1: D3	GPIO1_22	uSD access
 *  2: D4	GPIO1_23	active
 *  3: D5	GPIO1_24	MMC access
 *
 * uSD and MMC access LEDs are not used in nfs boot mode, but they are already requested
 * So that, we don't use gpio_request()/gpio_free() here.
 */

/* On-board button.
 *
 * HDMI interface must be disabled.
 */
#define LED_SD  GPIO_NUMBER(1, 22)
#define LED_MMC GPIO_NUMBER(1, 24)

struct led_obj {
	unsigned led;
	bool state;
	u64 delay_jiff;
	struct kobject *kobj;

};

static struct led_obj led = {
	.led = LED_SD,
	.delay_jiff = (HZ * 200L) / MSEC_PER_SEC,
};

static int led_init(unsigned gpio)
{
	int rc = gpio_direction_output(gpio, 0);
	return rc;
}

static ssize_t led_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d", led.state);
}

static ssize_t led_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	int ret;
	int val;

	ret = kstrtoint(buf, PAGE_SIZE, &val);
	if (ret < 0)
		return ret;

	led.state = (bool)val;

	gpio_set_value(led.led, led.state);

	return count;
}

static struct kobj_attribute led_attr =
	__ATTR(led, 0664, led_show, led_store);

static struct attribute *led_attrs[] = {
	&led_attr.attr,
	NULL
};

static struct attribute_group led_attr_group = {
	.attrs = led_attrs,
};

static int __init led_mod_init(void)
{
	int rc;

	rc = led_init(led.led);
	if (rc) {
		pr_err("Cannot init GPIO%d as an output \n", led.led);
		return rc;
	}

	pr_info("GPIO initialized\n");

	led.kobj = kobject_create_and_add("led", kernel_kobj);

	rc = sysfs_create_group(led.kobj, &led_attr_group);
	if (rc) {
		pr_err("Cannot create sysfs group\n");
		kobject_put(led.kobj);
		return rc;
	}

	pr_info("Initialized sysfs\n");
	return 0;
}

static void __exit led_mod_exit(void)
{
	kobject_put(led.kobj);
}

module_init(led_mod_init);
module_exit(led_mod_exit);
