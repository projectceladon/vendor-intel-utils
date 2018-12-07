# Rules to create bootloader zip file, a precursor to the bootloader
# image that is stored in the target-files-package. There's also
# metadata file which indicates how large to make the VFAT filesystem
# image

ifeq ($(TARGET_UEFI_ARCH),i386)
efi_default_name := bootia32.efi
LOADER_TYPE := linux-x86
else
efi_default_name := bootx64.efi
LOADER_TYPE := linux-x86_64
endif

# (pulled from build/core/Makefile as this gets defined much later)
# Pick a reasonable string to use to identify files.
ifneq "" "$(filter eng.%,$(BUILD_NUMBER))"
# BUILD_NUMBER has a timestamp in it, which means that
# it will change every time.  Pick a stable value.
FILE_NAME_TAG := eng.$(USER)
else
FILE_NAME_TAG := $(BUILD_NUMBER)
endif

BOARD_FIRST_STAGE_LOADER := $(PRODUCT_OUT)/efi/kernelflinger.efi
BOARD_EXTRA_EFI_MODULES :=

$(call flashfile_add_blob,capsule.fv,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(BIOS_VARIANT)/capsule.fv,,BOARD_SFU_UPDATE)
$(call flashfile_add_blob,ifwi.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(BIOS_VARIANT)/ifwi.bin,,EFI_IFWI_BIN)
$(call flashfile_add_blob,ifwi_dnx.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(BIOS_VARIANT)/ifwi_dnx.bin,,EFI_IFWI_DNX_BIN)
$(call flashfile_add_blob,firmware.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(BIOS_VARIANT)/emmc.bin,,EFI_EMMC_BIN)
$(call flashfile_add_blob,afu.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(BIOS_VARIANT)/afu.bin,,EFI_AFU_BIN)
$(call flashfile_add_blob,dnxp_0x1.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(BIOS_VARIANT)/dnxp_0x1.bin,,DNXP_BIN)
$(call flashfile_add_blob,cfgpart.xml,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(BIOS_VARIANT)/cfgpart.xml,,CFGPART_XML)
$(call flashfile_add_blob,cse_spi.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(BIOS_VARIANT)/cse_spi.bin,,CSE_SPI_BIN)

