######################################################################
# Define Specific Kernel Config for ACRN
######################################################################
KERNEL_ACRN_GUEST_DIFFCONFIG = $(wildcard $(KERNEL_CONFIG_PATH)/acrn_guest_diffconfig)
KERNEL_DIFFCONFIG += $(KERNEL_ACRN_GUEST_DIFFCONFIG)

######################################################################
# Acrn Flashfiles Contains Below Image Files
######################################################################
ACRN_FLASH_JSON := flash_AaaG.json
ACRN_IFWI_FW := ifwi.bin
ACRN_IFWI_DNX := ifwi_dnx.bin
ACRN_IFWI_DNXP := dnxp_0x1.bin
ACRN_IFWI_FV := capsule.fv
ACRN_FW_VERSION := fwversion.txt
ACRN_IOC_FW_D := ioc_firmware_gp_mrb_fab_d.ias_ioc
ACRN_IOC_FW_E := ioc_firmware_gp_mrb_fab_e.ias_ioc
ACRN_SOS_BOOT_IMAGE := sos_boot.img
ACRN_SOS_ROOTFS_IMAGE := sos_rootfs.img
ACRN_PARTITION_DESC_BIN := partition_desc.bin
ACRN_MD5SUM_MD5 = md5sum.md5

######################################################################
# Define The Script Path and ACRN Related Files
######################################################################
ACRN_TMP_DIR := $(PRODUCT_OUT)/acrn_fls
ACRN_GETLINK_SCRIPT := $(TARGET_DEVICE_DIR)/{{_extra_dir}}/getlink.py
ACRN_VERSION_CONFIG := $(TARGET_DEVICE_DIR)/{{_extra_dir}}/acrnversion.cfg
FLASH_JSON_CONFIG := $(TARGET_DEVICE_DIR)/{{_extra_dir}}/flash.json
PRE_BUILD_BLD := $(TARGET_DEVICE_DIR)/{{_extra_dir}}/bootloader_gr_mrb_b1
PRE_BUILD_GPT := $(TARGET_DEVICE_DIR)/{{_extra_dir}}/gpt_gr_mrb_b1.bin
LOCAL_SOS_PATH = $(TARGET_DEVICE_DIR)/acrn_sos
ACRN_EXT4_BIN = $(PRODUCT_OUT)/$(TARGET_PRODUCT)_AaaG.img
ACRN_EXT4_BIN_ZIP = $(PRODUCT_OUT)/$(TARGET_PRODUCT)_AaaG.zip
PUBLISH_DEST := $(TOP)/pub/$(TARGET_PRODUCT)/$(TARGET_BUILD_VARIANT)
ifeq ($(strip $(BUILD_NUMBER)),)
ACRN_FLASHFILES = $(PRODUCT_OUT)/$(TARGET_PRODUCT)-flashfiles.zip
GUEST_FLASHFILES = $(PRODUCT_OUT)/$(TARGET_PRODUCT)-guest.zip
else
ACRN_FLASHFILES = $(PRODUCT_OUT)/$(TARGET_PRODUCT)-flashfiles-$(BUILD_NUMBER).zip
GUEST_FLASHFILES = $(PRODUCT_OUT)/$(TARGET_PRODUCT)-guest-$(BUILD_NUMBER).zip
endif

######################################################################
# Get SOS Link and Version By getlink.py
######################################################################
SOS_LINK_CFG := $(shell sed -n '{/^#/!p}' $(ACRN_VERSION_CONFIG) | grep 'SOS_LINK' | sed 's/^.*=//g' | sed 's/ //g')
SOS_VERSION_CFG := $(shell sed -n '{/^#/!p}' $(ACRN_VERSION_CONFIG) | grep 'SOS_VERSION' | sed 's/^.*=//g' | sed 's/ //g')

ifeq ($(strip $(SOS_VERSION)),)
    SOS_VERSION = ""
    ifeq ($(strip $(SOS_VERSION_CFG)), 'latest')
        ACRN_LINK := $(word 2,$(strip $(shell python $(ACRN_GETLINK_SCRIPT) $(ACRN_VERSION_CONFIG) $(SOS_VERSION))))
    else
        ACRN_LINK := $(SOS_LINK_CFG)/$(SOS_VERSION_CFG)/pub
    endif
