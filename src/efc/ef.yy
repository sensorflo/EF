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
  COLON ":"
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
%type <std::list<std::string>*> param_ct_list pure_naked_param_ct_list
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

/* I'm not sure yet whether EF will allow omitting parentheses also in future,
but for now I want to have the option for that way */
param_ct_list
  : %empty { $$ = new std::list<std::string>(); }
  | LPAREN RPAREN { $$ = new std::list<std::string>(); }
  | pure_naked_param_ct_list opt_comma { std::swap($$,$1); }
  | LPAREN pure_naked_param_ct_list opt_comma RPAREN { std::swap($$,$2); }
  ;

/* 'pure' means at least one element, all elements separated by commas, no
trailing comma since comma is separator, not delimiter. 'naked' means no
enclosing parentheses. */
pure_naked_param_ct_list
  : ID { $$ = new std::list<std::string>(); ($$)->push_back($1); }
  | pure_naked_param_ct_list COMMA ID { ($1)->push_back($3); std::swap($$,$1); }
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
  : FUN ID COLON param_ct_list EQUAL maybe_empty_sa_expr END { $$ = new AstFunDef(new AstFunDecl($2, $4), $6); }
  ;

fun_decl
  : DECL FUN ID COLON param_ct_list END { $$ = new AstFunDecl($3, $5); }
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
  | ID { $$ = new AstSymbol(new std::string($1)); }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
