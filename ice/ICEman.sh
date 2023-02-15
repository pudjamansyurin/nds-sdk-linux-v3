#!/bin/bash
chmod +x ICEman openocd
if [ "$?" != "0" ]; then
	echo -e "[FAILED] Unable to change execute permission to ICEman/openocd"
	exit 1
fi

#function func_yes {
#	rmmod ftdi_sio 2> /dev/null
#
#	if [ ! -f /etc/modprobe.d/blacklist-ftdi_sio ]; then
#		echo 'blacklist ftdi_sio' >> /etc/modprobe.d/blacklist-ftdi_sio 2> /dev/null
#	fi
#
#	if [ ! -f /etc/modprobe.d/blacklist-ftdi_sio.conf ]; then
#		echo 'blacklist ftdi_sio' >> /etc/modprobe.d/blacklist-ftdi_sio.conf 2> /dev/null
#	fi
#}
#
#function func_no {
#	echo "Please issue the following commands before executing ICEman: sudo rmmod ftdi_sio"
#}
#
#echo -e "The kernel module ftdi_sio will be removed and added to the modprob blacklist.\n\
#This will stop the ftdi_sio module from being installed when the FTDI USB serial device converter is detected and\n\
#ensure ICEman to execute correctly.\n\
#Are you sure you want to continue? (Please enter a number)"
#select yn in "Yes" "No"; do
#	case $yn in
#		Yes ) func_yes; break;;
#		No  ) func_no;  break;;
#		*   ) echo "Please type a number corresponding to your choice!"
#	esac
#done

RULES=70-ndsusb-v1.rules
SCRIPT=ftdi_script-v1.sh

if [ ! -f "/etc/udev/rules.d/$RULES" ]
then
	echo "copy $RULES to /etc/udev/rules.d/"
	cp -f ./$RULES /etc/udev/rules.d/

	if [ "$?" != "0" ]; then
		echo -e "[FAILED] Can't not copy udev rules, please check the permission"
		exit 1
	fi
fi

if [ ! -f "/etc/udev/rules.d/$SCRIPT" ]
then
	chmod +x ./$SCRIPT
	echo "copy $SCRIPT to /etc/udev/rules.d/"
	cp -f ./$SCRIPT /etc/udev/rules.d/

	if [ "$?" != "0" ]; then
		echo -e "[FAILED] Can't not copy udev rules script, please check the permission"
		exit 1
	fi
fi

echo -e "[SUCCEED] Please disconnect and reconnect the USB cable to reset AICE adapter!!!"

#For AndeSight query usb device list
chmod o+w /dev/bus/usb/*/001

echo "Done!"

