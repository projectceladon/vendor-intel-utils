include $(CLEAR_VARS)
LOCAL_MODULE := mediaserver-radio
LOCAL_REQUIRED_MODULES := audioserver
LOCAL_POST_INSTALL_CMD := $(hide) sed -i 's/group audio [radio ]*/group audio radio /g' $(TARGET_OUT_ETC)/init/audioserver.rc
include $(BUILD_PHONY_PACKAGE)
