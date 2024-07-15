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