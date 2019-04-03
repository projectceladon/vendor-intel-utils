#include <algorithm>
#include <memory.h>
#include <string.h>

#include <android/log.h>
#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>

#include "device.h"
#include "prepare_model.h"
#include "executor_manager.h"
#include "validate.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

const char* kOperationNames[kNumberOfOperationTypes] = {
        "ADD",
        "AVERAGE_POOL",
        "CONCATENATION",
        "CONV",
        "DEPTHWISE_CONV",
        "DEPTH_TO_SPACE",
        "DEQUANTIZE",
        "EMBEDDING_LOOKUP",
        "FLOOR",
        "FULLY_CONNECTED",
        "HASHTABLE_LOOKUP",
        "L2_NORMALIZATION",
        "L2_POOL",
        "LOCAL_RESPONSE_NORMALIZATION",
        "LOGISTIC",
        "LSH_PROJECTION",
        "LSTM",
        "MAX_POOL",
        "MUL",
        "RELU",
        "RELU1",
        "RELU6",
        "RESHAPE",
        "RESIZE_BILINEAR",
        "RNN",
        "SOFTMAX",
        "SPACE_TO_DEPTH",
        "SVDF",
        "TANH",
        "BATCH_TO_SPACE_ND",
        "DIV",
        "MEAN",
        "PAD",
        "SPACE_TO_BATCH_ND",
        "SQUEEZE",
        "STRIDED_SLICE",
        "SUB",
        "TRANSPOSE",
};

template <typename EntryType, uint32_t entryCount, uint32_t entryCountOEM>
EntryType tableLookup(const EntryType (&table)[entryCount],
                      const EntryType (&tableOEM)[entryCountOEM],
                      uint32_t code)
{
    if (code < entryCount) {
        return table[code];
    } else if (code >= kOEMCodeBase && (code - kOEMCodeBase) < entryCountOEM) {
        return tableOEM[code - kOEMCodeBase];
    } else {
        nnAssert(!"tableLookup: bad code");
        return EntryType();
    }
}

#define COUNT(X) (sizeof(X) / sizeof(X[0]))

const char* kTypeNames[kNumberOfDataTypes] = {
        "FLOAT32",        "INT32",        "UINT32",
        "TENSOR_FLOAT32", "TENSOR_INT32", "TENSOR_QUANT8_ASYMM",
};

static_assert(COUNT(kTypeNames) == kNumberOfDataTypes, "kTypeNames is incorrect");

const char* kTypeNamesOEM[kNumberOfDataTypesOEM] = {
        "OEM",            "TENSOR_OEM_BYTE",
};

static_assert(COUNT(kTypeNamesOEM) == kNumberOfDataTypesOEM, "kTypeNamesOEM is incorrect");

const char* getOperandTypeName(OperandType type)
{
    uint32_t n = static_cast<uint32_t>(type);
    return tableLookup(kTypeNames, kTypeNamesOEM, n);
}

const uint32_t kSizeOfDataType[]{
        4, // ANEURALNETWORKS_FLOAT32
        4, // ANEURALNETWORKS_INT32
        4, // ANEURALNETWORKS_UINT32
        4, // ANEURALNETWORKS_TENSOR_FLOAT32
        4, // ANEURALNETWORKS_TENSOR_INT32
        1  // ANEURALNETWORKS_TENSOR_SYMMETRICAL_QUANT8
};

static_assert(COUNT(kSizeOfDataType) == kNumberOfDataTypes, "kSizeOfDataType is incorrect");

const bool kScalarDataType[]{
        true,  // ANEURALNETWORKS_FLOAT32
        true,  // ANEURALNETWORKS_INT32
        true,  // ANEURALNETWORKS_UINT32
        false, // ANEURALNETWORKS_TENSOR_FLOAT32
        false, // ANEURALNETWORKS_TENSOR_INT32
        false, // ANEURALNETWORKS_TENSOR_SYMMETRICAL_QUANT8
};

static_assert(COUNT(kScalarDataType) == kNumberOfDataTypes, "kScalarDataType is incorrect");

const uint32_t kSizeOfDataTypeOEM[]{
        0, // ANEURALNETWORKS_OEM
        1, // ANEURALNETWORKS_TENSOR_OEM_BYTE
};

