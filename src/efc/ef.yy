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
  IF "if"
  ELSE "else"
  ID "identifier"
  COMMA ","
  PLUS "+"
  MINUS "-"
  STAR "*"
  SLASH "/"
;

%token <int> NUMBER
%left PLUS
      MINUS
%left STAR
      SLASH

%type <AstSeq*> sa_expr pure_sa_expr
%type <AstValue*> sa_expr_leaf expr expr_leaf 

/* Grammar rules section
----------------------------------------------------------------------*/
%%

%start program;

program
  : sa_expr END_OF_FILE { driver.astRoot() = $1; }
  ;

sa_expr
  : %empty { $$ = new AstSeq(); }
  | expr { $$ = new AstSeq($1); }
  | pure_sa_expr { std::swap($$,$1); }
  | pure_sa_expr expr { $$ = ($1)->Add($2); }
  ;

pure_sa_expr
  : sa_expr_leaf { $$ = new AstSeq($1); }
  /* implicit sequence operator */
  | pure_sa_expr sa_expr_leaf { $$ = ($1)->Add($2); }
  ;

sa_expr_leaf
  : expr COMMA { std::swap($$,$1); }
  ;

expr
  : expr_leaf { std::swap($$,$1); }
  | expr PLUS expr { $$ = new AstOperator('+', $1, $3); }
  | expr MINUS expr { $$ = new AstOperator('-', $1, $3); }
  | expr STAR expr { $$ = new AstOperator('*', $1, $3); }
  | expr SLASH expr { $$ = new AstOperator('/', $1, $3); }
  ;

expr_leaf
  : NUMBER { $$ = new AstNumber($1); }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
