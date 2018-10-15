BOARD_SEPOLICY_DIRS += \
    packages/services/Car/car_product/sepolicy \
    device/generic/car/common/sepolicy \
    $(INTEL_PATH_SEPOLICY)/car

TARGET_USES_CAR_FUTURE_FEATURES := true
BOARD_SEPOLICY_M4DEFS += module_carservice_app=true
