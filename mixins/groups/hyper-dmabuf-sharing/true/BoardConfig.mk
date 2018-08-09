# Enable NESTED DISPLAY SUPPORT IN HWC
ENABLE_NESTED_DISPLAY_SUPPORT := true

# sepolicy to access hyper_dmabuf device and tcp socket
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/hyper-dmabuf-sharing
