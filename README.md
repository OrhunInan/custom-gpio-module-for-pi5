# Raspberry Pi 5 Parallel GPIO Bus Driver (`gpio_bus`)

An out-of-tree Linux kernel character driver designed to manage a 4-bit parallel GPIO bus on the Raspberry Pi 5. 

**Note:** This is a personal learning project created to explore modern Linux kernel module development, operating system architecture, and hardware interaction. It was built to understand the shift from traditional memory-mapped GPIO to the new PCIe-based RP1 southbridge architecture on the Raspberry Pi 5.

While developed as an educational exercise, the driver implements several standard architectural patterns to ensure stability and security.

## Key Learning Milestones Implemented
* **Dynamic Hardware Binding:** Passes specific GPIO pins at load-time via module parameters without needing to recompile the C code.
* **VFS Integration:** Reads and writes directly through `/dev/gpio_bus` using standard POSIX commands, bridging kernel space and user space.
* **Zero-Touch Node Creation:** Leverages Linux `sysfs` and `udev` to automatically create and destroy device nodes, moving away from manual `mknod` commands.
* **Defensive Input Handling:** Uses strict memory boundary checks (`copy_from_user`) and string-to-integer sanitization to prevent kernel panics from invalid inputs.

## Prerequisites
* **Hardware:** Raspberry Pi 5 
* **OS:** Linux (Tested on kernel 6.12.x)
* **Dependencies:** Kernel headers matching your current build.
  ```bash
  sudo apt install linux-headers-$(uname -r)
  ```

## Compilation
Build the Kernel Object (`.ko`) using the provided Makefile:
```bash
make
```

## Loading the Module (The RP1 Offset)
On the Raspberry Pi 5, GPIO pins are managed by the RP1 chip, which is dynamically assigned a block of pins by the kernel at boot. Raw physical pin numbers (e.g., 5) cannot be used directly. The RP1 base offset must be added.

1. Find the RP1 Base Offset:
```bash
sudo cat /sys/kernel/debug/gpio | grep "pinctrl-rp1" -B 1
```
(Look for the first number, e.g., 569-622. The base is 569.)


2. Calculate target pins:

To use physical pins 5, 6, 12, and 13, add the base offset to each (e.g., 569 + 5 = 574).

3. Inject the module:
```bash
sudo insmod gpio_bus.ko bus_pins=574,575,581,582
```
(If no pins are provided, it safely falls back to the default macro array)


## Security & Permissions (udev Rule)
This driver relies on udev to manage access policies. By default, the node is created as root-only. To allow a background service or specific user to control the bus securely:

1. Create a dedicated hardware group:
```bash
sudo groupadd gpio_bus_group
sudo usermod -aG gpio_bus_group $USER
newgrp gpio_bus_group
```

2. Install the udev rule:
Create a file at /etc/udev/rules.d/99-gpiobus.rules:
```
KERNEL=="gpio_bus", SUBSYSTEM=="gpio_bus_class", MODE="0660", GROUP="gpio_bus_group"
```

3. Reload udev:
```bash
sudo udevadm control --reload-rules
```
Re-insert the module, and /dev/gpio_bus will automatically mount with 0660 permissions owned by the custom group.


## Usage
Once loaded, interface with the hardware directly from the terminal or background scripts.

Write to the bus:
Send any integer (0-15). The driver will automatically translate it to binary and pull the respective pins HIGH/LOW.
```bash
echo "7" > /dev/gpio_bus
```

Read the current state:

```bash
cat /dev/gpio_bus
```

## Unloading the Driver
The teardown sequence safely zeroes out all physical pins, releases the hardware locks, destroys the sysfs class, and removes the VFS node before exiting.
```bash
sudo rmmod gpio_bus
```

## Author
**Orhun İnan** - Computer Engineering Student, Hacettepe University

License: GPL