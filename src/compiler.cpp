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
#include <cassert>
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

template <class T> static void concat(std::vector<T> &a, std::vector<T> const &b) {
  a.insert(a.end(), b.begin(), b.end());
}

Compiler::Compiler(std::vector<std::shared_ptr<ast::File>> files)
    : module_{BinaryenModuleCreate()}, files_{std::move(files)}, variantTypeMap_{std::make_shared<VariantTypeMap>()},
      resolver_(variantTypeMap_) {
  BinaryenMemoryRef a;
  BinaryenSetMemory(module_, 0, 1, nullptr, nullptr, nullptr, nullptr, nullptr, 0, false, false, "0");
}

void Compiler::compile() {
  for (auto const &file : files_) {
    // prepare
    std::vector<ast::ClassStatement *> pendingClasses{};
    for (auto &statement : file->statement()) {
      if (statement->type() == ast::TypeClassStatement) {
        try {
          prepareClassStatementLevel1(*std::dynamic_pointer_cast<ast::ClassStatement>(statement));
        } catch (UnknownSymbol const &) {
          pendingClasses.push_back(std::dynamic_pointer_cast<ast::ClassStatement>(statement).get());
        }
      }
    }
    bool isResolved = true;
    while (!pendingClasses.empty() && isResolved) {
      isResolved = false;
      std::vector<ast::ClassStatement *> currentPendingClasses{};
      std::swap(currentPendingClasses, pendingClasses);
      for (auto &pendingClass : currentPendingClasses) {
        try {
          prepareClassStatementLevel1(*pendingClass);
        } catch (UnknownSymbol const &) {
          pendingClasses.push_back(pendingClass);
          continue;
        }
        isResolved = true;
      }
    }
    for (auto pendingClass : pendingClasses) {
      prepareClassStatementLevel1(*pendingClass);
    }

    for (auto &statement : file->statement()) {
      if (statement->type() == ast::TypeFunctionStatement) {
        prepareFunctionStatement(*std::dynamic_pointer_cast<ast::FunctionStatement>(statement));
      }
    }
    for (auto &statement : file->statement()) {
      if (statement->type() == ast::TypeClassStatement) {
        prepareClassStatementLevel2(*std::dynamic_pointer_cast<ast::ClassStatement>(statement));
      }
    }
    // compile
    startFunction_ = std::make_shared<ir::Function>(
        "_start", std::vector<std::string>{}, std::vector<std::shared_ptr<ir::VariantType>>{},
        variantTypeMap_->findVariantType("void"), std::set<ir::Function::Flag>{}, module_);
    currentFunction_.push(startFunction_);
    resolver_.setCurrentFunction(currentFunction());
    std::vector<BinaryenExpressionRef> expressions{};
    for (auto &statement : file->statement()) {
      concat(expressions, compileStatement(statement));
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

// ██████  ██████  ███████ ██████   █████  ██████  ███████
// ██   ██ ██   ██ ██      ██   ██ ██   ██ ██   ██ ██
// ██████  ██████  █████   ██████  ███████ ██████  █████
// ██      ██   ██ ██      ██      ██   ██ ██   ██ ██
// ██      ██   ██ ███████ ██      ██   ██ ██   ██ ███████

void Compiler::prepareFunctionStatement(ast::FunctionStatement const &statement) {
  std::vector<std::string> argumentNames{};
  std::vector<std::shared_ptr<ir::VariantType>> argumentTypes{};
  for (auto const &argument : statement.arguments()) {
    argumentNames.push_back(argument.name_);
    argumentTypes.push_back(variantTypeMap_->findVariantType(argument.type_));
  }
  std::shared_ptr<ir::VariantType> returnType;
  if (statement.returnType().has_value()) {
    returnType = variantTypeMap_->findVariantType(statement.returnType().value());
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
  doPrepareFunction(statement.name(), argumentNames, argumentTypes, returnType, nullptr, statement.decorators());
}
std::shared_ptr<ir::Function> Compiler::prepareMethod(ast::FunctionStatement const &statement,
                                                      std::shared_ptr<ir::Class> const &classType) {
  std::vector<std::string> argumentNames{};
  std::vector<std::shared_ptr<ir::VariantType>> argumentTypes{};
  for (auto const &argument : statement.arguments()) {
    argumentNames.emplace_back(argument.name_);
    argumentTypes.emplace_back(variantTypeMap_->findVariantType(argument.type_));
  }
  std::shared_ptr<ir::VariantType> returnType;
  if (statement.returnType().has_value()) {
    returnType = variantTypeMap_->findVariantType(statement.returnType().value());
  } else {
    throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
  }
  return doPrepareFunction(classType->className() + "#" + statement.name(), argumentNames, argumentTypes, returnType,
                           classType, statement.decorators());
}

std::shared_ptr<ir::Function> Compiler::doPrepareFunction(std::string const &name,
                                                          std::vector<std::string> argumentNames,
                                                          std::vector<std::shared_ptr<ir::VariantType>> argumentTypes,
                                                          std::shared_ptr<ir::VariantType> const &returnType,
                                                          std::shared_ptr<ir::Class> const &classType,
                                                          std::vector<std::string> const &decorators) {
  std::set<ir::Function::Flag> flags{};
  if (std::count(decorators.begin(), decorators.end(), "readonly") > 0) {
    if (classType == nullptr) {
      throw ErrorDecorator{"readonly"};
    }
    flags.insert(ir::Function::Flag::Readonly);
  }
  if (classType != nullptr) {
    argumentNames.emplace_back("this");
    argumentTypes.emplace_back(classType);
    flags.insert(ir::Function::Flag::Method);
  }
  auto functionIr = std::make_shared<ir::Function>(name, argumentNames, argumentTypes, returnType, flags, module_);
  resolver_.addFunction(name, functionIr);
  return functionIr;
}

void Compiler::prepareClassStatementLevel1(ast::ClassStatement const &statement) {
  // re-define check
  RedefinedChecker redefinedChecker{};
  for (auto const &member : statement.members()) {
    redefinedChecker.check(member.name_);
  }
  for (auto const &method : statement.methods()) {
    redefinedChecker.check(method->name());
  }
  std::vector<ir::Class::ClassMember> members{};
  members.reserve(statement.members().size());
  for (auto const &member : statement.members()) {
    if (member.type_ == statement.name()) {
      auto e = RecursiveDefinedSymbol(member.type_);
      e.setRange(statement.range());
      throw e;
    }
    members.push_back(ir::Class::ClassMember{.memberName_ = member.name_,
                                             .memberType_ = variantTypeMap_->findVariantType(member.type_)});
  }

  auto classType = std::make_shared<ir::Class>(statement.name());
  variantTypeMap_->registerType(statement.name(), classType);

  classType->setMembers(members);
  compileClassConstructor(classType);
}
void Compiler::prepareClassStatementLevel2(ast::ClassStatement const &statement) {
  auto classType = std::dynamic_pointer_cast<ir::Class>(variantTypeMap_->findVariantType(statement.name()));
  assert(classType != nullptr);
  std::map<std::string, std::shared_ptr<ir::Function>> methodMap{};
  for (auto const &method : statement.methods()) {
    methodMap.insert(std::make_pair(method->name(), prepareMethod(*method, classType)));
  }
  classType->setMethodMap(methodMap);
}

// ███████ ████████  █████  ████████ ███████ ███    ███ ███████ ███    ██ ████████
// ██         ██    ██   ██    ██    ██      ████  ████ ██      ████   ██    ██
// ███████    ██    ███████    ██    █████   ██ ████ ██ █████   ██ ██  ██    ██
//      ██    ██    ██   ██    ██    ██      ██  ██  ██ ██      ██  ██ ██    ██
// ███████    ██    ██   ██    ██    ███████ ██      ██ ███████ ██   ████    ██

std::vector<BinaryenExpressionRef> Compiler::compileStatement(std::shared_ptr<ast::Statement> const &statement) {
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
std::vector<BinaryenExpressionRef>
Compiler::compileDeclareStatement(std::shared_ptr<ast::DeclareStatement> const &statement) {
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
std::vector<BinaryenExpressionRef>
Compiler::compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement) {
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
std::vector<BinaryenExpressionRef>
Compiler::compileExpressionStatement(std::shared_ptr<ast::ExpressionStatement> const &statement) {
  auto expectedType = std::make_shared<ir::TypeAuto>();
  auto valueVariant = compileExpressionToExpressionRef(statement->expr(), expectedType);
  if (expectedType->underlyingType() != BinaryenTypeNone()) {
    return {BinaryenDrop(module_, valueVariant)};
  } else {
    return {valueVariant};
  }
}
std::vector<BinaryenExpressionRef>
Compiler::compileBlockStatement(std::shared_ptr<ast::BlockStatement> const &statement) {
  std::vector<BinaryenExpressionRef> statementRefs{};
  currentFunction()->enterScope();
  for (auto &statement : statement->statements()) {
    concat(statementRefs, compileStatement(statement));
  }
  currentFunction()->exitScope();
  return {BinaryenBlock(module_, nullptr, statementRefs.data(), statementRefs.size(), BinaryenTypeNone())};
}
std::vector<BinaryenExpressionRef> Compiler::compileIfStatement(std::shared_ptr<ast::IfStatement> const &statement) {
  BinaryenExpressionRef condition =
      compileExpressionToExpressionRef(statement->condition(), std::make_shared<ir::TypeCondition>());
  BinaryenExpressionRef ifTrue =
      binaryen::Utils::combineExprRef(module_, compileBlockStatement(statement->thenBlock()));
  BinaryenExpressionRef ifElse =
      statement->elseBlock() == nullptr
          ? nullptr
          : binaryen::Utils::combineExprRef(module_, compileStatement(statement->elseBlock()));
  return {BinaryenIf(module_, condition, ifTrue, ifElse)};
}
std::vector<BinaryenExpressionRef>
Compiler::compileWhileStatement(std::shared_ptr<ast::WhileStatement> const &statement) {
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
  std::vector<BinaryenExpressionRef> block = compileBlockStatement(statement->block());
  block.push_back(BinaryenBreak(module_, continueLabel.c_str(), nullptr, nullptr));
  BinaryenExpressionRef body = BinaryenIf(
      module_, condition, BinaryenBlock(module_, nullptr, block.data(), block.size(), BinaryenTypeNone()), nullptr);
  currentFunction()->freeBreakLabel();
  currentFunction()->freeContinueLabel();
  BinaryenExpressionRef loop = BinaryenLoop(module_, continueLabel.c_str(), body);
  return {BinaryenBlock(module_, breakLabel.c_str(), &loop, 1U, BinaryenTypeNone())};
}
std::vector<BinaryenExpressionRef>
Compiler::compileBreakStatement(std::shared_ptr<ast::BreakStatement> const &statement) {
  return {BinaryenBreak(module_, currentFunction()->topBreakLabel().c_str(), nullptr, nullptr)};
}
std::vector<BinaryenExpressionRef>
Compiler::compileContinueStatement(std::shared_ptr<ast::ContinueStatement> const &statement) {
  return {BinaryenBreak(module_, currentFunction()->topContinueLabel().c_str(), nullptr, nullptr)};
}
std::vector<BinaryenExpressionRef>
Compiler::compileReturnStatement(std::shared_ptr<ast::ReturnStatement> const &statement) {
  auto signature = currentFunction()->signature();
  auto returnValue = compileExpression(statement->expr(), signature->returnType());
  switch (signature->returnType()->underlyingReturnTypeStatus()) {
  case ir::VariantType::UnderlyingReturnTypeStatus::None:
  case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory: {
    auto exprRefs = returnValue->assignToMemory(module_, ir::MemoryData{0, signature->returnType()});
    auto returnExprRefs = currentFunction()->finalizeReturn(module_, BinaryenReturn(module_, nullptr));
    concat(exprRefs, returnExprRefs);
    return exprRefs;
  }
  case ir::VariantType::UnderlyingReturnTypeStatus::ByReturnValue: {
    auto exprRefs = currentFunction()->finalizeReturn(
        module_,
        BinaryenReturn(module_, binaryen::Utils::combineExprRef(module_, returnValue->assignToStack(module_))));
    return exprRefs;
  }
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

std::vector<BinaryenExpressionRef>
Compiler::compileFunctionStatement(std::shared_ptr<ast::FunctionStatement> const &statement) {
  doCompileFunction(statement->name(), statement->body());
  return {};
}
std::shared_ptr<ir::Function> Compiler::compileClassMethod(std::shared_ptr<ir::Class> const &classType,
                                                           std::shared_ptr<ast::FunctionStatement> const &statement) {
  return doCompileFunction(classType->className() + "#" + statement->name(), statement->body());
}
std::shared_ptr<ir::Function> Compiler::doCompileFunction(std::string const &name,
                                                          std::shared_ptr<ast::BlockStatement> const &body) {
  auto it = resolver_.functions().find(name);
  assert(it != resolver_.functions().end());
  auto functionIr = it->second;
  currentFunction_.push(functionIr);
  resolver_.setCurrentFunction(currentFunction());
  BinaryenExpressionRef bodyRef = binaryen::Utils::combineExprRef(module_, compileBlockStatement(body));
  currentFunction()->finalize(module_, bodyRef);
  currentFunction_.pop();
  resolver_.setCurrentFunction(currentFunction());
  return functionIr;
}

std::vector<BinaryenExpressionRef>
Compiler::compileClassStatement(std::shared_ptr<ast::ClassStatement> const &statement) {
  if (currentFunction() != startFunction_) {
    throw std::runtime_error("class should only be defined in top scope");
  }
  auto classType = std::dynamic_pointer_cast<ir::Class>(variantTypeMap_->findVariantType(statement->name()));
  assert(classType != nullptr);
  for (auto const &method : statement->methods()) {
    compileClassMethod(classType, method);
  }
  return {};
}

void Compiler::compileClassConstructor(std::shared_ptr<ir::Class> const &classType) {
  auto constructor = std::make_shared<ir::Function>(classType->className() + "#constructor", std::vector<std::string>{},
                                                    std::vector<std::shared_ptr<ir::VariantType>>{}, classType,
                                                    std::set<ir::Function::Flag>{}, module_);
  BinaryenExpressionRef body;
  switch (classType->underlyingReturnTypeStatus()) {
  case ir::VariantType::UnderlyingReturnTypeStatus::None:
    body = BinaryenBlock(module_, nullptr, nullptr, 0, BinaryenTypeNone());
    break;
  case ir::VariantType::UnderlyingReturnTypeStatus::LoadFromMemory: {
    std::vector<BinaryenExpressionRef> exprRef{};
    for (auto underlyingType : classType->underlyingTypes()) {
      exprRef.push_back(ir::VariantType::from(underlyingType)->underlyingDefaultValue(module_));
    };
    body = binaryen::Utils::combineExprRef(
        module_, ir::StackData{exprRef, classType}.assignToMemory(module_, ir::MemoryData{0, classType}));
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

  return binaryen::Utils::combineExprRef(module_, compileExpressionToExpressionRefs(expression, expectedType));
}
std::vector<BinaryenExpressionRef>
Compiler::compileExpressionToExpressionRefs(std::shared_ptr<ast::Expression> const &expression,
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
    argumentExpressions.insert(argumentExpressions.end(),
                               std::dynamic_pointer_cast<ast::MemberExpression>(expression->caller())->expr());
  }
  // check
  try {
    functionCaller->checkArgumentAndReturnType(argumentExpressions, expectedType);
  } catch (CompilerErrorBase &e) {
    e.setRangeAndThrow(expression->range());
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
    auto argumentExprRefs =
        compileExpressionToExpressionRefs(argumentExpressions[index], signatureArgumentTypes[index]);
    operands.insert(operands.cend(), argumentExprRefs.begin(), argumentExprRefs.end());
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
    concat(exprRefs, ir::MemoryData{0, expectedType}.assignToStack(module_));
    return std::make_shared<ir::StackData>(exprRefs, expectedType);
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