static_assert(COUNT(kSizeOfDataTypeOEM) == kNumberOfDataTypesOEM,
              "kSizeOfDataTypeOEM is incorrect");

const bool kScalarDataTypeOEM[]{
        true,  // ANEURALNETWORKS_OEM
        false, // ANEURALNETWORKS_TENSOR_OEM_BYTE
};

static_assert(COUNT(kScalarDataTypeOEM) == kNumberOfDataTypesOEM,
              "kScalarDataTypeOEM is incorrect");

uint32_t sizeOfData(OperandType type, const std::vector<uint32_t>& dimensions) {
    int n = static_cast<int>(type);

    uint32_t size = tableLookup(kSizeOfDataType, kSizeOfDataTypeOEM, n);

    if (tableLookup(kScalarDataType, kScalarDataTypeOEM, n) == true) {
        return size;
    }

    for (auto d : dimensions) {
        size *= d;
    }
    return size;
}

static bool validateOperands(const hidl_vec<Operand>& operands,
                             const hidl_vec<uint8_t>& operandValues,
                             const hidl_vec<hidl_memory>& pools)
{
    uint32_t index = 0;
    MemoryAccessVerifier poolVerifier(pools);
    for (auto& operand : operands) {
        // Validate type and dimensions.
        switch (operand.type) {
            case OperandType::FLOAT32:
            case OperandType::INT32:
            case OperandType::UINT32:
            case OperandType::OEM: {
                size_t count = operand.dimensions.size();
                if (count != 0) {
                    LOG(ERROR) << "Operand " << index << ": Scalar data has dimensions of rank "
                               << count;
                    return false;
                }
                break;
            }
            case OperandType::TENSOR_FLOAT32:
            case OperandType::TENSOR_INT32:
            case OperandType::TENSOR_QUANT8_ASYMM:
            case OperandType::TENSOR_OEM_BYTE: {
                if (operand.dimensions.size() == 0) {
                    LOG(ERROR) << "Operand " << index << ": Tensor has dimensions of rank 0";
                    return false;
                }
                break;
            }
            default:
                LOG(ERROR) << "Operand " << index << ": Invalid operand type "
                           << toString(operand.type);
                return false;
        }

        // TODO Validate the numberOfConsumers.
        // TODO Since we have to validate it, there was no point in including it. For the next
        // release, consider removing unless we have an additional process in system space
        // that creates this value. In that case, it would not have to be validated.

        // Validate the scale.
        switch (operand.type) {
            case OperandType::FLOAT32:
            case OperandType::INT32:
            case OperandType::UINT32:
            case OperandType::TENSOR_FLOAT32:
                if (operand.scale != 0.f) {
                    LOG(ERROR) << "Operand " << index << ": Operand of type "
                               << getOperandTypeName(operand.type) << " with a non-zero scale ("
                               << operand.scale << ")";
                    return false;
                }
                break;
            case OperandType::TENSOR_INT32:
                // TENSOR_INT32 may be used with or without scale, depending on the operation.
                if (operand.scale < 0.f) {
                    LOG(ERROR) << "Operand " << index << ": Operand of type "
                               << getOperandTypeName(operand.type) << " with a negative scale";
                    return false;
                }
                break;
            case OperandType::TENSOR_QUANT8_ASYMM:
                if (operand.scale <= 0.f) {
                    LOG(ERROR) << "Operand " << index << ": Operand of type "
                               << getOperandTypeName(operand.type) << " with a non-positive scale";
                    return false;
                }
                break;
            default:
                // No validation for the OEM types.
                // TODO We should have had a separate type for TENSOR_INT32 that a scale
                // and those who don't.  Document now and fix in the next release.
                break;
        }

        // Validate the zeroPoint.
        switch (operand.type) {
            case OperandType::FLOAT32:
            case OperandType::INT32:
            case OperandType::UINT32:
            case OperandType::TENSOR_FLOAT32:
            case OperandType::TENSOR_INT32:
                if (operand.zeroPoint != 0) {
                    LOG(ERROR) << "Operand " << index << ": Operand of type "
                               << getOperandTypeName(operand.type) << " with an non-zero zeroPoint "
                               << operand.zeroPoint;
                    return false;
                }
                break;
            case OperandType::TENSOR_QUANT8_ASYMM:
                if (operand.zeroPoint < 0 || operand.zeroPoint > 255) {
                    LOG(ERROR) << "Operand " << index << ": Operand of type "
                               << getOperandTypeName(operand.type) << " with an invalid zeroPoint "
                               << operand.zeroPoint << ", must be in range [0, 255]";
                    return false;
                }
                break;
            default:
                // No validation for the OEM types.
                break;
        }

        // Validate the lifetime and the location.
        const DataLocation& location = operand.location;
        switch (operand.lifetime) {
            case OperandLifeTime::CONSTANT_COPY:
                if (location.poolIndex != 0) {
                    LOG(ERROR) << "Operand " << index
                               << ": CONSTANT_COPY with a non-zero poolIndex "
                               << location.poolIndex;
                    return false;
                }
                // Do the addition using size_t to avoid potential wrap-around problems.
                if (static_cast<size_t>(location.offset) + location.length > operandValues.size()) {
                    LOG(ERROR) << "Operand " << index
                               << ": OperandValue location out of range.  Starts at "
                               << location.offset << ", length " << location.length << ", max "
                               << operandValues.size();
                    return false;
                }
                break;
            case OperandLifeTime::CONSTANT_REFERENCE:
                if (!poolVerifier.validate(location)) {
                    return false;
                }
                break;
            case OperandLifeTime::TEMPORARY_VARIABLE:
            case OperandLifeTime::MODEL_INPUT:
            case OperandLifeTime::MODEL_OUTPUT:
            case OperandLifeTime::NO_VALUE:
                if (location.poolIndex != 0 || location.offset != 0 || location.length != 0) {
                    LOG(ERROR) << "Operand " << index << ": Unexpected poolIndex "
                               << location.poolIndex << ", offset " << location.offset
                               << ", or length " << location.length << " for operand of lifetime "
                               << toString(operand.lifetime);
                    return false;
                }
                break;
            default:
                LOG(ERROR) << "Operand " << index << ": Invalid lifetime "
                           << toString(operand.lifetime);
                return false;
        }

        // For constants, validate that the length is as expected. The other lifetimes
        // expect the length to be 0. Don't validate for OEM types.
        if (operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE ||
            operand.lifetime == OperandLifeTime::CONSTANT_COPY) {
            if (operand.type != OperandType::OEM &&
                operand.type != OperandType::TENSOR_OEM_BYTE) {
                uint32_t expectedLength = sizeOfData(operand.type, operand.dimensions);
                if (location.length != expectedLength) {
                    LOG(ERROR) << "Operand " << index << ": For operand " << toString(operand)
                               << " expected a size of " << expectedLength << " but got "
                               << location.length;
                    return false;
                }
            }
        }

        index++;
    }
    return true;
}


