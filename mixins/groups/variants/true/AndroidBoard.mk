# flashfile_add_blob <blob_name> <path> <mandatory> <var_name>
# - Replace ::variant:: from <path> by variant name
# - If the result does not exists and <mandatory> is set, error
# - If <var_name> is set, put the result in <var_name>_<variant>
# - Add the pair <result>:<blob_name> in BOARD_FLASHFILES_FIRMWARE_<variant>
define flashfile_add_blob
$(foreach VARIANT,$(FLASHFILE_VARIANTS), \
    $(eval blob := $(subst ::variant::,$(VARIANT),$(2))) \
    $(if $(wildcard $(blob)), \
        $(if $(4), $(eval $(4)_$(VARIANT) := $(blob))) \
        $(eval BOARD_FLASHFILES_FIRMWARE_$(VARIANT) += $(blob):$(1)) \
        , \
        $(if $(3), $(error $(blob) does not exist))))
endef

define add_variant_flashfiles
$(foreach VARIANT,$(FLASHFILE_VARIANTS), \
    $(eval var_flashfile := $(TARGET_DEVICE_DIR)/flashfiles/$(VARIANT)/flashfiles.ini) \
    $(if $(wildcard $(var_flashfile)), \
        $(eval $(call add_var_flashfile,$(VARIANT),$(var_flashfile),$(1)))))
endef

define add_var_flashfile
INSTALLED_RADIOIMAGE_TARGET += $(3)/flashfiles_$(1).ini
$(3)/flashfiles_$(1).ini: $(2) | $(ACP)
	$$(copy-file-to-target)
endef

# Define ROOT_VARIANTS and VARIANTS in variants.mk
include $(TARGET_DEVICE_DIR)/variants.mk

# Let the user define it's variants manually if desired
ifeq ($(FLASHFILE_VARIANTS),)
        OTA_VARIANTS := $(ROOT_VARIANTS)
        FLASHFILE_VARIANTS := $(ROOT_VARIANTS) $(VARIANTS)
        ifeq ($(GEN_ALL_OTA),true)
            OTA_VARIANTS += $(VARIANTS)
        endif
endif

ifeq ($(FLASHFILE_VARIANTS),)
        $(error variants enabled but ROOT_VARIANTS is not defined)
endif

BOARD_DEVICE_MAPPING := $(TARGET_DEVICE_DIR)/device_mapping.py
ifeq ($(wildcard $(BOARD_DEVICE_MAPPING)),)
$(error variants need device_mapping.py to generate ota packages)
endif
INSTALLED_RADIOIMAGE_TARGET += $(BOARD_DEVICE_MAPPING)
