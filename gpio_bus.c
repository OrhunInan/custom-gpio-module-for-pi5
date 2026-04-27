#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>   
#include <linux/cdev.h> 

#define BUS_LENGTH 4

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Orhun İnan");
MODULE_DESCRIPTION("4-bit parallel GPIO bus driver");

static int pin_count = 0;
static int bus_pins[BUS_LENGTH] = {5, 6, 12, 13};
module_param_array(bus_pins, int, &pin_count, 0000);
MODULE_PARM_DESC(bus_pins, "An array of GPIO pins in use for bus connection.");

struct gpio_bus {
    struct cdev cdev;                 
    struct gpio_desc *pins[BUS_LENGTH];   
    int current_value;                 
};

static struct gpio_bus gpio_bus_dev;
static dev_t bus_dev_num;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    // .read    = device_read,
    // .write   = device_write,
    // .open    = device_open,
    // .release = device_release,
};

static int __init gpio_bus_init(void)
{
    int ret;
    int i, j;

    if (pin_count != 0 && pin_count != BUS_LENGTH) {
        printk(KERN_ERR "GPIO_bus: Wrong Pin Count: %d pins required\n", BUS_LENGTH);
        return -EINVAL;
    }

    ret = alloc_chrdev_region(&bus_dev_num, 0, 1, "GPIO_bus");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate device numbers\n");
        return ret;
    }

    cdev_init(&gpio_bus_dev.cdev, &fops);
    gpio_bus_dev.cdev.owner = THIS_MODULE;
    ret = cdev_add(&gpio_bus_dev.cdev, bus_dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(bus_dev_num, 1);
        printk(KERN_ERR "Failed to add cdev\n");
        return ret;
    }

    for (i = 0; i < BUS_LENGTH; i++) {
        ret = gpio_request(bus_pins[i], "GPIO_bus_pin");
        if (ret) {
            printk(KERN_ERR "Failed to request GPIO %d\n", bus_pins[i]);

            for (j = 0; j < i; j++) {
                gpiod_put(gpio_bus_dev.pins[j]);
                gpio_free(bus_pins[j]);
            }
            
            cdev_del(&gpio_bus_dev.cdev);
            unregister_chrdev_region(bus_dev_num, 1);
            return ret; 
        }

        gpio_bus_dev.pins[i] = gpio_to_desc(bus_pins[i]);
        gpiod_direction_output(gpio_bus_dev.pins[i], 0); 
    }

    printk(KERN_INFO "GPIO bus initialized using pins %d, %d, %d, %d\n", bus_pins[0], bus_pins[1], bus_pins[2], bus_pins[3]);
    return 0;
}

static void __exit gpio_bus_exit(void)
{
    int i;

    for (i = 0; i < BUS_LENGTH; i++) {
        gpiod_set_value(gpio_bus_dev.pins[i], 0); 
        gpiod_put(gpio_bus_dev.pins[i]);
        gpio_free(bus_pins[i]);
    }

    cdev_del(&gpio_bus_dev.cdev);
    unregister_chrdev_region(bus_dev_num, 1);

    printk(KERN_INFO "GPIO bus safely unloaded.\n");
}

module_init(gpio_bus_init);
module_exit(gpio_bus_exit);