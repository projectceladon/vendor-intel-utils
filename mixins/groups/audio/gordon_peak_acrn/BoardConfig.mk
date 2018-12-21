DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_COMMON)/audio/overlay

#Enable SOF
SOF_AUDIO := false

# Enable configurable audio policy
USE_CONFIGURABLE_AUDIO_POLICY := 1

# Use XML audio policy configuration file
USE_XML_AUDIO_POLICY_CONF := 1

# Use Intel's custom PFW
USE_CUSTOM_PARAMETER_FRAMEWORK := true

BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/audio/coe-common

{{#treble}}
# Do not use audio HAL directly w/o hwbinder middleware
USE_LEGACY_LOCAL_AUDIO_HAL := false
{{/treble}}
{{^treble}}
# Use audio HAL directly w/o hwbinder middleware
USE_LEGACY_LOCAL_AUDIO_HAL := true
{{/treble}}
