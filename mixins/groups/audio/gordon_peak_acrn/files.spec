[mapping]
libfwdt: audio/libfwdt
parameter-framework: audio/parameter-framework

[extrafiles]
asound.conf: "alsa-lib layer configuration file"
dirana_config.sh: "shell script for dirana configuration"
audio_policy_configuration{{policy_suffix}}.xml: "audio policy configuration file"
audio_policy_criteria{{policy_suffix}}.conf: "configurable audio policy criteria file"
audio_effects{{policy_suffix}}.xml: "audio effects file"
load_audio_modules.in : "load audio modules"
route_criteria.conf: "audio HAL configuration file"
early_audio_alsa.sh: "early audio script"
early_audio_alsa_avb.sh: "early audio avb script"
boot.wav: "early audio boot sound"

[devicefiles]
libfwdt: "audio firmware configuration"
parameter-framework: "parameter framework configuration files"
