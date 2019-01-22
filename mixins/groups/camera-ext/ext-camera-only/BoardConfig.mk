# Enable only USB camera and disable all CSI Cameras
BOARD_CAMERA_USB_STANDALONE = true

# SELinux support for USB camera
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/camera-ext/ext-camera-only
