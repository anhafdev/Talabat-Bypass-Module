#!/system/bin/sh
MODDIR=${0%/*}
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 1
done
# Hide root properties from Talabat's detection
resetprop ro.debuggable 0
resetprop ro.secure 1
resetprop ro.build.tags release-keys
resetprop ro.build.type user
