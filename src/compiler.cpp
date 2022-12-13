#include "compiler.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "binaryen/utils.hpp"
#include "helper/diagnose.hpp"
#include "helper/overload.hpp"
#include "helper/redefined_checker.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include "resolver.hpp"
#include "variant_type_table.hpp"
#include <algorithm>
#include <array>
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
                                                    variantTypeMap_->findVariantType("void"), module_);
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
  auto initVariant = compileExpression(statement->init(), variantType);
  walang::ir::Variant const *assignedVariant;
  if (currentFunction() == startFunction_) {
    // in global
    auto global = std::make_shared<ir::Global>(statement->variantName(), variantType);
    global->makeDefinition(module_);
    resolver_.addGlobal(statement->variantName(), global);
    assignedVariant = global.get();
  } else {
    // in function
    auto local = currentFunction()->addLocal(statement->variantName(), variantType);
    assignedVariant = local.get();
  }
  return initVariant->assignTo(module_, assignedVariant);
}
BinaryenExpressionRef Compiler::compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement) {
  auto assignedVariant = resolver_.resolveExpression(statement->variant());
  auto valueVariant = compileExpression(statement->value(), assignedVariant->variantType());
  switch (assignedVariant->type()) {
  case ir::Symbol::Type::TypeGlobal:
  case ir::Symbol::Type::TypeLocal:
  case ir::Symbol::Type::TypeMemoryData:
  case ir::Symbol::Type::TypeStackData:
    return valueVariant->assignTo(module_, dynamic_cast<ir::Variant const *>(assignedVariant.get()));
  case ir::Symbol::Type::TypeFunction:
    // TODO(TypeConvertError)
    break;
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileExpressionStatement(std::shared_ptr<ast::ExpressionStatement> const &statement) {
  auto expectedType = std::make_shared<ir::TypeAuto>();
  auto valueVariant = compileExpressionToExpressionRef(statement->expr(), expectedType);
  if (expectedType->underlyingType() != BinaryenTypeNone()) {
    return BinaryenDrop(module_, valueVariant);
  } else {
    return valueVariant;
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
  BinaryenExpressionRef condition =
      compileExpressionToExpressionRef(statement->condition(), std::make_shared<ir::TypeCondition>());
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
  BinaryenExpressionRef condition =
      compileExpressionToExpressionRef(statement->condition(), std::make_shared<ir::TypeCondition>());
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
  auto signature = currentFunction()->signature();
  auto returnValue = compileExpression(statement->expr(), signature->returnType());
  switch (signature->returnType()->underlyingReturnTypeStatus()) {
  case ir::VariantType::UnderlyingReturnTypeStatus::None:
  case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory: {
    auto exprRefs = currentFunction()->finalizeReturn(module_, BinaryenReturn(module_, nullptr));
    exprRefs.insert(exprRefs.begin(), returnValue->assignToMemory(module_, ir::MemoryData{0, signature->returnType()}));
    return binaryen::Utils::combineExprRef(module_, exprRefs);
  }
  case ir::VariantType::UnderlyingReturnTypeStatus::ByReturnValue: {
    auto exprRefs =
        currentFunction()->finalizeReturn(module_, BinaryenReturn(module_, returnValue->assignToStack(module_)));
    return binaryen::Utils::combineExprRef(module_, exprRefs);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
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
  auto functionIr =
      std::make_shared<ir::Function>(statement->name(), argumentNames, argumentTypes, returnType, module_);
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
  compileClassConstructor(classType);
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
                                                   argumentTypes, returnType, module_);
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
void Compiler::compileClassConstructor(std::shared_ptr<ir::Class> const &classType) {
  auto constructor =
      std::make_shared<ir::Function>(classType->className() + "#constructor", std::vector<std::string>{},
                                     std::vector<std::shared_ptr<ir::VariantType>>{}, classType, module_);
  BinaryenExpressionRef body;
  switch (classType->underlyingReturnTypeStatus()) {
  case ir::VariantType::UnderlyingReturnTypeStatus::None:
    body = BinaryenBlock(module_, nullptr, nullptr, 0, BinaryenTypeNone());
    break;
  case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory: {
    std::vector<BinaryenExpressionRef> refs{};
    for (auto underlyingType : classType->underlyingTypes()) {
      refs.push_back(ir::VariantType::from(underlyingType)->underlyingDefaultValue(module_));
    }
    BinaryenExpressionRef initValueRef = BinaryenBlock(module_, nullptr, refs.data(), refs.size(), BinaryenTypeAuto());
    ir::StackData initValue{initValueRef, classType};
    body = initValue.assignToMemory(module_, ir::MemoryData{0, classType});
    break;
  }
  case ir::VariantType::UnderlyingReturnTypeStatus::ByReturnValue:
    body = classType->underlyingDefaultValue(module_);
    break;
  }
  constructor->finalize(module_, body);
  resolver_.addFunction(classType->className(), constructor);
}

// ███████ ██   ██ ██████  ██████  ███████ ███████ ███████ ██  ██████  ███    ██
// ██       ██ ██  ██   ██ ██   ██ ██      ██      ██      ██ ██    ██ ████   ██
// █████     ███   ██████  ██████  █████   ███████ ███████ ██ ██    ██ ██ ██  ██
// ██       ██ ██  ██      ██   ██ ██           ██      ██ ██ ██    ██ ██  ██ ██
// ███████ ██   ██ ██      ██   ██ ███████ ███████ ███████ ██  ██████  ██   ████

BinaryenExpressionRef Compiler::compileExpressionToExpressionRef(std::shared_ptr<ast::Expression> const &expression,
                                                                 std::shared_ptr<ir::VariantType> const &expectedType) {
  auto valueVariant = compileExpression(expression, expectedType);
  return valueVariant->assignToStack(module_);
}

std::shared_ptr<ir::Variant> Compiler::compileExpression(std::shared_ptr<ast::Expression> const &expression,
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

std::shared_ptr<ir::Variant> Compiler::compileIdentifier(std::shared_ptr<ast::Identifier> const &expression,
                                                         std::shared_ptr<ir::VariantType> const &expectedType) {
  return std::visit(
      overloaded{[this, &expectedType](uint64_t i) -> std::shared_ptr<ir::Variant> {
                   auto pendingType = std::dynamic_pointer_cast<ir::PendingResolveType>(expectedType);
                   if (pendingType != nullptr && !pendingType->isResolved()) {
                     // not resolve, default i32
                     pendingType->tryResolveTo(variantTypeMap_->findVariantType("i32"));
                   }
                   return std::make_shared<ir::StackData>(
                       expectedType->underlyingConst(module_, static_cast<int64_t>(i)),
                       variantTypeMap_->findVariantType("i32"));
                 },
                 [this, &expression, &expectedType](double d) -> std::shared_ptr<ir::Variant> {
                   auto pendingType = std::dynamic_pointer_cast<ir::PendingResolveType>(expectedType);
                   if (pendingType != nullptr && !pendingType->isResolved()) {
                     // not resolve, default f32
                     pendingType->tryResolveTo(variantTypeMap_->findVariantType("f32"));
                   }
                   return std::make_shared<ir::StackData>(expectedType->underlyingConst(module_, d),
                                                          variantTypeMap_->findVariantType("f32"));
                 },
                 [this, &expression, &expectedType](const std::string &s) -> std::shared_ptr<ir::Variant> {
                   auto symbol = resolver_.resolveIdentifier(expression); // TODO(FIXME)
                   if (!expectedType->tryResolveTo(symbol->variantType())) {
                     auto e = TypeConvertError(symbol->variantType()->to_string(), expectedType->to_string());
                     e.setRange(expression->range());
                     throw e;
                   }
                   auto variant = std::dynamic_pointer_cast<ir::Variant>(symbol);
                   if (variant == nullptr) {
                     throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
                   }
                   return variant;
                 }},
      expression->identifier());
}
std::shared_ptr<ir::Variant> Compiler::compilePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression,
                                                               std::shared_ptr<ir::VariantType> const &expectedType) {
  auto expr = compileExpressionToExpressionRef(expression->expr(), expectedType);
  return std::make_shared<ir::StackData>(expectedType->handlePrefixOp(module_, expression->op(), expr), expectedType);
}
std::shared_ptr<ir::Variant> Compiler::compileBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression,
                                                               std::shared_ptr<ir::VariantType> const &expectedType) {
  BinaryenExpressionRef leftExprRef = compileExpressionToExpressionRef(expression->leftExpr(), expectedType);
  BinaryenExpressionRef rightExprRef = compileExpressionToExpressionRef(expression->rightExpr(), expectedType);
  return std::make_shared<ir::StackData>(
      expectedType->handleBinaryOp(module_, expression->op(), leftExprRef, rightExprRef, currentFunction()),
      expectedType);
}
std::shared_ptr<ir::Variant>
Compiler::compileTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression,
                                   std::shared_ptr<ir::VariantType> const &expectedType) {
  return std::make_shared<ir::StackData>(
      BinaryenIf(module_,
                 compileExpressionToExpressionRef(expression->conditionExpr(), std::make_shared<ir::TypeCondition>()),
                 compileExpressionToExpressionRef(expression->leftExpr(), expectedType),
                 compileExpressionToExpressionRef(expression->rightExpr(), expectedType)),
      expectedType);
}
std::shared_ptr<ir::Variant> Compiler::compileCallExpression(std::shared_ptr<ast::CallExpression> const &expression,
                                                             std::shared_ptr<ir::VariantType> const &expectedType) {
  auto callerSymbol = resolver_.resolveExpression(expression->caller());
  if (callerSymbol->type() != ir::Symbol::Type::TypeFunction) {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
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
  uint32_t const returnValuePosition =
      ir::VariantType::getSize(functionCaller->signature()->returnType()->underlyingType());
  for (uint32_t index = 0; index < signatureArgumentTypes.size(); index++) {
    if (signatureArgumentTypes[index]->type() == ir::VariantType::Type::Class) {
      auto classType = std::dynamic_pointer_cast<ir::Class>(signatureArgumentTypes[index]);
      auto symbol = resolver_.resolveExpression(argumentExpressions[index]);

      switch (symbol->type()) {
      case ir::Symbol::Type::TypeGlobal:
      case ir::Symbol::Type::TypeLocal:
      case ir::Symbol::Type::TypeStackData:
      case ir::Symbol::Type::TypeMemoryData: {
        auto toMemoryExprRef = std::dynamic_pointer_cast<ir::Variant>(symbol)->assignToMemory(
            module_, ir::MemoryData{memoryPosition, classType});
        exprRefs.push_back(toMemoryExprRef);
        break;
      }
      case ir::Symbol::Type::TypeFunction:
        throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
      }

      BinaryenExpressionRef fromMemoryExprRef;
      switch (symbol->type()) {
      case ir::Symbol::Type::TypeGlobal:
      case ir::Symbol::Type::TypeLocal:
      case ir::Symbol::Type::TypeMemoryData:
      case ir::Symbol::Type::TypeStackData:
        fromMemoryExprRef = ir::MemoryData{memoryPosition + returnValuePosition, classType}.assignTo(
            module_, std::dynamic_pointer_cast<ir::Variant>(symbol).get());
        break;
      case ir::Symbol::Type::TypeFunction:
        throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
      }
      postPrecessExprRefs.push_back(fromMemoryExprRef);
      memoryPosition += ir::VariantType::getSize(classType->underlyingType());
    } else {
      operands.push_back(compileExpressionToExpressionRef(argumentExpressions[index], signatureArgumentTypes[index]));
    }
  }

  // handle return value
  switch (functionCaller->signature()->returnType()->underlyingReturnTypeStatus()) {
  case ir::VariantType::UnderlyingReturnTypeStatus::None:
  case ir::VariantType::UnderlyingReturnTypeStatus::ByReturnValue: {
    BinaryenExpressionRef callExprRef =
        BinaryenCall(module_, functionCaller->name().c_str(), operands.data(), operands.size(),
                     functionCaller->signature()->returnType()->underlyingType());
    exprRefs.push_back(callExprRef);
    exprRefs.insert(exprRefs.end(), postPrecessExprRefs.begin(), postPrecessExprRefs.end());

    return std::make_shared<ir::StackData>(binaryen::Utils::combineExprRef(module_, exprRefs), expectedType);
  }
  case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory: {
    BinaryenExpressionRef callExprRef =
        BinaryenCall(module_, functionCaller->name().c_str(), operands.data(), operands.size(), BinaryenTypeNone());
    exprRefs.push_back(callExprRef);
    exprRefs.insert(exprRefs.end(), postPrecessExprRefs.begin(), postPrecessExprRefs.end());
    auto assign = (ir::MemoryData{0, expectedType}.assignToStack(module_));
    auto assignSize = BinaryenBlockGetNumChildren(assign);
    for (uint32_t i = 0; i < assignSize; i++) {
      exprRefs.push_back(BinaryenBlockGetChildAt(assign, i));
    }
    return std::make_shared<ir::StackData>(binaryen::Utils::combineExprRef(module_, exprRefs), expectedType);
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
std::shared_ptr<ir::Variant> Compiler::compileMemberExpression(std::shared_ptr<ast::MemberExpression> const &expression,
                                                               std::shared_ptr<ir::VariantType> const &expectedType) {
  auto symbol = resolver_.resolveMemberExpression(expression);
  switch (symbol->type()) {
  case ir::Symbol::Type::TypeLocal:
  case ir::Symbol::Type::TypeGlobal:
  case ir::Symbol::Type::TypeMemoryData:
  case ir::Symbol::Type::TypeStackData:
    return std::dynamic_pointer_cast<ir::Variant>(symbol);
  case ir::Symbol::Type::TypeFunction:
    break;
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

} // namespace walang