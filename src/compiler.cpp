#include "compiler.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "helper/diagnose.hpp"
#include "helper/overload.hpp"
#include "helper/redefined_checker.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include "resolver.hpp"
#include "variant_type_table.hpp"
#include <algorithm>
#include <binaryen-c.h>
#include <cstdint>
#include <exception>
#include <fmt/core.h>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace walang {

Compiler::Compiler(std::vector<std::shared_ptr<ast::File>> files)
    : module_{BinaryenModuleCreate()}, files_{std::move(files)}, variantTypeMap_{std::make_shared<VariantTypeMap>()},
      resolver_(variantTypeMap_) {
  BinaryenMemoryRef a;
  BinaryenSetMemory(module_, 0, 1, nullptr, nullptr, nullptr, nullptr, nullptr, 0, false, false, "0");
}

void Compiler::compile() {
  for (auto const &file : files_) {
    startFunction_ = std::make_shared<ir::Function>("_start", std::vector<std::string>{},
                                                    std::vector<std::shared_ptr<ir::VariantType>>{},
                                                    variantTypeMap_->findVariantType("void"));
    currentFunction_.push(startFunction_);
    resolver_.setCurrentFunction(currentFunction());
    std::vector<BinaryenExpressionRef> expressions{};
    for (auto &statement : file->statement()) {
      expressions.emplace_back(compileStatement(statement));
    }
    BinaryenExpressionRef body = BinaryenBlock(module_, nullptr, expressions.data(), expressions.size(),
                                               startFunction_->signature()->returnType()->underlyingType());
    BinaryenFunctionRef startFunctionRef = startFunction_->finalize(module_, body);
    BinaryenSetStart(module_, startFunctionRef);
  }
}
std::string Compiler::wat() const {
  BinaryenSetColorsEnabled(false);
  std::string watBuf{};
  watBuf.resize(1024 * 1024);
  while (true) {
    uint32_t size = BinaryenModuleWriteText(module_, watBuf.data(), watBuf.size());
    if (size == watBuf.size()) {
      watBuf.resize(watBuf.size() * 2);
    } else {
      watBuf.resize(size);
      break;
    }
  }
  return watBuf;
}

// ███████ ████████  █████  ████████ ███████ ███    ███ ███████ ███    ██ ████████
// ██         ██    ██   ██    ██    ██      ████  ████ ██      ████   ██    ██
// ███████    ██    ███████    ██    █████   ██ ████ ██ █████   ██ ██  ██    ██
//      ██    ██    ██   ██    ██    ██      ██  ██  ██ ██      ██  ██ ██    ██
// ███████    ██    ██   ██    ██    ███████ ██      ██ ███████ ██   ████    ██

