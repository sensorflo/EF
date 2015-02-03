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
%parse-param { ParserExt& parserExt } { AstValue*& astRoot } 

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
  RAW_NEW "raw_new"
  RAW_DELETE "raw_delete"
  EQUAL "="
  COLON_EQUAL ":="
  COMMA ","
  SEMICOLON ";"
  DOLLAR "$"
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
  G_LPAREN "g("
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
%right EQUAL
%left PIPE_PIPE OR
%left AMPER_AMPER AND
%left PLUS
      MINUS
%left STAR
      SLASH
%precedence EXCL NOT
%precedence LPAREN


%type <AstCtList*> ct_list initializer
%type <std::list<AstArgDecl*>*> pure_naked_param_ct_list
%type <AstSeq*> pure2_standalone_expr_seq
%type <std::list<AstValue*>*> pure_ct_list
%type <AstValue*> block_expr standalone_expr_seq standalone_expr sub_expr operator_expr primary_expr list_expr naked_if opt_else
%type <std::list<AstIf::ConditionActionPair>*> opt_elif_list
%type <AstArgDecl*> param_decl
%type <ObjType::Qualifier> valvar
%type <RawAstDataDecl*> naked_data_decl
%type <RawAstDataDef*> naked_data_def
%type <std::pair<AstFunDecl*,SymbolTableEntry*>> naked_fun_decl
%type <AstFunDef*> naked_fun_def
%type <ObjType*> type opt_colon_type

/* Grammar rules section
----------------------------------------------------------------------*/
%%

/* terminology

pure / pure2
  For list constructs: At least one element (at least two for pure2), all
  elements separated by a separator token, no trailing separator token (since
  its a separator, not delimiter).

(unqualified)
  For list constructs: Allows an optional trailing separator token.

naked
  No enclosing delimiter like for example parentheses or keyword...;

opt
  Optional
*/


%start program;

program
  : block_expr END_OF_FILE                          { driver.astRoot() = $1; }
  ;

block_expr
  : standalone_expr_seq                             { std::swap($$,$1); }
  ;

/* Note that the trailing 'seq_operator' is not really a sequence operator,
i.e. it does not build an expression sequence */
standalone_expr_seq
  : standalone_expr seq_operator                    { std::swap($$,$1); }
  | pure2_standalone_expr_seq seq_operator          { $$ = $1; }
  ;

pure2_standalone_expr_seq
  : standalone_expr seq_operator standalone_expr           { $$ = new AstSeq($1,$3); }
  | pure2_standalone_expr_seq seq_operator standalone_expr { ($1)->Add($3); std::swap($$,$1); }
  ;

standalone_expr
  : sub_expr                                        { std::swap($$,$1); }
  ;

ct_list
  : %empty                                          { $$ = new AstCtList(); }
  | pure_ct_list opt_comma                          { $$ = new AstCtList($1); }
  ;

pure_ct_list
  : standalone_expr                                 { $$ = new std::list<AstValue*>(); ($$)->push_back($1); }
  | pure_ct_list opt_comma standalone_expr          { ($1)->push_back($3); std::swap($$,$1); }
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

opt_comma
  : %empty
  | COMMA
  ;

seq_operator
  : %empty
  | SEMICOLON
  ;

sub_expr
  : primary_expr                                    { std::swap($$,$1); }
  | operator_expr                                   { std::swap($$,$1); }
  ;

