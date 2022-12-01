#include "compiler.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "helper/diagnose.hpp"
#include "helper/overload.hpp"
#include "ir/function.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include <algorithm>
#include <binaryen-c.h>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fmt/core.h>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace walang {

Compiler::Compiler(std::vector<std::shared_ptr<ast::File>> files) : module_{BinaryenModuleCreate()}, files_{files} {}

void Compiler::compile() {
  for (auto const &file : files_) {
    startFunction_ = std::make_shared<ir::Function>("_start", std::vector<std::string>{},
                                                    std::vector<std::shared_ptr<ir::VariantType>>{},
                                                    ir::VariantType::resolveType("void"));
    currentFunction_.push(startFunction_);
    std::vector<BinaryenExpressionRef> expressions{};
    for (auto &statement : file->statement()) {
      expressions.emplace_back(compileStatement(statement));
    }
    BinaryenExpressionRef body = BinaryenBlock(module_, nullptr, expressions.data(), expressions.size(),
                                               startFunction_->signature()->returnType()->underlyingTypeName());
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
  switch (statement->type()) {
  case ast::Statement::_DeclareStatement:
    return compileDeclareStatement(std::dynamic_pointer_cast<ast::DeclareStatement>(statement));
  case ast::Statement::_AssignStatement:
    return compileAssignStatement(std::dynamic_pointer_cast<ast::AssignStatement>(statement));
  case ast::Statement::_ExpressionStatement:
    return compileExpressionStatement(std::dynamic_pointer_cast<ast::ExpressionStatement>(statement));
  case ast::Statement::_BlockStatement:
    return compileBlockStatement(std::dynamic_pointer_cast<ast::BlockStatement>(statement));
  case ast::Statement::_IfStatement:
    return compileIfStatement(std::dynamic_pointer_cast<ast::IfStatement>(statement));
  case ast::Statement::_WhileStatement:
    return compileWhileStatement(std::dynamic_pointer_cast<ast::WhileStatement>(statement));
  case ast::Statement::_BreakStatement:
    return compileBreakStatement(std::dynamic_pointer_cast<ast::BreakStatement>(statement));
  case ast::Statement::_ContinueStatement:
    return compileContinueStatement(std::dynamic_pointer_cast<ast::ContinueStatement>(statement));
  case ast::Statement::_FunctionStatement:
    return compileFunctionStatement(std::dynamic_pointer_cast<ast::FunctionStatement>(statement));
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileDeclareStatement(std::shared_ptr<ast::DeclareStatement> const &statement) {
  std::shared_ptr<ir::VariantType> const &variantType = ir::VariantType::getTypeFromDeclare(*statement);
  BinaryenExpressionRef init = compileExpression(statement->init(), variantType);
  if (currentFunction() == startFunction_) {
    // in global
    auto global = std::make_shared<ir::Global>(statement->name(), variantType);
    BinaryenAddGlobal(module_, global->name().c_str(), variantType->underlyingTypeName(), true,
                      variantType->underlyingDefaultValue(module_));
    globals_.emplace(statement->name(), global);
    return global->makeAssign(module_, init);
  } else {
    // in function
    auto local = currentFunction()->addLocal(statement->name(), variantType);
    return local->makeAssign(module_, init);
  }
}
BinaryenExpressionRef Compiler::compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement) {
  auto symbol = resolveVariant(statement->variant());
  auto exprRef = compileExpression(statement->value(), symbol->variantType());
  switch (symbol->type()) {
  case ir::Symbol::Type::_Function:
    // TODO(TypeConvertError)
    throw std::runtime_error(fmt::format("cannot assign to function {0}", statement->to_string()));
  case ir::Symbol::Type::_Global:
  case ir::Symbol::Type::_Local:
    return std::dynamic_pointer_cast<ir::Variant>(symbol)->makeAssign(module_, exprRef);
  }
  assert(false);
  std::abort();
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
  try {
    return BinaryenBreak(module_, currentFunction()->topBreakLabel().c_str(), nullptr, nullptr);
  } catch (JumpStatementError &e) {
    e.setRange(statement->range());
    throw e;
  }
}
BinaryenExpressionRef Compiler::compileContinueStatement(std::shared_ptr<ast::ContinueStatement> const &statement) {
  try {
    return BinaryenBreak(module_, currentFunction()->topContinueLabel().c_str(), nullptr, nullptr);
  } catch (JumpStatementError &e) {
    e.setRange(statement->range());
    throw e;
  }
}
BinaryenExpressionRef Compiler::compileFunctionStatement(std::shared_ptr<ast::FunctionStatement> const &statement) {
  std::vector<std::string> argumentNames{};
  std::vector<std::shared_ptr<ir::VariantType>> argumentTypes{};
  for (auto const &argument : statement->arguments()) {
    argumentNames.push_back(argument.name_);
    argumentTypes.push_back(ir::VariantType::resolveType(argument.type_));
  }
  std::shared_ptr<ir::VariantType> returnType = statement->returnType().has_value()
                                                    ? ir::VariantType::resolveType(statement->returnType().value())
                                                    : std::make_shared<ir::TypeAuto>();
  auto functionIr = std::make_shared<ir::Function>(statement->name(), argumentNames, argumentTypes, returnType);
  functions_.insert(std::make_pair(statement->name(), functionIr));
  currentFunction_.push(functionIr);
  BinaryenExpressionRef body = compileBlockStatement(statement->body());
  currentFunction()->finalize(module_, body);
  currentFunction_.pop();
  return BinaryenNop(module_);
}

// ███████ ██   ██ ██████  ██████  ███████ ███████ ███████ ██  ██████  ███    ██
// ██       ██ ██  ██   ██ ██   ██ ██      ██      ██      ██ ██    ██ ████   ██
// █████     ███   ██████  ██████  █████   ███████ ███████ ██ ██    ██ ██ ██  ██
// ██       ██ ██  ██      ██   ██ ██           ██      ██ ██ ██    ██ ██  ██ ██
// ███████ ██   ██ ██      ██   ██ ███████ ███████ ███████ ██  ██████  ██   ████

BinaryenExpressionRef Compiler::compileExpression(std::shared_ptr<ast::Expression> const &expression,
                                                  std::shared_ptr<ir::VariantType> const &expectedType) {
  switch (expression->type()) {
  case ast::Expression::_Identifier:
    return compileIdentifier(std::dynamic_pointer_cast<ast::Identifier>(expression), expectedType);
  case ast::Expression::_PrefixExpression:
    return compilePrefixExpression(std::dynamic_pointer_cast<ast::PrefixExpression>(expression), expectedType);
  case ast::Expression::_BinaryExpression:
    return compileBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression), expectedType);
  case ast::Expression::_TernaryExpression:
    return compileTernaryExpression(std::dynamic_pointer_cast<ast::TernaryExpression>(expression), expectedType);
  case ast::Expression::_CallExpression:
    return compileCallExpression(std::dynamic_pointer_cast<ast::CallExpression>(expression), expectedType);
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
                                 try {
                                   return expectedType->underlyingConst(module_, d);
                                 } catch (TypeConvertError &e) {
                                   e.setRange(expression->range());
                                   throw e;
                                 }
                               },
                               [this, &expression, &expectedType](const std::string &s) -> BinaryenExpressionRef {
                                 auto local = currentFunction()->findLocalByName(s);
                                 if (local != nullptr) {
                                   if (!expectedType->tryResolveTo(local->variantType())) {
                                     throw TypeConvertError(local->variantType(), expectedType, expression->range());
                                   }
                                   return local->makeGet(module_);
                                 }
                                 auto globalIt = globals_.find(s);
                                 if (globalIt != globals_.end()) {
                                   if (!expectedType->tryResolveTo(globalIt->second->variantType())) {
                                     throw TypeConvertError(globalIt->second->variantType(), expectedType,
                                                            expression->range());
                                   }
                                   return globalIt->second->makeGet(module_);
                                 }
                                 throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
                               }},
                    expression->id());
}
BinaryenExpressionRef Compiler::compilePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression,
                                                        std::shared_ptr<ir::VariantType> const &expectedType) {
  BinaryenExpressionRef exprRef = compileExpression(expression->expr(), expectedType);
  try {
    return expectedType->handlePrefixOp(module_, expression->op(), exprRef);
  } catch (InvalidOperator &e) {
    e.setRange(expression->range());
    throw e;
  }
}
BinaryenExpressionRef Compiler::compileBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression,
                                                        std::shared_ptr<ir::VariantType> const &expectedType) {
  BinaryenExpressionRef leftExprRef = compileExpression(expression->leftExpr(), expectedType);
  BinaryenExpressionRef rightExprRef = compileExpression(expression->rightExpr(), expectedType);
  try {
    return expectedType->handleBinaryOp(module_, expression->op(), leftExprRef, rightExprRef, currentFunction());
  } catch (InvalidOperator &e) {
    e.setRange(expression->range());
    throw e;
  }
}
BinaryenExpressionRef Compiler::compileTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression,
                                                         std::shared_ptr<ir::VariantType> const &expectedType) {
  return BinaryenIf(module_, compileExpression(expression->conditionExpr(), std::make_shared<ir::TypeCondition>()),
                    compileExpression(expression->leftExpr(), expectedType),
                    compileExpression(expression->rightExpr(), expectedType));
}
BinaryenExpressionRef Compiler::compileCallExpression(std::shared_ptr<ast::CallExpression> const &expression,
                                                      std::shared_ptr<ir::VariantType> const &expectedType) {
  auto callerType = resolveVariant(expression->caller());
  switch (callerType->type()) {
  case ir::Symbol::Type::_Global:
  case ir::Symbol::Type::_Local:
    break;
  case ir::Symbol::Type::_Function:
    auto functionCaller = std::dynamic_pointer_cast<ir::Function>(callerType);
    std::vector<std::shared_ptr<ir::VariantType>> const &signatureArgumentTypes =
        functionCaller->signature()->argumentTypes();
    std::vector<std::shared_ptr<ast::Expression>> const &argumentExpressions = expression->arguments();
    // check
    if (signatureArgumentTypes.size() != argumentExpressions.size()) {
      throw ArgumentCountError(signatureArgumentTypes.size(), argumentExpressions.size(), expression->range());
    }
    if (expectedType->tryResolveTo(functionCaller->signature()->returnType()) == false) {
      throw TypeConvertError(functionCaller->signature()->returnType(), expectedType, expression->range());
    }

    // compile
    std::vector<BinaryenExpressionRef> expressionRefs{};
    for (uint32_t index = 0; index < signatureArgumentTypes.size(); index++) {
      expressionRefs.push_back(compileExpression(argumentExpressions[index], signatureArgumentTypes[index]));
    }
    return BinaryenCall(module_, functionCaller->name().c_str(), expressionRefs.data(), expressionRefs.size(),
                        functionCaller->signature()->returnType()->underlyingTypeName());
  };
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

std::shared_ptr<ir::Symbol> Compiler::resolveVariant(std::shared_ptr<ast::Expression> const &expression) {
  if (std::dynamic_pointer_cast<ast::Identifier>(expression) != nullptr) {
    return std::visit(overloaded{[this](uint64_t i) -> std::shared_ptr<ir::Symbol> {
                                   throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
                                 },
                                 [](double d) -> std::shared_ptr<ir::Symbol> {
                                   throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
                                 },
                                 [this](const std::string &s) -> std::shared_ptr<ir::Symbol> {
                                   auto local = currentFunction()->findLocalByName(s);
                                   if (local != nullptr) {
                                     return local;
                                   }
                                   auto globalIt = globals_.find(s);
                                   if (globalIt != globals_.end()) {
                                     return globalIt->second;
                                   }
                                   auto functionIt = functions_.find(s);
                                   if (functionIt != functions_.cend()) {
                                     return functionIt->second;
                                   }
                                   throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
                                 }},
                      std::dynamic_pointer_cast<ast::Identifier>(expression)->id());
  }
  throw std::runtime_error("not support " __FILE__ "#" + std::to_string(__LINE__));
}

} // namespace walang