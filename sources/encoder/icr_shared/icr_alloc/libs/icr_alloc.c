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

#include "icr_alloc.h"
#include "icr_alloc_internal.h"


#if BUILD_FOR_HOST
#define alloc_log		printf
#else
#include <android/log.h>
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define alloc_log		ALOGI
#endif


#ifndef NDEBUG
#define DEBUG(...)  alloc_log(__VA_ARGS__)
#else
#define DEBUG(...) ((void)0)
#endif




static int icr_gen_ioctl(int fd, unsigned long request, void *arg)
{
    int ret;

    do {
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

static int icr_gem_param(int fd, int name)
{
   int val = -1; 

   struct drm_i915_getparam gp = { .param = name, .value = &val };
   if (icr_gen_ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp))
      return -1;

   return val;
}

static void icr_alloc_update_meminfo(icr_alloc_t* alloc, const struct drm_i915_query_memory_regions *meminfo)
{
	int i;
    for (i = 0; i < meminfo->num_regions; i++) {
        const struct drm_i915_memory_region_info *mem = &meminfo->regions[i];
        switch (mem->region.memory_class) {
        case I915_MEMORY_CLASS_SYSTEM:
            alloc->sys.region = mem->region;
            alloc->sys.size = mem->probed_size;
            break;
        case I915_MEMORY_CLASS_DEVICE:
            alloc->vram.region = mem->region;
            alloc->vram.size = mem->probed_size;
            break;
        default:
            break;
        }
    }
}

static bool icr_alloc_query_meminfo(icr_alloc_t* alloc)
{
    struct drm_i915_query_item item = {
      .query_id = PRELIM_DRM_I915_QUERY_MEMORY_REGIONS,
    };

    struct drm_i915_query query = {
      .num_items = 1,
      .items_ptr = (uintptr_t) &item,
    };

    if (drmIoctl(alloc->fd, DRM_IOCTL_I915_QUERY, &query)) {
        alloc_log("icr_alloc: Failed to DRM_IOCTL_I915_QUERY with prelim interface, use legacy drm uapi\n");
        item.query_id = DRM_I915_QUERY_MEMORY_REGIONS;
        if (drmIoctl(alloc->fd, DRM_IOCTL_I915_QUERY, &query)) {
            alloc_log("icr_alloc: Failed to DRM_IOCTL_I915_QUERY");
            return false;
        }
    }

    struct drm_i915_query_memory_regions *meminfo = (struct drm_i915_query_memory_regions *)calloc(1, item.length);
    item.data_ptr = (uintptr_t)meminfo;

    if (drmIoctl(alloc->fd, DRM_IOCTL_I915_QUERY, &query) ||
        item.length <= 0)
	{
		if(meminfo != NULL)
			free(meminfo);
        return false;
	}
	if(!meminfo)
		return false;

    icr_alloc_update_meminfo(alloc, meminfo);

	if(meminfo != NULL)
    	free(meminfo);

    return true;
}

bool icr_alloc_create(icr_alloc_t* alloc, int fd)
{
	int ret;

	alloc->fd = fd;

	int device_id;
	drm_i915_getparam_t get_param;

	memset(&get_param, 0, sizeof(get_param));
	get_param.param = I915_PARAM_CHIPSET_ID;
	get_param.value = &device_id;
	ret = drmIoctl(alloc->fd, DRM_IOCTL_I915_GETPARAM, &get_param);
	if (ret) {
		alloc_log("icr_alloc: Failed to get I915_PARAM_CHIPSET_ID\n");
		return false;
	}

	memset(&get_param, 0, sizeof(get_param));
	get_param.param = I915_PARAM_HAS_LLC;
	get_param.value = &alloc->has_llc;
	ret = drmIoctl(alloc->fd, DRM_IOCTL_I915_GETPARAM, &get_param);
	if (ret) {
		alloc_log("icr_alloc: Failed to get I915_PARAM_HAS_LLC\n");
		return false;
	}

	alloc->has_mmap_offset = icr_gem_param(alloc->fd, I915_PARAM_MMAP_GTT_VERSION) >= 4;

    icr_alloc_query_meminfo(alloc);

	return true;
}

size_t icr_num_planes_from_format(uint32_t format)
{
	switch (format) {
		case DRM_FORMAT_ABGR1555:
		case DRM_FORMAT_ABGR2101010:
		case DRM_FORMAT_ABGR4444:
		case DRM_FORMAT_ABGR8888:
		case DRM_FORMAT_ARGB1555:
		case DRM_FORMAT_ARGB2101010:
		case DRM_FORMAT_ARGB4444:
		case DRM_FORMAT_ARGB8888:
		case DRM_FORMAT_AYUV:
		case DRM_FORMAT_BGR233:
		case DRM_FORMAT_BGR565:
		case DRM_FORMAT_BGR888:
		case DRM_FORMAT_BGRA1010102:
		case DRM_FORMAT_BGRA4444:
		case DRM_FORMAT_BGRA5551:
		case DRM_FORMAT_BGRA8888:
		case DRM_FORMAT_BGRX1010102:
		case DRM_FORMAT_BGRX4444:
		case DRM_FORMAT_BGRX5551:
		case DRM_FORMAT_BGRX8888:
		case DRM_FORMAT_C8:
		case DRM_FORMAT_GR88:
		case DRM_FORMAT_R8:
		case DRM_FORMAT_RG88:
		case DRM_FORMAT_RGB332:
		case DRM_FORMAT_RGB565:
		case DRM_FORMAT_RGB888:
		case DRM_FORMAT_RGBA1010102:
		case DRM_FORMAT_RGBA4444:
		case DRM_FORMAT_RGBA5551:
		case DRM_FORMAT_RGBA8888:
		case DRM_FORMAT_RGBX1010102:
		case DRM_FORMAT_RGBX4444:
		case DRM_FORMAT_RGBX5551:
		case DRM_FORMAT_RGBX8888:
		case DRM_FORMAT_UYVY:
		case DRM_FORMAT_VYUY:
		case DRM_FORMAT_XBGR1555:
		case DRM_FORMAT_XBGR2101010:
		case DRM_FORMAT_XBGR4444:
		case DRM_FORMAT_XBGR8888:
		case DRM_FORMAT_XRGB1555:
		case DRM_FORMAT_XRGB2101010:
		case DRM_FORMAT_XRGB4444:
		case DRM_FORMAT_XRGB8888:
		case DRM_FORMAT_YUYV:
		case DRM_FORMAT_YVYU:
			return 1;
		case DRM_FORMAT_NV12:
		case DRM_FORMAT_NV21:
			return 2;
		case DRM_FORMAT_YVU420:
			return 3;
		default:
			alloc_log("unknown format!\n");
			return 0;
	}
}

uint32_t icr_bpp_from_format(uint32_t format, size_t plane)
{
	uint32_t bpp = 0;
	switch (format) {
		case DRM_FORMAT_BGR233:
		case DRM_FORMAT_C8:
		case DRM_FORMAT_R8:
		case DRM_FORMAT_RGB332:
		case DRM_FORMAT_YVU420:
			bpp = 8;
			break;

		case DRM_FORMAT_NV12:
		case DRM_FORMAT_NV21:
			bpp = (plane == 0) ? 8 : 4;
			break;

		case DRM_FORMAT_ABGR1555:
		case DRM_FORMAT_ABGR4444:
		case DRM_FORMAT_ARGB1555:
		case DRM_FORMAT_ARGB4444:
		case DRM_FORMAT_BGR565:
		case DRM_FORMAT_BGRA4444:
		case DRM_FORMAT_BGRA5551:
		case DRM_FORMAT_BGRX4444:
		case DRM_FORMAT_BGRX5551:
		case DRM_FORMAT_GR88:
		case DRM_FORMAT_RG88:
		case DRM_FORMAT_RGB565:
		case DRM_FORMAT_RGBA4444:
		case DRM_FORMAT_RGBA5551:
		case DRM_FORMAT_RGBX4444:
		case DRM_FORMAT_RGBX5551:
		case DRM_FORMAT_UYVY:
		case DRM_FORMAT_VYUY:
		case DRM_FORMAT_XBGR1555:
		case DRM_FORMAT_XBGR4444:
		case DRM_FORMAT_XRGB1555:
		case DRM_FORMAT_XRGB4444:
		case DRM_FORMAT_YUYV:
		case DRM_FORMAT_YVYU:
			bpp = 16;
			break;

		case DRM_FORMAT_BGR888:
		case DRM_FORMAT_RGB888:
			bpp = 24;
			break;

		case DRM_FORMAT_ABGR2101010:
		case DRM_FORMAT_ABGR8888:
		case DRM_FORMAT_ARGB2101010:
		case DRM_FORMAT_ARGB8888:
		case DRM_FORMAT_AYUV:
		case DRM_FORMAT_BGRA1010102:
		case DRM_FORMAT_BGRA8888:
		case DRM_FORMAT_BGRX1010102:
		case DRM_FORMAT_BGRX8888:
		case DRM_FORMAT_RGBA1010102:
		case DRM_FORMAT_RGBA8888:
		case DRM_FORMAT_RGBX1010102:
		case DRM_FORMAT_RGBX8888:
		case DRM_FORMAT_XBGR2101010:
		case DRM_FORMAT_XBGR8888:
		case DRM_FORMAT_XRGB2101010:
		case DRM_FORMAT_XRGB8888:
			bpp = 32;
			break;
	}
    return bpp;
}

uint32_t icr_stride_from_format(uint32_t format, uint32_t width, size_t plane)
{
	uint32_t bpp = icr_bpp_from_format(format, plane);

    uint32_t width_allignment = width;
    uint32_t w_mod = width % 16;
    if (w_mod != 0) {
        uint32_t i = width / 16;
        i = i + 1;
        width_allignment = i * 16;
    }

    uint32_t stride = ROUND_UP(width_allignment * bpp, 8);

	return stride;
}

struct bo *icr_bo_create(icr_alloc_t* alloc, uint32_t width, uint32_t height,
				       uint32_t format, uint64_t modifier)
{
	struct bo *bo;
	bo = (struct bo *)calloc(1, sizeof(*bo));

	if (!bo)
		return NULL;

	bo->width = width;
	bo->height = height;
	bo->format = format;
	bo->num_planes = icr_num_planes_from_format(format);

	if (!bo->num_planes) {
		free(bo);
		return NULL;
	}

	int ret;
	size_t plane;
	uint32_t stride;
	struct drm_i915_gem_create gem_create;

	switch (modifier) {
	case DRM_FORMAT_MOD_LINEAR:
		bo->tiling = I915_TILING_NONE;
		break;
	case I915_FORMAT_MOD_X_TILED:
		bo->tiling = I915_TILING_X;
		break;
	case I915_FORMAT_MOD_Y_TILED:
	case I915_FORMAT_MOD_Y_TILED_CCS:
    case I915_FORMAT_MOD_Yf_TILED:
    case I915_FORMAT_MOD_Yf_TILED_CCS:
		bo->tiling = I915_TILING_Y;
		break;
	}

	stride = icr_stride_from_format(format, width, 0);
	// to-do: size alignment according to intel HW

	// to-do: bo stride, offset, num_planes 4(deal with single right now), 
	bo->strides[0] = stride;
	bo->sizes[0] = stride * ROUND_UP(height, 1);
	bo->offsets[0] = 0;
	bo->total_size = bo->sizes[0];

	if (bo->tiling == I915_TILING_NONE)
		bo->total_size += 64;

	bo->aligned_width = width;
	bo->aligned_height = height;

	static bool force_mem_type = false;
	static bool force_mem_local = false;
	static bool force_mem_read = false;
	//Ignore KW issue, same as minigbm of Local memory
	if (!force_mem_read) {
		const char *force_mem_env = getenv("ICR_FORCE_MEM");
        if (force_mem_env) {
            if (!strcmp(force_mem_env, "local")) {
                force_mem_local = true; /* always use local memory */
                force_mem_type = true;
            } else if (!strcmp(force_mem_env, "system")) {
                force_mem_local = false; /* always use local memory */
                force_mem_type = true;
            }
        }
	    force_mem_read = true;

	    if (force_mem_type) {
	        DEBUG("icr_alloc: Forcing all memory allocation to come from: %s\n",
	          force_mem_local ? "local" : "system");
	    }
	}

	bool local = true;
	if (force_mem_type) {
	       local = force_mem_local;
	}
	uint32_t gem_handle;
	/* If we have vram size, we have multiple memory regions and should choose
	* one of them.
	*/
	if (alloc->vram.size > 0) {
	   /* All new BOs we get from the kernel are zeroed, so we don't need to
		* worry about that here.
		*/
	    struct drm_i915_gem_memory_class_instance regions[2];
	    uint32_t nregions = 0;
	    if (local) {
		  /* For vram allocations, still use system memory as a fallback. */
		  regions[nregions++] = alloc->vram.region;
		  regions[nregions++] = alloc->sys.region;
	    } else {
		  regions[nregions++] = alloc->sys.region;
	    }

	    struct drm_i915_gem_object_param region_param = {
		  .size = nregions,
		  .data = (uintptr_t)regions,
		  .param = PRELIM_I915_OBJECT_PARAM | PRELIM_I915_PARAM_MEMORY_REGIONS,
	    };

	    struct drm_i915_gem_create_ext_setparam setparam_region = {
		  .base = { .name = PRELIM_I915_GEM_CREATE_EXT_SETPARAM },
		  .param = region_param,
	    };

	    struct drm_i915_gem_create_ext create = {
		  .size = bo->total_size,
		  .extensions = (uintptr_t)&setparam_region,
	    };

	   /* It should be safe to use GEM_CREATE_EXT without checking, since we are
		* in the side of the branch where discrete memory is available. So we
		* can assume GEM_CREATE_EXT is supported already.
		*/
	    ret = drmIoctl(alloc->fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &create);   
	    if (ret) {
	        alloc_log("icr_alloc: DRM_IOCTL_I915_GEM_CREATE_EXT with PRELIM interface failed (size=%llu), use legacy uapi\n",
	          create.size);

	        // Try to fallback to legacy uapi
	        setparam_region.param.param = I915_OBJECT_PARAM | I915_PARAM_MEMORY_REGIONS;
	        setparam_region.base.name = I915_GEM_CREATE_EXT_SETPARAM;
	        ret = drmIoctl(alloc->fd, DRM_IOCTL_I915_GEM_CREATE_EXT, &create);
	        if (ret) {
	            alloc_log("icr_alloc: DRM_IOCTL_I915_GEM_CREATE_EXT lagacy interface failed (size=%llu)\n", create.size);
	            free(bo);
	            return NULL;
	        }
	    }
	    gem_handle = create.handle;
	    DEBUG("icr_alloc: DRM_IOCTL_I915_GEM_CREATE_EXT handle\n");
	} else {
	    struct drm_i915_gem_create create = { .size = bo->total_size };
	   /* All new BOs we get from the kernel are zeroed, so we don't need to
		* worry about that here.
		*/
	    ret = drmIoctl(alloc->fd, DRM_IOCTL_I915_GEM_CREATE, &create);
	    if (ret) {
		    alloc_log("icr_alloc: DRM_IOCTL_I915_GEM_CREATE failed (size=%llu)\n",
                  create.size);
			free(bo);
            return NULL;
	   }
	   gem_handle = create.handle;
	   DEBUG("icr_alloc: DRM_IOCTL_I915_GEM_CREATE handle\n");
	}

	bo->gem_handle = gem_handle;

	return bo;
}

int icr_bo_get_prime_fd(icr_alloc_t* alloc, struct bo *bo)
{
	int ret = 0;
    int prime_fd = -1;

	ret = drmPrimeHandleToFD(alloc->fd, bo->gem_handle, DRM_CLOEXEC, &prime_fd);
    if(ret) {
        alloc_log("icr_alloc: drmPrimeHandleToFD failed(fd = %d, handle = %d, prime_fd = %d) ret = %d\n",
            alloc->fd, bo->gem_handle, prime_fd, ret);
    }

	return prime_fd;
}

void icr_bufmgr_unmap_internal(icr_alloc_t* alloc, icr_buffer_t*buffer)
{
	// buffer flush
	void *p = (void *)(((uintptr_t)buffer->map_address) & ~I915_CACHELINE_MASK);
	void *end = (void *)((uintptr_t)buffer->map_address + buffer->total_size);

	__builtin_ia32_mfence();
	while (p < end) {
		__builtin_ia32_clflush(p);
		p = (void *)((uintptr_t)p + I915_CACHELINE_SIZE);
	}

    //buffer unmap
	munmap(buffer->map_address, buffer->total_size);
	buffer->mapped = 0;
}

icr_alloc_t* icr_bufmgr_init() {
	int fd = -1;
	drmVersionPtr version;
        
	char const *str = "%s/renderD%d";

	const char *rnode = getenv("ICR_RNODE");
	int rnode_id = 0;
	if (rnode) {
		rnode_id = atoi(rnode);
	}
	if(rnode_id < 0 || rnode_id >= 1024)
	{
		alloc_log("Invalid node id, use default one node 0.\n");
		rnode_id = 0;
	}
	char *node;
	if (asprintf(&node, str, DRM_DIR_NAME, rnode_id+128) < 0)
		return NULL;

    DEBUG("open(%s)\n", node);
	fd = open(node, O_RDWR, 0); //Ignore KW issue, same as minigbm of Local Rendering
	free(node);

	if (fd < 0)
		return NULL;

	version = drmGetVersion(fd);
	if (!version) {
		close(fd);
		return NULL;
	}

    DEBUG("icr_alloc: drm version->name =%s\n", version->name);
	icr_alloc_t * icr_alloc = (struct _icr_alloc *)calloc(1, sizeof(*icr_alloc));

	if (!icr_alloc){
		close(fd);
		return NULL;
	}

	if (icr_alloc_create(icr_alloc, fd)) {
		pthread_mutexattr_init(&icr_alloc->mutexattr);
		pthread_mutexattr_settype(&icr_alloc->mutexattr, PTHREAD_MUTEX_RECURSIVE);
		if(pthread_mutex_init(&icr_alloc->lock, &icr_alloc->mutexattr) != 0)
			alloc_log("pthread_mutex_init fail!\n");
		return icr_alloc;
	}

	close(fd);
	free(icr_alloc);
	return NULL;
}

icr_buffer_t* icr_bufmgr_alloc(icr_alloc_t* alloc, icr_buffer_desc_t* info) {
	if(!alloc){
		alloc_log("icr_alloc: Fail to alloc icr_buffer, alloc is NULL!\n");
		return NULL;
	}
	if(pthread_mutex_lock(&alloc->lock) != 0)
		alloc_log("pthread_mutex_lock fail!\n");

	struct bo *bo;
	bo = icr_bo_create(alloc, info->width, info->height, info->format, info->modifier);
	if (!bo) {
		alloc_log("icr_alloc: Fail to create bo\n");
		if(pthread_mutex_unlock(&alloc->lock) != 0)
			alloc_log("pthread_mutex_unlock fail!\n");
		return NULL;
	}

	icr_buffer_t* buffer = (struct _icr_buffer *)calloc(1, sizeof(*buffer));
	if(!buffer){
		alloc_log("icr_alloc: Fail to alloc icr_buffer, buffer is NULL!\n");
		free(bo);
		if(pthread_mutex_unlock(&alloc->lock) != 0)
			alloc_log("pthread_mutex_unlock fail!\n");
		return NULL;
	}
	buffer->fds[0] = icr_bo_get_prime_fd(alloc, bo);
	buffer->width = bo->width;
	buffer->height = bo->height;
	buffer->format = bo->format;
	buffer->tiling_mode = bo->tiling;
	buffer->pixel_stride = ROUND_UP(bo->strides[0], ROUND_UP(icr_bpp_from_format(bo->format, 0), 8));
	buffer->strides[0] = bo->strides[0];
	buffer->offsets[0] = bo->offsets[0];
	buffer->sizes[0] = bo->sizes[0];
	buffer->aligned_width = bo->aligned_width;
	buffer->aligned_height = bo->aligned_height;
	buffer->total_size = bo->total_size;
	buffer->gem_handle = bo->gem_handle;

	DEBUG("icr_alloc: buffer width = %d, height = %d, stride = %d\n", buffer->width, buffer->height, buffer->pixel_stride);
	DEBUG("icr_alloc: buffer format = %x, tiling = %d\n", buffer->format, buffer->tiling_mode);
	DEBUG("icr_alloc: buffer aligned_width = %d, aligned_height = %d\n", buffer->aligned_width, buffer->aligned_height);

	free(bo);
	if(pthread_mutex_unlock(&alloc->lock) != 0)
		alloc_log("pthread_mutex_unlock fail!\n");
	return buffer;
}

void  icr_bufmgr_free(icr_alloc_t* alloc, icr_buffer_t* buffer) {
	if(!alloc){
		alloc_log("icr_alloc: Fail to free icr_buffer, alloc is NULL!\n");
		return;
	}
	
	if(pthread_mutex_lock(&alloc->lock) != 0)
		alloc_log("pthread_mutex_lock fail!\n");

	if(buffer->mapped){
		icr_bufmgr_unmap_internal(alloc, buffer);
	}

	if (buffer->fds[0] != -1) {
		close(buffer->fds[0]);
	}

	struct drm_gem_close gem_close;
	memset(&gem_close, 0, sizeof(gem_close));
	gem_close.handle = buffer->gem_handle;
	drmIoctl(alloc->fd, DRM_IOCTL_GEM_CLOSE, &gem_close);

	free(buffer);
	if(pthread_mutex_unlock(&alloc->lock) != 0)
		alloc_log("pthread_mutex_unlock fail!\n");
	return;
}

void icr_bufmgr_deinit(icr_alloc_t* alloc) {
	if (alloc->fd) {
		close(alloc->fd);
	}

	if(pthread_mutex_destroy(&alloc->lock) != 0)
		alloc_log("pthread_mutex_destroy fail!\n");
	if(pthread_mutexattr_destroy(&alloc->mutexattr) != 0)
		alloc_log("pthread_mutexattr_destroy fail!\n");

	free(alloc);
	return;
}

bool icr_bufmgr_map(icr_alloc_t* alloc, icr_buffer_t* buffer) {
	if (buffer->mapped) {
		return true;
	}

	if(pthread_mutex_lock(&alloc->lock) != 0)
		alloc_log("pthread_mutex_lock fail!\n");

	void *addr;
	uint32_t handle = -1;
	int ret = drmPrimeFDToHandle(alloc->fd, buffer->fds[0], &handle);
	if (ret) {
        alloc_log("icr_alloc: drmPrimeFDToHandle failed(fd = %d, prime_fd = %d, handle = %d) ret = %d\n",
            alloc->fd, buffer->fds[0], handle, ret);
		if(pthread_mutex_unlock(&alloc->lock) != 0)
			alloc_log("pthread_mutex_unlock fail!\n");
		return false;
	}

	if(alloc->has_mmap_offset) {
		bool wc = true;

		struct drm_i915_gem_mmap_offset mmap_arg = {
			.handle = handle,
			.flags = wc ? I915_MMAP_OFFSET_WC : I915_MMAP_OFFSET_WB,
		};

		/* Get the fake offset back */
		int ret = icr_gen_ioctl(alloc->fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_arg);
		if (ret != 0) {
			alloc_log("icr_alloc: DRM_IOCTL_I915_GEM_MMAP_OFFSET failed\n");
			if(pthread_mutex_unlock(&alloc->lock) != 0)
				alloc_log("pthread_mutex_unlock fail!\n");
			return false;
		}
	alloc_log("icr_alloc: DRM_IOCTL_I915_GEM_MMAP_OFFSET success\n");
		/* And map it */
#ifdef __x86_64__
		addr = mmap(0, buffer->total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
							 alloc->fd, mmap_arg.offset);
#else
		addr = mmap64(0, buffer->total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
							 alloc->fd, mmap_arg.offset);
#endif
	}
	else if (buffer->tiling_mode == I915_TILING_NONE) {
		bool wc = false;
		struct drm_i915_gem_mmap gem_map;
		memset(&gem_map, 0, sizeof(gem_map));

		gem_map.flags = wc ? I915_MMAP_WC : 0;
		gem_map.handle = handle;
		gem_map.offset = 0;
		gem_map.size = buffer->total_size;

		ret = drmIoctl(alloc->fd, DRM_IOCTL_I915_GEM_MMAP, &gem_map);
		if (ret) {
			alloc_log("icr_alloc: DRM_IOCTL_I915_GEM_MMAP failed\n");
			if(pthread_mutex_unlock(&alloc->lock) != 0)
				alloc_log("pthread_mutex_unlock fail!\n");
			return false;
		}

		addr = (void *)(uintptr_t)gem_map.addr_ptr;
	} else {
		struct drm_i915_gem_mmap_gtt gem_map;
		memset(&gem_map, 0, sizeof(gem_map));

		gem_map.handle = handle;

		ret = drmIoctl(alloc->fd, DRM_IOCTL_I915_GEM_MMAP_GTT, &gem_map);
		if (ret) {
			alloc_log("icr_alloc: DRM_IOCTL_I915_GEM_MMAP_GTT failed\n");
			if(pthread_mutex_unlock(&alloc->lock) != 0)
				alloc_log("pthread_mutex_unlock fail!\n");
			return false;
		}

		addr = mmap(0, buffer->total_size, PROT_READ | PROT_WRITE, MAP_SHARED, alloc->fd,
				    gem_map.offset);
	}

	if (addr == MAP_FAILED) {
		alloc_log("icr_alloc: i915 GEM mmap failed : %d(%s)", errno, strerror(errno));
		if(pthread_mutex_unlock(&alloc->lock) != 0)
			alloc_log("pthread_mutex_unlock fail!\n");
		return false;
	}
	alloc_log("icr_alloc: i915 GEM mmap success\n");
	buffer->map_address = addr;
	buffer->mapped = 1;

	if(pthread_mutex_unlock(&alloc->lock) != 0)
		alloc_log("pthread_mutex_unlock fail!\n");

	return true;
}

void icr_bufmgr_unmap(icr_alloc_t* alloc, icr_buffer_t* buffer) {
	if (!buffer->mapped) {
		alloc_log("icr_alloc: icr buffer not been mapped or already unmapped\n");
		return;
	}

	if(pthread_mutex_lock(&alloc->lock) != 0)
		alloc_log("pthread_mutex_lock fail!\n");

	icr_bufmgr_unmap_internal(alloc, buffer);
    
	if(pthread_mutex_unlock(&alloc->lock) != 0)
		alloc_log("pthread_mutex_unlock fail!\n");
}

