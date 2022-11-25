#include "compiler.hpp"
#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "binaryen-c.h"
#include "helper/overload.hpp"
#include "ir/function.hpp"
#include <cassert>
#include <exception>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

namespace walang {

Compiler::Compiler(std::vector<std::shared_ptr<ast::File>> files)
    : module_{BinaryenModuleCreate()}, files_{files}, globals_{} {}

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

BinaryenExpressionRef Compiler::compileStatement(std::shared_ptr<ast::Statement> const &statement) {
  switch (statement->type()) {
  case ast::Statement::DeclareStatement:
    return compileDeclareStatement(std::dynamic_pointer_cast<ast::DeclareStatement>(statement));
  case ast::Statement::AssignStatement:
    return compileAssignStatement(std::dynamic_pointer_cast<ast::AssignStatement>(statement));
  case ast::Statement::ExpressionStatement:
    return compileExpressionStatement(std::dynamic_pointer_cast<ast::ExpressionStatement>(statement));
  }
  if (std::dynamic_pointer_cast<ast::DeclareStatement>(statement) != nullptr) {
  }
  throw std::runtime_error("not support");
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

BinaryenExpressionRef Compiler::compileExpression(std::shared_ptr<ast::Expression> const &expression) {
  switch (expression->type()) {
  case ast::Expression::Identifier:
    return compileIdentifier(std::dynamic_pointer_cast<ast::Identifier>(expression));
  case ast::Expression::PrefixExpression:
    return compilePrefixExpression(std::dynamic_pointer_cast<ast::PrefixExpression>(expression));
  case ast::Expression::BinaryExpression:
    return compileBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression));
  case ast::Expression::TernaryExpression:
    break;
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
                                 throw std::runtime_error("not support");
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
    return BinaryenUnary(module_, BinaryenNeInt32(), exprRef);
  }
  }
  throw std::runtime_error("unknown");
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
  throw std::runtime_error("unknown");
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
                     throw std::runtime_error("not support");
                   }},
        std::dynamic_pointer_cast<ast::Identifier>(expression)->id());
  }
  throw std::runtime_error("not support");
}

} // namespace walang