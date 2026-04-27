savedcmd_/home/orhun/kernel_module/gpio_bus.mod := printf '%s\n'   gpio_bus.o | awk '!x[$$0]++ { print("/home/orhun/kernel_module/"$$0) }' > /home/orhun/kernel_module/gpio_bus.mod
