# Those 3 lines are required to enable vendor image generation.
# Remove them if vendor partition is not used.
TARGET_COPY_OUT_VENDOR := vendor
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := {{system_fs}}
BOARD_VENDORIMAGE_PARTITION_SIZE := $(shell echo {{partition_size}}*1048576 | bc)
{{#slot-ab}}
AB_OTA_PARTITIONS += vendor
{{/slot-ab}}
