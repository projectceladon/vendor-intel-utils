PRODUCT_PACKAGES_DEBUG += \
    DBGA_Main \
    fdk-modules-structure

{{#mock_dev}}
ifneq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_PACKAGES_DEBUG += \
    DBGA_cAVSUnitTest
endif
{{/mock_dev}}

{{#func_test}}
ifneq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_PACKAGES_DEBUG += \
    DBGA_FunctionalTests \
    functionalTestsDatafiles
endif
{{/func_test}}
