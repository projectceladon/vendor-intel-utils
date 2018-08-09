LK_ELF := $(TRUSTY_BUILDROOT)build-{{{lk_project}}}/lk.elf

SIGNING_KEY := $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_VERITY_SIGNING_KEY).pk8
SIGNING_CERT := $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_VERITY_SIGNING_KEY).x509.pem

INSTALLED_TOS_IMAGE_TARGET := $(PRODUCT_OUT)/tos.img

tosimage: $(INSTALLED_TOS_IMAGE_TARGET)

ifeq ($(wildcard $(TOS_PREBUILT)), )
ifeq (true,$(BOARD_AVB_ENABLE)) # BOARD_AVB_ENABLE == true
$(INSTALLED_TOS_IMAGE_TARGET): lk $(MKBOOTIMG) $(AVBTOOL)
	@echo "mkbootimg to create boot image for TOS file: $@"
	$(hide) $(MKBOOTIMG) --kernel $(LK_ELF) --output $@
	@echo "$(AVBTOOL): add hashfooter to TOS file: $@"
	$(hide) $(AVBTOOL) add_hash_footer \
		--image $@ \
		--partition_size $(BOARD_TOSIMAGE_PARTITION_SIZE) \
		--partition_name tos $(INTERNAL_AVB_SIGNING_ARGS)
INSTALLED_VBMETAIMAGE_TARGET ?= $(PRODUCT_OUT)/vbmeta.img
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --include_descriptors_from_image $(INSTALLED_TOS_IMAGE_TARGET)
$(INSTALLED_VBMETAIMAGE_TARGET): $(INSTALLED_TOS_IMAGE_TARGET)
else
$(INSTALLED_TOS_IMAGE_TARGET): lk $(MKBOOTIMG) $(BOOT_SIGNER)
	@echo "mkbootimg to create boot image for TOS file: $@"
	$(hide) $(MKBOOTIMG) --kernel $(LK_ELF) --output $@
	$(if $(filter true,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_SUPPORTS_BOOT_SIGNER)),\
		@echo "$(BOOT_SIGNER): sign prebuilt TOS file: $@" &&\
		$(BOOT_SIGNER) /tos $@ $(SIGNING_KEY) $(SIGNING_CERT) $@)
endif
else
$(INSTALLED_TOS_IMAGE_TARGET):
	@echo "Use prebuilt tos image $(TOS_PREBUILT)"
	$(hide) (cp $(TOS_PREBUILT) $(INSTALLED_TOS_IMAGE_TARGET))

ifeq (true,$(BOARD_AVB_ENABLE)) # BOARD_AVB_ENABLE == true
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --include_descriptors_from_image $(INSTALLED_TOS_IMAGE_TARGET)
endif

endif

INSTALLED_RADIOIMAGE_TARGET += $(INSTALLED_TOS_IMAGE_TARGET)
