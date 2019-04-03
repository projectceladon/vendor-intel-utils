
Intel® Neural Networks GPU HAL
===

Introduction
---

The Intel® Neural Network GPU Hardware Abstraction Layer (NN HAL) provides the GPU hardware acceleration for Android Neural Networks (NN) API. Intel Neural Networks GPU HAL takes the advantage of the Intel GEN Graphics engine, enables high performance and low power implementation of Neural Networks API.<br>
For the first version, Intel® Neural Network GPU HAL utilizes Intel® OpenGL Driver Compute Shader for TensorFlow Operations acceleration. Plan to switch to utilize Intel® Vulkan Driver in the next stage.

Supported Operations
---

Intel MKL-DNN Neural Networks HAL supports essential Android Neural Networks Operations. Below are the operations supported by Intel Neural Networks GPU HAL:

* ANEURALNETWORKS_ADD
* ANEURALNETWORKS_MUX
* ANEURALNETWORKS_LOGISTIC
* ANEURALNETWORKS_CONV_2D
* ANEURALNETWORKS_DEPTHWISE_CONV_2D
* ANEURALNETWORKS_AVERAGE_POOL_2D
* ANEURALNETWORKS_MAX_POOL_2D
* ANEURALNETWORKS_SOFTMAX
* ANEURALNETWORKS_RESHAPE
* ANEURALNETWORKS_CONCATENATION
* ANEURALNETWORKS_LOCAL_RESPONSE_NORMALIZATION

Prerequisite
---

[Intel® Mesa Driver](https://github.com/projectceladon/external-mesa)

Validated Models
---

TBD

Known Issues
---

Only support TENSOR_FLOAT32 type Operands.

License
---

Intel® Neural Networks GPU HAL is distributed under the Apache License, Version 2.0 You may obtain a copy of the License at: http://www.apache.org/licenses/LICENSE-2.0

How to provide feedback
---

By default, please submit an issue using native github.com interface: https://github.com/projectceladon/nn_gpu/issues

How to contribute
---

Create a pull request on github.com with your patch. Make sure your change is cleanly building and passing ULTs. A maintainer will contact you if there are questions or concerns.
