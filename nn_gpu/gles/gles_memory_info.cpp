#include <sys/mman.h>
#include "gles_memory_info.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

void GlesMemoryInfo::clean()
{
    if (ssbo != 0)
    {
        glDeleteBuffers(1, &ssbo);
        ssbo = 0;
    }
    userptr = nullptr;
}

void GlesMemoryInfo::setNotInUsing()
{
    ASSERT(refCount > 0);
    refCount--;
    if (refCount == 0)
    {
        inUsing = false;
    }
}

bool GlesMemoryInfo::sync(std::string name)
{
    if (needSync)
    {
        ASSERT(ssbo != 0);
    }

    if (name == "mmap_fd")
    {
        if (needSync)
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            uint8_t* p = (uint8_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, length, GL_MAP_READ_BIT);
            for (size_t i = 0; i < length; ++i)
            {
                userptr[i] = p[i];
            }
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            msync(userptr, length, MS_SYNC);
        }
    }
    else if (name == "ashmem")
    {
        if (needSync)
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            uint8_t* p = (uint8_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, length, GL_MAP_READ_BIT);
            for (size_t i = 0; i < length; ++i)
            {
                userptr[i] = p[i];
            }
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
    }
    else
    {
        NOT_IMPLEMENTED;
    }

    return true;
}

GLuint GlesMemoryInfo::getSSbo()
{
    if (ssbo == 0)
    {
        ASSERT(length > 0);
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, length, userptr, GL_STATIC_DRAW);
    }

    return ssbo;
}


}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