static bool validOperationType(OperationType operation)
{
    switch (operation) {
        case OperationType::ADD:
        case OperationType::AVERAGE_POOL_2D:
        case OperationType::CONCATENATION:
        case OperationType::CONV_2D:
        case OperationType::DEPTHWISE_CONV_2D:
        case OperationType::DEPTH_TO_SPACE:
        case OperationType::DEQUANTIZE:
        case OperationType::EMBEDDING_LOOKUP:
        case OperationType::FLOOR:
        case OperationType::FULLY_CONNECTED:
        case OperationType::HASHTABLE_LOOKUP:
        case OperationType::L2_NORMALIZATION:
        case OperationType::L2_POOL_2D:
        case OperationType::LOCAL_RESPONSE_NORMALIZATION:
        case OperationType::LOGISTIC:
        case OperationType::LSH_PROJECTION:
        case OperationType::LSTM:
        case OperationType::MAX_POOL_2D:
        case OperationType::MUL:
        case OperationType::RELU:
        case OperationType::RELU1:
        case OperationType::RELU6:
        case OperationType::RESHAPE:
        case OperationType::RESIZE_BILINEAR:
        case OperationType::RNN:
        case OperationType::SOFTMAX:
        case OperationType::SPACE_TO_DEPTH:
        case OperationType::SVDF:
        case OperationType::TANH:
        case OperationType::OEM_OPERATION:
            return true;
        default:
            return false;
    }
}

