INSTALLED_CONFIGIMAGE_TARGET := $(PRODUCT_OUT)/config.img

selinux_fc := $(TARGET_ROOT_OUT)/file_contexts.bin

make_oem_config_dir:
{{^slot-ab}}
	@mkdir -p $(PRODUCT_OUT)/root/oem_config
	@mkdir -p $(PRODUCT_OUT)/recovery/root/oem_config
{{/slot-ab}}
{{#slot-ab}}
	@mkdir -p $(PRODUCT_OUT)/root/mnt/vendor
	@mkdir -p $(PRODUCT_OUT)/root/mnt/vendor/oem_config
	@mkdir -p $(PRODUCT_OUT)/recovery/root/mnt/vendor
	@mkdir -p $(PRODUCT_OUT)/recovery/root/mnt/vendor/oem_config
{{/slot-ab}}

$(INSTALLED_CONFIGIMAGE_TARGET) : PRIVATE_SELINUX_FC := $(selinux_fc)
$(INSTALLED_CONFIGIMAGE_TARGET) : $(MKEXTUSERIMG) $(MAKE_EXT4FS) $(E2FSCK) $(selinux_fc) bootimage make_oem_config_dir
	$(call pretty,"Target config fs image: $(INSTALLED_CONFIGIMAGE_TARGET)")
	@mkdir -p $(PRODUCT_OUT)/config
	$(hide)	PATH=$(HOST_OUT_EXECUTABLES):$$PATH \
		$(MKEXTUSERIMG) -s \
		$(PRODUCT_OUT)/config \
		$(PRODUCT_OUT)/config.img \
		ext4 \
		oem_config \
		$(BOARD_CONFIGIMAGE_PARTITION_SIZE) \
		$(PRIVATE_SELINUX_FC)

INSTALLED_RADIOIMAGE_TARGET += $(INSTALLED_CONFIGIMAGE_TARGET)

selinux_fc :=
