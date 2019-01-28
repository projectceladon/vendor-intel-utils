{{^disable-ipu}}
# Car device required kernel diff config
KERNEL_CAR_DIFFCONFIG = $(wildcard $(KERNEL_CONFIG_PATH)/car_diffconfig)
KERNEL_DIFFCONFIG += $(KERNEL_CAR_DIFFCONFIG)
{{/disable-ipu}}
