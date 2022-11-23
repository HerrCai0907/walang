#include "compiler.hpp"
#include "ast/statement.hpp"
#include "binaryen-c.h"
#include "helper/overload.hpp"
#include <cassert>
#include <exception>
#include <stdexcept>
#include <variant>
#include <vector>

namespace walang {

Compiler::Compiler(std::vector<std::shared_ptr<ast::File>> files)
    : module_{BinaryenModuleCreate()}, files_{files}, globals_{} {}

void Compiler::compile() {
  for (auto const &file : files_) {
    std::vector<BinaryenExpressionRef> expressions{};
    for (auto &statement : file->statement()) {
      expressions.emplace_back(compileStatement(statement));
    }
    BinaryenAddFunction(module_, "_start", BinaryenTypeNone(), BinaryenTypeNone(), nullptr, 0,
                        BinaryenBlock(module_, nullptr, expressions.data(), expressions.size(), BinaryenTypeNone()));
  }
}

BinaryenExpressionRef Compiler::compileStatement(std::shared_ptr<ast::Statement> const &statement) {
  if (std::dynamic_pointer_cast<ast::DeclareStatement>(statement) != nullptr) {
    return compileDeclareStatement(std::dynamic_pointer_cast<ast::DeclareStatement>(statement));
  } else if (std::dynamic_pointer_cast<ast::AssignStatement>(statement) != nullptr) {
    return compileAssignStatement(std::dynamic_pointer_cast<ast::AssignStatement>(statement));
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

BinaryenExpressionRef Compiler::compileExpression(std::shared_ptr<ast::Expression> const &expression) {
  if (std::dynamic_pointer_cast<ast::BinaryExpression>(expression) != nullptr) {
    return compileBinaryExpression(std::dynamic_pointer_cast<ast::BinaryExpression>(expression));
  } else if (std::dynamic_pointer_cast<ast::Identifier>(expression) != nullptr) {
    return compileIdentifier(std::dynamic_pointer_cast<ast::Identifier>(expression));
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
                                   return BinaryenGlobalGet(module_, s.c_str(), BinaryenTypeAuto());
                                 }
                                 throw std::runtime_error("not support");
                               }},
                    expression->id());
}
BinaryenExpressionRef Compiler::compileBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression) {
  BinaryenExpressionRef leftExprRef = compileExpression(expression->leftExpr());
  BinaryenExpressionRef rightExprRef = compileExpression(expression->rightExpr());
  BinaryenOp op;
  switch (expression->op()) {
  case ast::BinaryOp::ADD:
    op = BinaryenAddInt32();
    break;
  case ast::BinaryOp::MUL:
    op = BinaryenMulInt32();
    break;
  default:
    std::terminate();
  }
  return BinaryenBinary(module_, op, leftExprRef, rightExprRef);
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