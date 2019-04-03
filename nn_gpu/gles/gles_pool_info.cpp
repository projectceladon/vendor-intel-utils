#include <sys/mman.h>
#include "gles_pool_info.h"
#include "gles_memory_info.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool GlesPoolInfo::clean()
{
    ASSERT(userptr != nullptr);
    if (name == "mmap_fd")
    {
        munmap(userptr, size);
    }
    userptr = nullptr;

    for (auto& mem : memInfos)
    {
        mem->clean();
    }
    memInfos.clear();

    return true;
}

bool GlesPoolInfo::sync()
{
    if (name == "mmap_fd")
    {
        int prot = hidlMemory.handle()->data[1];
        if (prot & PROT_WRITE)
        {
            for (auto& mem : memInfos)
            {
                mem->sync(name);
            }
        }
    }
    else if (name == "ashmem")
    {
        for (auto& mem : memInfos)
        {
            mem->sync(name);
        }
        memory->commit();
    }
    else
    {
        NOT_IMPLEMENTED;
    }

    return true;
}

bool GlesPoolInfo::set(const hidl_memory& hidlMemory)
{
    this->hidlMemory = hidlMemory;
    name = hidlMemory.name();
    if (name == "mmap_fd")
    {
        size = hidlMemory.size();
        int fd = hidlMemory.handle()->data[0];
        int prot = hidlMemory.handle()->data[1];
        size_t offset = getSizeFromInts(hidlMemory.handle()->data[2],
                                        hidlMemory.handle()->data[3]);
        userptr = static_cast<uint8_t*>(mmap(nullptr, size, prot, MAP_SHARED, fd, offset));
        if (userptr == MAP_FAILED)
        {
            ALOGE("Can't mmap the file descriptor.");
            return false;
        }
        return true;
    }
    else if (name == "ashmem")
    {
        memory = mapMemory(hidlMemory);
        if (memory == nullptr) {
            ALOGE("Can't map shared memory.");
            return false;
        }
        memory->update();
        userptr = reinterpret_cast<uint8_t*>(static_cast<void*>(memory->getPointer()));
        if (userptr == nullptr) {
            ALOGE("Can't access shared memory.");
            return false;
        }
        return true;
    }
    else
    {
        NOT_IMPLEMENTED;
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
