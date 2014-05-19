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

  struct RawAstDataDecl {
    RawAstDataDecl(const std::string& name) : m_name(name) {};
    std::string m_name;
  };  

  struct RawAstDataDef {
    RawAstDataDef(RawAstDataDecl* decl, AstValue* initValue = NULL) :
      m_decl(decl), m_initValue(initValue) {};
    RawAstDataDecl* m_decl;
    AstValue* m_initValue;
  };  
}

%param { Driver& driver }
%parse-param { AstSeq*& astRoot }

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
  ARROW "->"
;

%token <char> OP_LPAREN 
%token <std::string> ID "identifier"
%token <int> NUMBER "number"
%precedence ASSIGNEMENT
%left PLUS
      MINUS
%left STAR
      SLASH

%type <AstCtList*> ct_list pure_ct_list
%type <std::list<std::string>*> param_ct_list pure_naked_param_ct_list
%type <AstSeq*> maybe_empty_seq seq pure_seq seq_without_comma opt_else
%type <AstValue*> expr expr_leaf naked_if
%type <std::list<AstIf::ConditionActionPair>*> opt_elif_list
%type <std::string> param_decl
%type <AstDataDecl::EStorage> valvar
%type <RawAstDataDecl*> naked_data_decl naked_data_decl_opt_type
%type <RawAstDataDef*> naked_data_def
%type <AstFunDecl*> naked_fun_decl
%type <AstFunDef*> naked_fun_def

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

/* I'm not sure yet whether EF will allow omitting parentheses also in future,
but for now I want to have the option for that way */
param_ct_list
  : %empty                                          { $$ = new std::list<std::string>(); }
  | LPAREN RPAREN                                   { $$ = new std::list<std::string>(); }
  | pure_naked_param_ct_list opt_comma              { std::swap($$,$1); }
  | LPAREN pure_naked_param_ct_list opt_comma RPAREN{ std::swap($$,$2); }
  ;

pure_naked_param_ct_list
  : param_decl                                      { $$ = new std::list<std::string>(); ($$)->push_back($1); }
  | pure_naked_param_ct_list COMMA param_decl       { ($1)->push_back($3); std::swap($$,$1); }
  ;

param_decl
  : ID COLON type                                   { swap($$,$1); /* currently ignores type */ }
  ;
  
type
  : ID
  ;

opt_colon_type
  : %empty
  | COLON 
  | COLON type
  ;

opt_arrow_type
  : %empty
  | ARROW 
  | ARROW type
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


  /* binary operators */
  | ID   EQUAL       expr         %prec ASSIGNEMENT { $$ = new AstOperator('=', new AstSymbol(new std::string($1), AstSymbol::eLValue), $3); }
  | ID   COLON_EQUAL expr         %prec ASSIGNEMENT { $$ = new AstDataDef(new AstDataDecl($1), $3); }
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
  | valvar        naked_data_def SEMICOLON                           { $$ = new AstDataDef( new AstDataDecl($2->m_decl->m_name, $1), $2->m_initValue); delete $2->m_decl; delete $2; }
  | valvar LPAREN naked_data_def RPAREN                              { $$ = new AstDataDef( new AstDataDecl($3->m_decl->m_name, $1), $3->m_initValue); delete $3->m_decl; delete $3; }


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
  : ID COLON type                                                    { $$ = new RawAstDataDecl($1); }
  ;

naked_data_decl_opt_type
  : ID opt_colon_type                                                { $$ = new RawAstDataDecl($1); }
  ;

naked_data_def
  : naked_data_decl_opt_type EQUAL expr                              { $$ = new RawAstDataDef($1, $3); }
  | naked_data_decl                                                  { $$ = new RawAstDataDef($1); }
  ;

naked_fun_def
  : naked_fun_decl EQUAL maybe_empty_seq                             { $$ = new AstFunDef($1, $3); }
  ;
  
naked_fun_decl
  : ID COLON param_ct_list opt_arrow_type                            { $$ = new AstFunDecl($1, $3); }
  ;

naked_if
  : expr COLON seq opt_elif_list opt_else                            { ($4)->push_front(AstIf::ConditionActionPair($1, $3)); $$ = new AstIf($4, $5); }
  ;

valvar
  : VAL                                                              { $$ = AstDataDecl::eValue; }
  | VAR	                                                             { $$ = AstDataDecl::eAlloca; }
  ;

opt_elif_list
  : %empty                                                           { $$ = new std::list<AstIf::ConditionActionPair>(); }  
  | opt_elif_list ELIF expr COLON seq                                { ($1)->push_back(AstIf::ConditionActionPair($3, $5)); std::swap($$,$1); }
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
