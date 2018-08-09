TARGET_USE_TRUSTY := true

ifneq (, $(filter abl sbl, {{boot-arch}}))
TARGET_USE_MULTIBOOT := true
endif

{{#enable_hw_sec}}
BOARD_USES_TRUSTY := true
BOARD_USES_KEYMASTER1 := true
{{/enable_hw_sec}}
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/trusty
BOARD_SEPOLICY_M4DEFS += module_trusty=true

LKBUILD_TOOLCHAIN_ROOT = $(PWD)/$(INTEL_PATH_VENDOR)/external/prebuilts/elf/
LKBUILD_X86_TOOLCHAIN = $(LKBUILD_TOOLCHAIN_ROOT)i386-elf-4.9.1-Linux-x86_64/bin
LKBUILD_X64_TOOLCHAIN = $(LKBUILD_TOOLCHAIN_ROOT)x86_64-elf-4.9.1-Linux-x86_64/bin
TRUSTY_BUILDROOT = $(PWD)/$(PRODUCT_OUT)/obj/trusty/

TRUSTY_ENV_VAR += TRUSTY_REF_TARGET={{ref_target}}

#for trusty lk
TRUSTY_ENV_VAR += BUILDROOT=$(TRUSTY_BUILDROOT)
TRUSTY_ENV_VAR += PATH=$$PATH:$(LKBUILD_X86_TOOLCHAIN):$(LKBUILD_X64_TOOLCHAIN)
TRUSTY_ENV_VAR += CLANG_BINDIR=$(PWD)/$(LLVM_PREBUILTS_PATH)
TRUSTY_ENV_VAR += ARCH_x86_64_TOOLCHAIN_PREFIX=${PWD}/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-${TARGET_GCC_VERSION}/bin/x86_64-linux-android-

#for trusty vmm
# use same toolchain as android kernel
TRUSTY_ENV_VAR += COMPILE_TOOLCHAIN=$(YOCTO_CROSSCOMPILE)
TRUSTY_ENV_VAR += TARGET_BUILD_VARIANT=$(TARGET_BUILD_VARIANT)
TRUSTY_ENV_VAR += BOOT_ARCH={{boot-arch}}

# output build dir to android out folder
TRUSTY_ENV_VAR += BUILD_DIR=$(TRUSTY_BUILDROOT)
TRUSTY_ENV_VAR += LKBIN_DIR=$(TRUSTY_BUILDROOT)/build-{{{lk_project}}}/

#Fix the cpu hotplug fail due to the trusty.
#Trusty will introduce some delay for cpu_up().
#Experiment show need wait at least 60us after
#apic_icr_write(APIC_DM_STARTUP | (start_eip >> 12), phys_apicid);
#So here override the cpu_init_udelay to have the cpu wait for 300us
#to guarantee the cpu_up success.
BOARD_KERNEL_CMDLINE += cpu_init_udelay=10

#TOS_PREBUILT := $(PWD)/$(INTEL_PATH_VENDOR)/fw/evmm/tos.img
#EVMM_PREBUILT := $(PWD)/$(INTEL_PATH_VENDOR)/fw/evmm/multiboot.img
