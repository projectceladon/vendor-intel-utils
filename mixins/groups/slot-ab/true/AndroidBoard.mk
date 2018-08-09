make_ramdisk_dir:
	@mkdir -p $(PRODUCT_OUT)/root/metadata

$(PRODUCT_OUT)/ramdisk.img: make_ramdisk_dir

make_vendor_dir:
	@mkdir -p $(PRODUCT_OUT)/vendor/oem_config

$(PRODUCT_OUT)/vendor.img: make_vendor_dir
