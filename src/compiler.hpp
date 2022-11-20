#pragma once

#include "ast/expression.hpp"
#include "ast/file.hpp"
#include "ast/statement.hpp"
#include "ir/variant.hpp"
#include <binaryen-c.h>
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

  void dumpModule() { BinaryenModulePrint(module_); }

private:
  BinaryenExpressionRef compileStatement(std::shared_ptr<ast::Statement> const &statement);
  BinaryenExpressionRef compileDeclareStatement(std::shared_ptr<ast::DeclareStatement> const &statement);
  BinaryenExpressionRef compileAssignStatement(std::shared_ptr<ast::AssignStatement> const &statement);

  BinaryenExpressionRef compileExpression(std::shared_ptr<ast::Expression> const &expression);
  BinaryenExpressionRef compileIdentifier(std::shared_ptr<ast::Identifier> const &expression);
  BinaryenExpressionRef compileBinaryExpression(std::shared_ptr<ast::BinaryExpression> const &expression);

  BinaryenExpressionRef compileAssignment(std::shared_ptr<ast::Expression> const &expression,
                                          BinaryenExpressionRef value);

private:
  BinaryenModuleRef module_;

  std::vector<std::shared_ptr<ast::File>> files_;
  std::unordered_map<std::string, std::shared_ptr<ir::Global>> globals_;
};

} // namespace walang