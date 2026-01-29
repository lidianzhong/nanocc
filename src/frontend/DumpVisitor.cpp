#include "nanocc/frontend/DumpVisitor.h"

#include <iostream>

#include "nanocc/frontend/AST.h"

namespace nanocc {

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
  for (auto &item : node.items) {
    if (item)
      item->Accept(*this);
  }
}

void DumpVisitor::Visit(FuncFParamAST &node) {
  print_indent();
  out_file << "FuncFParamAST { BType: " << node.btype
           << ", Ident: " << node.ident << " }" << std::endl;
}

void DumpVisitor::Visit(FuncDefAST &node) {
  print_indent();
  out_file << "FuncDefAST { Ident: " << node.ident
           << ", RetType: " << node.ret_type << " }" << std::endl;

  IndentGuard _{indent_level};
  if (!node.params.empty()) {
    print_indent();
    out_file << "Params:" << std::endl;
    IndentGuard _t{indent_level};
    for (auto &param : node.params) {
      if (param)
        param->Accept(*this);
    }
  }

  print_indent();
  out_file << "Block:" << std::endl;
  IndentGuard _b{indent_level};
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
  print_indent();
  out_file << "ConstDeclAST { BType: " << node.btype << " }" << std::endl;
  IndentGuard _{indent_level};
  for (auto &def : node.const_defs) {
    if (def)
      def->Accept(*this);
  }
}

void DumpVisitor::Visit(ConstDefAST &node) {
  print_indent();
  out_file << "ConstDefAST { Ident: " << node.ident << " }" << std::endl;
  IndentGuard _{indent_level};
  if (!node.dims.empty()) {
    print_indent();
    out_file << "Dims:" << std::endl;
    IndentGuard _i{indent_level};
    for (auto &dim : node.dims) {
      dim->Accept(*this);
    }
  }
  if (node.init)
    node.init->Accept(*this);
}

void DumpVisitor::Visit(VarDeclAST &node) {
  print_indent();
  out_file << "VarDeclAST { BType: " << node.btype << " }" << std::endl;
  IndentGuard _{indent_level};
  for (auto &def : node.var_defs) {
    if (def)
      def->Accept(*this);
  }
}

void DumpVisitor::Visit(VarDefAST &node) {
  print_indent();
  out_file << "VarDefAST { Ident: " << node.ident << " }" << std::endl;
  IndentGuard _{indent_level};
  if (!node.dims.empty()) {
    print_indent();
    out_file << "Dims:" << std::endl;
    IndentGuard _i{indent_level};
    for (auto &dim : node.dims) {
      dim->Accept(*this);
    }
  }
  if (node.init)
    node.init->Accept(*this);
}

void DumpVisitor::Visit(InitVarAST &node) {
  print_indent();
  if (node.IsList()) {
    out_file << "InitVarListAST" << std::endl;
  } else {
    out_file << "InitVarExprAST" << std::endl;
  }
  IndentGuard _{indent_level};
  if (node.IsList()) {
    for (auto &val : node.initList) {
      if (val)
        val->Accept(*this);
    }
  } else {
    if (node.initExpr)
      node.initExpr->Accept(*this);
  }
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

void DumpVisitor::Visit(WhileStmtAST &node) {
  print_node("WhileStmtAST");
  IndentGuard _{indent_level};
  if (node.cond)
    node.cond->Accept(*this);
  if (node.body)
    node.body->Accept(*this);
}

void DumpVisitor::Visit(BreakStmtAST &node) {
  print_node("BreakStmtAST");
  IndentGuard _{indent_level};
}

void DumpVisitor::Visit(ContinueStmtAST &node) {
  print_node("ContinueStmtAST");
  IndentGuard _{indent_level};
}

void DumpVisitor::Visit(ReturnStmtAST &node) {
  print_node("ReturnStmtAST");
  IndentGuard _{indent_level};
  if (node.exp)
    node.exp->Accept(*this);
}

void DumpVisitor::Visit(LValAST &node) {
  print_indent();
  out_file << "LValAST { Ident: " << node.ident << " }" << std::endl;
  IndentGuard _{indent_level};
  if (!node.indices.empty()) {
    print_indent();
    out_file << "Indices:" << std::endl;
    IndentGuard _i{indent_level};
    for (auto &index : node.indices) {
      index->Accept(*this);
    }
  }
}

void DumpVisitor::Visit(NumberAST &node) {
  print_indent();
  out_file << "NumberAST { Val: " << node.val << " }" << std::endl;
}

void DumpVisitor::Visit(UnaryExpAST &node) {
  print_indent();
  out_file << "UnaryExpAST { Op: " << node.op << " }" << std::endl;
  IndentGuard _{indent_level};
  if (node.exp)
    node.exp->Accept(*this);
}

void DumpVisitor::Visit(BinaryExpAST &node) {
  print_indent();
  out_file << "BinaryExpAST { Op: " << node.op << " }" << std::endl;
  IndentGuard _{indent_level};
  if (node.lhs)
    node.lhs->Accept(*this);
  if (node.rhs)
    node.rhs->Accept(*this);
}

void DumpVisitor::Visit(FuncCallAST &node) {
  print_indent();
  out_file << "FuncCallAST { Ident: " << node.ident << " }" << std::endl;
  IndentGuard _{indent_level};
  for (auto &arg : node.args) {
    if (arg)
      arg->Accept(*this);
  }
}

} // namespace nanocc