operator_expr
  /* function call */
  : sub_expr LPAREN ct_list RPAREN                  { $$ = new AstFunCall($1, $3); }

  /* unary prefix */
  | NOT  sub_expr                                   { $$ = new AstOperator(AstOperator::eNot, $2); }
  | EXCL sub_expr                                   { $$ = new AstOperator(AstOperator::eNot, $2); }

  /* binary operators */
  | sub_expr EQUAL       sub_expr                   { $$ = new AstOperator('=', $1, $3); }
  | ID       COLON_EQUAL sub_expr %prec ASSIGNEMENT { $$ = new AstDataDef(new AstDataDecl($1, new ObjTypeFunda(ObjTypeFunda::eInt)), $3); }
  | sub_expr OR          sub_expr                   { $$ = new AstOperator(AstOperator::eOr, $1, $3); }
  | sub_expr PIPE_PIPE   sub_expr                   { $$ = new AstOperator(AstOperator::eOr, $1, $3); }
  | sub_expr AND         sub_expr                   { $$ = new AstOperator(AstOperator::eAnd, $1, $3); }
  | sub_expr AMPER_AMPER sub_expr                   { $$ = new AstOperator(AstOperator::eAnd, $1, $3); }
  | sub_expr PLUS        sub_expr                   { $$ = new AstOperator('+', $1, $3); }
  | sub_expr MINUS       sub_expr                   { $$ = new AstOperator('-', $1, $3); }
  | sub_expr STAR        sub_expr                   { $$ = new AstOperator('*', $1, $3); }
  | sub_expr SLASH       sub_expr                   { $$ = new AstOperator('/', $1, $3); }
  ;

primary_expr
  : list_expr                                       { std::swap($$,$1); }
  | NUMBER                                          { $$ = new AstNumber($1); }
  | G_LPAREN standalone_expr_seq RPAREN             { $$ = $2; }
  | ID                                              { $$ = new AstSymbol(new std::string($1)); }
  ;
  
list_expr
  : valvar kwao naked_data_def kwac                 { $$ = parserExt.createAstDataDef($1, $3); }
  | DECL kwao FUN naked_fun_decl kwac               { $$ = ($4).first; }
  | FUN kwao naked_fun_def kwac                     { $$ = $3; }
  | IF kwao naked_if kwac                           { std::swap($$,$3); }
  | OP_LPAREN ct_list RPAREN                        { $$ = new AstOperator($1, $2); }
  | RAW_NEW kwao type initializer kwac              { $$ = NULL; }
  | RAW_DELETE kwao sub_expr kwac                   { $$ = NULL; }
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
  | DOLLAR
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
  | naked_data_decl initializer                                      { $$ = new RawAstDataDef($1, $2); }
  | ID initializer opt_colon_type                                    { $$ = new RawAstDataDef(new RawAstDataDecl($1, $3), $2); }
  ;

naked_fun_def
  : naked_fun_decl EQUAL block_expr                                  { $$ = parserExt.createAstFunDef(($1).first, $3, *(($1).second)); }
  ;
  
naked_fun_decl
  : ID                                                        opt_ret_type  { $$ = parserExt.createAstFunDecl($1); }
  | ID COLON                                                  opt_ret_type  { $$ = parserExt.createAstFunDecl($1); }
  | ID COLON LPAREN                                    RPAREN opt_ret_type  { $$ = parserExt.createAstFunDecl($1); }
  | ID COLON LPAREN pure_naked_param_ct_list opt_comma RPAREN opt_ret_type  { $$ = parserExt.createAstFunDecl($1, $4); }
  ;

opt_ret_type
  : %empty
  | type
  ;

initializer
  : EQUAL standalone_expr                                            { $$ = new AstCtList($2); }
  | EQUAL LPAREN ct_list RPAREN                                      { swap($$,$3); }
  ;

naked_if
  : standalone_expr opt_colon block_expr opt_elif_list opt_else      { ($4)->push_front(AstIf::ConditionActionPair($1, $3)); $$ = new AstIf($4, $5); }
  ;

valvar
  : VAL                                                              { $$ = ObjType::eNoQualifier; }
  | VAR	                                                             { $$ = ObjType::eMutable; }
  ;

opt_elif_list
  : %empty                                                           { $$ = new std::list<AstIf::ConditionActionPair>(); }  
  | opt_elif_list ELIF standalone_expr opt_colon block_expr          { ($1)->push_back(AstIf::ConditionActionPair($3, $5)); std::swap($$,$1); }
  ;  

opt_else
  : %empty                                                           { $$ = NULL; }
  | ELSE block_expr                                                  { $$ = $2; }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
