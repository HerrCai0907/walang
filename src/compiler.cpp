#include "compiler.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "helper/overload.hpp"
#include "ir/function.hpp"
#include "ir/variant_type.hpp"
#include <algorithm>
#include <binaryen-c.h>
#include <cassert>
#include <cstdint>
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
    currentFunction_ = std::make_shared<ir::Function>("_start");
    std::vector<BinaryenExpressionRef> expressions{};
    for (auto &statement : file->statement()) {
      expressions.emplace_back(compileStatement(statement));
    }
    currentFunction_->finialize(
        module_, BinaryenBlock(module_, nullptr, expressions.data(), expressions.size(), BinaryenTypeNone()));
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
  }
  throw std::runtime_error("not support" __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileDeclareStatement(std::shared_ptr<ast::DeclareStatement> const &statement) {
  auto global = std::make_shared<ir::Global>(*statement);
  std::shared_ptr<ir::VariantType> const &variantType = global->variantType();
  BinaryenExpressionRef init = compileExpression(statement->init(), global->variantType());
  BinaryenGlobalRef globalRef = BinaryenAddGlobal(module_, global->name().c_str(), variantType->underlyingTypeName(),
                                                  true, variantType->underlyingDefaultValue(module_));
  globals_.emplace(statement->name(), global);
  return BinaryenGlobalSet(module_, statement->name().c_str(), init);
}
BinaryenExpressionRef Compiler::compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement) {
  auto variant = resolveVariant(statement->variant());
  auto exprRef = compileExpression(statement->value(), variant->variantType());
  return variant->makeAssign(module_, exprRef);
}
BinaryenExpressionRef Compiler::compileExpressionStatement(std::shared_ptr<ast::ExpressionStatement> const &statement) {
  return BinaryenDrop(module_, compileExpression(statement->expr(), std::make_shared<ir::TypeAuto>()));
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
  auto breakLabel = createBreakLabel("while");
  auto continueLabel = createContinueLabel("while");
  BinaryenExpressionRef condition = compileExpression(statement->condition(), std::make_shared<ir::TypeCondition>());
  std::vector<BinaryenExpressionRef> block = {
      compileBlockStatement(statement->block()),
      BinaryenBreak(module_, continueLabel.c_str(), nullptr, nullptr),
  };
  BinaryenExpressionRef body = BinaryenIf(
      module_, condition, BinaryenBlock(module_, nullptr, block.data(), block.size(), BinaryenTypeNone()), nullptr);
  freeBreakLabel();
  freeContinueLabel();
  BinaryenExpressionRef loop = BinaryenLoop(module_, continueLabel.c_str(), body);
  return BinaryenBlock(module_, breakLabel.c_str(), &loop, 1U, BinaryenTypeNone());
}
BinaryenExpressionRef Compiler::compileBreakStatement(std::shared_ptr<ast::BreakStatement> const &statement) {
  if (currentBreakLabel_.size() == 0) {
    throw std::runtime_error("invalid break statement");
  }
  return BinaryenBreak(module_, currentBreakLabel_.top().c_str(), nullptr, nullptr);
}
BinaryenExpressionRef Compiler::compileContinueStatement(std::shared_ptr<ast::ContinueStatement> const &statement) {
  if (currentContinueLabel_.size() == 0) {
    throw std::runtime_error("invalid continue statement");
  }
  return BinaryenBreak(module_, currentContinueLabel_.top().c_str(), nullptr, nullptr);
}

// ███████ ██   ██ ██████  ██████  ███████ ███████ ███████ ██  ██████  ███    ██
// ██       ██ ██  ██   ██ ██   ██ ██      ██      ██      ██ ██    ██ ████   ██
// █████     ███   ██████  ██████  █████   ███████ ███████ ██ ██    ██ ██ ██  ██
// ██       ██ ██  ██      ██   ██ ██           ██      ██ ██ ██    ██ ██  ██ ██
// ███████ ██   ██ ██      ██   ██ ███████ ███████ ███████ ██  ██████  ██   ████

