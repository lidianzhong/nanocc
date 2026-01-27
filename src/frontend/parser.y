%code requires {
  #include <memory>
  #include <string>
  #include "frontend/AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "frontend/AST.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<ldz::BaseAST> &ast, const char *s);

using namespace std;
using namespace ldz;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<ldz::BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  ldz::BaseAST *ast_val;
  std::vector<std::unique_ptr<ldz::BaseAST>> *ast_list;
  std::vector<std::unique_ptr<ldz::FuncFParamAST>> *param_list;
}

// lexer 返回的所有 token 种类的声明
/* 关键字 */
%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE VOID
/* 标识符与数值 */
%token <str_val> IDENT
%token <int_val> INT_CONST
/* 运算符与标点 */
%token LE GE EQ NE  /* <=, >=, ==, != */
%token AND OR       /* &&, || */

// 非终结符的类型定义
%type <ast_val> CompUnit Decl ConstDecl VarDecl ConstDef VarDef ConstInitVal InitVal
%type <ast_val> FuncFParam FuncDef Block BlockItem Stmt MatchedStmt UnMatchedStmt
%type <ast_val> Exp LVal PrimaryExp Number UnaryExp
%type <ast_val> MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <str_val> BType UnaryOp
%type <ast_list> ConstDefList VarDefList BlockItemList ConstInitValList InitValList FuncRParams ArrayIndices ArrayDims
%type <param_list> FuncFParams

%%

// CompUnit ::= [CompUnit] ( Decl | FuncDef )
CompUnit
  : CompUnit FuncDef {
    auto comp_unit = static_cast<CompUnitAST*>($1);
    comp_unit->items.push_back(unique_ptr<BaseAST>($2));
    $$ = comp_unit;
  }
  | CompUnit Decl {
    auto comp_unit = static_cast<CompUnitAST*>($1);
    comp_unit->items.push_back(unique_ptr<BaseAST>($2));
    $$ = comp_unit;
  }
  | FuncDef {
    auto comp_unit = new CompUnitAST();
    comp_unit->items.push_back(unique_ptr<BaseAST>($1));
    $$ = comp_unit;
    ast.reset(comp_unit);
  }
  | Decl {
    auto comp_unit = new CompUnitAST();
    comp_unit->items.push_back(unique_ptr<BaseAST>($1));
    $$ = comp_unit;
    ast.reset(comp_unit);
  }
  ;

// BType ::= "int"
BType
  : INT {
    $$ = new string("int");
  }
  ;

// Decl ::= ConstDecl | VarDecl
Decl
  : ConstDecl { $$ = $1; }
  | VarDecl   { $$ = $1; }
  ;

// ConstDecl ::= "const" BType ConstDef {"," ConstDef} ";"
ConstDecl
  : CONST BType ConstDefList ';' {
    auto ast = new ConstDeclAST();
    ast->btype = *unique_ptr<string>($2);
    ast->const_defs = std::move(*$3);
    delete $3;
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    $$ = new vector<unique_ptr<BaseAST>>();
    $$->push_back(unique_ptr<BaseAST>($1));
  }
  | ConstDefList ',' ConstDef {
    $1->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

// ConstDef ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal
ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init = std::unique_ptr<InitVarAST>(dynamic_cast<InitVarAST*>($3));
    $$ = ast;
  }
  | IDENT ArrayDims '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->dims = std::move(*static_cast<std::vector<std::unique_ptr<BaseAST>>*>($2));
    ast->init = std::unique_ptr<InitVarAST>(dynamic_cast<InitVarAST*>($4));
    $$ = ast;
  }

