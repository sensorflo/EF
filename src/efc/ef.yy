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
  END "end"
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

%token <ObjTypeFunda::EType> FUNDAMENTAL_TYPE
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

%type <AstCtList*> ct_list
%type <std::list<AstArgDecl*>*> param_ct_list pure_naked_param_ct_list
%type <AstSeq*> seq
%type <std::list<AstValue*>*> expr_list pure_expr_list
%type <AstValue*> expr expr_leaf naked_if opt_else
%type <std::list<AstIf::ConditionActionPair>*> opt_elif_list
%type <AstArgDecl*> param_decl
%type <ObjType::Qualifier> valvar
%type <RawAstDataDecl*> naked_data_decl
%type <RawAstDataDef*> naked_data_def
%type <std::pair<AstFunDecl*,SymbolTableEntry*>> naked_fun_decl
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
*/


%start program;

program
  : seq END_OF_FILE                                 { driver.astRoot() = $1; }
  ;

/** The elements of a sequence are sequentially executed at compile time */
seq
  : expr_list                                       { $$ = new AstSeq($1); }
  ;
  
/** The elements of an compile-time list form a list in the sence of a data
structure (it says nothing about wheter its implemented as array, linked-list
or whatever) */
ct_list
  : expr_list                                       { $$ = new AstCtList($1); }
  ;

expr_list
  : %empty                                          { $$ = new std::list<AstValue*>(); }
  | pure_expr_list opt_comma                        { std::swap($$,$1); }
  ;

pure_expr_list
  : expr                                            { $$ = new std::list<AstValue*>(); ($$)->push_back($1); }
  | pure_expr_list opt_comma expr                   { ($1)->push_back($3); std::swap($$,$1); }
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
  : FUNDAMENTAL_TYPE                                { $$ = new ObjTypeFunda($1); }
  | ID                                              { assert(false); /* user defined names not yet supported; but I wanted to have ID already in grammar*/ }
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
  | ID   EQUAL       expr         %prec ASSIGNEMENT { $$ = new AstOperator('=', new AstSymbol(new std::string($1)), $3); }
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
  | valvar kwao naked_data_def kwac                                  { $$ = parserExt.createAstDataDef($1, $3); }


  /* declarations of code object */
  | DECL kwao FUN naked_fun_decl kwac                                { $$ = ($4).first; }

  /* definitions of code object */
  | FUN kwao naked_fun_def kwac                                      { $$ = $3; }


  /* flow control - conditionals */
  | IF kwao naked_if kwac                                            { std::swap($$,$3); }
  ;

/* keyword argument list open delimiter */
kwao
  : %empty
  | LPAREN
  ;

/* keyword argument list close delimiter */
kwac
  : kwac_raw
  | END id_or_keyword kwac_raw
  ;      

kwac_raw
  : RPAREN
  | SEMICOLON
  ;

id_or_keyword
  : ID | IF | ELIF | ELSE | FUN | VAL | VAR | DECL | END | NOT | AND | OR | FUNDAMENTAL_TYPE
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
  : naked_fun_decl EQUAL seq                                         { $$ = parserExt.createAstFunDef(($1).first, $3, *(($1).second)); }
  ;
  
naked_fun_decl
  : ID opt_colon param_ct_list opt_arrow_type                        { $$ = parserExt.createAstFunDecl($1, $3); }
  | ID opt_colon                                                     { $$ = parserExt.createAstFunDecl($1); }
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
  | ELSE seq                                                         { $$ = $2; }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
