#pragma once

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

// Basic types enumeration
enum class BasicDataType {
    NONE = 0,
    VOID,
    INT8,
    INT16,
    INT32,
    INT64,
    CHAR,
};

enum class CompatibilityStatus { Compatible, NotCompatible, CompatibleWithWarning };

// Abstract base class for DataType
class DataType {
   public:
    static const std::map<std::string, BasicDataType> data_type_name_to_value;
    static const std::map<BasicDataType, size_t> data_type_to_size_bytes;

    virtual ~DataType() = default;
    virtual std::string toString() const = 0;
    virtual bool operator==(const DataType& other) const = 0;
    virtual CompatibilityStatus isCompatible(const DataType& other) const = 0;
    virtual bool isVoid() const { return false; }
    virtual size_t get_size_bytes() const = 0;
};

// Class for BasicDataType
class BasicType : public DataType {
   public:
    explicit BasicType(BasicDataType type) : type(type) {}
    // Helper function to create basic types
    static std::shared_ptr<DataType> makeBasicType(BasicDataType type);
    static std::shared_ptr<DataType> makeBasicType(const std::string& type_str);
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
    CompatibilityStatus isCompatible(const DataType& other) const override {
        // Basic types are compatible if they are the same
        if (const auto* otherBasic = dynamic_cast<const BasicType*>(&other)) {
            return type == otherBasic->type ? CompatibilityStatus::Compatible
                                            : CompatibilityStatus::CompatibleWithWarning;
        }
        return CompatibilityStatus::NotCompatible;
    }

    bool isVoid() const override { return type == BasicDataType::VOID; }

    size_t get_size_bytes() const override { return data_type_to_size_bytes.at(type); }

   private:
    BasicDataType type;
};

// Class for StructType
// This is basically here as a filler lol
class StructType : public DataType {
   public:
    explicit StructType(const std::string& name) : name(name) {}

    std::string toString() const override { return "struct " + name; }

    bool operator==(const DataType& other) const override {
        if (const auto* otherStruct = dynamic_cast<const StructType*>(&other)) {
            return name == otherStruct->name;
        }
        return false;
    }

    CompatibilityStatus isCompatible(const DataType& other) const override {
        // Struct types are compatible only if they are the same
        if (const auto* otherStruct = dynamic_cast<const StructType*>(&other)) {
            return name == otherStruct->name ? CompatibilityStatus::Compatible : CompatibilityStatus::NotCompatible;
        }
        return CompatibilityStatus::NotCompatible;
    }
    size_t get_size_bytes() const override {
        // TODO: when implementing struct implement this as well
        return 0;
    }

   private:
    std::string name;
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

    CompatibilityStatus isCompatible(const DataType& other) const override {
        // Pointers are compatible if they point to compatible types
        if (const auto* otherPointer = dynamic_cast<const PointerType*>(&other)) {
            return baseType->isCompatible(*(otherPointer->baseType));
        }
        return CompatibilityStatus::NotCompatible;
    }

    size_t get_size_bytes() const override {
        // NOTE: pointer size is qword, or 8 bytes
        return 8;
    }

   public:
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

    CompatibilityStatus isCompatible(const DataType& other) const override {
        // Arrays are compatible if their element types are compatible and sizes are the same
        if (const auto* otherArray = dynamic_cast<const ArrayType*>(&other)) {
            return (*elementType == *(otherArray->elementType) && size == otherArray->size)
                       ? CompatibilityStatus::Compatible
                       : CompatibilityStatus::NotCompatible;
        }
        // Arrays are also compatible with pointers to compatible types
        if (const auto* otherPointer = dynamic_cast<const PointerType*>(&other)) {
            auto compatibility_status = elementType->isCompatible(*(otherPointer->baseType));
            return compatibility_status == CompatibilityStatus::Compatible ? CompatibilityStatus::CompatibleWithWarning
                                                                           : CompatibilityStatus::NotCompatible;
        }
        return CompatibilityStatus::NotCompatible;
    }

    size_t get_size_bytes() const override {
        //
        return size * elementType->get_size_bytes();
    }

   public:
    std::shared_ptr<DataType> elementType;
    size_t size;
};