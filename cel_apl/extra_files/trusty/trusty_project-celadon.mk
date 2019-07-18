INSTALLED_TOS_IMAGE_TARGET := $(PRODUCT_OUT)/tos.img
INSTALLED_MULTIBOOT_IMAGE_TARGET := none
TOS_SIGNING_KEY := $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_VERITY_SIGNING_KEY).pk8
TOS_SIGNING_CERT := $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_VERITY_SIGNING_KEY).x509.pem

tosimage: $(INSTALLED_TOS_IMAGE_TARGET)

ifeq ($(INTEL_PREBUILT), true)
$(INSTALLED_TOS_IMAGE_TARGET):
	$(hide) (cp $(INTEL_PATH_PREBUILTS)/tos.img $(INSTALLED_TOS_IMAGE_TARGET))
else
ifeq ($(wildcard $(TOS_PREBUILT)), )
ifeq (true,$(BOARD_AVB_ENABLE))
$(INSTALLED_TOS_IMAGE_TARGET): $(LK_ELF) $(EVMM_LK_PKG) $(MKBOOTIMG) $(AVBTOOL)
	@echo "mkbootimg to create boot image for TOS file: $@"
	$(hide) $(MKBOOTIMG) --kernel $(EVMM_LK_PKG) --output $@
	@echo "$(AVBTOOL): add hashfooter to TOS file: $@"
	$(hide) $(AVBTOOL) add_hash_footer \
		--image $@ \
		--partition_size $(BOARD_TOSIMAGE_PARTITION_SIZE) \
		--partition_name tos
	$(hide) mkdir -p $(INTEL_PATH_PREBUILTS_OUT)
	$(hide) (cp $@ $(INTEL_PATH_PREBUILTS_OUT))
else # BOARD_AVB_ENABLE == false
$(INSTALLED_TOS_IMAGE_TARGET): $(LK_ELF) $(EVMM_LK_PKG) $(MKBOOTIMG) $(BOOT_SIGNER)
	@echo "mkbootimg to create boot image for TOS file: $@"
	$(hide) $(MKBOOTIMG) --kernel $(EVMM_LK_PKG) --output $@
	$(if $(filter true,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_SUPPORTS_BOOT_SIGNER)),\
		@echo "$(BOOT_SIGNER): sign prebuilt TOS file: $@" &&\
		$(BOOT_SIGNER) /tos $@ $(TOS_SIGNING_KEY) $(TOS_SIGNING_CERT) $@)
	$(hide) mkdir -p $(INTEL_PATH_PREBUILTS_OUT)
	$(hide) (cp $@ $(INTEL_PATH_PREBUILTS_OUT))
endif # BOARD_AVB_ENABLE
else  # TOS_PREBUILT == true
$(INSTALLED_TOS_IMAGE_TARGET):
	@echo "Use prebuilt tos image $(TOS_PREBUILT)"
	$(hide) (cp $(TOS_PREBUILT) $(INSTALLED_TOS_IMAGE_TARGET))
	$(hide) mkdir -p $(INTEL_PATH_PREBUILTS_OUT)
	$(hide) (cp $@ $(INTEL_PATH_PREBUILTS_OUT))
endif # TOS_PREBUILT
endif # INTEL_PREBUILT

ifeq (true,$(BOARD_AVB_ENABLE))
INSTALLED_VBMETAIMAGE_TARGET ?= $(PRODUCT_OUT)/vbmeta.img
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --include_descriptors_from_image $(INSTALLED_TOS_IMAGE_TARGET)
$(INSTALLED_VBMETAIMAGE_TARGET): $(INSTALLED_TOS_IMAGE_TARGET)
endif

INSTALLED_RADIOIMAGE_TARGET += $(INSTALLED_TOS_IMAGE_TARGET)
