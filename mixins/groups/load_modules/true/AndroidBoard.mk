include $(CLEAR_VARS)
LOCAL_MODULE := load_modules.sh
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC := $(LOAD_MODULES_H_IN) $(LOAD_MODULES_IN)
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): $(LOCAL_SRC)
	$(hide) mkdir -p "$(dir $@)"
{{^treble}}
	echo "#!/system/bin/sh" > $@
{{/treble}}
{{#treble}}
	echo "#!/vendor/bin/sh" > $@
{{/treble}}
	echo "modules=\`getprop ro.vendor.boot.moduleslocation\`" >> $@
	cat $(LOAD_MODULES_H_IN) >> $@
	echo wait >> $@
	cat $(LOAD_MODULES_IN) >> $@
