# Use external/drm-bxt
TARGET_USE_PRIVATE_LIBDRM := true
LIBDRM_VER ?= intel

BOARD_KERNEL_CMDLINE += vga=current i915.modeset=1 drm.atomic=1 i915.nuclear_pageflip=1 drm.vblankoffdelay=1 i915.fastboot=1
{{^acrn-guest}}
BOARD_KERNEL_CMDLINE += i915.enable_guc=2
{{/acrn-guest}}
USE_OPENGL_RENDERER := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
USE_INTEL_UFO_DRIVER := false
BOARD_GPU_DRIVERS := i965
BOARD_USE_CUSTOMIZED_MESA := true

# System's VSYNC phase offsets in nanoseconds
VSYNC_EVENT_PHASE_OFFSET_NS := 7500000
SF_VSYNC_EVENT_PHASE_OFFSET_NS := 5000000

BOARD_GPU_DRIVERS ?= i965 swrast
ifneq ($(strip $(BOARD_GPU_DRIVERS)),)
TARGET_HARDWARE_3D := true
TARGET_USES_HWC2 := true
endif

{{#drmhwc}}
BOARD_USES_DRM_HWCOMPOSER := true
BOARD_USES_IA_HWCOMPOSER := false
BOARD_USES_IA_PLANNER := true
{{/drmhwc}}

{{^drmhwc}}
BOARD_USES_DRM_HWCOMPOSER := false
BOARD_USES_IA_HWCOMPOSER := true
{{/drmhwc}}

{{#minigbm}}
BOARD_USES_MINIGBM := true
BOARD_ENABLE_EXPLICIT_SYNC := true
INTEL_MINIGBM := $(INTEL_PATH_HARDWARE)/external/minigbm-intel
{{/minigbm}}

{{^minigbm}}
BOARD_USES_MINIGBM := false
BOARD_ENABLE_EXPLICIT_SYNC := false
INTEL_DRM_GRALLOC := external/drm_gralloc/
{{/minigbm}}

{{#gralloc1}}
BOARD_USES_GRALLOC1 := true
{{/gralloc1}}

{{^gralloc1}}
BOARD_USES_GRALLOC1 := false
{{/gralloc1}}

{{#cursor_wa}}
BOARD_CURSOR_WA := true
{{/cursor_wa}}

{{^cursor_wa}}
BOARD_CURSOR_WA := false
{{/cursor_wa}}

{{#threedis_underrun_wa}}
BOARD_THREEDIS_UNDERRUN_WA := true
{{/threedis_underrun_wa}}

{{^threedis_underrun_wa}}
BOARD_THREEDIS_UNDERRUN_WA := false
{{/threedis_underrun_wa}}

{{#coreu_msync}}
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/graphics/mesa
{{/coreu_msync}}

{{#mesa_acrn_sepolicy}}
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/graphics/mesa_acrn
{{/mesa_acrn_sepolicy}}
