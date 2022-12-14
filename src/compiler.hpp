#pragma once

#include "ast/expression.hpp"
#include "ast/file.hpp"
#include "ast/statement.hpp"
#include "ir/variant.hpp"
#include "ir/variant_type.hpp"
#include "resolver.hpp"
#include "variant_type_table.hpp"
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
  void prepareFunctionStatement(ast::FunctionStatement const &statement);
  std::shared_ptr<ir::Function> prepareMethod(ast::FunctionStatement const &statement,
                                              std::shared_ptr<ir::Class> const &classType);
  std::shared_ptr<ir::Function> doPrepareFunction(std::string const &name, std::vector<std::string> argumentNames,
                                                  std::vector<std::shared_ptr<ir::VariantType>> argumentTypes,
                                                  std::shared_ptr<ir::VariantType> const &returnType,
                                                  std::shared_ptr<ir::Class> const &classType,
                                                  std::vector<std::string> const &decorators);
  /// @brief prepare memory layout
  void prepareClassStatementLevel1(ast::ClassStatement const &statement);
  /// @brief prepare method map
  void prepareClassStatementLevel2(ast::ClassStatement const &statement);

private:
  std::vector<BinaryenExpressionRef> compileStatement(std::shared_ptr<ast::Statement> const &statement);
  std::vector<BinaryenExpressionRef> compileDeclareStatement(std::shared_ptr<ast::DeclareStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement);
  std::vector<BinaryenExpressionRef>
  compileExpressionStatement(std::shared_ptr<ast::ExpressionStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileBlockStatement(std::shared_ptr<ast::BlockStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileIfStatement(std::shared_ptr<ast::IfStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileWhileStatement(std::shared_ptr<ast::WhileStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileBreakStatement(std::shared_ptr<ast::BreakStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileContinueStatement(std::shared_ptr<ast::ContinueStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileReturnStatement(std::shared_ptr<ast::ReturnStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileClassStatement(std::shared_ptr<ast::ClassStatement> const &statement);
  std::vector<BinaryenExpressionRef> compileFunctionStatement(std::shared_ptr<ast::FunctionStatement> const &statement);

  std::shared_ptr<ir::Function> compileClassMethod(std::shared_ptr<ir::Class> const &classType,
                                                   std::shared_ptr<ast::FunctionStatement> const &statement);
  void compileClassConstructor(std::shared_ptr<ir::Class> const &classType);

  std::shared_ptr<ir::Function> doCompileFunction(std::string const &name,
                                                  std::shared_ptr<ast::BlockStatement> const &body);

  BinaryenExpressionRef compileExpressionToExpressionRef(std::shared_ptr<ast::Expression> const &expression,
                                                         std::shared_ptr<ir::VariantType> const &expectedType);
  std::vector<BinaryenExpressionRef>
  compileExpressionToExpressionRefs(std::shared_ptr<ast::Expression> const &expression,
                                    std::shared_ptr<ir::VariantType> const &expectedType);

  std::shared_ptr<ir::Variant> compileExpression(std::shared_ptr<ast::Expression> const &expression,
                                                 std::shared_ptr<ir::VariantType> const &expectedType);
  std::shared_ptr<ir::Variant> compileIdentifier(std::shared_ptr<ast::Identifier> const &expression,
                                                 std::shared_ptr<ir::VariantType> const &expectedType);
  std::shared_ptr<ir::Variant> compilePrefixExpression(std::shared_ptr<ast::PrefixExpression> const &expression,
                                                       std::shared_ptr<ir::VariantType> const &expectedType);
  std::shared_ptr<ir::Variant> compileBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression,
                                                       std::shared_ptr<ir::VariantType> const &expectedType);
  std::shared_ptr<ir::Variant> compileTernaryExpression(std::shared_ptr<ast::TernaryExpression> const &expression,
                                                        std::shared_ptr<ir::VariantType> const &expectedType);
  std::shared_ptr<ir::Variant> compileCallExpression(std::shared_ptr<ast::CallExpression> const &expression,
                                                     std::shared_ptr<ir::VariantType> const &expectedType);
  std::shared_ptr<ir::Variant> compileMemberExpression(std::shared_ptr<ast::MemberExpression> const &expression,
                                                       std::shared_ptr<ir::VariantType> const &expectedType);

  [[nodiscard]] std::shared_ptr<ir::Function> const &currentFunction() const { return currentFunction_.top(); }

private:
  BinaryenModuleRef module_;
  std::vector<std::shared_ptr<ast::File>> files_;
  std::shared_ptr<VariantTypeMap> variantTypeMap_;
  Resolver resolver_;

  std::stack<std::shared_ptr<ir::Function>> currentFunction_{};
  std::shared_ptr<ir::Function> startFunction_{};
};

} // namespace walang