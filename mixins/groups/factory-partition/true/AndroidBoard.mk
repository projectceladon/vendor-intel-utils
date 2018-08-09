INSTALLED_FACTORYIMAGE_TARGET := $(PRODUCT_OUT)/factory.img
selinux_fc := $(TARGET_ROOT_OUT)/file_contexts.bin

$(INSTALLED_FACTORYIMAGE_TARGET) : PRIVATE_SELINUX_FC := $(selinux_fc)
$(INSTALLED_FACTORYIMAGE_TARGET) : $(MKEXTUSERIMG) $(MAKE_EXT4FS) $(E2FSCK) $(selinux_fc)
	$(call pretty,"Target factory fs image: $(INSTALLED_FACTORYIMAGE_TARGET)")
	@mkdir -p $(PRODUCT_OUT)/factory
	$(hide)	$(MKEXTUSERIMG) -s \
		$(PRODUCT_OUT)/factory \
		$(PRODUCT_OUT)/factory.img \
		ext4 \
		factory \
		$(BOARD_FACTORYIMAGE_PARTITION_SIZE) \
		$(PRIVATE_SELINUX_FC)

INSTALLED_RADIOIMAGE_TARGET += $(INSTALLED_FACTORYIMAGE_TARGET)

selinux_fc :=

.PHONY: factoryimage
factoryimage: $(INSTALLED_FACTORYIMAGE_TARGET)
