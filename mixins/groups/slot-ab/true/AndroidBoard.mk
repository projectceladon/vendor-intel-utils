make_ramdisk_dir:
	@mkdir -p $(PRODUCT_OUT)/root/metadata

$(PRODUCT_OUT)/ramdisk.img: make_ramdisk_dir

