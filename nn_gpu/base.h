#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_BASE_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_BASE_H

#include <android/log.h>
#include <android-base/logging.h>

#define NAME_SPACE_BEGIN \
namespace android { \
namespace hardware { \
namespace neuralnetworks { \
namespace V1_2 { \
namespace implementation {

#define NAME_SPACE_STOP \
} \
} \
} \
} \
}

static const char* kTAG = "NN_GPU_HAL";

#define NN_TRACE 0

#define NN_GPU_ENTRY()                                                  \
    do {                                                                \
        if (NN_TRACE)                                                   \
            LOG(INFO) << "NN_GPU_HAL enter: " << __FUNCTION__;          \
    } while(0);

#define NN_GPU_EXIT()                                                   \
    do {                                                                \
        if (NN_TRACE)                                                   \
            LOG(INFO) << "NN_GPU_HAL exit: " << __FUNCTION__;           \
    } while(0);

#define NN_GPU_CALL()                                                   \
        do {                                                            \
            if (NN_TRACE)                                               \
                LOG(INFO) << "NN_GPU_HAL call: " << __FUNCTION__;       \
        } while(0);

#define NN_DEBUG 0
#define NN_GPU_DEBUG(...) \
        if (NN_DEBUG) \
            ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))

#define NN_PERF 0
#define NN_GPU_PERF(...) \
        if (NN_PERF) \
            ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))

// Android log function wrappers

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGD(...) \
      ((void)__android_log_print(ANDROID_LOG_DEBUG, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

// Assert macro, as Android does not generally support assert.
#define ASSERT(v)                                                               \
    do                                                                          \
    {                                                                           \
        if (!(v))                                                               \
        {                                                                       \
            LOGE("ASSERT failed at %s:%d - %s", __FILE__, __LINE__, #v);        \
            abort();                                                            \
        }                                                                       \
    } while(0)

// Macro to check if the input parameters for operation are valid or not.
#define NN_CHECK(v)                                                             \
          do {                                                                  \
            if (!(v)) {                                                         \
              LOGE("NN_CHECK failed: File[%s], line[%d]", __FILE__, __LINE__);  \
              return false;                                                     \
            }                                                                   \
          } while(0);

#define NN_CHECK_EQ(actual, expected)           \
          NN_CHECK((actual) == (expected))

#define NN_OPS_CHECK NN_CHECK

#define CHECKGLERROR()                                              \
            do {                                                    \
                GLenum err = glGetError();                          \
                if (err != GL_NO_ERROR)                             \
                {                                                   \
                    LOGE("glGetError returns error 0x%x at %s:%d", err, __FILE__, __LINE__); \
                }                                                   \
            } while(0);

#define CHECK_GL_STATE_RET()                                        \
        {                                                           \
            GLenum err = glGetError();                              \
            if (err != GL_NO_ERROR)                                 \
            {                                                       \
                LOGE("glGetError returns %d, func:%s, line: %d\n", err, __func__, __LINE__); \
                return false;                                       \
            }                                                       \
            return true;                                            \
        }

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, "Vulkan-NN ",              \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

#define VK_CHECK_RESULT(f)                               \
{                                                        \
        if (f != VK_SUCCESS)                             \
        {                                                \
            LOGE("Vulkan check failed, result = %d", f); \
        }                                                \
}

#define VKCOM_CHECK_BOOL_RET_VAL(val, ret)               \
{                                                        \
    bool res = (val);                                    \
    if (!res)                                            \
    {                                                    \
        LOGW(NULL, "Check bool failed");                 \
        return ret;                                      \
    }                                                    \
}

#define VKCOM_CHECK_POINTER_RET_VOID(p)                  \
{                                                        \
    if (NULL == (p))                                     \
    {                                                    \
        LOGW(NULL, "Check pointer failed");              \
        return;                                          \
    }                                                    \
}

#define VKCOM_CHECK_POINTER_RET_VAL(p, val)              \
{                                                        \
    if (NULL == (p))                                     \
    {                                                    \
        LOGW(NULL, "Check pointer failed");              \
        return (val);                                    \
    }                                                    \
}

#endif
