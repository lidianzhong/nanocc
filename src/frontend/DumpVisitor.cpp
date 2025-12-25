#include "frontend/DumpVisitor.h"
#include "frontend/AST.h"

// 用于缩进
struct IndentGuard {
  int &indent;
  explicit IndentGuard(int &i) : indent(i) { ++indent; }
  ~IndentGuard() { --indent; }
};

void DumpVisitor::print_indent() const {
  for (int i = 0; i < indent_level; ++i)
    std::cout << "  ";
}

void DumpVisitor::print_node(const std::string &name) {
  print_indent();
  std::cout << name << std::endl;
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
  std::cout << "RetType: " << node.ret_type << std::endl;
  print_indent();
  std::cout << "Ident: " << node.ident << std::endl;
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
  std::cout << "BType: " << node.btype << std::endl;
  for (auto &def : node.const_defs) {
    if (def)
      def->Accept(*this);
  }
}

void DumpVisitor::Visit(ConstDefAST &node) {
  print_node("ConstDefAST");
  IndentGuard _{indent_level};
  print_indent();
  std::cout << "Ident: " << node.ident << std::endl;
  if (node.init_val)
    node.init_val->Accept(*this);
}

void DumpVisitor::Visit(VarDeclAST &node) {
  print_node("VarDeclAST");
  IndentGuard _{indent_level};
  print_indent();
  std::cout << "BType: " << node.btype << std::endl;
  for (auto &def : node.var_defs) {
    if (def)
      def->Accept(*this);
  }
}

void DumpVisitor::Visit(VarDefAST &node) {
  print_node("VarDefAST");
  IndentGuard _{indent_level};
  print_indent();
  std::cout << "Ident: " << node.ident << std::endl;
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

void DumpVisitor::Visit(ReturnStmtAST &node) {
  print_node("ReturnStmtAST");
  IndentGuard _{indent_level};
  if (node.exp)
    node.exp->Accept(*this);
}

void DumpVisitor::Visit(LValAST &node) {
  print_indent();
  std::cout << "LValAST Ident: " << node.ident << std::endl;
}

void DumpVisitor::Visit(NumberAST &node) {
  print_indent();
  std::cout << "NumberAST Val: " << node.val << std::endl;
}

void DumpVisitor::Visit(UnaryExpAST &node) {
  print_node("UnaryExpAST");
  IndentGuard _{indent_level};
  print_indent();
  std::cout << "Op: " << node.op << std::endl;
  if (node.exp)
    node.exp->Accept(*this);
}

void DumpVisitor::Visit(BinaryExpAST &node) {
  print_node("BinaryExpAST");
  IndentGuard _{indent_level};
  print_indent();
  std::cout << "Op: " << node.op << std::endl;
  if (node.lhs)
    node.lhs->Accept(*this);
  if (node.rhs)
    node.rhs->Accept(*this);
}