ArrayDims
  : /* empty */ {
    $$ = new std::vector<std::unique_ptr<BaseAST>>();
  }
  | ArrayDims '[' ConstExp ']' {
    $1->push_back(std::unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

// ConstInitVal ::= ConstExp | "{" [ConstExp {"," ConstExp}] "}";
ConstInitVal
  : ConstExp { 
    auto ast = new InitVarAST();
    ast->initExpr = unique_ptr<BaseAST>($1);
    $$ = ast;
   }
  | '{' '}' {
    auto ast = new InitVarAST();
    ast->initExpr = nullptr;
    ast->initList = std::vector<std::unique_ptr<InitVarAST>>();
    $$ = ast;
  }
  | '{' ConstInitValList '}' {
    auto ast = new InitVarAST();
    ast->initExpr = nullptr;
    ast->initList.reserve($2->size());
    for (auto &ptr : *$2) {
      ast->initList.push_back(unique_ptr<InitVarAST>(dynamic_cast<InitVarAST*>(ptr.release())));
    }
    delete $2;
    $$ = ast;
  }
  ;

// ConstInitValList ::= ConstInitVal {"," ConstInitValList}
ConstInitValList
  : ConstInitVal {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    vec->push_back(std::unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstInitValList ',' ConstInitVal {
    $1->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

// VarDecl ::= BType VarDef {"," VarDef} ";"
VarDecl
  : BType VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->btype = *unique_ptr<string>($1);
    ast->var_defs = std::move(*$2);
    delete $2;
    $$ = ast;
  }
  ;

VarDefList
  : VarDef {
    $$ = new vector<unique_ptr<BaseAST>>();
    $$->push_back(unique_ptr<BaseAST>($1));
  }
  | VarDefList ',' VarDef {
    $1->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

// VarDef ::= IDENT {"[" ConstExp "]"}
//          | IDENT {"[" ConstExp "]"} "=" InitVal
VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init = nullptr;
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init = unique_ptr<InitVarAST>(dynamic_cast<InitVarAST*>($3));
    $$ = ast;
  }
  | IDENT ArrayIndices {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->dims = std::move(*static_cast<std::vector<std::unique_ptr<BaseAST>>*>($2));
    ast->init = nullptr;
    $$ = ast;
  }
  | IDENT ArrayIndices '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->dims = std::move(*static_cast<std::vector<std::unique_ptr<BaseAST>>*>($2));
    ast->init = unique_ptr<InitVarAST>(dynamic_cast<InitVarAST*>($4));
    $$ = ast;
  }
  ;

// ArrayIndices ::= "[" ConstExp "]" { "[" ConstExp "]" }
ArrayIndices
  : '[' ConstExp ']' {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    vec->push_back(std::unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | ArrayIndices '[' ConstExp ']' {
    $1->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

// InitVal ::= Exp | "{" [InitVal {"," InitVal}] "}";
InitVal
  : Exp { 
    auto ast = new InitVarAST();
    ast->initExpr = unique_ptr<BaseAST>($1);
    $$ = ast;
   }
  | '{' '}' {
    auto ast = new InitVarAST();
    ast->initExpr = nullptr;
    ast->initList = std::vector<std::unique_ptr<InitVarAST>>();
    $$ = ast;
  }
  | '{' InitValList '}' {
    auto ast = new InitVarAST();
    ast->initExpr = nullptr;
    ast->initList.reserve($2->size());
    for (auto &ptr : *$2) {
      ast->initList.push_back(unique_ptr<InitVarAST>(dynamic_cast<InitVarAST*>(ptr.release())));
    }
    delete $2;
    $$ = ast;
  }
  ;

// InitValList ::= InitVal {"," InitVal}
InitValList
  : InitVal {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    vec->push_back(std::unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | InitValList ',' InitVal {
    $1->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

// FuncFParam ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];
FuncFParam
  : BType IDENT {
    auto ast = new FuncFParamAST();
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | BType IDENT '[' ']' ArrayDims {
    auto ast = new FuncFParamAST();
    ast->btype = "*" + *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->dims = std::move(*static_cast<std::vector<std::unique_ptr<BaseAST>>*>($5));
    delete $5;
    $$ = ast;
  }
  ;

// FuncFParams ::= FuncFParam {"," FuncFParam}
FuncFParams
  : FuncFParam {
    auto vec = new std::vector<std::unique_ptr<FuncFParamAST>>();
    vec->push_back(std::unique_ptr<FuncFParamAST>(static_cast<FuncFParamAST *>($1)));
    $$ = vec;
  }
  | FuncFParams ',' FuncFParam {
    $1->push_back(std::unique_ptr<FuncFParamAST>(static_cast<FuncFParamAST *>($3)));
    $$ = $1;
  }
  ;

// FuncDef ::= FuncType IDENT "(" [FuncFParams] ")" Block
FuncDef
  : BType IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->ret_type = *unique_ptr<string>($1); // $1 是 BType 返回的 string*
    ast->ident = *unique_ptr<string>($2);
    ast->params = std::move(*$4);
    delete $4;
    ast->block = std::unique_ptr<BlockAST>(static_cast<BlockAST *>($6));
    $$ = ast;
  }
  | BType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->ret_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->params.clear();
    ast->block = std::unique_ptr<BlockAST>(static_cast<BlockAST *>($5));
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->ret_type = "void"; // 直接赋值 "void"
    ast->ident = *unique_ptr<string>($2);
    ast->params = std::move(*$4);
    delete $4;
    ast->block = std::unique_ptr<BlockAST>(static_cast<BlockAST *>($6));
    $$ = ast;
  }
  | VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->ret_type = "void";
    ast->ident = *unique_ptr<string>($2);
    ast->params.clear();
    ast->block = std::unique_ptr<BlockAST>(static_cast<BlockAST *>($5));
    $$ = ast;
  }
  ;

// Block ::= "{" {BlockItem} "}"
Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->items = std::move(*$2);
    delete $2;
    $$ = ast;
  }
  ;

BlockItemList
  : /* empty */ {
    $$ = new vector<unique_ptr<BaseAST>>();
  }
  | BlockItemList BlockItem {
    $1->push_back(unique_ptr<BaseAST>($2));
    $$ = $1;
  }
  ;

// BlockItem ::= Decl | Stmt
BlockItem
  : Decl { $$ = $1; }
  | Stmt { $$ = $1; }
  ;

// Stmt ::= MatchedStmt | UnMatchedStmt
Stmt
  : MatchedStmt { $$ = $1; }
  | UnMatchedStmt { $$ = $1; }
  ;

// MatchedStmt ::= IF '(' Exp ')' MatchedStmt ELSE MatchedStmt
//              | WHILE '(' Exp ')' MatchedStmt
//              | BREAK ';'
//              | CONTINUE ';'
//              | LVal '=' Exp ';'
//              | [Exp] ';'
//              | Block
//              | RETURN Exp ';'
//              | RETURN ';'
MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new IfStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' MatchedStmt {
    auto ast = new WhileStmtAST();
    ast->cond = unique_ptr<BaseAST>($3);
    ast->body = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new BreakStmtAST();
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new ContinueStmtAST();
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new AssignStmtAST();
    ast->lval = unique_ptr<LValAST>(static_cast<LValAST*>($1));
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new ExpStmtAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new ExpStmtAST();
    ast->exp = nullptr;
    $$ = ast;
  }
  | Block { $$ = $1; }
  | RETURN Exp ';' {
    auto ast = new ReturnStmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new ReturnStmtAST();
    ast->exp = nullptr;
    $$ = ast;
  }
  ;

// UnMatchedStmt ::= IF '(' Exp ')' Stmt
//                | IF '(' Exp ')' MatchedStmt ELSE UnMatchedStmt
//                | WHILE '(' Exp ')' Stmt
UnMatchedStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new IfStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = nullptr;
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE UnMatchedStmt {
    auto ast = new IfStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new WhileStmtAST();
    ast->cond = unique_ptr<BaseAST>($3);
    ast->body = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// PrimaryExp ::= "(" Exp ")" | LVal | Number
PrimaryExp
  : '(' Exp ')' { $$ = $2; }
  | LVal { $$ = $1; }
  | Number { $$ = $1; }
  ;

// LVal ::= IDENT {"[" Exp "]"}
LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->indices = std::vector<std::unique_ptr<BaseAST>>();
    $$ = ast;
  }
  | IDENT ArrayIndices {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->indices = std::move(*static_cast<std::vector<std::unique_ptr<BaseAST>>*>($2));
    $$ = ast;
  }
  ;

// Number ::= INT_CONST
Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->val = $1;
    $$ = ast;
  }
  ;

// FuncRParams ::= Exp {"," Exp}
FuncRParams
  : Exp {
    auto vec = new std::vector<std::unique_ptr<BaseAST>>();
    vec->push_back(std::unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncRParams ',' Exp {
    $1->push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp | IDENT "(" [FuncRParams] ")"
UnaryExp
  : PrimaryExp { $$ = $1; }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = *unique_ptr<string>($1);
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new FuncCallAST();
    ast->ident = *unique_ptr<string>($1);
    ast->args = std::vector<std::unique_ptr<BaseAST>>();
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new FuncCallAST();
    ast->ident = *unique_ptr<string>($1);
    ast->args = std::move(*$3);
    delete $3;
    $$ = ast;
  }
  ;

// UnaryOp ::= "+" | "-" | "!"
UnaryOp
  : '+' { $$ = new string("+"); }
  | '-' { $$ = new string("-"); }
  | '!' { $$ = new string("!"); }
  ;

// MulExp ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp
MulExp
  : UnaryExp { $$ = $1; }
  | MulExp '*' UnaryExp {
    auto ast = new BinaryExpAST();
    ast->op = "*";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new BinaryExpAST();
    ast->op = "/";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new BinaryExpAST();
    ast->op = "%";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// AddExp ::= MulExp | AddExp ("+" | "-") MulExp
AddExp
  : MulExp { $$ = $1; }
  | AddExp '+' MulExp {
    auto ast = new BinaryExpAST();
    ast->op = "+";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new BinaryExpAST();
    ast->op = "-";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// RelExp ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp
RelExp
  : AddExp { $$ = $1; }
  | RelExp '<' AddExp {
    auto ast = new BinaryExpAST();
    ast->op = "<";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new BinaryExpAST();
    ast->op = ">";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp LE AddExp {
    auto ast = new BinaryExpAST();
    ast->op = "<=";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GE AddExp {
    auto ast = new BinaryExpAST();
    ast->op = ">=";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// EqExp ::= RelExp | EqExp ("==" | "!=") RelExp
EqExp
  : RelExp { $$ = $1; }
  | EqExp EQ RelExp {
    auto ast = new BinaryExpAST();
    ast->op = "==";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | EqExp NE RelExp {
    auto ast = new BinaryExpAST();
    ast->op = "!=";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// LAndExp ::= EqExp | LAndExp "&&" EqExp
LAndExp
  : EqExp { $$ = $1; }
  | LAndExp AND EqExp {
    auto ast = new BinaryExpAST();
    ast->op = "&&";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// LOrExp ::= LAndExp | LOrExp "||" LAndExp
LOrExp
  : LAndExp { $$ = $1; }
  | LOrExp OR LAndExp {
    auto ast = new BinaryExpAST();
    ast->op = "||";
    ast->lhs = unique_ptr<BaseAST>($1);
    ast->rhs = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// Exp ::= LOrExp
Exp
  : LOrExp { $$ = $1; }
  ;

// ConstExp ::= Exp
ConstExp
  : Exp { $$ = $1; }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
