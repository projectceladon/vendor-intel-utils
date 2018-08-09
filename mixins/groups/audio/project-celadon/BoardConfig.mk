BOARD_USES_ALSA_AUDIO := true
BOARD_USES_TINY_ALSA_AUDIO := true
BOARD_USES_GENERIC_AUDIO ?= false
USE_CUSTOM_PARAMETER_FRAMEWORK := false
ifneq ($(BOARD_USES_GENERIC_AUDIO), true)
# Audio HAL selection Flag default setting.
#  INTEL_AUDIO_HAL:= audio     -> baseline HAL
#  INTEL_AUDIO_HAL:= audio_pfw -> PFW-based HAL
INTEL_AUDIO_HAL := audio
else
INTEL_AUDIO_HAL := stub
endif

# Use XML audio policy configuration file
USE_XML_AUDIO_POLICY_CONF ?= 1
# Use configurable audio policy
USE_CONFIGURABLE_AUDIO_POLICY ?= 1
