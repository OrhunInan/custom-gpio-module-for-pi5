#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x92997ed8, "_printk" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xa01f13a6, "cdev_init" },
	{ 0x3a6d85d3, "cdev_add" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x8073dfc1, "gpiod_put" },
	{ 0xfe990052, "gpio_free" },
	{ 0x27271c6b, "cdev_del" },
	{ 0x31da6023, "gpio_to_desc" },
	{ 0xb986a570, "gpiod_direction_output" },
	{ 0x60634223, "gpiod_set_value" },
	{ 0xdf42cd46, "param_ops_int" },
	{ 0x4da93ae9, "param_array_ops" },
	{ 0x474e54d2, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "CFC7B5382EE56F42E3601A1");
