#include <sys/mman.h>
#include "gles_memory_info.h"

NAME_SPACE_BEGIN

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

void GlesMemoryInfo::dump()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    // only dump first 16 float numbers
    const float* p = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 16, GL_MAP_READ_BIT);
    for (size_t i = 0; i < 15; ++i)
    {
        NN_GPU_DEBUG("dumpped out buffer content: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    }

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void GlesMemoryInfo::dumpToFile(const char* fileName, const int channels)
{
    static uint32_t file_no = 0;
    std::string file = std::string("/data/") + std::string("gles_") + std::string(fileName)
                    + std::to_string(file_no) + ".txt";
    FILE* file_ptr = fopen(file.c_str(), "w+");

    file_no++;

    if (file_ptr == nullptr)
    {
        NN_GPU_DEBUG("call %s, create file %s failed", __func__, file.c_str());
        return;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    const size_t f_len = length / 4;
    const float* fp = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, f_len, GL_MAP_READ_BIT);
    int cur_c = 1;

    NN_GPU_DEBUG("%s: dumpped file length is %zu", __func__, f_len);

    for (size_t i = 0; i < f_len; i++)
    {
        fprintf(file_ptr, "%f,", fp[i]);
        if (channels != 0 && cur_c % channels == 0) {
            fprintf(file_ptr, "\n");
        }
        ++cur_c;
    }

    fclose(file_ptr);

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
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

NAME_SPACE_STOP