int validateOperandList(uint32_t count, const uint32_t* list, uint32_t operandCount,
                        const char* tag)
{
    for (uint32_t i = 0; i < count; i++) {
        if (list[i] >= operandCount) {
            LOG(ERROR) << tag << " invalid operand index at " << i << " = " << list[i]
                       << ", operandCount " << operandCount;
            return ANEURALNETWORKS_BAD_DATA;
        }
    }
    return ANEURALNETWORKS_NO_ERROR;
}

int validateOperationOperandTypes(const std::vector<Operand>& operands,
                                  uint32_t inOperandCount, const uint32_t* inOperandIndexes,
                                  const std::vector<OperandType>& inExpectedTypes,
                                  uint32_t outOperandCount, const uint32_t* outOperandIndexes,
                                  const std::vector<OperandType>& outExpectedInTypes) {
    if (inOperandCount > static_cast<uint32_t>(inExpectedTypes.size()) ||
        outOperandCount > static_cast<uint32_t>(outExpectedInTypes.size())) {
        return ANEURALNETWORKS_BAD_DATA;
    }
    for (uint32_t i = 0; i < inOperandCount; i++) {
        if (operands[inOperandIndexes[i]].type != inExpectedTypes[i]) {
            LOG(ERROR) << "Invalid input tensor type "
                       << toString(operands[inOperandIndexes[i]].type)
                       << " for input " << i << ", expected " << toString(inExpectedTypes[i]);
            return ANEURALNETWORKS_BAD_DATA;
        }
    }
    for (uint32_t i = 0; i < outOperandCount; i++) {
        if (operands[outOperandIndexes[i]].type != outExpectedInTypes[i]) {
            LOG(ERROR) << "Invalid output tensor type "
                       << toString(operands[outOperandIndexes[i]].type)
                       << " for input " << i << ", expected " << toString(outExpectedInTypes[i]);
            return ANEURALNETWORKS_BAD_DATA;
        }
    }

    return ANEURALNETWORKS_NO_ERROR;
}