BinaryenExpressionRef Compiler::compileExpression(std::shared_ptr<ast::Expression> const &expression,
                                                  std::shared_ptr<ir::VariantType> const &expectedType) {
  switch (expression->type()) {
  case ast::Expression::Identifier:
    return compileIdentifier(std::dynamic_pointer_cast<ast::Identifier>(expression), expectedType);
  case ast::Expression::PrefixExpression:
    return compilePrefixExpression(std::dynamic_pointer_cast<ast::PrefixExpression>(expression), expectedType);
  case ast::Expression::BinaryExpression:
    return compileBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression), expectedType);
  case ast::Expression::TernaryExpression:
    return compileTernaryExpression(std::dynamic_pointer_cast<ast::TernaryExpression>(expression), expectedType);
  }
  throw std::runtime_error("not support");
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
                               [this, &expectedType](double d) -> BinaryenExpressionRef {
                                 auto pendingType = std::dynamic_pointer_cast<ir::PendingResolveType>(expectedType);
                                 if (pendingType != nullptr && !pendingType->isResolved()) {
                                   // not resolve, default f32
                                   pendingType->tryResolveTo(std::make_shared<ir::TypeF32>());
                                 }
                                 return expectedType->underlyingConst(module_, d);
                               },
                               [this, &expectedType](const std::string &s) -> BinaryenExpressionRef {
                                 auto globalIt = globals_.find(s);
                                 if (globalIt != globals_.end()) {
                                   if (!expectedType->tryResolveTo(globalIt->second->variantType())) {
                                     throw std::runtime_error(fmt::format("invalid convert from {0} to {1}",
                                                                          globalIt->second->variantType()->to_string(),
                                                                          expectedType->to_string()));
                                   }
                                   return BinaryenGlobalGet(module_, s.c_str(), expectedType->underlyingTypeName());
                                 }
                                 throw std::runtime_error("not support" __FILE__ "#" + std::to_string(__LINE__));
                               }},
                    expression->id());
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
  return expectedType->handleBinaryOp(module_, expression->op(), leftExprRef, rightExprRef, currentFunction_);
}
BinaryenExpressionRef Compiler::compileTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression,
                                                         std::shared_ptr<ir::VariantType> const &expectedType) {
  return BinaryenIf(module_, compileExpression(expression->conditionExpr(), std::make_shared<ir::TypeCondition>()),
                    compileExpression(expression->leftExpr(), expectedType),
                    compileExpression(expression->rightExpr(), expectedType));
}

std::shared_ptr<ir::Variant> Compiler::resolveVariant(std::shared_ptr<ast::Expression> const &expression) {
  if (std::dynamic_pointer_cast<ast::Identifier>(expression) != nullptr) {
    return std::visit(
        overloaded{[this](uint64_t i) -> std::shared_ptr<ir::Variant> { throw std::runtime_error("not support"); },
                   [](double d) -> std::shared_ptr<ir::Variant> { throw std::runtime_error("not support"); },
                   [this](const std::string &s) -> std::shared_ptr<ir::Variant> {
                     auto it = globals_.find(s);
                     if (it != globals_.end()) {
                       return it->second;
                     }
                     throw std::runtime_error("not support" __FILE__ "#" + std::to_string(__LINE__));
                   }},
        std::dynamic_pointer_cast<ast::Identifier>(expression)->id());
  }
  throw std::runtime_error("not support" __FILE__ "#" + std::to_string(__LINE__));
}

std::string const &Compiler::createBreakLabel(std::string const &prefix) {
  std::string const &str = currentBreakLabel_.emplace(prefix + "|break|" + std::to_string(breakLabelIndex_));
  breakLabelIndex_++;
  return str;
}
std::string const &Compiler::createContinueLabel(std::string const &prefix) {
  std::string const &str = currentContinueLabel_.emplace(prefix + "|continue|" + std::to_string(continueLabelIndex_));
  continueLabelIndex_++;
  return str;
}
void Compiler::freeBreakLabel() { currentBreakLabel_.pop(); }
void Compiler::freeContinueLabel() { currentContinueLabel_.pop(); }

} // namespace walang