else
    SOS_VERSION_CFG := $(SOS_VERSION)
    ACRN_LINK := $(word 2,$(strip $(shell python $(ACRN_GETLINK_SCRIPT) $(ACRN_VERSION_CONFIG) $(SOS_VERSION))))
endif

######################################################################
# Download files from the link, $2 was the link, $1 was
# the download file, $3 was the dir
######################################################################
ARIA2C := $(TARGET_DEVICE_DIR)/{{_extra_dir}}/aria2c

define load-image
	retry=1; while [ $$retry -le 5 ]; \
	do \
	echo "Begin $$retry time download $2/$1"; \
	retry=`expr $$retry + 1`; \
	$(ARIA2C) -c -s 10 -x 10 -t 600 $2/$1 -d $3 || continue; \
	echo "Download $1 successful" && exit 0; \
	done; \
	echo "Download $1 FAILED, please check your network!"; \
	$(if $(findstring $1, $(ACRN_MD5SUM_MD5)), echo "Warning: Check SoS Release!", \
	echo "Error: Mandatory Images failed!" && exit 1)
endef

define load-fw
	echo "Begin to load firmware...";
	{{#ifwi}}
	@$(ACP) $(TOP)/hardware/intel/fw_capsules/{{target}}/release/ifwi/$(ACRN_IFWI_FW) $(ACRN_TMP_DIR);
	@$(ACP) $(TOP)/hardware/intel/fw_capsules/{{target}}/release/ifwi/$(ACRN_IFWI_FV) $(ACRN_TMP_DIR);
	@$(ACP) $(TOP)/hardware/intel/fw_capsules/{{target}}/release/ifwi/$(ACRN_IFWI_DNX) $(ACRN_TMP_DIR);
	@$(ACP) $(TOP)/hardware/intel/fw_capsules/{{target}}/release/ifwi/$(ACRN_IFWI_DNXP) $(ACRN_TMP_DIR);
	@$(ACP) $(TOP)/hardware/intel/fw_capsules/{{target}}/$(ACRN_FW_VERSION) $(ACRN_TMP_DIR);
	{{/ifwi}}
	{{#ioc}}
	@$(ACP) $(TOP)/hardware/intel/fw_capsules/{{target}}/release/ioc/$(ACRN_IOC_FW_D) $(ACRN_TMP_DIR);
	@$(ACP) $(TOP)/hardware/intel/fw_capsules/{{target}}/release/ioc/$(ACRN_IOC_FW_E) $(ACRN_TMP_DIR);
	{{/ioc}}
endef

######################################################################
# Generate ACRN AaaG Image
######################################################################
.PHONY: acrn_ext4_bin
acrn_ext4_bin: $(ACRN_GPTIMAGE_BIN)
	$(hide) mkdir -p $(ACRN_DATA_DIR)
	$(hide) mkdir -p $(ACRN_AND_DIR)
	$(hide) rm -f $(ACRN_GPT_BIN)
	$(hide) cp $(ACRN_GPTIMAGE_BIN) $(ACRN_GPT_BIN)
	$(hide) echo "Try making $@"
	$(MAKE_EXT4FS_ACRN) -s -l $(ACRN_DATA_SIZE) $(ACRN_EXT4_BIN) $(ACRN_DATA_DIR)
	echo ">>> $@ is generated successfully"

######################################################################
# Download Extra ACRN Images
######################################################################
.PHONY: img_download
ifeq ($(strip $(SOS_VERSION)),local)
img_download:
	$(hide) rm -rf $(ACRN_TMP_DIR)
	$(hide) mkdir -p $(ACRN_TMP_DIR)
	$(call load-fw)
	echo -e "**********************************" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e "* SoS_Version: Local Images" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e "**********************************" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e ">>> Path: $(LOCAL_SOS_PATH)" >> $(ACRN_TMP_DIR)/acrnversion.txt
	@$(ACP) $(LOCAL_SOS_PATH) $(ACRN_TMP_DIR)
	$(hide) cp $(ACRN_TMP_DIR)/acrnversion.txt $(PRODUCT_OUT)/
	echo ">>> $@ is successful !!!"
else
img_download:
	$(hide) rm -rf $(ACRN_TMP_DIR)
	$(hide) mkdir -p $(ACRN_TMP_DIR)
	$(call load-fw)
	echo "Start to download SoS files from: $(ACRN_LINK) ..."
	$(call load-image,$(ACRN_MD5SUM_MD5),$(ACRN_LINK),$(ACRN_TMP_DIR))
	$(call load-image,$(ACRN_FLASH_JSON),$(ACRN_LINK),$(ACRN_TMP_DIR))
	$(call load-image,$(ACRN_PARTITION_DESC_BIN),$(ACRN_LINK),$(ACRN_TMP_DIR))
	$(call load-image,$(ACRN_SOS_BOOT_IMAGE),$(ACRN_LINK),$(ACRN_TMP_DIR))
	$(call load-image,$(ACRN_SOS_ROOTFS_IMAGE),$(ACRN_LINK),$(ACRN_TMP_DIR))
	echo -e "**********************************" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e "* SoS_Version: $(SOS_VERSION_CFG) " >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e "**********************************" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e ">>> Download SOS files:" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e "    - $(ACRN_SOS_BOOT_IMAGE)" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e "    - $(ACRN_SOS_ROOTFS_IMAGE)" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e "    - $(ACRN_PARTITION_DESC_BIN)" >> $(ACRN_TMP_DIR)/acrnversion.txt
	echo -e "    - $(ACRN_FLASH_JSON)" >> $(ACRN_TMP_DIR)/acrnversion.txt
	$(hide) cp $(ACRN_TMP_DIR)/acrnversion.txt $(PRODUCT_OUT)/
	echo ">>> $@ is successfull !!!"
endif

droid: img_download

######################################################################
# Generate ACRN AaaG *.zip
######################################################################
.PHONY: acrn_image
acrn_image: acrn_ext4_bin
	$(hide) mkdir -p $(PUBLISH_DEST)
	$(hide) zip -qjX $(ACRN_EXT4_BIN_ZIP) $(ACRN_EXT4_BIN)
	@$(ACP) $(ACRN_EXT4_BIN_ZIP) $(PUBLISH_DEST)
	echo ">>> $@ is generated successfully!"

######################################################################
# Generate ACRN E2E flashfiles *.zip
######################################################################
.PHONY: acrn_flashfiles
acrn_flashfiles: acrn_ext4_bin
	$(hide) mkdir -p $(ACRN_TMP_DIR)
	$(hide) cp $(ACRN_EXT4_BIN) $(ACRN_TMP_DIR)
	$(hide) mkdir -p $(PUBLISH_DEST)
	{{#md5_check}}
	$(TARGET_DEVICE_DIR)/{{_extra_dir}}/md5_check.sh $(ACRN_TMP_DIR)
	{{/md5_check}}
	$(hide) zip -qrjX $(ACRN_FLASHFILES) $(ACRN_TMP_DIR)
	@$(ACP) $(ACRN_FLASHFILES) $(PUBLISH_DEST)
	$(hide) rm -rf $(ACRN_TMP_DIR)
	echo ">>> $@ is generated successfully"

######################################################################
# Generate ACRN Guest flashfiles *.zip
######################################################################
.PHONY: acrn_guest
acrn_guest: acrn_ext4_bin
	$(hide) mkdir -p $(PUBLISH_DEST)
	@$(ACP) $(PRE_BUILD_BLD) $(PRODUCT_OUT)
	@$(ACP) $(PRE_BUILD_GPT) $(PRODUCT_OUT)
	@$(ACP) $(FLASH_JSON_CONFIG) $(PRODUCT_OUT)
	$(hide) zip -qrjX $(GUEST_FLASHFILES) \
	$(PRODUCT_OUT)/flash.json \
	$(PRODUCT_OUT)/gpt.img \
	$(PRODUCT_OUT)/gpt_gr_mrb_b1.bin \
	$(PRODUCT_OUT)/bootloader_gr_mrb_b1 \
	$(PRODUCT_OUT)/boot.img \
	$(PRODUCT_OUT)/tos.img \
	$(PRODUCT_OUT)/vbmeta.img \
	$(PRODUCT_OUT)/vendor.img \
	$(PRODUCT_OUT)/system.img
	@$(ACP) $(GUEST_FLASHFILES) $(PUBLISH_DEST)
	echo ">>> $@ is generated successfully"
