# Enable NESTED DISPLAY SUPPORT IN HWC
ENABLE_HYPER_DMABUF_SHARING := true

# sepolicy to access hyper_dmabuf device
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/hyper-dmabuf-sharing