BinaryenExpressionRef Compiler::compileStatement(std::shared_ptr<ast::Statement> const &statement) {
  try {
    switch (statement->type()) {
    case ast::StatementType::TypeDeclareStatement:
      return compileDeclareStatement(std::dynamic_pointer_cast<ast::DeclareStatement>(statement));
    case ast::StatementType::TypeAssignStatement:
      return compileAssignStatement(std::dynamic_pointer_cast<ast::AssignStatement>(statement));
    case ast::StatementType::TypeExpressionStatement:
      return compileExpressionStatement(std::dynamic_pointer_cast<ast::ExpressionStatement>(statement));
    case ast::StatementType::TypeBlockStatement:
      return compileBlockStatement(std::dynamic_pointer_cast<ast::BlockStatement>(statement));
    case ast::StatementType::TypeIfStatement:
      return compileIfStatement(std::dynamic_pointer_cast<ast::IfStatement>(statement));
    case ast::StatementType::TypeWhileStatement:
      return compileWhileStatement(std::dynamic_pointer_cast<ast::WhileStatement>(statement));
    case ast::StatementType::TypeBreakStatement:
      return compileBreakStatement(std::dynamic_pointer_cast<ast::BreakStatement>(statement));
    case ast::StatementType::TypeContinueStatement:
      return compileContinueStatement(std::dynamic_pointer_cast<ast::ContinueStatement>(statement));
    case ast::StatementType::TypeFunctionStatement:
      return compileFunctionStatement(std::dynamic_pointer_cast<ast::FunctionStatement>(statement));
    case ast::StatementType::TypeClassStatement:
      return compileClassStatement(std::dynamic_pointer_cast<ast::ClassStatement>(statement));
    case ast::TypeReturnStatement:
      return compileReturnStatement(std::dynamic_pointer_cast<ast::ReturnStatement>(statement));
    }
  } catch (CompilerErrorBase &e) {
    e.setRangeAndThrow(statement->range());
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileDeclareStatement(std::shared_ptr<ast::DeclareStatement> const &statement) {
  std::shared_ptr<ir::VariantType> const &variantType =
      statement->variantType().empty() ? resolver_.resolveTypeExpression(statement->init())
                                       : variantTypeMap_->findVariantType(statement->variantType());
  BinaryenExpressionRef init = compileExpression(statement->init(), variantType);
  if (currentFunction() == startFunction_) {
    // in global
    auto global = std::make_shared<ir::Global>(statement->variantName(), variantType);
    global->makeDefinition(module_);
    resolver_.addGlobal(statement->variantName(), global);
    switch (variantType->underlyingReturnTypeStatus()) {
    case ir::VariantType::UnderlyingReturnTypeStatus::None:
      return BinaryenNop(module_);
    case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory:
      return ir::MemoryData{0, variantType}.assignToGlobal(module_, *global);
    case ir::VariantType::UnderlyingReturnTypeStatus::ByReturnValue:
      return global->assignFromStack(module_, init);
    }
  } else {
    // in function
    auto local = currentFunction()->addLocal(statement->variantName(), variantType);
    switch (variantType->underlyingReturnTypeStatus()) {
    case ir::VariantType::UnderlyingReturnTypeStatus::None:
      return BinaryenNop(module_);
    case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory:
      return ir::MemoryData{0, variantType}.assignToLocal(module_, *local);
    case ir::VariantType::UnderlyingReturnTypeStatus::ByReturnValue:
      return local->assignFromStack(module_, init);
    }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement) {
  auto symbol = resolver_.resolveExpression(statement->variant());
  auto exprRef = compileExpression(statement->value(), symbol->variantType());
  switch (symbol->type()) {
  case ir::Symbol::Type::TypeGlobal:
  case ir::Symbol::Type::TypeLocal:
  case ir::Symbol::Type::TypeMemoryData:
    return std::dynamic_pointer_cast<ir::Variant>(symbol)->assignFromStack(module_, exprRef);
  case ir::Symbol::Type::TypeFunction:
    // TODO(TypeConvertError)
    break;
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileExpressionStatement(std::shared_ptr<ast::ExpressionStatement> const &statement) {
  auto expectedType = std::make_shared<ir::TypeAuto>();
  BinaryenExpressionRef exprRef = compileExpression(statement->expr(), expectedType);
  if (expectedType->resolvedType()->type() == ir::VariantType::Type::None) {
    return exprRef;
  } else {
    return BinaryenDrop(module_, exprRef);
  }
}
BinaryenExpressionRef Compiler::compileBlockStatement(std::shared_ptr<ast::BlockStatement> const &statement) {
  std::vector<BinaryenExpressionRef> statementRefs{};
  auto statements = statement->statements();
  std::transform(
      statements.cbegin(), statements.cend(), std::back_inserter(statementRefs),
      [this](std::shared_ptr<ast::Statement> const &innerStatement) { return compileStatement(innerStatement); });
  return BinaryenBlock(module_, nullptr, statementRefs.data(), statementRefs.size(), BinaryenTypeNone());
}
BinaryenExpressionRef Compiler::compileIfStatement(std::shared_ptr<ast::IfStatement> const &statement) {
  BinaryenExpressionRef condition = compileExpression(statement->condition(), std::make_shared<ir::TypeCondition>());
  BinaryenExpressionRef ifTrue = compileBlockStatement(statement->thenBlock());
  BinaryenExpressionRef ifElse = statement->elseBlock() == nullptr ? nullptr : compileStatement(statement->elseBlock());
  return BinaryenIf(module_, condition, ifTrue, ifElse);
}
BinaryenExpressionRef Compiler::compileWhileStatement(std::shared_ptr<ast::WhileStatement> const &statement) {
  /**
    loop A (
      if (
        this->condition
        block (
          this->block
          br A
        )
      )
    )
   */
  auto breakLabel = currentFunction()->createBreakLabel("while");
  auto continueLabel = currentFunction()->createContinueLabel("while");
  BinaryenExpressionRef condition = compileExpression(statement->condition(), std::make_shared<ir::TypeCondition>());
  std::vector<BinaryenExpressionRef> block = {
      compileBlockStatement(statement->block()),
      BinaryenBreak(module_, continueLabel.c_str(), nullptr, nullptr),
  };
  BinaryenExpressionRef body = BinaryenIf(
      module_, condition, BinaryenBlock(module_, nullptr, block.data(), block.size(), BinaryenTypeNone()), nullptr);
  currentFunction()->freeBreakLabel();
  currentFunction()->freeContinueLabel();
  BinaryenExpressionRef loop = BinaryenLoop(module_, continueLabel.c_str(), body);
  return BinaryenBlock(module_, breakLabel.c_str(), &loop, 1U, BinaryenTypeNone());
}
BinaryenExpressionRef Compiler::compileBreakStatement(std::shared_ptr<ast::BreakStatement> const &statement) {
  return BinaryenBreak(module_, currentFunction()->topBreakLabel().c_str(), nullptr, nullptr);
}
BinaryenExpressionRef Compiler::compileContinueStatement(std::shared_ptr<ast::ContinueStatement> const &statement) {
  return BinaryenBreak(module_, currentFunction()->topContinueLabel().c_str(), nullptr, nullptr);
}
BinaryenExpressionRef Compiler::compileReturnStatement(std::shared_ptr<ast::ReturnStatement> const &statement) {
  BinaryenExpressionRef exprRef = compileExpression(statement->expr(), currentFunction()->signature()->returnType());
  return BinaryenReturn(module_, exprRef);
}

BinaryenExpressionRef Compiler::compileFunctionStatement(std::shared_ptr<ast::FunctionStatement> const &statement) {
  std::vector<std::string> argumentNames{};
  std::vector<std::shared_ptr<ir::VariantType>> argumentTypes{};
  for (auto const &argument : statement->arguments()) {
    argumentNames.push_back(argument.name_);
    argumentTypes.push_back(variantTypeMap_->findVariantType(argument.type_));
  }
  std::shared_ptr<ir::VariantType> returnType = statement->returnType().has_value()
                                                    ? variantTypeMap_->findVariantType(statement->returnType().value())
                                                    : std::make_shared<ir::TypeAuto>();
  auto functionIr = std::make_shared<ir::Function>(statement->name(), argumentNames, argumentTypes, returnType);
  resolver_.addFunction(statement->name(), functionIr);
  currentFunction_.push(functionIr);
  resolver_.setCurrentFunction(currentFunction());
  BinaryenExpressionRef body = compileBlockStatement(statement->body());
  currentFunction()->finalize(module_, body);
  currentFunction_.pop();
  resolver_.setCurrentFunction(currentFunction());
  return BinaryenNop(module_);
}
BinaryenExpressionRef Compiler::compileClassStatement(std::shared_ptr<ast::ClassStatement> const &statement) {
  if (currentFunction() != startFunction_) {
    throw std::runtime_error("class should only be defined in top scope");
  }
  auto classType = std::make_shared<ir::Class>(statement->name());
  variantTypeMap_->registerType(statement->name(), classType);

  RedefinedChecker redefinedChecker{};

  std::vector<ir::Class::ClassMember> members{};
  members.reserve(statement->members().size());
  for (auto const &member : statement->members()) {
    redefinedChecker.check(member.name_);
    if (member.type_ == statement->name()) {
      auto e = RecursiveDefinedSymbol(member.type_);
      e.setRange(statement->range());
      throw e;
    }
    members.push_back(ir::Class::ClassMember{.memberName_ = member.name_,
                                             .memberType_ = variantTypeMap_->findVariantType(member.type_)});
  }
  classType->setMembers(members);

  auto constructor = std::make_shared<ir::Function>(statement->name() + "#constructor", std::vector<std::string>{},
                                                    std::vector<std::shared_ptr<ir::VariantType>>{}, classType);
  std::vector<BinaryenExpressionRef> constructorChildren{};
  switch (classType->underlyingReturnTypeStatus()) {
  case ir::VariantType::UnderlyingReturnTypeStatus::None:
    break;
  case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory: {
    int32_t offset = 0;
    for (auto underlyingType : classType->underlyingTypes()) {
      int32_t dataSize = (underlyingType == BinaryenTypeInt64() || underlyingType == BinaryenTypeFloat64()) ? 8U : 4U;
      constructorChildren.push_back(
          BinaryenStore(module_, dataSize, 0, 0, BinaryenConst(module_, BinaryenLiteralInt32(offset)),
                        ir::VariantType::from(underlyingType)->underlyingDefaultValue(module_), underlyingType, "0"));
      offset += dataSize;
    }
    break;
  }
  case ir::VariantType::UnderlyingReturnTypeStatus::ByReturnValue: {
    constructorChildren.push_back(classType->underlyingDefaultValue(module_));
    break;
  }
  }
  BinaryenExpressionRef body =
      BinaryenBlock(module_, nullptr, constructorChildren.data(), constructorChildren.size(), BinaryenTypeAuto());
  constructor->finalize(module_, body);
  resolver_.addFunction(statement->name(), constructor);

  std::map<std::string, std::shared_ptr<ir::Function>> methodMap{};
  for (auto const &method : statement->methods()) {
    redefinedChecker.check(method->name());
    auto emplaceResult = methodMap.insert(std::make_pair(method->name(), compileClassMethod(classType, method)));
    if (!emplaceResult.second) {
      auto e = RedefinedSymbol(method->name());
      e.setRange(method->range());
      throw e;
    }
  }
  classType->setMethodMap(methodMap);

  return BinaryenNop(module_);
}
std::shared_ptr<ir::Function> Compiler::compileClassMethod(std::shared_ptr<ir::Class> const &classType,
                                                           std::shared_ptr<ast::FunctionStatement> const &statement) {
  std::vector<std::string> argumentNames{};
  std::vector<std::shared_ptr<ir::VariantType>> argumentTypes{};
  argumentNames.emplace_back("this");
  argumentTypes.emplace_back(classType);
  for (auto const &argument : statement->arguments()) {
    argumentNames.emplace_back(argument.name_);
    argumentTypes.emplace_back(variantTypeMap_->findVariantType(argument.type_));
  }
  std::shared_ptr<ir::VariantType> returnType = statement->returnType().has_value()
                                                    ? variantTypeMap_->findVariantType(statement->returnType().value())
                                                    : std::make_shared<ir::TypeAuto>();
  auto functionIr = std::make_shared<ir::Function>(classType->className() + "#" + statement->name(), argumentNames,
                                                   argumentTypes, returnType);
  functionIr->setThisClassType(classType);
  resolver_.addFunction(classType->className() + "#" + statement->name(), functionIr);
  currentFunction_.push(functionIr);
  resolver_.setCurrentFunction(currentFunction());
  BinaryenExpressionRef body = compileBlockStatement(statement->body());
  currentFunction()->finalize(module_, body);
  currentFunction_.pop();
  resolver_.setCurrentFunction(currentFunction());
  return functionIr;
}

// ███████ ██   ██ ██████  ██████  ███████ ███████ ███████ ██  ██████  ███    ██
// ██       ██ ██  ██   ██ ██   ██ ██      ██      ██      ██ ██    ██ ████   ██
// █████     ███   ██████  ██████  █████   ███████ ███████ ██ ██    ██ ██ ██  ██
// ██       ██ ██  ██      ██   ██ ██           ██      ██ ██ ██    ██ ██  ██ ██
// ███████ ██   ██ ██      ██   ██ ███████ ███████ ███████ ██  ██████  ██   ████

BinaryenExpressionRef Compiler::compileExpression(std::shared_ptr<ast::Expression> const &expression,
                                                  std::shared_ptr<ir::VariantType> const &expectedType) {
  try {
    switch (expression->type()) {
    case ast::ExpressionType::TypeIdentifier:
      return compileIdentifier(std::dynamic_pointer_cast<ast::Identifier>(expression), expectedType);
    case ast::ExpressionType::TypePrefixExpression:
      return compilePrefixExpression(std::dynamic_pointer_cast<ast::PrefixExpression>(expression), expectedType);
    case ast::ExpressionType::TypeBinaryExpression:
      return compileBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression), expectedType);
    case ast::ExpressionType::TypeTernaryExpression:
      return compileTernaryExpression(std::dynamic_pointer_cast<ast::TernaryExpression>(expression), expectedType);
    case ast::ExpressionType::TypeCallExpression:
      return compileCallExpression(std::dynamic_pointer_cast<ast::CallExpression>(expression), expectedType);
    case ast::ExpressionType::TypeMemberExpression:
      return compileMemberExpression(std::dynamic_pointer_cast<ast::MemberExpression>(expression), expectedType);
    }
  } catch (CompilerErrorBase &e) {
    e.setRangeAndThrow(expression->range());
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

BinaryenExpressionRef Compiler::compileIdentifier(std::shared_ptr<ast::Identifier> const &expression,
                                                  std::shared_ptr<ir::VariantType> const &expectedType) {
  return std::visit(overloaded{[this, &expectedType](uint64_t i) -> BinaryenExpressionRef {
                                 auto pendingType = std::dynamic_pointer_cast<ir::PendingResolveType>(expectedType);
                                 if (pendingType != nullptr && !pendingType->isResolved()) {
                                   // not resolve, default i32
                                   pendingType->tryResolveTo(std::make_shared<ir::TypeI32>());
                                 }
                                 return expectedType->underlyingConst(module_, static_cast<int64_t>(i));
                               },
                               [this, &expression, &expectedType](double d) -> BinaryenExpressionRef {
                                 auto pendingType = std::dynamic_pointer_cast<ir::PendingResolveType>(expectedType);
                                 if (pendingType != nullptr && !pendingType->isResolved()) {
                                   // not resolve, default f32
                                   pendingType->tryResolveTo(std::make_shared<ir::TypeF32>());
                                 }
                                 return expectedType->underlyingConst(module_, d);
                               },
                               [this, &expression, &expectedType](const std::string &s) -> BinaryenExpressionRef {
                                 auto symbol = resolver_.resolveIdentifier(expression); // TODO(FIXME)
                                 switch (symbol->type()) {
                                 case ir::Symbol::Type::TypeGlobal:
                                 case ir::Symbol::Type::TypeLocal:
                                 case ir::Symbol::Type::TypeMemoryData:
                                   if (!expectedType->tryResolveTo(symbol->variantType())) {
                                     auto e = TypeConvertError(symbol->variantType()->to_string(),
                                                               expectedType->to_string());
                                     e.setRange(expression->range());
                                     throw e;
                                   }
                                   return std::dynamic_pointer_cast<ir::Variant>(symbol)->assignToStack(module_);
                                 case ir::Symbol::Type::TypeFunction:
                                   break;
                                 }
                                 throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
                               }},
                    expression->identifier());
}
BinaryenExpressionRef Compiler::compilePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression,
                                                        std::shared_ptr<ir::VariantType> const &expectedType) {
  BinaryenExpressionRef exprRef = compileExpression(expression->expr(), expectedType);
  return expectedType->handlePrefixOp(module_, expression->op(), exprRef);
}
BinaryenExpressionRef Compiler::compileBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression,
                                                        std::shared_ptr<ir::VariantType> const &expectedType) {
  BinaryenExpressionRef leftExprRef = compileExpression(expression->leftExpr(), expectedType);
  BinaryenExpressionRef rightExprRef = compileExpression(expression->rightExpr(), expectedType);
  return expectedType->handleBinaryOp(module_, expression->op(), leftExprRef, rightExprRef, currentFunction());
}
BinaryenExpressionRef Compiler::compileTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression,
                                                         std::shared_ptr<ir::VariantType> const &expectedType) {
  return BinaryenIf(module_, compileExpression(expression->conditionExpr(), std::make_shared<ir::TypeCondition>()),
                    compileExpression(expression->leftExpr(), expectedType),
                    compileExpression(expression->rightExpr(), expectedType));
}
BinaryenExpressionRef Compiler::compileCallExpression(std::shared_ptr<ast::CallExpression> const &expression,
                                                      std::shared_ptr<ir::VariantType> const &expectedType) {
  auto callerSymbol = resolver_.resolveExpression(expression->caller());
  switch (callerSymbol->type()) {
  case ir::Symbol::Type::TypeFunction: {
    auto functionCaller = std::dynamic_pointer_cast<ir::Function>(callerSymbol);
    std::vector<std::shared_ptr<ir::VariantType>> const &signatureArgumentTypes =
        functionCaller->signature()->argumentTypes();
    std::vector<std::shared_ptr<ast::Expression>> argumentExpressions = expression->arguments();
    if (expression->caller()->type() == ast::ExpressionType::TypeMemberExpression) {
      argumentExpressions.insert(argumentExpressions.begin(),
                                 std::dynamic_pointer_cast<ast::MemberExpression>(expression->caller())->expr());
    }
    // check
    if (signatureArgumentTypes.size() != argumentExpressions.size()) {
      auto e = ArgumentCountError(signatureArgumentTypes.size(), argumentExpressions.size());
      e.setRange(expression->range());
      throw e;
    }
    if (!expectedType->tryResolveTo(functionCaller->signature()->returnType())) {
      auto e = TypeConvertError(functionCaller->signature()->returnType()->to_string(), expectedType->to_string());
      e.setRange(expression->range());
      throw e;
    }

    // compile
    std::vector<BinaryenExpressionRef> exprRefs{};

    // handle arguments
    std::vector<BinaryenExpressionRef> postPrecessExprRefs{};
    std::vector<BinaryenExpressionRef> operands{};
    uint32_t memoryPosition = 0;
    for (uint32_t index = 0; index < signatureArgumentTypes.size(); index++) {
      if (signatureArgumentTypes[index]->type() == ir::VariantType::Type::Class) {
        auto classType = std::dynamic_pointer_cast<ir::Class>(signatureArgumentTypes[index]);
        auto symbol = resolver_.resolveExpression(argumentExpressions[index]);
        switch (symbol->type()) {
        case ir::Symbol::Type::TypeGlobal:
        case ir::Symbol::Type::TypeLocal:
        case ir::Symbol::Type::TypeMemoryData: {
          auto toMemoryExprRef = std::dynamic_pointer_cast<ir::Variant>(symbol)->assignToMemory(
              module_, ir::MemoryData{memoryPosition, classType});
          exprRefs.push_back(toMemoryExprRef);
        }
        case ir::Symbol::Type::TypeFunction:
          throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
        }
        BinaryenExpressionRef fromMemoryExprRef;
        switch (symbol->type()) {
        case ir::Symbol::Type::TypeGlobal:
          fromMemoryExprRef = ir::MemoryData{memoryPosition, classType}.assignToGlobal(
              module_, *std::dynamic_pointer_cast<ir::Global>(symbol));
        case ir::Symbol::Type::TypeLocal:
          fromMemoryExprRef = ir::MemoryData{memoryPosition, classType}.assignToLocal(
              module_, *std::dynamic_pointer_cast<ir::Local>(symbol));
        case ir::Symbol::Type::TypeMemoryData:
          fromMemoryExprRef = ir::MemoryData{memoryPosition, classType}.assignToMemory(
              module_, *std::dynamic_pointer_cast<ir::MemoryData>(symbol));
        case ir::Symbol::Type::TypeFunction:
          throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
        }
        postPrecessExprRefs.push_back(fromMemoryExprRef);

        memoryPosition += ir::VariantType::getSize(classType->underlyingType());
      } else {
        operands.push_back(compileExpression(argumentExpressions[index], signatureArgumentTypes[index]));
      }
    }

    // handle return value
    // TODO(merge logic)
    switch (functionCaller->signature()->returnType()->underlyingReturnTypeStatus()) {
    case ir::VariantType::UnderlyingReturnTypeStatus::None:
    case ir::VariantType::UnderlyingReturnTypeStatus::ByReturnValue: {
      BinaryenExpressionRef callExprRef =
          BinaryenCall(module_, functionCaller->name().c_str(), operands.data(), operands.size(),
                       functionCaller->signature()->returnType()->underlyingType());
      exprRefs.push_back(callExprRef);

      exprRefs.insert(exprRefs.end(), postPrecessExprRefs.begin(), postPrecessExprRefs.end());

      return exprRefs.size() == 1 ? callExprRef
                                  : BinaryenBlock(module_, nullptr, exprRefs.data(), exprRefs.size(),
                                                  functionCaller->signature()->returnType()->underlyingType());
    }
    case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory: {
      BinaryenExpressionRef callExprRef =
          BinaryenCall(module_, functionCaller->name().c_str(), operands.data(), operands.size(), BinaryenTypeNone());
      exprRefs.push_back(callExprRef);
      exprRefs.insert(exprRefs.end(), postPrecessExprRefs.begin(), postPrecessExprRefs.end());
      return exprRefs.size() == 1 ? callExprRef
                                  : BinaryenBlock(module_, nullptr, exprRefs.data(), exprRefs.size(),
                                                  functionCaller->signature()->returnType()->underlyingType());
    }
    }
  }
  case ir::Symbol::Type::TypeGlobal:
  case ir::Symbol::Type::TypeLocal:
  case ir::Symbol::Type::TypeMemoryData:
    break;
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileMemberExpression(std::shared_ptr<ast::MemberExpression> const &expression,
                                                        std::shared_ptr<ir::VariantType> const &expectedType) {
  auto symbol = resolver_.resolveMemberExpression(expression);
  switch (symbol->type()) {
  case ir::Symbol::Type::TypeLocal:
  case ir::Symbol::Type::TypeGlobal:
  case ir::Symbol::Type::TypeMemoryData:
    return std::dynamic_pointer_cast<ir::Local>(symbol)->assignToStack(module_);
  case ir::Symbol::Type::TypeFunction:
    break;
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

} // namespace walang