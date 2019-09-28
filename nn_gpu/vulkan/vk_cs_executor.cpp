/*
 * Copyright @2017 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "vk_cs_executor.h"
#include "vk_wrapper.h"
#include "vk_op_base.h"
#include "vk_cpu_timer.h"

NAME_SPACE_BEGIN

// We will call this function the window is opened.
// This is where we will initialise everything
bool initialized = false;

//bool enableValidationLayers = false;
VkInstance kInstance;
VkPhysicalDevice kPhysicalDevice;
VkPhysicalDeviceProperties kDeviceProps;
VkDevice kDevice;
VkQueue kQueue;
VkCommandPool kCmdPool;
//VkDebugReportCallbackEXT kDebugReportCallback;
uint32_t kQueueFamilyIndex;
//std::vector<const char *> kEnabledLayers;

static uint32_t getComputeQueueFamilyIndex()
{
    uint32_t queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(kPhysicalDevice, &queueFamilyCount, NULL);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(kPhysicalDevice,
                                             &queueFamilyCount,
                                             queueFamilies.data());

    uint32_t i = 0;
    for (; i < queueFamilies.size(); ++i)
    {
        VkQueueFamilyProperties props = queueFamilies[i];

        if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            break;
        }
    }

    if (i == queueFamilies.size())
    {
        LOGE("could not find a queue family that supports operations");
    }

    return i;
}

bool VkCsExecutor::initPerProcess()
{
    NN_GPU_CALL();

    if (initialized) {
        NN_GPU_DEBUG("have already initialized, directly return");
        return true;
    }
    // Load Android vulkan and retrieve vulkan API function pointers
    if (!InitVulkan()) {
        LOGE("Vulkan is unavailable, install vulkan and re-start");
        return false;
    }
	
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .apiVersion = VK_MAKE_VERSION(1, 0, 0),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .pApplicationName = "Vk NN GPU",
        .pEngineName = "vk_nn",
    };
	
    // prepare necessary extensions: Vulkan on Android need these to function
    std::vector<const char *> instanceExt, deviceExt;
#if 0
    instanceExt.push_back("VK_KHR_surface");
    instanceExt.push_back("VK_KHR_android_surface");
    deviceExt.push_back("VK_KHR_swapchain");
#endif

    // Create the Vulkan instance
    VkInstanceCreateInfo instanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(instanceExt.size()),
        .ppEnabledExtensionNames = instanceExt.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
    };
    CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &kInstance));
	
    // Find one GPU to use:
    // On Android, every GPU device is equal -- supporting
    // graphics/compute/present
    // for this sample, we use the very first GPU device found on the system
    uint32_t gpuCount = 0;
    VkPhysicalDevice tmpGpus[32];
    CALL_VK(vkEnumeratePhysicalDevices(kInstance, &gpuCount, nullptr));

    if (gpuCount > 0)
    {
        CALL_VK(vkEnumeratePhysicalDevices(kInstance, &gpuCount, tmpGpus));
        kPhysicalDevice = tmpGpus[0];  // Pick up the first GPU Device
    }
    else
    {
        initialized = false;
        NN_GPU_DEBUG("enumerate physical devices failed");
        return false;
    }

    // check for vulkan info on this GPU device
    vkGetPhysicalDeviceProperties(kPhysicalDevice, &kDeviceProps);
    NN_GPU_DEBUG("Vulkan Physical Device Name: %s", kDeviceProps.deviceName);
    NN_GPU_DEBUG("Vulkan Physical Device Info: apiVersion: %x \n\t driverVersion: %x",
        kDeviceProps.apiVersion, kDeviceProps.driverVersion);
    NN_GPU_DEBUG("API Version Supported: %d.%d.%d",
        VK_VERSION_MAJOR(kDeviceProps.apiVersion),
        VK_VERSION_MINOR(kDeviceProps.apiVersion),
        VK_VERSION_PATCH(kDeviceProps.apiVersion));
    NN_GPU_DEBUG("limit invocations is %d, group size limit is %d, %d, %d, group count is %d, %d, %d",
        kDeviceProps.limits.maxComputeWorkGroupInvocations,
        kDeviceProps.limits.maxComputeWorkGroupSize[0],
        kDeviceProps.limits.maxComputeWorkGroupSize[1],
        kDeviceProps.limits.maxComputeWorkGroupSize[2],
        kDeviceProps.limits.maxComputeWorkGroupCount[0],
        kDeviceProps.limits.maxComputeWorkGroupCount[1],
        kDeviceProps.limits.maxComputeWorkGroupCount[2]); 

    kQueueFamilyIndex = getComputeQueueFamilyIndex();
	
    // Create a logical device from GPU we picked
    float priorities[] = { 1.0f };

    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCount = 1,
        .queueFamilyIndex = kQueueFamilyIndex,
        .pQueuePriorities = priorities,
    };
	
    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExt.size()),
        .ppEnabledExtensionNames = deviceExt.data(),
        .pEnabledFeatures = nullptr,
    };
	
    CALL_VK(vkCreateDevice(kPhysicalDevice, &deviceCreateInfo, nullptr, &kDevice));
    NN_GPU_DEBUG("device is 0x%llx", reinterpret_cast<unsigned long long>(kDevice));

    // Get a handle to the only member of the queue family.
    vkGetDeviceQueue(kDevice, kQueueFamilyIndex, 0, &kQueue);

    // create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // the queue family of this command pool. All command buffers allocated from this command pool,
    // must be submitted to queues of this family ONLY.
    commandPoolCreateInfo.queueFamilyIndex = kQueueFamilyIndex;
    VK_CHECK_RESULT(vkCreateCommandPool(kDevice, &commandPoolCreateInfo, NULL, &kCmdPool));

    initialized = true;

    NN_GPU_EXIT();

    return true;
}

void VkCsExecutor::deinitPerProcess()
{
    NN_GPU_CALL();

    vkDestroyCommandPool(kDevice, kCmdPool, NULL);
	vkDestroyDevice(kDevice, nullptr);
	vkDestroyInstance(kInstance, nullptr);
	initialized = false;
}

VkCsExecutor::VkCsExecutor(const Model& model) :
                        GpuExecutor(model)
{

}

VkCsExecutor::~VkCsExecutor()
{

}

bool VkCsExecutor::initPerModel()
{
    NN_GPU_CALL();

    memMgr.initFromModel(model);
    initOperands();
    initOperationTimers();

    return true;
}

void VkCsExecutor::initOperationTimers()
{
    operationTimers.resize(model.operations.size());
    for (size_t i = 0; i < operationTimers.size(); ++i)
    {
        const Operation& operation = model.operations[i];
        operationTimers[i].set(i, getOpName(operation));
    }
}

void VkCsExecutor::showOperationTimers()
{
    float sum = 0.f;
    for (size_t i = 0; i < operationTimers.size(); ++i)
    {
        OperationCpuTimer& timer = operationTimers[i];
        long avg = timer.getAvg();
        if (avg == 0)   // it means that no timer recorded
        {
            return;
        }
        sum += avg;
    }

    std::sort(operationTimers.begin(), operationTimers.end(), OperationCpuTimer::sort);

    NN_GPU_PERF("each operation average CPU time (with glFinish), the first run ignored when run more than once");
    long acc = 0;
    for (size_t i = 0; i < operationTimers.size(); ++i)
    {
        OperationCpuTimer& timer = operationTimers[i];
        acc = timer.show(acc, sum);
    }
    NN_GPU_PERF("all operations average sum: %f ms", sum/1000.f);
}

void VkCsExecutor::deinitPerModel()
{
    NN_GPU_CALL();

    showOperationTimers();

    memMgr.clean();
}

bool VkCsExecutor::initPerExecThread()
{
    NN_GPU_CALL();
    return true;
}

void VkCsExecutor::deinitPerExecThread()
{

}

void VkCsExecutor::initOperands()
{
    NN_GPU_CALL();

    const size_t count = model.operands.size();
    operands.resize(count, memMgr);

    for (size_t i = 0; i < count; i++)
    {
        const Operand& from = model.operands[i];
        VkOperand& to = operands[i];
        to.set(from, const_cast<uint8_t*>(&model.operandValues[from.location.offset]), i);
    }
}

void VkCsExecutor::restoreOperands()
{
    NN_GPU_CALL();

    const size_t count = model.operands.size();

    for (size_t i = 0; i < count; i++)
    {
        const Operand& from = model.operands[i];
        VkOperand& to = operands[i];
        to.restore(from);
    }
}

void VkCsExecutor::setArgOperands(const Request& request)
{
    NN_GPU_CALL();

    auto updateForArguments = [this](const hidl_vec<uint32_t>& indexes,
                                  const hidl_vec<RequestArgument>& arguments) {
        ASSERT(indexes.size() == arguments.size());
        for (size_t i = 0; i < indexes.size(); i++)
        {
            const RequestArgument& from = arguments[i];
            const size_t operandIndex = indexes[i];
            VkOperand& to = operands[operandIndex];
            to.setArg(from);
        }
    };
    updateForArguments(model.inputIndexes, request.inputs);
    updateForArguments(model.outputIndexes, request.outputs);
}

bool VkCsExecutor::run(const Operation& operation, OperationCpuTimer* timer)
{
    NN_GPU_CALL();

    VkCpuTimer t(timer);

    //maybe some checking here
    const hidl_vec<uint32_t>& inputs = operation.inputs;
    const hidl_vec<uint32_t>& outputs = operation.outputs;

    bool ret = true;

	opBase.reset(new VkOpBase());

    switch (operation.type)
    {

#define SETUP_OP(op)                \
    case OperationType::op:         \
        NN_GPU_DEBUG("run operation type with %d", OperationType::op); \
        ret = do##op(operation);    \
        break;
#include "vk_setup_op.hxx"
#undef SETUP_OP

    default:
        NOT_IMPLEMENTED;
        break;
    }

    for (uint32_t i : inputs)
    {
        operands[i].markOpFinished();
    }

    for (uint32_t i : outputs)
    {
        UNUSED(i);
        //operands[i].retrieveData();
    }

    return ret;
}

bool VkCsExecutor::run(const Request& request)
{
    restoreOperands();
    memMgr.resetFromRequest(request);
    setArgOperands(request);
    for (size_t i = 0; i < model.operations.size(); ++i)
    {
        const Operation& operation = model.operations[i];
        NN_GPU_DEBUG("run loop on Operation %d", operation.type);
        OperationCpuTimer* timer = &operationTimers[i];
        if (!run(operation, timer))
        {
            return false;
        }
    }
    memMgr.sync();
    return true;
}

void VkCsExecutor::getCapabilities(V1_0::Capabilities &cap)
{
    NN_GPU_CALL();

    cap = {.float32Performance = {.execTime = 0.91f, .powerUsage = 0.91f},
           .quantized8Performance = {.execTime = 0.91f, .powerUsage = 0.91f}};
}

std::vector<bool> VkCsExecutor::getSupportedOperations(const Model& model)
{
    NN_GPU_CALL();

    const size_t count = model.operations.size();
    std::vector<bool> supported(count, true);
    for (size_t i = 0; i < count; ++i)
    {
        const Operation& operation = model.operations[i];
        const hidl_vec<uint32_t>& inputs = operation.inputs;
        for (auto input : inputs)
        {
            if (model.operands[input].type == OperandType::TENSOR_QUANT8_ASYMM)
            {
                LOGW("data type TENSOR_QUANT8_ASYMM not supported.");
                supported[i] = false;
                break;
            }
        }

        switch (operation.type)
        {

#define SETUP_OP(op)                    \
        case OperationType::op:         \
            /* do nothing */            \
            supported[i] = true; \
            break;
#include "vk_setup_op.hxx"
#undef SETUP_OP

        default:
            supported[i] = false;
            break;
        }
    }

    return supported;
}

std::string VkCsExecutor::getOpName(const Operation& operation)
{
    switch (operation.type)
    {

#define SETUP_OP(op)                \
    case OperationType::op:         \
        return std::string(#op);    \
        break;
#include "vk_setup_op.hxx"
#undef SETUP_OP

    default:
        break;
    }
    return "unknown";
}

NAME_SPACE_STOP
