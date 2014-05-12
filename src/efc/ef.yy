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
  ELIF "elif"
  ELSE "else"
  FUN "fun"
  VAL "val"
  VAR "var"
  EQUAL "="
  COLON_EQUAL ":="
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
%type <AstSeq*> maybe_empty_expr expr pure_expr expr_without_comma opt_else
%type <AstValue*> sub_expr sub_expr_leaf
%type <std::list<AstIf::ConditionActionPair>*> opt_elif_list

/* Grammar rules section
----------------------------------------------------------------------*/
%%

/* terminology

pure
  For list constructs: At least one element, all elements separated by
  separator token, no trailing separator token (since its a separator, not
  delimiter)

(unqualified)
  For list constructs: Allows an optional trailing separator token

naked
  No enclosing parentheses.
*/


%start program;

program
  : maybe_empty_expr END_OF_FILE                    { driver.astRoot() = $1; }
  ;

maybe_empty_expr
  : %empty                                          { $$ = new AstSeq(); }
  | expr                                            { std::swap($$,$1); }
  ;

/* the _only_ difference between expr and expr_without_comma is that in the
former commas as separator are optionally allowed, while in the later they are
disallowed */
expr
  : pure_expr opt_comma                             { std::swap($$,$1); }
  ;
  
pure_expr
  : sub_expr                                        { $$ = new AstSeq($1); }
  | pure_expr opt_comma sub_expr                    { $$ = ($1)->Add($3); }
  ;

expr_without_comma
  : sub_expr                                        { $$ = new AstSeq($1); }
  | expr_without_comma sub_expr                     { $$ = ($1)->Add($2); } 
  ;

ct_list
  : %empty                                          { $$ = new AstCtList(); }
  | pure_ct_list opt_comma                          { std::swap($$,$1); }
  ;

pure_ct_list
  : expr_without_comma                              { $$ = new AstCtList($1); }
  | pure_ct_list COMMA expr_without_comma           { $$ = ($1)->Add($3); }
  ;

/* I'm not sure yet whether EF will allow omitting parentheses also in future,
but for now I want to have the option for that way */
param_ct_list
  : %empty                                          { $$ = new std::list<std::string>(); }
  | LPAREN RPAREN                                   { $$ = new std::list<std::string>(); }
  | pure_naked_param_ct_list opt_comma              { std::swap($$,$1); }
  | LPAREN pure_naked_param_ct_list opt_comma RPAREN{ std::swap($$,$2); }
  ;

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
  | ID       COLON_EQUAL sub_expr %prec ASSIGNEMENT { $$ = new AstDataDef(new AstDataDecl($1), new AstSeq($3)); }
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

  /* flow control - conditionals */
  | IF expr COLON expr opt_elif_list opt_else SEMICOLON              { ($5)->push_front(AstIf::ConditionActionPair($2, $4)); $$ = new AstIf($5, $6); }
  ;

opt_elif_list
  : %empty                                                           { $$ = new std::list<AstIf::ConditionActionPair>(); }  
  | opt_elif_list ELIF expr COLON expr                               { ($1)->push_back(AstIf::ConditionActionPair($3, $5)); std::swap($$,$1); }
  ;  

opt_else
  : %empty                                                           { $$ = NULL; }
  | ELSE expr                                                        { std::swap($$,$2); }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
