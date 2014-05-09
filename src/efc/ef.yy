/* Bison declaration section
----------------------------------------------------------------------*/
%skeleton "lalr1.cc"
%token-table
%require "3.0.2"
%expect 0
%defines
%define parser_class_name {Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires
{
  class Driver;
  #include "../ast.h"
}

%param { Driver& driver }
%parse-param { AstNode*& astRoot }

%locations
%initial-action
{
  @$.begin.filename = @$.end.filename = &driver.fileName();
};

/*%define parse.trace*/
%define parse.error verbose

%code
{
  #include "../driver.h"
  using namespace std;
  using namespace yy;
}

%define api.token.prefix {TOK_}
%token
  END_OF_FILE  0  "end of file"
  END "end"
  DECL "decl"
  IF "if"
  ELSE "else"
  FUN "fun"
  EQUAL "="
  COMMA ","
  SEMICOLON ";"
  PLUS "+"
  MINUS "-"
  STAR "*"
  SLASH "/"
  LPAREN "("
  RPAREN ")"
  LBRACE "{"
  RBRACE "}"
;

%token <std::string> ID "identifier"
%token <int> NUMBER "number"
%left PLUS
      MINUS
%left STAR
      SLASH

%type <AstCtList*> ct_list pure_ct_list
%type <AstSeq*> maybe_empty_sa_expr sa_expr pure_sa_expr
%type <AstValue*> expr expr_leaf 
%type <AstNode*> fun_def fun_decl sa_expr_leaf

/* Grammar rules section
----------------------------------------------------------------------*/
%%

%start program;

program
  : maybe_empty_sa_expr END_OF_FILE { driver.astRoot() = $1; }
  ;

maybe_empty_sa_expr
  : %empty { $$ = new AstSeq(); }
  | sa_expr { std::swap($$,$1); }
  ;

sa_expr
  : pure_sa_expr opt_semicolon { std::swap($$,$1); }
  ;

pure_sa_expr
  : sa_expr_leaf { $$ = new AstSeq($1); }
  | pure_sa_expr opt_semicolon sa_expr_leaf { $$ = ($1)->Add($3); }
  ;

sa_expr_leaf
  : expr { $$=$1; }
  | fun_def { $$=$1; }
  | fun_decl { $$=$1; }
  ;

ct_list
  : %empty { $$ = new AstCtList(); }
  | pure_ct_list opt_comma { std::swap($$,$1); }
  ;

pure_ct_list
  : sa_expr { $$ = new AstCtList($1); }
  | pure_ct_list COMMA sa_expr { $$ = ($1)->Add($3); }
  ;

opt_semicolon
  : %empty
  | SEMICOLON
  ;

opt_comma
  : %empty
  | COMMA
  ;

fun_def
  : FUN ID EQUAL maybe_empty_sa_expr END { $$ = new AstFunDef($2, $4); }
  ;

fun_decl
  : DECL FUN ID END { $$ = new AstFunDecl($3); }
  ;

expr
  : expr_leaf { std::swap($$,$1); }
  | ID LPAREN ct_list RPAREN { $$ = new AstFunCall($1, $3); }
  | expr PLUS expr { $$ = new AstOperator('+', $1, $3); }
  | expr MINUS expr { $$ = new AstOperator('-', $1, $3); }
  | expr STAR expr { $$ = new AstOperator('*', $1, $3); }
  | expr SLASH expr { $$ = new AstOperator('/', $1, $3); }
  ;

expr_leaf
  : NUMBER { $$ = new AstNumber($1); }
  | LBRACE expr RBRACE { std::swap($$,$2); }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