{{#ifwi_debug}}
ifneq ($(TARGET_BUILD_VARIANT),user)
# Allow to add debug ifwi file only on userdebug and eng flashfiles
$(call flashfile_add_blob,ifwi_debug.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/debug/ifwi.bin,,EFI_IFWI_DEBUG_BIN)
$(call flashfile_add_blob,ifwi_debug_dnx.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/debug/ifwi_dnx.bin,,EFI_IFWI_DEBUG_DNX_BIN)
endif
{{/ifwi_debug}}

ifneq ($(EFI_IFWI_BIN),)
$(call dist-for-goals,droidcore,$(EFI_IFWI_BIN):$(TARGET_PRODUCT)-ifwi-$(FILE_NAME_TAG).bin)
endif

ifneq ($(EFI_IFWI_DNX_BIN),)
$(call dist-for-goals,droidcore,$(EFI_IFWI_DNX_BIN):$(TARGET_PRODUCT)-ifwi_dnx-$(FILE_NAME_TAG).bin)
endif

ifneq ($(EFI_AFU_BIN),)
$(call dist-for-goals,droidcore,$(EFI_AFU_BIN):$(TARGET_PRODUCT)-afu-$(FILE_NAME_TAG).bin)
endif

ifneq ($(BOARD_SFU_UPDATE),)
$(call dist-for-goals,droidcore,$(BOARD_SFU_UPDATE):$(TARGET_PRODUCT)-sfu-$(FILE_NAME_TAG).fv)
endif

ifneq ($(CALLED_FROM_SETUP),true)
ifeq ($(wildcard $(BOARD_SFU_UPDATE)),)
$(warning BOARD_SFU_UPDATE not found, OTA updates will not provide a firmware capsule)
endif
ifeq ($(wildcard $(EFI_EMMC_BIN)),)
$(warning EFI_EMMC_BIN not found, flashfiles will not include 2nd stage EMMC firmware)
endif
ifeq ($(wildcard $(EFI_IFWI_BIN)),)
$(warning EFI_IFWI_BIN not found, IFWI binary will not be provided in out/dist/)
endif
ifeq ($(wildcard $(EFI_AFU_BIN)),)
$(warning EFI_AFU_BIN not found, AFU IFWI binary will not be provided in out/dist/)
endif
endif

# We stash a copy of BIOSUPDATE.fv so the FW sees it, applies the
# update, and deletes the file. Follows Google's desire to update all
# bootloader pieces with a single "fastboot flash bootloader" command.
# Since it gets deleted we can't do incremental updates of it, we keep
# a copy as capsules/current.fv for this purpose.
intermediates := $(call intermediates-dir-for,PACKAGING,bootloader_zip)
bootloader_zip := $(intermediates)/bootloader.zip
$(bootloader_zip): intermediates := $(intermediates)
$(bootloader_zip): efi_root := $(intermediates)/root
$(bootloader_zip): \
		$(TARGET_DEVICE_DIR)/AndroidBoard.mk \
		$(BOARD_FIRST_STAGE_LOADER) \
		$(BOARD_EXTRA_EFI_MODULES) \
		$(BOARD_SFU_UPDATE) \
		| $(ACP) \

	$(hide) rm -rf $(efi_root)
	$(hide) rm -f $@
	$(hide) mkdir -p $(efi_root)/capsules
	$(hide) mkdir -p $(efi_root)/EFI/BOOT
	$(foreach EXTRA,$(BOARD_EXTRA_EFI_MODULES), \
		$(hide) $(ACP) $(EXTRA) $(efi_root)/)
ifneq ($(BOARD_SFU_UPDATE),)
	$(hide) $(ACP) $(BOARD_SFU_UPDATE) $(efi_root)/BIOSUPDATE.fv
	$(hide) $(ACP) $(BOARD_SFU_UPDATE) $(efi_root)/capsules/current.fv
endif
	$(hide) $(ACP) $(BOARD_FIRST_STAGE_LOADER) $(efi_root)/loader.efi
	$(hide) $(ACP) $(BOARD_FIRST_STAGE_LOADER) $(efi_root)/EFI/BOOT/$(efi_default_name)
	$(hide) echo "Android-IA=\\EFI\\BOOT\\$(efi_default_name)" > $(efi_root)/manifest.txt
	$(hide) (cd $(efi_root) && zip -qry ../$(notdir $@) .)

bootloader_info := $(intermediates)/bootloader_image_info.txt
$(bootloader_info):
	$(hide) mkdir -p $(dir $@)
	$(hide) echo "size=$(BOARD_BOOTLOADER_PARTITION_SIZE)" > $@
	$(hide) echo "block_size=$(BOARD_BOOTLOADER_BLOCK_SIZE)" >> $@

INSTALLED_RADIOIMAGE_TARGET += $(bootloader_zip) $(bootloader_info)

# Rule to create $(OUT)/bootloader image, binaries within are signed with
# testing keys

bootloader_bin := $(PRODUCT_OUT)/bootloader
$(bootloader_bin): \
		$(bootloader_zip) \
		$(IMG2SIMG) \
		$(BOOTLOADER_ADDITIONAL_DEPS) \
		$(INTEL_PATH_BUILD)/bootloader_from_zip \

	$(hide) $(INTEL_PATH_BUILD)/bootloader_from_zip \
		--size $(BOARD_BOOTLOADER_PARTITION_SIZE) \
		--block-size $(BOARD_BOOTLOADER_BLOCK_SIZE) \
		$(BOOTLOADER_ADDITIONAL_ARGS) \
		--zipfile $(bootloader_zip) \
		$@

droidcore: $(bootloader_bin)

.PHONY: bootloader
bootloader: $(bootloader_bin)
$(call dist-for-goals,droidcore,$(bootloader_bin):$(TARGET_PRODUCT)-bootloader-$(FILE_NAME_TAG))

$(call dist-for-goals,droidcore,$(INTEL_PATH_BUILD)/testkeys/testkeys_lockdown.txt:test-keys_efi_lockdown.txt)
$(call dist-for-goals,droidcore,$(INTEL_PATH_BUILD)/testkeys/unlock.txt:efi_unlock.txt)

{{#bootloader_policy}}
{{#blpolicy_use_efi_var}}
ifeq ($(TARGET_BOOTLOADER_POLICY),$(filter $(TARGET_BOOTLOADER_POLICY),static external))
# The bootloader policy is not built but is provided statically in the
# repository or in $(PRODUCT_OUT)/.
else
# Bootloader policy values are generated based on the
# TARGET_BOOTLOADER_POLICY value and the
# $(INTEL_PATH_BUILD)/testkeys/{odm,OAK} keys.  The OEM must provide
# its own keys.
GEN_BLPOLICY_OEMVARS := $(INTEL_PATH_BUILD)/generate_blpolicy_oemvars
TARGET_ODM_KEY_PAIR := $(INTEL_PATH_BUILD)/testkeys/odm
TARGET_OAK_KEY_PAIR := $(INTEL_PATH_BUILD)/testkeys/OAK

$(BOOTLOADER_POLICY_OEMVARS):
	$(GEN_BLPOLICY_OEMVARS) -K $(TARGET_ODM_KEY_PAIR) \
		-O $(TARGET_OAK_KEY_PAIR).x509.pem -B $(TARGET_BOOTLOADER_POLICY) \
		$(BOOTLOADER_POLICY_OEMVARS)
endif
{{/blpolicy_use_efi_var}}
{{/bootloader_policy}}

{{#slot-ab}}
# Used by updater_ab_esp
$(PRODUCT_OUT)/vendor.img: $(PRODUCT_OUT)/vendor/firmware/kernelflinger.efi
$(PRODUCT_OUT)/vendor/firmware/kernelflinger.efi: $(PRODUCT_OUT)/efi/kernelflinger.efi
	$(ACP) $(PRODUCT_OUT)/efi/kernelflinger.efi $@

make_bootloader_dir:
	@mkdir -p $(PRODUCT_OUT)/root/bootloader

$(PRODUCT_OUT)/ramdisk.img: make_bootloader_dir
{{/slot-ab}}
