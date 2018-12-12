PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.heapstartsize=16m \
    dalvik.vm.heapgrowthlimit=288m \
    dalvik.vm.heapsize=512m \
    dalvik.vm.heaptargetutilization=0.75 \
    dalvik.vm.heapminfree=512k \
    dalvik.vm.heapmaxfree=8m

#We want to use default GC as GENCOPYING,with bpssize=128m for large heaps
ART_DEFAULT_GC_TYPE?=GENCOPYING
ifeq ($(ART_DEFAULT_GC_TYPE),GENCOPYING)
    PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heap.bpssize=128m
endif

{{#large-region-size}}
ENABLE_LARGE_REGION_SIZE?=true
{{/large-region-size}}

{{^large-region-size}}
ENABLE_LARGE_REGION_SIZE?=false
{{/large-region-size}}
