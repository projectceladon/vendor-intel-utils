KERNEL_VSBL_DIFFCONFIG = $(wildcard $(KERNEL_CONFIG_PATH)/vsbl_diffconfig)
KERNEL_DIFFCONFIG += $(KERNEL_VSBL_DIFFCONFIG)

OPTIONIMAGE_BIN = $(TARGET_DEVICE_DIR)/boot_option.bin

# Partition table configuration file
BOARD_GPT_INI ?= $(TARGET_DEVICE_DIR)/gpt.ini
BOARD_GPT_BIN = $(PRODUCT_OUT)/gpt.bin
GPT_INI2BIN := ./$(INTEL_PATH_COMMON)/gpt_bin/gpt_ini2bin.py
BOARD_BOOTLOADER_PARTITION_SIZE_KILO := $$(({{bootloader_len}} * 1024))

$(BOARD_GPT_BIN): $(BOARD_GPT_INI)
	$(hide) $(GPT_INI2BIN) $< > $@
	$(hide) echo GEN $(notdir $@)

BOARD_FLASHFILES += $(BOARD_GPT_BIN):gpt.bin

# REVERT-ME create GPT IMG for ELK installer script
BOARD_GPT_IMG = $(PRODUCT_OUT)/gpt.img
BOARD_FLASHFILES += $(BOARD_GPT_IMG):gpt.img

GPT_INI2IMG := ./$(INTEL_PATH_BUILD)/create_gpt_image.py

intermediate_img := $(call intermediates-dir-for,PACKAGING,flashfiles)/gpt.img

$(BOARD_GPT_IMG): $(BOARD_GPT_INI)
	$(hide) mkdir -p $(dir $(intermediate_img))
	$(hide) $(GPT_INI2IMG) --create --table $< --size $(gptimage_size) $(intermediate_img)
	$(hide) dd if=$(intermediate_img) of=$@ bs=512 count=34
	$(hide) echo GEN $(notdir $@)

# (pulled from build/core/Makefile as this gets defined much later)
# Pick a reasonable string to use to identify files.
ifneq "" "$(filter eng.%,$(BUILD_NUMBER))"
# BUILD_NUMBER has a timestamp in it, which means that
# it will change every time.  Pick a stable value.
FILE_NAME_TAG := eng.$(USER)
else
FILE_NAME_TAG := $(BUILD_NUMBER)
endif

bootloader_bin := $(PRODUCT_OUT)/bootloader
BOARD_BOOTLOADER_DEFAULT_IMG := $(PRODUCT_OUT)/bootloader.img
BOARD_BOOTLOADER_DIR := $(PRODUCT_OUT)/abl
BOARD_BOOTLOADER_IASIMAGE := $(BOARD_BOOTLOADER_DIR)/kf4abl.abl
BOARD_BOOTLOADER_VAR_IMG := $(BOARD_BOOTLOADER_DIR)/bootloader.img
INSTALLED_RADIOIMAGE_TARGET += $(BOARD_BOOTLOADER_DEFAULT_IMG)

define add_board_flashfiles_variant
$(eval BOARD_FLASHFILES_FIRMWARE_$(1) += $(BOARD_BOOTLOADER_DEFAULT_IMG):bootloader) \
$(eval BOARD_FLASHFILES_FIRMWARE_$(1) += $(TARGET_DEVICE_DIR)/{{_extra_dir}}/fftf_build.opt:fftf_build.opt) \
$(eval BOARD_FLASHFILES_FIRMWARE_$(1) += $(BOARD_BOOTLOADER_IASIMAGE):fastboot)
endef

SBL_AVAILABLE_CONFIG := $(ROOT_VARIANTS)
$(foreach config,$(SBL_AVAILABLE_CONFIG),$(call add_board_flashfiles_variant,$(config)))

$(call flashfile_add_blob,capsule.fv,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(IFWI_VARIANT)/capsule.fv,,BOARD_SFU_UPDATE)
$(call flashfile_add_blob,ifwi.bin,$(INTEL_PATH_HARDWARE)/fw_capsules/{{target}}/::variant::/$(IFWI_VARIANT)/ifwi.bin,,EFI_IFWI_BIN)

ifneq ($(EFI_IFWI_BIN),)
$(call dist-for-goals,droidcore,$(EFI_IFWI_BIN):$(TARGET_PRODUCT)-ifwi-$(FILE_NAME_TAG).bin)
endif

ifneq ($(BOARD_SFU_UPDATE),)
$(call dist-for-goals,droidcore,$(BOARD_SFU_UPDATE):$(TARGET_PRODUCT)-sfu-$(FILE_NAME_TAG).fv)
endif

ifeq ($(wildcard $(EFI_IFWI_BIN)),)
$(warning ##### EFI_IFWI_BIN not found, IFWI binary will not be provided in out/dist/)
endif

BOARD_FLASHFILES += $(BOARD_BOOTLOADER_DEFAULT_IMG):bootloader

$(BOARD_BOOTLOADER_DIR):
	$(hide) rm -rf $(BOARD_BOOTLOADER_DIR)
	$(hide) mkdir -p $(BOARD_BOOTLOADER_DIR)

ifneq ($(BOARD_BOOTLOADER_PARTITION_SIZE),0)
define generate_bootloader_var
rm -f $(BOARD_BOOTLOADER_VAR_IMG)
$(hide) if [ -e "$(BOARD_SFU_UPDATE)" ]; then \
    dd of=$(BOARD_BOOTLOADER_VAR_IMG) if=$(BOARD_SFU_UPDATE) bs=1024; \
else \
    dd of=$(BOARD_BOOTLOADER_VAR_IMG) if=/dev/zero bs=1024 count=16384; \
fi
$(hide) dd of=$(BOARD_BOOTLOADER_VAR_IMG) if=$(BOARD_BOOTLOADER_IASIMAGE) bs=1024 seek=16384
cp $(BOARD_BOOTLOADER_VAR_IMG) $(BOARD_BOOTLOADER_DEFAULT_IMG)
cp $(BOARD_BOOTLOADER_VAR_IMG) $(bootloader_bin)
echo "Bootloader image successfully generated $(BOARD_BOOTLOADER_VAR_IMG)"
endef

fastboot_image: fb4abl-$(TARGET_BUILD_VARIANT)
bootloader: $(BOARD_BOOTLOADER_DIR) mkext2img kf4abl-$(TARGET_BUILD_VARIANT)
	$(call generate_bootloader_var,$(config))
else
bootloader: $(BOARD_BOOTLOADER_DIR)
	$(ACP) -f $(ABL_PREBUILT_PATH)/bldr_utils.img $(BOARD_BOOTLOADER_DEFAULT_IMG)
	$(foreach config,$(ABL_AVAILABLE_CONFIG),cp -v $(BOARD_BOOTLOADER_DEFAULT_IMG) $(BOARD_BOOTLOADER_DIR)/$(config)/)
endif

$(BOARD_BOOTLOADER_DIR)/%/bootloader.img: bootloader
	@echo "Generate bootloader: $@ finished."

$(BOARD_BOOTLOADER_DEFAULT_IMG): bootloader
	@echo "Generate default bootloader: $@ finished."
droidcore: bootloader

$(bootloader_bin): bootloader

.PHONY: bootloader
