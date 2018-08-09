[mapping]
variants-{{target}}.mk: variants.mk
device_mapping-{{target}}.py: device_mapping.py

[devicefiles]:
variants-{{target}}.mk: "variants cfg file"
device_mapping-{{target}}.py: "variants device mapping file"

