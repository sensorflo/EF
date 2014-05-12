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
  DECL "decl"
  IF "if"
  ELSE "else"
  FUN "fun"
  VAL "val"
  VAR "var"
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
%precedence ASSIGNEMENT
%left PLUS
      MINUS
%left STAR
      SLASH

%type <AstCtList*> ct_list pure_ct_list
%type <std::list<std::string>*> param_ct_list pure_naked_param_ct_list
%type <AstSeq*> maybe_empty_expr expr
%type <AstValue*> sub_expr sub_expr_leaf


/* Grammar rules section
----------------------------------------------------------------------*/
%%

%start program;

program
  : maybe_empty_expr END_OF_FILE                    { driver.astRoot() = $1; }
  ;

maybe_empty_expr
  : %empty                                          { $$ = new AstSeq(); }
  | expr                                            { std::swap($$,$1); }
  ;

expr
  : sub_expr                                        { $$ = new AstSeq($1); }
  | expr sub_expr                                   { $$ = ($1)->Add($2); }
  ;

ct_list
  : %empty                                          { $$ = new AstCtList(); }
  | pure_ct_list opt_comma                          { std::swap($$,$1); }
  ;

pure_ct_list
  : expr                                            { $$ = new AstCtList($1); }
  | pure_ct_list COMMA expr                         { $$ = ($1)->Add($3); }
  ;

/* I'm not sure yet whether EF will allow omitting parentheses also in future,
but for now I want to have the option for that way */
param_ct_list
  : %empty                                          { $$ = new std::list<std::string>(); }
  | LPAREN RPAREN                                   { $$ = new std::list<std::string>(); }
  | pure_naked_param_ct_list opt_comma              { std::swap($$,$1); }
  | LPAREN pure_naked_param_ct_list opt_comma RPAREN{ std::swap($$,$2); }
  ;

/* 'pure' means at least one element, all elements separated by commas, no
trailing comma since comma is separator, not delimiter. 'naked' means no
enclosing parentheses. */
pure_naked_param_ct_list
  : ID                                              { $$ = new std::list<std::string>(); ($$)->push_back($1); }
  | pure_naked_param_ct_list COMMA ID               { ($1)->push_back($3); std::swap($$,$1); }
  ;

opt_comma
  : %empty
  | COMMA
  ;

sub_expr
  /* misc */
  : sub_expr_leaf                                   { std::swap($$,$1); }
  | ID LPAREN ct_list RPAREN                        { $$ = new AstFunCall($1, $3); }

  /* binary operators */
  | ID       EQUAL       sub_expr %prec ASSIGNEMENT { $$ = new AstOperator('=', new AstSymbol(new std::string($1), AstSymbol::eLValue), $3); }
  | ID       COLON EQUAL sub_expr %prec ASSIGNEMENT { $$ = new AstDataDef(new AstDataDecl($1), new AstSeq($4)); }
  | sub_expr PLUS        sub_expr                   { $$ = new AstOperator('+', $1, $3); }
  | sub_expr MINUS       sub_expr                   { $$ = new AstOperator('-', $1, $3); }
  | sub_expr STAR        sub_expr                   { $$ = new AstOperator('*', $1, $3); }
  | sub_expr SLASH       sub_expr                   { $$ = new AstOperator('/', $1, $3); }
  ;

sub_expr_leaf
  /* misc */
  : NUMBER                                          { $$ = new AstNumber($1); }
  | LBRACE expr RBRACE                              { $$ = $2; }
  | ID                                              { $$ = new AstSymbol(new std::string($1)); }

  /* declarations of data object */
  | DECL VAL ID COLON /*type*/            SEMICOLON { $$ = new AstDataDecl($3); }
  | DECL VAL ID                           SEMICOLON { $$ = new AstDataDecl($3); }
  | DECL VAR ID COLON /*type*/            SEMICOLON { $$ = new AstDataDecl($3, AstDataDecl::eAlloca); }
  | DECL VAR ID                           SEMICOLON { $$ = new AstDataDecl($3, AstDataDecl::eAlloca); }

  /* definitions of data object */
  |      VAL ID COLON /*type*/ EQUAL expr SEMICOLON { $$ = new AstDataDef(new AstDataDecl($2), $5); }
  |      VAL ID                EQUAL expr SEMICOLON { $$ = new AstDataDef(new AstDataDecl($2), $4); }
  |      VAR ID COLON /*type*/ EQUAL expr SEMICOLON { $$ = new AstDataDef(new AstDataDecl($2, AstDataDecl::eAlloca), $5); }
  |      VAR ID                EQUAL expr SEMICOLON { $$ = new AstDataDef(new AstDataDecl($2, AstDataDecl::eAlloca), $4); }
  |      VAL ID COLON /*type*/            SEMICOLON { $$ = new AstDataDef(new AstDataDecl($2)); }
  |      VAL ID                           SEMICOLON { $$ = new AstDataDef(new AstDataDecl($2)); }
  |      VAR ID COLON /*type*/            SEMICOLON { $$ = new AstDataDef(new AstDataDecl($2, AstDataDecl::eAlloca)); }
  |      VAR ID                           SEMICOLON { $$ = new AstDataDef(new AstDataDecl($2, AstDataDecl::eAlloca)); }

  /* declaration of code object */
  | DECL FUN ID COLON param_ct_list                        SEMICOLON { $$ = new AstFunDecl($3, $5); }

  /* definition of code object */
  |      FUN ID COLON param_ct_list EQUAL maybe_empty_expr SEMICOLON { $$ = new AstFunDef(new AstFunDecl($2, $4), $6); }
  ;


/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
