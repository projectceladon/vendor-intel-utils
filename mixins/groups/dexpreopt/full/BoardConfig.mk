# Enable dex-preoptimization to speed up the first boot sequence
# Note that this operation only works on Linux for now
# Enable for non-eng builds
ifneq ($(TARGET_BUILD_VARIANT),eng)
WITH_DEXPREOPT := true
endif

