#include "gles_cs_program_manager.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

void GlesCsProgramManager::getProgName(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyBasic* key = reinterpret_cast<const GlesCsProgramKeyBasic*>(progKey);
    switch (key->opType)
    {

#define SETUP_OP(op)                            \
    case OperationType::op:                     \
        getProgName##op(progKey, name);      \
        break;
#include "setup_op.hxx"
#undef SETUP_OP

        default:
            NOT_IMPLEMENTED;
            break;
    }
}

void GlesCsProgramManager::getShaderSource(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyBasic* key = reinterpret_cast<const GlesCsProgramKeyBasic*>(progKey);
    switch (key->opType)
    {

#define SETUP_OP(op)                            \
    case OperationType::op:                     \
        getShaderSource##op(progKey, src);      \
        break;
#include "setup_op.hxx"
#undef SETUP_OP

        default:
            NOT_IMPLEMENTED;
            break;
    }
}

void GlesCsProgramManager::clean()
{
    glUseProgram(0);
    for(auto &kv: programs)
    {
        glDeleteProgram(kv.second);
    }
    programs.clear();
}

GLuint GlesCsProgramManager::createProgram(const char* pSource)
{
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &pSource, nullptr);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        ALOGE("shader is not compiled successfully with following log:");
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        char* buf = (char*) malloc(infoLen);
        glGetShaderInfoLog(shader, infoLen, nullptr, buf);
        ALOGE("%s\nEND OF LOG", buf);
        free(buf);
        glDeleteShader(shader);
        ALOGV("shader source code:\n%s\nEND OF SOURCE CODE", pSource);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glDeleteShader(shader);
    glLinkProgram(program);

    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE)
    {
        ALOGE("program is not linked successfully with following log:");
        GLint bufLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
        char* buf = (char*) malloc(bufLength);
        glGetProgramInfoLog(program, bufLength, nullptr, buf);
        ALOGE("%s\nEND OF LOG", buf);
        free(buf);
        glDeleteProgram(program);
        ALOGV("shader source code:\n%s\nEND OF SOURCE CODE", pSource);
        return 0;
    }

    return program;
}

GLuint GlesCsProgramManager::getProgram(const void* key)
{
    std::string progName;
    getProgName(key, progName);
    if (programs.find(progName) != programs.end())
    {
        return programs[progName];
    }

    std::string src;
    getShaderSource(key, src);

    GLuint prog = createProgram(src.c_str());
    programs[progName] = prog;
    return prog;
}

void GlesCsProgramManager::deleteProgram(std::string& progName)
{
    std::map<std::string, GLuint>::iterator it = programs.find(progName);
    if (it != programs.end())
    {
        glUseProgram(0);
        glDeleteProgram(it->second);
        programs.erase(it);
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
