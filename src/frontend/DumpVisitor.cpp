#include "frontend/DumpVisitor.h"

#include <iostream>

#include "frontend/AST.h"

DumpVisitor::DumpVisitor(const std::string &filename) : out_file(filename) {
  if (!out_file.is_open()) {
    std::cerr << "Error: Cannot open file " << filename << std::endl;
  }
}

DumpVisitor::~DumpVisitor() {
  if (out_file.is_open()) {
    out_file.close();
  }
}

// 用于缩进
struct IndentGuard {
  int &indent;
  explicit IndentGuard(int &i) : indent(i) { ++indent; }
  ~IndentGuard() { --indent; }
};

void DumpVisitor::print_indent() const {
  for (int i = 0; i < indent_level; ++i)
    out_file << "  ";
}

void DumpVisitor::print_node(const std::string &name) {
  print_indent();
  out_file << name << std::endl;
}

void DumpVisitor::Visit(CompUnitAST &node) {
  print_node("CompUnitAST");
  IndentGuard _{indent_level};
  if (node.func_def)
    node.func_def->Accept(*this);
}

void DumpVisitor::Visit(FuncDefAST &node) {
  print_node("FuncDefAST");
  IndentGuard _{indent_level};
  print_indent();
  out_file << "RetType: " << node.ret_type << std::endl;
  print_indent();
  out_file << "Ident: " << node.ident << std::endl;
  if (node.block)
    node.block->Accept(*this);
}

void DumpVisitor::Visit(BlockAST &node) {
  print_node("BlockAST");
  IndentGuard _{indent_level};
  for (auto &item : node.items) {
    if (item)
      item->Accept(*this);
  }
}

void DumpVisitor::Visit(ConstDeclAST &node) {
  print_node("ConstDeclAST");
  IndentGuard _{indent_level};
  print_indent();
  out_file << "BType: " << node.btype << std::endl;
  for (auto &def : node.const_defs) {
    if (def)
      def->Accept(*this);
  }
}

void DumpVisitor::Visit(ConstDefAST &node) {
  print_node("ConstDefAST");
  IndentGuard _{indent_level};
  print_indent();
  out_file << "Ident: " << node.ident << std::endl;
  if (node.init_val)
    node.init_val->Accept(*this);
}

void DumpVisitor::Visit(VarDeclAST &node) {
  print_node("VarDeclAST");
  IndentGuard _{indent_level};
  print_indent();
  out_file << "BType: " << node.btype << std::endl;
  for (auto &def : node.var_defs) {
    if (def)
      def->Accept(*this);
  }
}

void DumpVisitor::Visit(VarDefAST &node) {
  print_node("VarDefAST");
  IndentGuard _{indent_level};
  print_indent();
  out_file << "Ident: " << node.ident << std::endl;
  if (node.init_val)
    node.init_val->Accept(*this);
}

void DumpVisitor::Visit(AssignStmtAST &node) {
  print_node("AssignStmtAST");
  IndentGuard _{indent_level};
  if (node.lval)
    node.lval->Accept(*this);
  if (node.exp)
    node.exp->Accept(*this);
}

void DumpVisitor::Visit(ExpStmtAST &node) {
  print_node("ExpStmtAST");
  IndentGuard _{indent_level};
  if (node.exp)
    node.exp->Accept(*this);
}

void DumpVisitor::Visit(IfStmtAST &node) {
  print_node("IfStmtAST");
  IndentGuard _{indent_level};
  if (node.exp)
    node.exp->Accept(*this);
  if (node.then_stmt)
    node.then_stmt->Accept(*this);
  if (node.else_stmt)
    node.else_stmt->Accept(*this);
}

void DumpVisitor::Visit(ReturnStmtAST &node) {
  print_node("ReturnStmtAST");
  IndentGuard _{indent_level};
  if (node.exp)
    node.exp->Accept(*this);
}

void DumpVisitor::Visit(LValAST &node) {
  print_indent();
  out_file << "LValAST Ident: " << node.ident << std::endl;
}

void DumpVisitor::Visit(NumberAST &node) {
  print_indent();
  out_file << "NumberAST Val: " << node.val << std::endl;
}

void DumpVisitor::Visit(UnaryExpAST &node) {
  print_node("UnaryExpAST");
  IndentGuard _{indent_level};
  print_indent();
  out_file << "Op: " << node.op << std::endl;
  if (node.exp)
    node.exp->Accept(*this);
}

void DumpVisitor::Visit(BinaryExpAST &node) {
  print_node("BinaryExpAST");
  IndentGuard _{indent_level};
  print_indent();
  out_file << "Op: " << node.op << std::endl;
  if (node.lhs)
    node.lhs->Accept(*this);
  if (node.rhs)
    node.rhs->Accept(*this);
}