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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_SPV_SHADER_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_SPV_SHADER_H

NAME_SPACE_BEGIN

extern const unsigned int elewise_spv[890];
extern const unsigned int conv_spv[1700];
extern const unsigned int concat_spv[626];
extern const unsigned int softmax_spv[900];
extern const unsigned int avg_pool_spv[2057];
extern const unsigned int max_pool_spv[1894];
extern const unsigned int lrn_spv[1730];
extern const unsigned int dw_conv_spv[2231];
extern const unsigned int logistic_spv[368];
extern const unsigned int conv_chn3to4_spv[729];
extern const unsigned int conv_gemmShader4_8_spv[7691];
extern const unsigned int conv_gemm1_spv[1320];

NAME_SPACE_STOP

#endif
