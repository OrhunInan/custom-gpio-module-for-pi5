#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>   
#include <linux/cdev.h> 
#include <linux/uaccess.h>

#define BUS_LENGTH 4 // Defined default pin length as macro in case of future change 
#define DEFAULT_BUS_PINS {5, 6, 12, 13} // Defined default bus pins as macro in case of future change 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Orhun İnan");
MODULE_DESCRIPTION("4-bit parallel GPIO bus driver");

static int pin_count = 0;
static int bus_pins[BUS_LENGTH] = DEFAULT_BUS_PINS;
module_param_array(bus_pins, int, &pin_count, 0000);
MODULE_PARM_DESC(bus_pins, "Array of GPIO pins in use for bus connection.");

struct gpio_bus {
    struct cdev cdev;                 
    struct gpio_desc *pins[BUS_LENGTH];   
    int current_value;                 
};

static struct gpio_bus gpio_bus_dev;
static dev_t bus_dev_num;
static struct class *gpio_bus_class;
static struct device *gpio_bus_device;

static int gpio_bus_open(struct inode *inode, struct file *file) 
{
    struct gpio_bus *bus_ptr;

    bus_ptr = container_of(inode->i_cdev, struct gpio_bus, cdev);

    file->private_data = bus_ptr; 

    return 0;
}

static ssize_t gpio_bus_read(struct file *file, char __user *user_buffer, size_t count, loff_t *pos) 
{
    struct gpio_bus *bus_ptr = file->private_data;
    char kbuf[16];
    int len;

    // Make sure user can only read once per open function
    if (*pos > 0) {
        return 0; 
    }

    len = snprintf(kbuf, sizeof(kbuf), "%d\n", bus_ptr->current_value);

    // Set string length as count
    if (count < len) {
        len = count;
    }

    if (copy_to_user(user_buffer, kbuf, len)) {
        return -EFAULT;
    }

    *pos += len;

    return len; 
}

static int gpio_bus_release(struct inode *inode, struct file *file) 
{
    return 0;
}

static ssize_t gpio_bus_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *pos) 
{
    char kbuf[16] = {0};
    long user_value;
    int i;
    size_t bytes_to_copy;
    
    struct gpio_bus *bus_ptr = file->private_data;

    bytes_to_copy = min(count, sizeof(kbuf) - 1);

    // Copy input from userspace
    if (copy_from_user(kbuf, user_buffer, bytes_to_copy)) {
        return -EFAULT;
    }

    // Enforce integer input type
    if (kstrtol(kbuf, 10, &user_value) < 0) {
        printk(KERN_ERR "GPIO_bus: Invalid data written\n");
        return -EINVAL;
    }

    // Only take the first 4 bits of data
    user_value = user_value & 0xF;

    bus_ptr->current_value = user_value;

    // Write to GPIO pins
    for (i = 0; i < BUS_LENGTH; i++) {
        int bit_state = (user_value >> i) & 1; 
        
        gpiod_set_value(bus_ptr->pins[i], bit_state); 
    }

    //printk(KERN_INFO "gpio_bus: Successfully wrote %ld to hardware\n", user_value);

    return count; 
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read    = gpio_bus_read,
    .write   = gpio_bus_write,
    .open    = gpio_bus_open,
    .release = gpio_bus_release,
};    

static int __init gpio_bus_init(void)
{
    int ret;
    int i, j;

    // Enforce pin_count == BUS_LENGTH
    if (pin_count != 0 && pin_count != BUS_LENGTH) {
        printk(KERN_ERR "GPIO_bus: Wrong pin count %d pins required\n", BUS_LENGTH);
        return -EINVAL;
    }    

    // Create char device 
    ret = alloc_chrdev_region(&bus_dev_num, 0, 1, "GPIO_bus");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate device numbers\n");
        return ret;
    }    

    // Initialize VFS
    cdev_init(&gpio_bus_dev.cdev, &fops);
    gpio_bus_dev.cdev.owner = THIS_MODULE;
    ret = cdev_add(&gpio_bus_dev.cdev, bus_dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(bus_dev_num, 1);
        printk(KERN_ERR "Failed to add cdev\n");
        return ret;
    }    

    // Broadcast to udev
    gpio_bus_class = class_create("gpio_bus_class");
    if (IS_ERR(gpio_bus_class)) {
        cdev_del(&gpio_bus_dev.cdev);
        unregister_chrdev_region(bus_dev_num, 1);
        printk(KERN_ERR "Failed to create sysfs class\n");
        return PTR_ERR(gpio_bus_class);
    }

    // Create /dev/gpio_bus
    gpio_bus_device = device_create(gpio_bus_class, NULL, bus_dev_num, NULL, "gpio_bus");
    if (IS_ERR(gpio_bus_device)) {
        class_destroy(gpio_bus_class);
        cdev_del(&gpio_bus_dev.cdev);
        unregister_chrdev_region(bus_dev_num, 1);
        printk(KERN_ERR "Failed to create sysfs device\n");
        return PTR_ERR(gpio_bus_device);
    }

    // Lock requested GPIO pins and set pins to 0
    for (i = 0; i < BUS_LENGTH; i++) {
        ret = gpio_request(bus_pins[i], "GPIO_bus_pin");

        // If can't lock a pin release all acquired pins and remove VFS
        if (ret) { 
            printk(KERN_ERR "Failed to request GPIO %d\n", bus_pins[i]);

            for (j = 0; j < i; j++) {
                gpiod_put(gpio_bus_dev.pins[j]);
                gpio_free(bus_pins[j]);
            }    

            device_destroy(gpio_bus_class, bus_dev_num);
            class_destroy(gpio_bus_class);
            
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

    //Release all acquired pins
    for (i = 0; i < BUS_LENGTH; i++) {
        gpiod_set_value(gpio_bus_dev.pins[i], 0); 
        gpiod_put(gpio_bus_dev.pins[i]);
        gpio_free(bus_pins[i]);
    }   
    
    device_destroy(gpio_bus_class, bus_dev_num);
    class_destroy(gpio_bus_class);

    cdev_del(&gpio_bus_dev.cdev);
    unregister_chrdev_region(bus_dev_num, 1);

    printk(KERN_INFO "GPIO bus safely unloaded.\n");
}    


module_init(gpio_bus_init);
module_exit(gpio_bus_exit);