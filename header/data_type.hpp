#pragma once

#include <map>
#include <memory>
#include <string>

// Basic types enumeration
enum class BasicDataType {
    NONE = 0,
    VOID,
    INT8,
    INT16,
    INT32,
    INT64,
};

// Abstract base class for DataType
class DataType {
   public:
    static const std::map<std::string, BasicDataType> data_type_name_to_value;

    virtual ~DataType() = default;
    virtual std::string toString() const = 0;
    virtual bool operator==(const DataType& other) const = 0;
};

// Class for BasicDataType
class BasicType : public DataType {
   public:
    explicit BasicType(BasicDataType type) : type(type) {}
    std::string toString() const override {
        static const std::map<BasicDataType, std::string> basicTypeToString = {
            {BasicDataType::NONE, "none"},   {BasicDataType::VOID, "void"},   {BasicDataType::INT8, "int8"},
            {BasicDataType::INT16, "int16"}, {BasicDataType::INT32, "int32"}, {BasicDataType::INT64, "int64"},
        };
        return basicTypeToString.at(type);
    }
    bool operator==(const DataType& other) const override {
        if (const auto* otherBasic = dynamic_cast<const BasicType*>(&other)) {
            return type == otherBasic->type;
        }
        return false;
    }

   private:
    BasicDataType type;
};

// Class for PointerType
class PointerType : public DataType {
   public:
    explicit PointerType(std::shared_ptr<DataType> baseType) : baseType(std::move(baseType)) {}
    std::string toString() const override { return baseType->toString() + "*"; }
    bool operator==(const DataType& other) const override {
        if (const auto* otherPointer = dynamic_cast<const PointerType*>(&other)) {
            return *baseType == *(otherPointer->baseType);
        }
        return false;
    }

   private:
    std::shared_ptr<DataType> baseType;
};

// Class for ArrayType
class ArrayType : public DataType {
   public:
    ArrayType(std::shared_ptr<DataType> elementType, size_t size) : elementType(std::move(elementType)), size(size) {}
    std::string toString() const override { return elementType->toString() + "[" + std::to_string(size) + "]"; }
    bool operator==(const DataType& other) const override {
        if (const auto* otherArray = dynamic_cast<const ArrayType*>(&other)) {
            return *elementType == *(otherArray->elementType) && size == otherArray->size;
        }
        return false;
    }

   private:
    std::shared_ptr<DataType> elementType;
    size_t size;
};

// Helper function to create basic types
std::shared_ptr<DataType> makeBasicType(BasicDataType type) { return std::make_shared<BasicType>(type); }
