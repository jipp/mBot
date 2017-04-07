# create /dev/rfcomm0 for bluetooth paired device (mac address needed)

#!/bin/sh

sudo rfcomm bind /dev/rfcomm0 00:1B:10:10:1B:E4
