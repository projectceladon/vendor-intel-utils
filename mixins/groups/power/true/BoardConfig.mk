{{#app_launch_boost}}
APP_LAUNCH_BOOST := true
{{/app_launch_boost}}

{{#power_throttle}}
POWER_THROTTLE := true
{{/power_throttle}}

{{#has_thd}}
HAS_THD := true
{{/has_thd}}

BOARD_SEPOLICY_M4DEFS += module_power=true
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/power
