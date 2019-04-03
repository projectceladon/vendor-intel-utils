#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_CS_PROGRAM_MANAGER_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_CS_PROGRAM_MANAGER_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>

#include "base_executor.h"
#include "gles_cs_program_key.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class GlesCsProgramManager
{
public:
    GlesCsProgramManager() {}
    ~GlesCsProgramManager() {}

#if 0
    GLuint getProgram(const void* key, size_t keysize);
    void deleteProgram(const void* key, size_t keysize);
#endif
    GLuint getProgram(const void* key);
    void deleteProgram(std::string& progName);
    void getProgName(const void* key, std::string& name);
    void clean();

private:
    struct ProgramKey
    {
        ProgramKey(const void* key, size_t keysize)
        {
            ASSERT(keysize > 0 && key != nullptr);
            ASSERT(sizeof(data) >= keysize);
            size = keysize;
            memset(data, 0, sizeof(data));
            memcpy(data, key, size);
        }

        uint8_t data[128];
        size_t size;

        bool operator<(const ProgramKey& rhs) const
        {
            if (size < rhs.size)
            {
                return true;
            }
            else if (size > rhs.size)
            {
                return false;
            }

            for (size_t i = 0; i < size; ++i)
            {
                if (data[i] < rhs.data[i])
                {
                    return true;
                }
                else if (data[i] > rhs.data[i])
                {
                    return false;
                }
            }
            return false;
        }

        bool operator==(const ProgramKey& rhs) const
        {
            if (size != rhs.size)
            {
                return false;
            }

            for (size_t i = 0; i < size; ++i)
            {
                if (data[i] != rhs.data[i])
                {
                    return false;
                }
            }
            return true;
        }
    };

    //std::map<ProgramKey, GLuint> programs;
    std::map<std::string, GLuint> programs;

    GLuint createProgram(const char* pSource);

    //void getShaderSource(const ProgramKey& progKey, std::string& src);
    void getShaderSource(const void* progKey, std::string& src);

#define SETUP_OP(op) \
    void getShaderSource##op(const void* progKey, std::string& src); \
    void getProgName##op(const void* progKey, std::string& name); 
#include "setup_op.hxx"
#undef SETUP_OP

};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
