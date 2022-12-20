// Copyright (C) 2018-2022 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions
// and limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <i915_drm.h>
#include "icr_alloc.h"

#ifndef ICR_ALLOC_INTERNAL_H
#define ICR_ALLOC_INTERNAL_H

#define ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))

#define I915_CACHELINE_SIZE 64
#define I915_CACHELINE_MASK (I915_CACHELINE_SIZE - 1)

#define DRM_I915_GEM_MMAP_OFFSET       DRM_I915_GEM_MMAP_GTT
#define DRM_IOCTL_I915_GEM_MMAP_OFFSET         DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP_OFFSET, struct drm_i915_gem_mmap_offset)
#define DRM_I915_QUERY_MEMORY_REGIONS   4
#define DRM_IOCTL_I915_QUERY			DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_QUERY, struct drm_i915_query)
#define DRM_IOCTL_I915_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_CREATE, struct drm_i915_gem_create)
#define DRM_IOCTL_I915_GEM_CREATE_EXT	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_CREATE, struct drm_i915_gem_create_ext)

// PRELIM DRM interface definitions
// we should use prelim uapi for ATS-M alpha release BKC. but still fallback to legacy uapi for back compatible
#define PRELIM_DRM_I915_QUERY			(1 << 16)
#define PRELIM_DRM_I915_QUERY_MEMORY_REGIONS	(PRELIM_DRM_I915_QUERY | 4)
#define PRELIM_I915_OBJECT_PARAM  (1ull << 48)
#define PRELIM_I915_PARAM_MEMORY_REGIONS ((1 << 16) | 0x1)
#define PRELIM_I915_USER_EXT		(1 << 16)
#define PRELIM_I915_GEM_CREATE_EXT_SETPARAM		(PRELIM_I915_USER_EXT | 1)

#define ROUND_UP(x, y) (((x) + (y)-1) / (y))

enum drm_i915_gem_memory_class {
	I915_MEMORY_CLASS_SYSTEM = 0,
	I915_MEMORY_CLASS_DEVICE,
	I915_MEMORY_CLASS_STOLEN_SYSTEM,
	I915_MEMORY_CLASS_STOLEN_DEVICE,
};

struct drm_i915_gem_create_ext {

	/**
	 * Requested size for the object.
	 *
	 * The (page-aligned) allocated size for the object will be returned.
	 */
	__u64 size;
	/**
	 * Returned handle for the object.
	 *
	 * Object handles are nonzero.
	 */
	__u32 handle;
	__u32 pad;
#define I915_GEM_CREATE_EXT_SETPARAM (1u << 0)
#define I915_GEM_CREATE_EXT_FLAGS_UNKNOWN \
	(-(I915_GEM_CREATE_EXT_SETPARAM << 1))
	__u64 extensions;
};

struct drm_i915_gem_memory_class_instance {
	__u16 memory_class; /* see enum drm_i915_gem_memory_class */
	__u16 memory_instance;
};

struct drm_i915_memory_region_info {
	/** class:instance pair encoding */
	struct drm_i915_gem_memory_class_instance region;

	/** MBZ */
	__u32 rsvd0;

	/** MBZ */
	__u64 caps;

	/** MBZ */
	__u64 flags;

	/** Memory probed by the driver (-1 = unknown) */
	__u64 probed_size;

	/** Estimate of memory remaining (-1 = unknown) */
	__u64 unallocated_size;

	/** MBZ */
	__u64 rsvd1[8];
};

struct drm_i915_gem_object_param {
	/* Object handle (0 for I915_GEM_CREATE_EXT_SETPARAM) */
	__u32 handle;

	/* Data pointer size */
	__u32 size;

/*
 * I915_OBJECT_PARAM:
 *
 * Select object namespace for the param.
 */
#define I915_OBJECT_PARAM  (1ull<<32)

/*
 * I915_PARAM_MEMORY_REGIONS:
 *
 * Set the data pointer with the desired set of placements in priority
 * order(each entry must be unique and supported by the device), as an array of
 * drm_i915_gem_memory_class_instance, or an equivalent layout of class:instance
 * pair encodings. See DRM_I915_QUERY_MEMORY_REGIONS for how to query the
 * supported regions.
 *
 * Note that this requires the I915_OBJECT_PARAM namespace:
 *	.param = I915_OBJECT_PARAM | I915_PARAM_MEMORY_REGIONS
 */
#define I915_PARAM_MEMORY_REGIONS 0x1
	__u64 param;

	/* Data value or pointer */
	__u64 data;
};

struct drm_i915_gem_create_ext_setparam {
	struct i915_user_extension base;
	struct drm_i915_gem_object_param param;
};

struct drm_i915_query_memory_regions {
	/** Number of supported regions */
	__u32 num_regions;

	/** MBZ */
	__u32 rsvd[3];

	/* Info about each supported region */
	struct drm_i915_memory_region_info regions[];
};

struct drm_i915_gem_mmap_offset {
        /** Handle for the object being mapped. */
        __u32 handle;
        __u32 pad;
        /**
         * Fake offset to use for subsequent mmap call
         *
         * This is a fixed-size type for 32/64 compatibility.
         */
        __u64 offset;

        /**
         * Flags for extended behaviour.
         *
         * It is mandatory that either one of the _WC/_WB flags
         * should be passed here.
         */
        __u64 flags;
#define I915_MMAP_OFFSET_WC (1 << 0)
#define I915_MMAP_OFFSET_WB (1 << 1)
#define I915_MMAP_OFFSET_UC (1 << 2)
#define I915_MMAP_OFFSET_FLAGS \
        (I915_MMAP_OFFSET_WC | I915_MMAP_OFFSET_WB | I915_MMAP_OFFSET_UC)
};

struct icr_memregion {
   struct drm_i915_gem_memory_class_instance region;
   uint64_t size;
};

struct _icr_alloc {
    /// ... driver, device fd
	int fd;
	int32_t has_llc;
	int32_t has_mmap_offset;
	struct icr_memregion vram, sys;
	pthread_mutex_t lock;
	pthread_mutexattr_t mutexattr;
};

struct bo {
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t tiling;
	size_t num_planes;
	uint32_t offsets[DRV_MAX_PLANES];
	uint32_t sizes[DRV_MAX_PLANES];
	uint32_t strides[DRV_MAX_PLANES];
	uint32_t total_size;
    uint32_t gem_handle;
	uint32_t aligned_width;
	uint32_t aligned_height;
	void *priv;
};

#endif