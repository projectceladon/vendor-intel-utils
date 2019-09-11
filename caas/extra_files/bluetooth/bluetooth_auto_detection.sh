#!/vendor/bin/sh

bt_server="com.android.bluetooth"
bt_server_enable=1

pm list packages -d | grep $bt_server
if [ $? == 0 ]; then
	bt_server_enable=0
fi

hciconfig | grep -q hci

if [ $? != 0 ]; then
	echo "no bluetooth device"
	if [ $bt_server_enable == 1 ]; then
		echo "disable bluetooth server"
		pm disable $bt_server
		echo "pm disable: $?"
	fi
else
	echo "bluetooth device exists"
	if [ $bt_server_enable == 0 ]; then
		echo "bluetooth server is disabled, re-enable it"
		pm enable $bt_server
		echo "pm enable ret: $?"
	fi
fi

