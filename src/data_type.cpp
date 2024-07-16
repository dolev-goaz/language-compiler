#include "data_type.hpp"

const std::map<std::string, BasicDataType> DataType::data_type_name_to_value = {
    {"void", BasicDataType::VOID},    {"int_8", BasicDataType::INT8},   {"int_16", BasicDataType::INT16},
    {"int_32", BasicDataType::INT32}, {"int_64", BasicDataType::INT64}, {"char", BasicDataType::CHAR},
};

const std::map<BasicDataType, std::string> DataType::data_type_value_to_name = []() {
    std::map<BasicDataType, std::string> reverse_map;
    for (const auto& pair : DataType::data_type_name_to_value) {
        reverse_map[pair.second] = pair.first;
    }
    return reverse_map;
}();

const std::map<BasicDataType, size_t> DataType::data_type_to_size_bytes = {
    {BasicDataType::VOID, 0},  {BasicDataType::INT8, 1},  {BasicDataType::INT16, 2},
    {BasicDataType::INT32, 4}, {BasicDataType::INT64, 8}, {BasicDataType::CHAR, 1},
};

std::shared_ptr<DataType> BasicType::makeBasicType(BasicDataType type) { return std::make_shared<BasicType>(type); }
std::shared_ptr<DataType> BasicType::makeBasicType(const std::string& type_str) {
    if (data_type_name_to_value.count(type_str) == 0) {
        throw std::invalid_argument("Unknown data type " + type_str);
    }
    return BasicType::makeBasicType(data_type_name_to_value.at(type_str));
}

bool DataTypeUtils::is_array_type(const std::shared_ptr<DataType>& data_type) {
    return data_type && (bool)dynamic_cast<ArrayType*>(data_type.get());
}

CompatibilityStatus BasicType::is_compatible(const DataType& other) const {
    // Basic types are compatible if they are the same
    // also compatible with pointers
    if (const auto* otherBasic = dynamic_cast<const BasicType*>(&other)) {
        return get_size_bytes() == otherBasic->get_size_bytes() ? CompatibilityStatus::Compatible
                                                                : CompatibilityStatus::CompatibleWithWarning;
    }
    if (const auto* otherPointer = dynamic_cast<const PointerType*>(&other)) {
        return CompatibilityStatus::CompatibleWithWarning;
    }
    return CompatibilityStatus::NotCompatible;
}

CompatibilityStatus StructType::is_compatible(const DataType& other) const {
    // Struct types are compatible only if they are the same
    if (const auto* otherStruct = dynamic_cast<const StructType*>(&other)) {
        return name == otherStruct->name ? CompatibilityStatus::Compatible : CompatibilityStatus::NotCompatible;
    }
    return CompatibilityStatus::NotCompatible;
}

CompatibilityStatus PointerType::is_compatible(const DataType& other) const {
    // Pointers are compatible
    if (const auto* otherPointer = dynamic_cast<const PointerType*>(&other)) {
        return baseType == otherPointer->baseType ? CompatibilityStatus::Compatible
                                                  : CompatibilityStatus::CompatibleWithWarning;
    }

    // could also cast to basic types(numerics)
    if (const auto* other_basic = dynamic_cast<const BasicType*>(&other)) {
        return CompatibilityStatus::CompatibleWithWarning;
    }
    return CompatibilityStatus::NotCompatible;
}

CompatibilityStatus ArrayType::is_compatible(const DataType& other) const {
    // Arrays are compatible if their element types are compatible and sizes are the same
    if (const auto* otherArray = dynamic_cast<const ArrayType*>(&other)) {
        return (*elementType == *(otherArray->elementType) && size == otherArray->size)
                   ? CompatibilityStatus::Compatible
                   : CompatibilityStatus::NotCompatible;
    }
    // Arrays are also compatible with pointers to compatible types
    if (const auto* otherPointer = dynamic_cast<const PointerType*>(&other)) {
        auto compatibility_status = elementType->is_compatible(*(otherPointer->baseType));
        return compatibility_status == CompatibilityStatus::Compatible ? CompatibilityStatus::CompatibleWithWarning
                                                                       : CompatibilityStatus::NotCompatible;
    }
    return CompatibilityStatus::NotCompatible;
}