# This is needed to enable silver art optimizer.
VENDOR_ART_PATH ?= $(INTEL_PATH_VENDOR)/art-extension
{{#config_dex2oat}}
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += dalvik.vm.dex2oat-flags={{{flags}}}
{{/config_dex2oat}}

PRODUCT_PACKAGES_TESTS += \
    art-run-tests \
    libarttest \
    libnativebridgetest \
    libart-gtest
