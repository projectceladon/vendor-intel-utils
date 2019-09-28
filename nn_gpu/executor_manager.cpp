#include "executor_manager.h"
#include "gles/gles_cs_executor.h"
#include "vulkan/vk_cs_executor.h"

NAME_SPACE_BEGIN

ExecutorManager::ExecutorType ExecutorManager::type = ExecutorManager::ET_GLES_CS;

bool ExecutorManager::initPerProcess()
{
    NN_GPU_CALL();

    char prop[PROPERTY_VALUE_MAX] = "\0";

    if (property_get("nn.gpgpu.vulkan", prop, nullptr) > 0)
    {
        int flag = -1;
        sscanf(prop, "%d", &flag);

        if (flag == 1)
        {
            LOGD("ExecutorManager: switched to vulkan backend from nn.gpgpu.vulkan");
            type = ET_VK_CS;
            return VkCsExecutor::initPerProcess();
        }
        else if (flag == 0)
        {
            LOGD("ExecutorManager: switched to gles backend from nn.gpgpu.vulkan");
            type = ET_GLES_CS;
            return GlesCsExecutor::initPerProcess();
        }
    }
    else
    {
        LOGD("ExecutorManager: gles backend by default");
        type = ET_GLES_CS;
        return GlesCsExecutor::initPerProcess();
    }

    return false;
}

void ExecutorManager::deinitPerProcess()
{
    NN_GPU_ENTRY();
    if (type == ET_GLES_CS)
    {
        GlesCsExecutor::deinitPerProcess();
    }
    else if (type == ET_VK_CS)
    {
        VkCsExecutor::deinitPerProcess();
    }
    NN_GPU_EXIT();
}

void ExecutorManager::getCapabilities(V1_0::Capabilities &cap)
{
    NN_GPU_ENTRY();
    if (type == ET_GLES_CS)
    {
        GlesCsExecutor::getCapabilities(cap);
    }
    else if (type == ET_VK_CS)
    {
        VkCsExecutor::getCapabilities(cap);
    }

    // dynamically getprop from "nn.gpgpu.cap" for test purpose
    char prop[PROPERTY_VALUE_MAX] = "\0";
    if (property_get("nn.gpgpu.cap", prop, nullptr) > 0)
    {
        float performance = 0.0f;
        sscanf(prop, "%f", &performance);
        LOGD("ExecutorManager: get performance from nn.gpgpu.cap, cap is %f", performance);

        if (performance > 0.0f && performance < 10.0f)
        {
            LOGD("ExecutorManager: get performance from nn.gpgpu.cap %f", performance);

            cap = {.float32Performance = {.execTime = performance, .powerUsage = performance},
                   .quantized8Performance = {.execTime = performance, .powerUsage = performance}};
        }
    }

    NN_GPU_EXIT();
}

std::vector<bool> ExecutorManager::getSupportedOperations(const Model& model)
{
    NN_GPU_CALL();
    if (type == ET_GLES_CS)
    {
        return GlesCsExecutor::getSupportedOperations(model);
    }
    else if (type == ET_VK_CS)
    {
        return VkCsExecutor::getSupportedOperations(model);
    }
	else
	{
        const size_t count = model.operations.size();
        std::vector<bool> supported(count, false);
	    return supported;
	}
}

BaseExecutor* ExecutorManager::createExecutor(const Model& model)
{
    NN_GPU_CALL();
    if (type == ET_GLES_CS)
    {
        return new GlesCsExecutor(model);
    }
    else if (type == ET_VK_CS)
    {
        return new VkCsExecutor(model);
    }

	return NULL;
}

NAME_SPACE_STOP
