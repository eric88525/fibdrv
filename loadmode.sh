sudo insmod ./fibdrv.ko
ls -l /dev/fibonacci
cat /sys/class/fibonacci/fibonacci/dev
cat /sys/module/fibdrv/version 
lsmod | grep fibdrv
cat /sys/module/fibdrv/refcnt