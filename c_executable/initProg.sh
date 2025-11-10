#!/bin/sh
	
#hearbeat
/chargerFiles/som_heartbeat &
echo "heart beating start"
echo -e "\033[1;32m (3/7)Heartbeat Started\033[0m"

#ZRAM as swap
echo "1" > /sys/block/zram0/reset
echo $(( 150 * 1024 * 1024 )) >"/sys/block/zram0/disksize"
mkswap /dev/zram0
swapon /dev/zram0
echo -e "\033[1;32m (4/7)Swap Started\033[0m"

#boot count record
dir="/nboot"
nboot=`cat $dir`
echo "Number of restarts: "$nboot
nboot=$((nboot+1))
echo "$nboot" > $dir
echo -e "\033[1;32m (5/7)Number of boots recorded\033[0m"


#gpio intr
insmod /chargerFiles/modules/gpio_irq.ko
echo -e "\033[1;32m (6/7)GPIO kernel module added\033[0m"

#Load balancer
chmod +x /chargerFiles/lb_select.sh
/chargerFiles/lb_select.sh &



# Base Program
chargerFiles/mcu_reset.sh
sleep 1                         
forever -s start /chargerFiles/L2/base.js
echo -e "\033[1;32m (7/7)Base code initialized\033[0m"

date
echo -e "\033[0;32m (7/7)Second init script completed \033[0m"


