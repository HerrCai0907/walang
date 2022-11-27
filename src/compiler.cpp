#include "compiler.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "binaryen-c.h"
#include "helper/overload.hpp"
#include "ir/function.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
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
  if (std::dynamic_pointer_cast<ast::DeclareStatement>(statement) != nullptr) {
  }
  throw std::runtime_error("not support" __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileDeclareStatement(std::shared_ptr<ast::DeclareStatement> const &statement) {
  BinaryenExpressionRef init = compileExpression(statement->init());
  BinaryenGlobalRef globalRef = BinaryenAddGlobal(module_, statement->name().c_str(), BinaryenTypeInt32(), true,
                                                  BinaryenConst(module_, BinaryenLiteralInt32(0)));
  auto global = std::make_shared<ir::Global>(*statement);
  globals_.emplace(statement->name(), global);
  return BinaryenGlobalSet(module_, statement->name().c_str(), init);
}
BinaryenExpressionRef Compiler::compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement) {
  return compileAssignment(statement->variant(), compileExpression(statement->value()));
}
BinaryenExpressionRef Compiler::compileExpressionStatement(std::shared_ptr<ast::ExpressionStatement> const &statement) {
  return BinaryenDrop(module_, compileExpression(statement->expr()));
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
  BinaryenExpressionRef condition = compileExpression(statement->condition());
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
  BinaryenExpressionRef condition = compileExpression(statement->condition());
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

BinaryenExpressionRef Compiler::compileExpression(std::shared_ptr<ast::Expression> const &expression) {
  switch (expression->type()) {
  case ast::Expression::Identifier:
    return compileIdentifier(std::dynamic_pointer_cast<ast::Identifier>(expression));
  case ast::Expression::PrefixExpression:
    return compilePrefixExpression(std::dynamic_pointer_cast<ast::PrefixExpression>(expression));
  case ast::Expression::BinaryExpression:
    return compileBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression));
  case ast::Expression::TernaryExpression:
    return compileTernaryExpression(std::dynamic_pointer_cast<ast::TernaryExpression>(expression));
  }
  throw std::runtime_error("not support");
}

BinaryenExpressionRef Compiler::compileIdentifier(std::shared_ptr<ast::Identifier> const &expression) {
  return std::visit(overloaded{[this](uint64_t i) -> BinaryenExpressionRef {
                                 return BinaryenConst(module_, BinaryenLiteralInt32(static_cast<int>(i)));
                               },
                               [](double d) -> BinaryenExpressionRef { throw std::runtime_error("not support"); },
                               [this](const std::string &s) -> BinaryenExpressionRef {
                                 if (globals_.find(s) != globals_.end()) {
                                   return BinaryenGlobalGet(module_, s.c_str(), BinaryenTypeInt32());
                                 }
                                 throw std::runtime_error("not support" __FILE__ "#" + std::to_string(__LINE__));
                               }},
                    expression->id());
}
BinaryenExpressionRef Compiler::compilePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression) {
  BinaryenExpressionRef exprRef = compileExpression(expression->expr());
  switch (expression->op()) {
  case ast::PrefixOp::ADD: {
    return exprRef;
  }
  case ast::PrefixOp::SUB: {
    BinaryenExpressionRef leftRef = BinaryenConst(module_, BinaryenLiteralInt32(0));
    return BinaryenBinary(module_, BinaryenSubInt32(), leftRef, exprRef);
  }
  case ast::PrefixOp::NOT: {
    return BinaryenUnary(module_, BinaryenEqZInt32(), exprRef);
  }
  }
  throw std::runtime_error("unknown" __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression) {
  BinaryenExpressionRef leftExprRef = compileExpression(expression->leftExpr());
  BinaryenExpressionRef rightExprRef = compileExpression(expression->rightExpr());
  switch (expression->op()) {
  case ast::BinaryOp::ADD: {
    BinaryenOp op = BinaryenAddInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::SUB: {
    BinaryenOp op = BinaryenSubInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::MUL: {
    BinaryenOp op = BinaryenMulInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::DIV: {
    BinaryenOp op = BinaryenDivUInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::MOD: {
    BinaryenOp op = BinaryenRemUInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::LEFT_SHIFT: {
    BinaryenOp op = BinaryenShlInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::RIGHT_SHIFT: {
    BinaryenOp op = BinaryenShrUInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::LESS_THAN: {
    BinaryenOp op = BinaryenLtUInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::GREATER_THAN: {
    BinaryenOp op = BinaryenGtUInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::NO_LESS_THAN: {
    BinaryenOp op = BinaryenGeUInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::NO_GREATER_THAN: {
    BinaryenOp op = BinaryenLeUInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::EQUAL: {
    BinaryenOp op = BinaryenEqInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::NOT_EQUAL: {
    BinaryenOp op = BinaryenNeInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::AND: {
    BinaryenOp op = BinaryenAndInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::OR: {
    BinaryenOp op = BinaryenOrInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::XOR: {
    BinaryenOp op = BinaryenXorInt32();
    return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
  }
  case ast::BinaryOp::LOGIC_AND: {
    auto tempLocal = currentFunction_->addTempLocal();
    auto conditionalExprRef = BinaryenLocalTee(module_, tempLocal->index(), leftExprRef, BinaryenTypeInt32());
    auto loadLeftRexprResult = BinaryenLocalGet(module_, tempLocal->index(), BinaryenTypeInt32());
    return BinaryenIf(module_, conditionalExprRef, rightExprRef, loadLeftRexprResult);
  }
  case ast::BinaryOp::LOGIC_OR: {
    auto tempLocal = currentFunction_->addTempLocal();
    auto conditionalExprRef = BinaryenLocalTee(module_, tempLocal->index(), leftExprRef, BinaryenTypeInt32());
    auto loadLeftRexprResult = BinaryenLocalGet(module_, tempLocal->index(), BinaryenTypeInt32());
    return BinaryenIf(module_, conditionalExprRef, loadLeftRexprResult, rightExprRef);
  }
  }
  throw std::runtime_error("unknown" __FILE__ "#" + std::to_string(__LINE__));
}
BinaryenExpressionRef Compiler::compileTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression) {
  return BinaryenIf(module_, compileExpression(expression->conditionExpr()), compileExpression(expression->leftExpr()),
                    compileExpression(expression->rightExpr()));
}

BinaryenExpressionRef Compiler::compileAssignment(std::shared_ptr<ast::Expression> const &expression,
                                                  BinaryenExpressionRef value) {
  if (std::dynamic_pointer_cast<ast::Identifier>(expression) != nullptr) {

    return std::visit(
        overloaded{[this](uint64_t i) -> BinaryenExpressionRef { throw std::runtime_error("not support"); },
                   [](double d) -> BinaryenExpressionRef { throw std::runtime_error("not support"); },
                   [this, value](const std::string &s) -> BinaryenExpressionRef {
                     if (globals_.find(s) != globals_.end()) {
                       return BinaryenGlobalSet(module_, s.c_str(), value);
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