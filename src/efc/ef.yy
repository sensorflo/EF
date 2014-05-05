/* Bison declaration section
----------------------------------------------------------------------*/
%skeleton "lalr1.cc"
%token-table
%require "3.0.2"
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
;

%token <int> NUMBER
%left COMMA ","
%left PLUS "+"
      MINUS "-"
%left STAR "*"
      SLASH "/"

%type <AstSeq*> expr_seq 
%type <AstNode*> expr

/* Grammar rules section
----------------------------------------------------------------------*/
%%

%start program;

program
  : expr_seq END_OF_FILE { driver.astRoot() = $1; }
  ;

expr_seq
  : %empty { $$ = new AstSeq(); }
  | expr { $$ = new AstSeq($1); }
  | expr_seq COMMA expr { $$ = ($1)->Add($3); }
  ;

expr
  : NUMBER { $$ = new AstNumber($1); }
  | expr PLUS expr { $$ = new AstOperator('+', $1, $3); }
  | expr MINUS expr { $$ = new AstOperator('-', $1, $3); }
  | expr STAR expr { $$ = new AstOperator('*', $1, $3); }
  | expr SLASH expr { $$ = new AstOperator('/', $1, $3); }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
