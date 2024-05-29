#include "data_type.hpp"

const std::map<std::string, BasicDataType> DataType::data_type_name_to_value = {
    {"void", BasicDataType::VOID},
    {"int_8", BasicDataType::INT8},
    {"int_16", BasicDataType::INT16},
    {"int_32", BasicDataType::INT32},
    {"int_64", BasicDataType::INT64},
};
