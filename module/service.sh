#!/system/bin/sh
MODDIR=${0%/*}
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 1
done
# Hide root traces (Magisk/KernelSU) from Talabat
resetprop ro.debuggable 0
resetprop ro.secure 1
resetprop ro.build.tags release-keys
resetprop ro.build.type user
resetprop ro.boot.verifiedbootstate green
resetprop ro.boot.flash.locked 1
