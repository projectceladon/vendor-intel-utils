#include <sys/mman.h>
#include "gles_memory_manager.h"

NAME_SPACE_BEGIN

GlesMemoryInfo* GlesMemoryManager::createMemoryInfo(std::vector<GlesMemoryInfo>& memInfos, uint8_t* userptr, size_t length) const
{
    size_t count = memInfos.size();

    ASSERT(memInfos.capacity() > count);

    GlesMemoryInfo info(userptr, length);
    memInfos.push_back(info);

    return &memInfos[count];
}

GlesMemoryInfo* GlesMemoryManager::createRequestMemoryInfo(uint8_t* userptr, size_t length)
{
    return createMemoryInfo(requestMemInfos, userptr, length);
}

GlesMemoryInfo* GlesMemoryManager::createModelMemoryInfo(uint8_t* userptr, size_t length)
{
    return createMemoryInfo(modelMemInfos, userptr, length);
}

GlesMemoryInfo* GlesMemoryManager::createIntermediumMemoryInfo(size_t length)
{
    size_t count = intermediumMemInfos.size();
    for (size_t i = 0; i < count; i++)
    {
        GlesMemoryInfo& info = intermediumMemInfos[i];
        if (!info.inUsing && info.length == length && info.userptr == nullptr)
        {
            info.inUsing = true;
            info.refCount = 1;
            return &info;
        }
    }

    return createMemoryInfo(intermediumMemInfos, nullptr, length);
}

GlesMemoryInfo* GlesMemoryManager::createIntermediumMemoryInfo(uint8_t* userptr, size_t length)
{
    return createMemoryInfo(intermediumMemInfos, userptr, length);
}

bool GlesMemoryManager::initFromModel(const Model& model)
{
    modelPoolInfos.resize(model.pools.size());
    for (size_t i = 0; i < model.pools.size(); i++)
    {
        if (!modelPoolInfos[i].set(model.pools[i]))
        {
            LOGE("Could not map pool");
            return false;
        }
    }

    // just reserve size of big enough
    modelMemInfos.reserve(model.operands.size());
    requestMemInfos.reserve(model.operands.size());
    intermediumMemInfos.reserve(model.operands.size());
    return true;
}

bool GlesMemoryManager::resetFromRequest(const Request& request)
{
    ASSERT(requestPoolInfos.size() == request.pools.size() ||
                                requestPoolInfos.size() == 0);
    cleanPoolInfos(requestPoolInfos);
    requestMemInfos.clear();

    requestPoolInfos.resize(request.pools.size());
    for (size_t i = 0; i < request.pools.size(); i++)
    {
        if (!requestPoolInfos[i].set(request.pools[i]))
        {
            LOGE("Could not map pool");
            return false;
        }
    }

    for (auto& mem : intermediumMemInfos)
    {
        mem.resetRef();
    }

    return true;
}

bool GlesMemoryManager::sync()
{
    // should we try for modelPoolInfos?

    for (size_t i = 0; i < requestPoolInfos.size(); i++)
    {
        requestPoolInfos[i].sync();
    }
    return true;
}

void GlesMemoryManager::cleanPoolInfos(std::vector<GlesPoolInfo>& poolInfos) const
{
    for (size_t i = 0; i < poolInfos.size(); i++)
    {
        poolInfos[i].clean();
    }
}

void GlesMemoryManager::clean()
{
    cleanPoolInfos(modelPoolInfos);
    cleanPoolInfos(requestPoolInfos);

    for (auto& mem : intermediumMemInfos)
    {
        mem.clean();
    }
}

NAME_SPACE_STOP