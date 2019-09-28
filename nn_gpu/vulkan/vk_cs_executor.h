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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_CS_EXECUTOR_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_CS_EXECUTOR_H

#include "gpu_executor.h"
#include "vk_operand.h"
#include "vk_memory_manager.h"
#include "vk_op_base.h"
#include "operation_cpu_timer.h"

NAME_SPACE_BEGIN

struct ShaderConfig
{
public:
    ShaderConfig():
        local_size_x(0), local_size_y(0), local_size_z(0),
        block_width(0), block_height(0), block_depth(0)
    {};

    ShaderConfig(const int lsz_x,
                 const int lsz_y,
                 const int lsz_z,
                 const int block_w,
                 const int block_h,
                 const int block_d):
        local_size_x(lsz_x), local_size_y(lsz_y), local_size_z(lsz_z),
        block_width(block_w), block_height(block_h), block_depth(block_d)
    {};

    int local_size_x;
    int local_size_y;
    int local_size_z;
    int block_width;
    int block_height;
    int block_depth;
};

struct VkConvSpecializedConst {
public:
    VkConvSpecializedConst():
        local_sz_x(0), local_sz_y(0), local_sz_z(0), in_h(0), in_w(0), out_h(0), out_w(0),
        stride_h(0), stride_w(0), pad_h(0), pad_w(0), filter_h(0), filter_w(0), channels(0),
        batch(0), m(0), k(0), n(0), activation(0), num_items(0), tail_m(0)
    {};

    VkConvSpecializedConst(int ih, int iw, int oh, int ow, int fh,
                           int fw, int chn, int bat, int M, int K,
                           int N, int tm):
        local_sz_x(0), local_sz_y(0), local_sz_z(0), in_h(ih), in_w(iw), out_h(oh), out_w(ow),
        stride_h(0), stride_w(0), pad_h(0), pad_w(0), filter_h(fh), filter_w(fw),
        channels(chn), batch(bat), m(M), k(K), n(N), activation(0), num_items(0), tail_m(tm)
    {};

    int local_sz_x;
    int local_sz_y;
    int local_sz_z;
    int in_h;
    int in_w;
    int out_h;
    int out_w;
    int stride_h;
    int stride_w;
    int pad_h;
    int pad_w;
    int filter_h;
    int filter_w;
    int channels;
    int batch;
    int m;
    int k;
    int n;
    int activation;
    int num_items;    // for chn3tochn4
    int tail_m;       // for gemm_4_4 & gemm_4_8
};

class VkCsExecutor : public GpuExecutor
{
public:
    static bool initPerProcess();
    static void deinitPerProcess();
    static void getCapabilities(V1_0::Capabilities& cap);
    static std::vector<bool> getSupportedOperations(const Model& model);
    //static bool checkGroupParam(uint32_t* localSize, uint32_t* groupCount);

    VkCsExecutor(const Model& model);
    ~VkCsExecutor() override;

    bool initPerModel() override;
    bool initPerExecThread() override;
    bool run(const Request& request) override;
    void deinitPerExecThread() override;
    void deinitPerModel() override;
    std::string getOpName(const Operation& operation);

private:
    //cannot be a global memMgr per process since the gl objects belong to one context (_ctx)
    VkMemoryManager memMgr;
    std::vector<VkOperand> operands;
    std::vector<OperationCpuTimer> operationTimers;
    std::shared_ptr<VkOpBase> opBase;

    void initOperands();
    void restoreOperands();
    void setArgOperands(const Request& request);

    void initOperationTimers();
    void showOperationTimers();
    void deinitOperationResources();

    bool run(const Operation& operation, OperationCpuTimer* timer);

    bool doEleWise(const Operation& operation, const int type);
    bool convolve(const Operation& operation, ShaderConfig& config);
    bool depthConvolve(const Operation& operation, ShaderConfig& config);
    bool doPool(const Operation& operation, ShaderConfig& config, const int type);

    // for convolve tuning
    void tune(VkConvSpecializedConst& param, ShaderConfig& conf,
              VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out);
    bool tuning_convolve(VkConvSpecializedConst& param,
                         const ShaderConfig& conf,
                         VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out);
    bool tryShaderConfig(VkConvSpecializedConst& param,
                         ShaderConfig& best, const std::vector<ShaderConfig>& configs,
                         VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out);
    void prepareShaderConfig(VkConvSpecializedConst& convParam, ShaderConfig& conf,
                             VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out);
    bool verifyShader(VkConvSpecializedConst& param, ShaderConfig& conf,
                      VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out);
    bool verifyResult(VkConvSpecializedConst& param,
                      float* in_buffer, float* filter_buffer, float* bias_buffer, float* result_buffer);

#define SETUP_OP(op) bool do##op(const Operation& operation);
#include "vk_setup_op.hxx"
#undef SETUP_OP
};

NAME_SPACE_STOP

#endif
