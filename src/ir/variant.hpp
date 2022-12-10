#pragma once

#include "ast/statement.hpp"
#include "ir/variant_type.hpp"
#include <binaryen-c.h>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace walang::ir {

class Symbol {
public:
  enum class Type {
    TypeGlobal,
    TypeLocal,
    TypeMemoryData,
    TypeStackData,
    TypeFunction,
  };
  explicit Symbol(Type type, std::shared_ptr<VariantType> variantType)
      : type_(type), variantType_(std::move(variantType)) {}
  virtual ~Symbol() = default;

  [[nodiscard]] Type type() const noexcept { return type_; }
  [[nodiscard]] std::shared_ptr<VariantType> const &variantType() const noexcept { return variantType_; }

protected:
  const Type type_;
  std::shared_ptr<VariantType> variantType_;
};

class Global;
class Local;
class MemoryData;

class Variant : public Symbol {
public:
  explicit Variant(std::string name, Type type, std::shared_ptr<VariantType> const &variantType)
      : Symbol(type, variantType), name_(std::move(name)) {}
  ~Variant() override = default;

  [[nodiscard]] std::string name() const noexcept { return name_; }

  BinaryenExpressionRef assignTo(BinaryenModuleRef module, Variant const *to) const;
  virtual BinaryenExpressionRef assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const = 0;
  virtual BinaryenExpressionRef assignToLocal(BinaryenModuleRef module, Local const &local) const = 0;
  virtual BinaryenExpressionRef assignToGlobal(BinaryenModuleRef module, Global const &global) const = 0;
  virtual BinaryenExpressionRef assignToStack(BinaryenModuleRef module) const = 0;

protected:
  std::string name_;
};

class Global : public Variant {
public:
  Global(std::string name, std::shared_ptr<VariantType> const &type)
      : Variant(std::move(name), Type::TypeGlobal, type) {
    initMembers(type);
  }
  ~Global() override = default;
  void makeDefinition(BinaryenModuleRef module);
  [[nodiscard]] std::shared_ptr<Global> findMemberByName(std::string const &name) const;

  BinaryenExpressionRef assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const override;
  BinaryenExpressionRef assignToLocal(BinaryenModuleRef module, Local const &local) const override;
  BinaryenExpressionRef assignToGlobal(BinaryenModuleRef module, Global const &global) const override;
  BinaryenExpressionRef assignToStack(BinaryenModuleRef module) const override;

private:
  std::map<std::string, std::shared_ptr<Global>> members_{};
  void initMembers(std::shared_ptr<VariantType> const &type);
};

class Local : public Variant {
public:
  Local(uint32_t index, std::shared_ptr<VariantType> const &type) : Variant("", Type::TypeLocal, type), index_{index} {
    initMembers(type);
  }
  Local(uint32_t index, std::string name, std::shared_ptr<VariantType> const &type)
      : Variant(std::move(name), Type::TypeLocal, type), index_{index} {
    initMembers(type);
  }
  ~Local() override = default;

  [[nodiscard]] uint32_t index() const noexcept { return index_; }
  [[nodiscard]] std::shared_ptr<Local> findMemberByName(std::string const &name) const;

  BinaryenExpressionRef assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const override;
  BinaryenExpressionRef assignToLocal(BinaryenModuleRef module, Local const &local) const override;
  BinaryenExpressionRef assignToGlobal(BinaryenModuleRef module, Global const &global) const override;
  BinaryenExpressionRef assignToStack(BinaryenModuleRef module) const override;

private:
  uint32_t index_;
  std::map<std::string, std::shared_ptr<Local>> members_{};
  void initMembers(std::shared_ptr<VariantType> const &type);
};

class MemoryData : public Variant {
public:
  MemoryData(uint32_t memoryPosition, std::shared_ptr<VariantType> const &type)
      : Variant("MemoryData", Type::TypeMemoryData, type), memoryPosition_{memoryPosition} {}

  [[nodiscard]] uint32_t memoryPosition() const noexcept { return memoryPosition_; }

  BinaryenExpressionRef assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const override;
  BinaryenExpressionRef assignToLocal(BinaryenModuleRef module, Local const &local) const override;
  BinaryenExpressionRef assignToGlobal(BinaryenModuleRef module, Global const &global) const override;
  BinaryenExpressionRef assignToStack(BinaryenModuleRef module) const override;

private:
  uint32_t memoryPosition_;
};

class StackData : public Variant {
public:
  StackData(BinaryenExpressionRef exprRef, std::shared_ptr<VariantType> const &type)
      : Variant("StackData", Type::TypeStackData, type), exprRef_(exprRef) {}

  [[nodiscard]] BinaryenExpressionRef exprRef() const noexcept { return exprRef_; }

  BinaryenExpressionRef assignToMemory(BinaryenModuleRef module, MemoryData const &memoryData) const override;
  BinaryenExpressionRef assignToLocal(BinaryenModuleRef module, Local const &local) const override;
  BinaryenExpressionRef assignToGlobal(BinaryenModuleRef module, Global const &global) const override;
  BinaryenExpressionRef assignToStack(BinaryenModuleRef module) const override { return exprRef_; }

private:
  BinaryenExpressionRef exprRef_;
};

class Function : public Symbol {
public:
  Function(std::string name, std::vector<std::string> const &argumentNames,
           std::vector<std::shared_ptr<VariantType>> const &argumentTypes,
           std::shared_ptr<VariantType> const &returnType);

  [[nodiscard]] std::string name() const noexcept { return name_; }
  [[nodiscard]] std::shared_ptr<Signature> signature() const noexcept {
    return std::dynamic_pointer_cast<Signature>(variantType_);
  }
  [[nodiscard]] std::vector<std::shared_ptr<Local>> const &locals() const noexcept { return locals_; }

  std::shared_ptr<Class> thisClassType() { return thisClassType_.lock(); }
  void setThisClassType(std::shared_ptr<Class> const &thisClassType) { thisClassType_ = thisClassType; }

  std::shared_ptr<Local> addLocal(std::string const &name, std::shared_ptr<VariantType> const &localType);
  std::shared_ptr<Local> addTempLocal(std::shared_ptr<VariantType> const &localType);
  [[nodiscard]] std::shared_ptr<Local> findLocalByName(std::string const &name) const;

  std::string const &createBreakLabel(std::string const &prefix);
  [[nodiscard]] std::string const &topBreakLabel() const;
  void freeBreakLabel();
  std::string const &createContinueLabel(std::string const &prefix);
  [[nodiscard]] std::string const &topContinueLabel() const;
  void freeContinueLabel();

  BinaryenFunctionRef finalize(BinaryenModuleRef module, BinaryenExpressionRef body);

private:
  std::string name_;
  uint32_t argumentSize_;

  std::vector<std::shared_ptr<Local>> locals_{};
  uint32_t localIndex_{0U};

  std::weak_ptr<Class> thisClassType_{};

  std::stack<std::string> currentBreakLabel_{};
  uint32_t breakLabelIndex_{0U};
  std::stack<std::string> currentContinueLabel_{};
  uint32_t continueLabelIndex_{0U};
};

} // namespace walang::ir
