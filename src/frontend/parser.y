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
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST>> *ast_list;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token CONST INT RETURN LE GE EQ NE AND OR
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block BlockItem Decl ConstDecl VarDecl
%type <ast_val> ConstDef VarDef ConstInitVal InitVal
%type <ast_val> Stmt ExpStmt LVal Exp PrimaryExp Number UnaryExp
%type <ast_val> MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <str_val> BType FuncType UnaryOp
%type <ast_list> ConstDefList VarDefList BlockItemList

%%

// CompUnit ::= FuncDef
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = std::move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT "(" ")" Block
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->ret_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// FuncType ::= "int"
FuncType
  : INT {
    $$ = new string("int");
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

// ConstDef ::= IDENT "=" ConstInitVal
ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// ConstInitVal ::= ConstExp
ConstInitVal
  : ConstExp { $$ = $1; }
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

// VarDef ::= IDENT | IDENT "=" InitVal
VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = nullptr;
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

// InitVal ::= Exp
InitVal
  : Exp { $$ = $1; }
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

// Stmt ::= LVal "=" Exp ";" | [Exp] ";" | Block | "return" [Exp] ";"
Stmt
  : LVal '=' Exp ';' {
    auto ast = new AssignStmtAST();
    ast->lval = unique_ptr<LValAST>(static_cast<LValAST*>($1));
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | ExpStmt ';' { $$ = $1; }
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

// ExpStmt ::= Exp | /* empty */
ExpStmt
  : Exp {
    auto ast = new ExpStmtAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | /* empty */ {
    auto ast = new ExpStmtAST();
    ast->exp = nullptr;
    $$ = ast;
  }
  ;

// Exp ::= LOrExp
Exp
  : LOrExp { $$ = $1; }
  ;

// LVal ::= IDENT
LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

// PrimaryExp ::= "(" Exp ")" | LVal | Number
PrimaryExp
  : '(' Exp ')' { $$ = $2; }
  | LVal { $$ = $1; }
  | Number { $$ = $1; }
  ;

// Number ::= INT_CONST
Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->val = $1;
    $$ = ast;
  }
  ;

// UnaryExp ::= PrimaryExp | UnaryOp UnaryExp
UnaryExp
  : PrimaryExp { $$ = $1; }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = *unique_ptr<string>($1);
    ast->exp = unique_ptr<BaseAST>($2);
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