int validateOperation(OperationType type,
                      uint32_t inputCount, const uint32_t* inputIndexes,
                      uint32_t outputCount, const uint32_t* outputIndexes,
                      const std::vector<Operand>& operands)
{
    int n = validateOperandList(inputCount, inputIndexes, static_cast<uint32_t>(operands.size()),
                                "ANeuralNetworksModel_addOperation inputs");
    if (n != ANEURALNETWORKS_NO_ERROR) {
        return n;
    }
    n = validateOperandList(outputCount, outputIndexes, static_cast<uint32_t>(operands.size()),
                            "ANeuralNetworksModel_addOperation outputs");
    if (n != ANEURALNETWORKS_NO_ERROR) {
        return n;
    }
    int32_t opType = static_cast<int32_t>(type);

    auto logInvalidInOutNumber = [opType, inputCount, outputCount](int expIn, int expOut) {
        LOG(ERROR) << "Invalid number of input operands ("
                   << inputCount << ", expected " << expIn << ") or output operands ("
                   << outputCount << ", expected " << expOut << ") for operation "
                   << kOperationNames[opType];
    };

    switch (type) {
        case OperationType::OEM_OPERATION: {
            return ANEURALNETWORKS_NO_ERROR;
        }
        case OperationType::ADD: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::MUL: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::FLOOR: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::DEQUANTIZE: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::DEPTHWISE_CONV_2D: {
            if ((inputCount != 11 && inputCount != 8) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands ("
                           << inputCount << ", expected 11 or 8) or output operands ("
                           << outputCount << ", expected 1) for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount == 11) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::CONV_2D: {
            if ((inputCount != 10 && inputCount != 7) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands ("
                           << inputCount << ", expected 10 or 7) or output operands ("
                           << outputCount << ", expected 1) for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount == 10) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::AVERAGE_POOL_2D: {
            if ((inputCount != 10 && inputCount != 7) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands ("
                           << inputCount << ", expected 10 or 7) or output operands ("
                           << outputCount << ", expected 1) for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount == 10) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::L2_POOL_2D: {
            if ((inputCount != 10 && inputCount != 7) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands ("
                           << inputCount << ", expected 10 or 7) or output operands ("
                           << outputCount << ", expected 1) for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount == 10) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::MAX_POOL_2D: {
            if ((inputCount != 10 && inputCount != 7) || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands ("
                           << inputCount << ", expected 10 or 7) or output operands ("
                           << outputCount << ", expected 1) for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }

            if (inputCount == 10) {
                std::vector<OperandType> explicitScalarTypes(3, OperandType::INT32);
                inExpectedTypes.insert(inExpectedTypes.end(),
                                       explicitScalarTypes.begin(),
                                       explicitScalarTypes.end());
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::RELU: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::RELU1: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::RELU6: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::TANH: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::LOGISTIC: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::SOFTMAX: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::FULLY_CONNECTED: {
            if (inputCount != 4 || outputCount != 1) {
                logInvalidInOutNumber(4, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::CONCATENATION: {
            if (inputCount < 2 || outputCount != 1) {
                LOG(ERROR) << "Invalid number of input operands ("
                           << inputCount << ", expected at least 2) or output operands ("
                           << outputCount << ", expected 1) for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes(inputCount, inputType);
            std::vector<OperandType> outExpectedTypes = {inputType};
            // The last one is the activation function.
            inExpectedTypes.back() = OperandType::INT32;
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::L2_NORMALIZATION: {
            if (inputCount != 1 || outputCount != 1) {
                logInvalidInOutNumber(1, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::LOCAL_RESPONSE_NORMALIZATION: {
            if (inputCount != 5 || outputCount != 1) {
                logInvalidInOutNumber(5, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::FLOAT32,
                                   OperandType::FLOAT32,
                                   OperandType::FLOAT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::RESHAPE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::RESIZE_BILINEAR: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::DEPTH_TO_SPACE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::SPACE_TO_DEPTH: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::EMBEDDING_LOOKUP: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[1]].type;
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_INT32,
                                                        inputType};
            std::vector<OperandType> outExpectedTypes = {inputType};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::HASHTABLE_LOOKUP: {
            if (inputCount != 3 || outputCount != 2) {
                logInvalidInOutNumber(3, 2);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[2]].type;
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_INT32,
                                                        OperandType::TENSOR_INT32,
                                                        inputType};
            std::vector<OperandType> outExpectedTypes = {inputType,
                                                         OperandType::TENSOR_QUANT8_ASYMM};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::LSH_PROJECTION: {
            if (inputCount != 4 || outputCount != 1) {
                logInvalidInOutNumber(4, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[1]].type;
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                        inputType,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_INT32};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::LSTM: {
            if (inputCount != 23 || outputCount != 4) {
                logInvalidInOutNumber(23, 4);
                return ANEURALNETWORKS_BAD_DATA;
            }
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::INT32,
                                                        OperandType::FLOAT32,
                                                        OperandType::FLOAT32};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::RNN: {
            if (inputCount != 6 || outputCount != 2) {
                logInvalidInOutNumber(6, 2);
                return ANEURALNETWORKS_BAD_DATA;
            }
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::SVDF: {
            if (inputCount != 7 || outputCount != 2) {
                logInvalidInOutNumber(7, 2);
                return ANEURALNETWORKS_BAD_DATA;
            }
            std::vector<OperandType> inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::TENSOR_FLOAT32,
                                                        OperandType::INT32,
                                                        OperandType::INT32};
            std::vector<OperandType> outExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                                         OperandType::TENSOR_FLOAT32};
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
#if 0
        case OperationType::BATCH_TO_SPACE_ND: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::SPACE_TO_BATCH_ND: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::PAD: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::SQUEEZE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::TRANSPOSE: {
            if (inputCount != 2 || outputCount != 1) {
                logInvalidInOutNumber(2, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::STRIDED_SLICE: {
            if (inputCount != 7 || outputCount != 1) {
                logInvalidInOutNumber(7, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32,
                                   OperandType::INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::DIV: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::SUB: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_FLOAT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
        case OperationType::MEAN: {
            if (inputCount != 3 || outputCount != 1) {
                logInvalidInOutNumber(3, 1);
                return ANEURALNETWORKS_BAD_DATA;
            }
            auto inputType = operands[inputIndexes[0]].type;
            std::vector<OperandType> inExpectedTypes;
            std::vector<OperandType> outExpectedTypes;
            if (inputType == OperandType::TENSOR_FLOAT32) {
                inExpectedTypes = {OperandType::TENSOR_FLOAT32,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_FLOAT32};
            } else if (inputType == OperandType::TENSOR_QUANT8_ASYMM) {
                inExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM,
                                   OperandType::TENSOR_INT32,
                                   OperandType::INT32};
                outExpectedTypes = {OperandType::TENSOR_QUANT8_ASYMM};
            } else {
                LOG(ERROR) << "Unsupported input tensor type for operation "
                           << kOperationNames[opType];
                return ANEURALNETWORKS_BAD_DATA;
            }
            return validateOperationOperandTypes(operands,
                                                 inputCount, inputIndexes,
                                                 inExpectedTypes,
                                                 outputCount, outputIndexes,
                                                 outExpectedTypes);
        }
#endif
        default:
            return ANEURALNETWORKS_BAD_DATA;
    }
}

static bool validateOperations(const hidl_vec<Operation>& operations,
                               const hidl_vec<Operand>& operands) {
    const size_t operandCount = operands.size();
    // This vector keeps track of whether there's an operation that writes to
    // each operand. It is used to validate that temporary variables and
    // model outputs will be written to.
    std::vector<bool> writtenTo(operandCount, false);
    for (auto& op : operations) {
        if (!validOperationType(op.type)) {
            LOG(ERROR) << "Invalid operation type " << toString(op.type);
            return false;
        }
        // TODO Validate the shapes and any known values. This is currently
        // done in CpuExecutor but should be done here for all drivers.
        int error =
            validateOperation(op.type, op.inputs.size(),
                              op.inputs.size() > 0 ? op.inputs.data() : nullptr, op.outputs.size(),
                              op.outputs.size() > 0 ? op.outputs.data() : nullptr, operands);
        if (error != ANEURALNETWORKS_NO_ERROR) {
            return false;
        }

        for (uint32_t i : op.outputs) {
            const Operand& operand = operands[i];
            if (operand.lifetime != OperandLifeTime::TEMPORARY_VARIABLE &&
                operand.lifetime != OperandLifeTime::MODEL_OUTPUT) {
                LOG(ERROR) << "Writing to an operand with incompatible lifetime "
                           << toString(operand.lifetime);
                return false;
            }

            // Check that we only write once to an operand.
            if (writtenTo[i]) {
                LOG(ERROR) << "Operand " << i << " written a second time";
                return false;
            }
            writtenTo[i] = true;
        }
    }
    for (size_t i = 0; i < operandCount; i++) {
        if (!writtenTo[i]) {
            const Operand& operand = operands[i];
            if (operand.lifetime == OperandLifeTime::TEMPORARY_VARIABLE ||
                operand.lifetime == OperandLifeTime::MODEL_OUTPUT) {
                LOG(ERROR) << "Operand " << i << " with lifetime " << toString(operand.lifetime)
                           << " is not being written to.";
                return false;
            }
        }
    }
    // TODO More whole graph verifications are possible, for example that an
    // operand is not use as input & output for the same op, and more
    // generally that it is acyclic.
    return true;
}

static bool validatePools(const hidl_vec<hidl_memory>& pools) {
    for (const hidl_memory& memory : pools) {
        const auto name = memory.name();
        if (name != "ashmem" && name != "mmap_fd") {
            LOG(ERROR) << "Unsupported memory type " << name;
            return false;
        }
        if (memory.handle() == nullptr) {
            LOG(ERROR) << "Memory of type " << name << " is null";
            return false;
        }
    }
    return true;
}

static bool validateModelInputOutputs(const hidl_vec<uint32_t> indexes,
                                      const hidl_vec<Operand>& operands, OperandLifeTime lifetime) {
    const size_t operandCount = operands.size();
    for (uint32_t i : indexes) {
        if (i >= operandCount) {
            LOG(ERROR) << "Model input or output index out of range: " << i << "/" << operandCount;
            return false;
        }
        const Operand& operand = operands[i];
        if (operand.lifetime != lifetime) {
            LOG(ERROR) << "Model input or output has lifetime of " << toString(operand.lifetime)
                       << " instead of the expected " << toString(lifetime);
            return false;
        }
    }

    std::vector<uint32_t> sortedIndexes = indexes;
    std::sort(sortedIndexes.begin(), sortedIndexes.end());
    auto adjacentI = std::adjacent_find(sortedIndexes.begin(), sortedIndexes.end());
    if (adjacentI != sortedIndexes.end()) {
        LOG(ERROR) << "Model input or output occurs multiple times: " << *adjacentI;
        return false;
    }
    return true;
}

bool validateModel(const Model& model)
{
    return (validateOperands(model.operands, model.operandValues, model.pools) &&
            validateOperations(model.operations, model.operands) &&
            validateModelInputOutputs(model.inputIndexes, model.operands,
                                      OperandLifeTime::MODEL_INPUT) &&
            validateModelInputOutputs(model.outputIndexes, model.operands,
                                      OperandLifeTime::MODEL_OUTPUT) &&
            validatePools(model.pools));
}

// Validates the arguments of a request. type is either "input" or "output" and is used
// for printing error messages. The operandIndexes is the appropriate array of input
// or output operand indexes that was passed to the ANeuralNetworksModel_identifyInputsAndOutputs.
static bool validateRequestArguments(const hidl_vec<RequestArgument>& requestArguments,
                                     const hidl_vec<uint32_t>& operandIndexes,
                                     const hidl_vec<Operand>& operands,
                                     const hidl_vec<hidl_memory>& pools, const char* type)
{
    MemoryAccessVerifier poolVerifier(pools);
    // The request should specify as many arguments as were described in the model.
    const size_t requestArgumentCount = requestArguments.size();
    if (requestArgumentCount != operandIndexes.size()) {
        LOG(ERROR) << "Request specifies " << requestArgumentCount << " " << type
                   << "s but the model has " << operandIndexes.size();
        return false;
    }
    for (size_t requestArgumentIndex = 0; requestArgumentIndex < requestArgumentCount;
         requestArgumentIndex++) {
        const RequestArgument& requestArgument = requestArguments[requestArgumentIndex];
        const DataLocation& location = requestArgument.location;
        // Get the operand index for this argument. We extract it from the list
        // that was provided in the call to ANeuralNetworksModel_identifyInputsAndOutputs.
        // We assume in this function that the model has been validated already.
        const uint32_t operandIndex = operandIndexes[requestArgumentIndex];
        const Operand& operand = operands[operandIndex];
        if (requestArgument.hasNoValue) {
            if (location.poolIndex != 0 || location.offset != 0 || location.length != 0 ||
                requestArgument.dimensions.size() != 0) {
                LOG(ERROR) << "Request " << type << " " << requestArgumentIndex
                           << " has no value yet has details.";
                return false;
            }
        } else {
            // Validate the location.
            if (!poolVerifier.validate(location)) {
                return false;
            }
            // If the argument specified a dimension, validate it.
            uint32_t rank = requestArgument.dimensions.size();
            if (rank == 0) {
                // Validate that all the dimensions are specified in the model.
                for (size_t i = 0; i < operand.dimensions.size(); i++) {
                    if (operand.dimensions[i] == 0) {
                        LOG(ERROR) << "Model has dimension " << i
                                   << " set to 0 but the request does specify the dimension.";
                        return false;
                    }
                }
            } else {
                if (rank != operand.dimensions.size()) {
                    LOG(ERROR) << "Request " << type << " " << requestArgumentIndex
                               << " has number of dimensions (" << rank
                               << ") different than the model's (" << operand.dimensions.size()
                               << ")";
                    return false;
                }
                for (size_t i = 0; i < rank; i++) {
                    if (requestArgument.dimensions[i] != operand.dimensions[i] &&
                        operand.dimensions[i] != 0) {
                        LOG(ERROR) << "Request " << type << " " << requestArgumentIndex
                                   << " has dimension " << i << " of "
                                   << requestArgument.dimensions[i]
                                   << " different than the model's " << operand.dimensions[i];
                        return false;
                    }
                    if (requestArgument.dimensions[i] == 0) {
                        LOG(ERROR) << "Request " << type << " " << requestArgumentIndex
                                   << " has dimension " << i << " of zero";
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool validateRequest(const Request& request, const Model& model)
{
    return (validateRequestArguments(request.inputs, model.inputIndexes, model.operands,
                                     request.pools, "input") &&
            validateRequestArguments(request.outputs, model.outputIndexes, model.operands,
                                     request.pools, "output") &&
            validatePools(request.pools));
}

}// namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
