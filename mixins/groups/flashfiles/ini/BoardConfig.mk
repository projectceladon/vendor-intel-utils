FLASHFILES_CONFIG ?= $(TARGET_DEVICE_DIR)/flashfiles.ini
USE_INTEL_FLASHFILES := true
VARIANT_SPECIFIC_FLASHFILES ?= false
{{#fast_flashfiles}}
FAST_FLASHFILES := true
{{/fast_flashfiles}}
{{^fast_flashfiles}}
FAST_FLASHFILES := false
{{/fast_flashfiles}}
