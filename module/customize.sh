SKIPUNZIP=0
ui_print "- Installing Audio Enhancer Service"

if [ -d "$MODPATH/lib/arm64-v8a" ]; then
    mkdir -p "$MODPATH/zygisk"
    cp -f "$MODPATH/lib/arm64-v8a/libzygisk.so" "$MODPATH/zygisk/arm64-v8a.so"
    rm -rf "$MODPATH/lib"
else
    ui_print "! Error: libzygisk.so not found!"
fi

set_perm_recursive $MODPATH 0 0 0755 0644
set_perm $MODPATH/service.sh 0 0 0755
set_perm $MODPATH/customize.sh 0 0 0755

ui_print "- Installation complete!"
