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
  #include "../objtype.h"
  #include "../parserext.h"                        
}

%param { Driver& driver }
%parse-param { ParserExt& parserExt } { AstSeq*& astRoot } 

%locations
%initial-action
{
  @$.initialize(&driver.fileName());
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
  INT "int"
  BOOL "bool"
  EQUAL "="
  COLON_EQUAL ":="
  COMMA ","
  SEMICOLON ";"
  COLON ":"
  PLUS "+"
  MINUS "-"
  STAR "*"
  NOT "not"
  EXCL "!"
  AND "and"
  AMPER_AMPER "&&"
  OR "or"
  PIPE_PIPE "||"
  SLASH "/"
  LPAREN "("
  RPAREN ")"
  LBRACE "{"
  RBRACE "}"
  ARROW "->"
;

%token <std::string> OP_LPAREN 
%token <std::string> ID "identifier"
%token <int> NUMBER "number"
%precedence ASSIGNEMENT
%left PIPE_PIPE OR
%left AMPER_AMPER AND
%left PLUS
      MINUS
%left STAR
      SLASH
%precedence EXCL NOT

%type <AstCtList*> ct_list pure_ct_list
%type <std::list<AstArgDecl*>*> param_ct_list pure_naked_param_ct_list
%type <AstSeq*> maybe_empty_seq seq pure_seq seq_without_comma opt_else
%type <AstValue*> expr expr_leaf naked_if
%type <std::list<AstIf::ConditionActionPair>*> opt_elif_list
%type <AstArgDecl*> param_decl
%type <ObjType::Qualifier> valvar
%type <RawAstDataDecl*> naked_data_decl
%type <RawAstDataDef*> naked_data_def
%type <AstFunDecl*> naked_fun_decl
%type <AstFunDef*> naked_fun_def
%type <ObjType*> type opt_colon_type opt_arrow_type

/* Grammar rules section
----------------------------------------------------------------------*/
%%

/* terminology

pure
  For list constructs: At least one element, all elements separated by a
  separator token, no trailing separator token (since its a separator, not
  delimiter).

(unqualified)
  For list constructs: Allows an optional trailing separator token.

naked
  No enclosing delimiter like for example parentheses or keyword...;

opt
  Optional

maybe_empty
  Contains an empty rule among other non-empty rules.
*/


%start program;

program
  : maybe_empty_seq END_OF_FILE                     { driver.astRoot() = $1; }
  ;

maybe_empty_seq
  : %empty                                          { $$ = new AstSeq(); }
  | seq                                             { std::swap($$,$1); }
  ;

/* the _only_ difference between seq and seq_without_comma is that in the
former commas as separator are optionally allowed, while in the later they are
disallowed */
seq
  : pure_seq opt_comma                              { std::swap($$,$1); }
  ;
  
pure_seq
  : expr                                            { $$ = new AstSeq($1); }
  | pure_seq opt_comma expr                         { $$ = ($1)->Add($3); }
  ;

seq_without_comma
  : expr                                            { $$ = new AstSeq($1); }
  | seq_without_comma expr                          { $$ = ($1)->Add($2); } 
  ;

ct_list
  : %empty                                          { $$ = new AstCtList(); }
  | pure_ct_list opt_comma                          { std::swap($$,$1); }
  ;

pure_ct_list
  : seq_without_comma                               { $$ = new AstCtList($1); }
  | pure_ct_list COMMA seq_without_comma            { $$ = ($1)->Add($3); }
  ;

param_ct_list
  : LPAREN RPAREN                                   { $$ = new std::list<AstArgDecl*>(); }
  | LPAREN pure_naked_param_ct_list opt_comma RPAREN{ std::swap($$,$2); }
  ;

pure_naked_param_ct_list
  : param_decl                                      { $$ = new std::list<AstArgDecl*>(); ($$)->push_back($1); }
  | pure_naked_param_ct_list COMMA param_decl       { ($1)->push_back($3); std::swap($$,$1); }
  ;

param_decl
  : ID COLON type                                   { $$ = new AstArgDecl($1, &(($3)->addQualifier(ObjType::eMutable))); }
  ;
  
type
  : ID                                              { $$ = new ObjTypeFunda(ObjTypeFunda::eInt); }
  ;

opt_colon
  : %empty
  | COLON
  ;

opt_colon_type
  : opt_colon                                       { $$ = new ObjTypeFunda(ObjTypeFunda::eInt); }
  | COLON type                                      { std::swap($$, $2); }
  ;

/* Most probably the grammar will not have the '->' since the closing ')' of
the parameter list is already the delimiter. But for now I want to have the
option of being able to allow the '->'.*/
opt_arrow_type
  : %empty                                          { $$ = new ObjTypeFunda(ObjTypeFunda::eInt); }
  | ARROW                                           { $$ = new ObjTypeFunda(ObjTypeFunda::eInt); }
  | ARROW type                                      { std::swap($$, $2); }
  | type                                            { std::swap($$, $1); } 
  ;

opt_comma
  : %empty
  | COMMA
  ;

expr
  /* misc */
  : expr_leaf                                       { std::swap($$,$1); }

  /* calls */
  | ID LPAREN ct_list RPAREN                        { $$ = new AstFunCall($1, $3); }
  | OP_LPAREN ct_list RPAREN                        { $$ = new AstOperator($1, $2); }


  /* unary prefix */
  | NOT  expr                                       { $$ = new AstOperator(AstOperator::eNot, $2); }
  | EXCL expr                                       { $$ = new AstOperator(AstOperator::eNot, $2); }

  /* binary operators */
  | ID   EQUAL       expr         %prec ASSIGNEMENT { $$ = new AstOperator('=', new AstSymbol(new std::string($1), AstSymbol::eLValue), $3); }
  | ID   COLON_EQUAL expr         %prec ASSIGNEMENT { $$ = new AstDataDef(new AstDataDecl($1, new ObjTypeFunda(ObjTypeFunda::eInt)), $3); }
  | expr OR          expr                           { $$ = new AstOperator(AstOperator::eOr, $1, $3); }
  | expr PIPE_PIPE   expr                           { $$ = new AstOperator(AstOperator::eOr, $1, $3); }
  | expr AND         expr                           { $$ = new AstOperator(AstOperator::eAnd, $1, $3); }
  | expr AMPER_AMPER expr                           { $$ = new AstOperator(AstOperator::eAnd, $1, $3); }
  | expr PLUS        expr                           { $$ = new AstOperator('+', $1, $3); }
  | expr MINUS       expr                           { $$ = new AstOperator('-', $1, $3); }
  | expr STAR        expr                           { $$ = new AstOperator('*', $1, $3); }
  | expr SLASH       expr                           { $$ = new AstOperator('/', $1, $3); }
  ;

expr_leaf
  /* misc */
  : NUMBER                                          { $$ = new AstNumber($1); }
  | LBRACE seq RBRACE                               { $$ = $2; }
  | ID                                              { $$ = new AstSymbol(new std::string($1)); }


  /* definitions of data object. Declarations make no sense for local variables, and globals are not yet available. */
  | valvar        naked_data_def SEMICOLON                           { $$ = parserExt.createAstDataDef($1, $2); }
  | valvar LPAREN naked_data_def RPAREN                              { $$ = parserExt.createAstDataDef($1, $3); }


  /* declarations of code object */
  | DECL        FUN naked_fun_decl SEMICOLON                         { $$ = $3; }
  | DECL LPAREN FUN naked_fun_decl RPAREN                            { $$ = $4; }

  /* definitions of code object */
  | FUN        naked_fun_def SEMICOLON                               { $$ = $2; }
  | FUN LPAREN naked_fun_def RPAREN                                  { $$ = $3; }


  /* flow control - conditionals */
  | IF        naked_if SEMICOLON                                     { std::swap($$,$2); }
  | IF LPAREN naked_if RPAREN                                        { std::swap($$,$3); }
  ;

naked_data_decl
  : ID COLON type                                                    { $$ = new RawAstDataDecl($1, $3); }
  | ID COLON                                                         { $$ = new RawAstDataDecl($1, new ObjTypeFunda(ObjTypeFunda::eInt)); }
  ;

naked_data_def
  : naked_data_decl                                                  { $$ = new RawAstDataDef($1); }
  | naked_data_decl EQUAL expr                                       { $$ = new RawAstDataDef($1, $3); }
  | ID EQUAL expr opt_colon_type                                     { $$ = new RawAstDataDef(new RawAstDataDecl($1, $4), $3); }
  ;

naked_fun_def
  : naked_fun_decl EQUAL maybe_empty_seq                             { $$ = new AstFunDef($1, $3); }
  ;
  
naked_fun_decl
  : ID opt_colon param_ct_list opt_arrow_type                        { $$ = new AstFunDecl($1, $3); }
  | ID opt_colon                                                     { $$ = new AstFunDecl($1, new std::list<AstArgDecl*>()); }
  ;

naked_if
  : expr opt_colon seq opt_elif_list opt_else                        { ($4)->push_front(AstIf::ConditionActionPair($1, $3)); $$ = new AstIf($4, $5); }
  ;

valvar
  : VAL                                                              { $$ = ObjType::eNoQualifier; }
  | VAR	                                                             { $$ = ObjType::eMutable; }
  ;

opt_elif_list
  : %empty                                                           { $$ = new std::list<AstIf::ConditionActionPair>(); }  
  | opt_elif_list ELIF expr opt_colon seq                            { ($1)->push_back(AstIf::ConditionActionPair($3, $5)); std::swap($$,$1); }
  ;  

opt_else
  : %empty                                                           { $$ = NULL; }
  | ELSE seq                                                         { std::swap($$,$2); }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
