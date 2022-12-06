#pragma once

#include "ast/expression.hpp"
#include "ast/file.hpp"
#include "ast/statement.hpp"

#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include "symbol_table.hpp"
#include <binaryen-c.h>
#include <memory>
#include <stack>
#include <vector>

namespace walang {

class Compiler {
public:
  explicit Compiler(std::vector<std::shared_ptr<ast::File>> files);
  explicit Compiler(Compiler const &) = delete;
  explicit Compiler(Compiler &&) = delete;
  Compiler &operator=(Compiler const &) = delete;
  Compiler &operator=(Compiler &&) = delete;

  ~Compiler() { BinaryenModuleDispose(module_); }

  void compile();
  [[nodiscard]] BinaryenModuleRef module() const noexcept { return module_; }
  [[nodiscard]] std::string wat() const;

private:
  BinaryenExpressionRef compileStatement(std::shared_ptr<ast::Statement> const &statement);
  BinaryenExpressionRef compileDeclareStatement(std::shared_ptr<ast::DeclareStatement> const &statement);
  BinaryenExpressionRef compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement);
  BinaryenExpressionRef compileExpressionStatement(std::shared_ptr<ast::ExpressionStatement> const &statement);
  BinaryenExpressionRef compileBlockStatement(std::shared_ptr<ast::BlockStatement> const &statement);
  BinaryenExpressionRef compileIfStatement(std::shared_ptr<ast::IfStatement> const &statement);
  BinaryenExpressionRef compileWhileStatement(std::shared_ptr<ast::WhileStatement> const &statement);
  BinaryenExpressionRef compileBreakStatement(std::shared_ptr<ast::BreakStatement> const &statement);
  BinaryenExpressionRef compileContinueStatement(std::shared_ptr<ast::ContinueStatement> const &statement);
  BinaryenExpressionRef compileReturnStatement(std::shared_ptr<ast::ReturnStatement> const &statement);
  BinaryenExpressionRef compileClassStatement(std::shared_ptr<ast::ClassStatement> const &statement);
  std::shared_ptr<ir::Function> compileClassMethod(std::shared_ptr<ir::Class> const &classType,
                                                   std::shared_ptr<ast::FunctionStatement> const &statement);

  BinaryenExpressionRef compileFunctionStatement(std::shared_ptr<ast::FunctionStatement> const &statement);

  BinaryenExpressionRef compileExpression(std::shared_ptr<ast::Expression> const &expression,
                                          std::shared_ptr<ir::VariantType> const &expectedType);
  BinaryenExpressionRef compileIdentifier(std::shared_ptr<ast::Identifier> const &expression,
                                          std::shared_ptr<ir::VariantType> const &expectedType);
  BinaryenExpressionRef compilePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression,
                                                std::shared_ptr<ir::VariantType> const &expectedType);
  BinaryenExpressionRef compileBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression,
                                                std::shared_ptr<ir::VariantType> const &expectedType);
  BinaryenExpressionRef compileTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression,
                                                 std::shared_ptr<ir::VariantType> const &expectedType);
  BinaryenExpressionRef compileCallExpression(std::shared_ptr<ast::CallExpression> const &expression,
                                              std::shared_ptr<ir::VariantType> const &expectedType);
  BinaryenExpressionRef compileMemberExpression(std::shared_ptr<ast::MemberExpression> const &expression,
                                                std::shared_ptr<ir::VariantType> const &expectedType);

  std::shared_ptr<ir::Symbol> resolveVariant(std::shared_ptr<ast::Expression> const &expression);

  [[nodiscard]] std::shared_ptr<ir::Function> const &currentFunction() const { return currentFunction_.top(); }

private:
  BinaryenModuleRef module_;

  std::vector<std::shared_ptr<ast::File>> files_;

  VariantTypeMap variantTypeMap_{};

  std::unordered_map<std::string, std::shared_ptr<ir::Global>> globals_{};
  std::unordered_map<std::string, std::shared_ptr<ir::Function>> functions_{};

  std::stack<std::shared_ptr<ir::Function>> currentFunction_{};
  std::shared_ptr<ir::Function> startFunction_{};
};

} // namespace walang