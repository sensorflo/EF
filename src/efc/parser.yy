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

/* Declarations needed to declare semantic value types. Semantic values of
tokens are also needed by scanner. */
%code requires
{
  #include "../generalvalue.h"

  class ObjTypeFunda;

  struct NumberToken {
    NumberToken() : m_value(0), m_objType(NULL) {}
    NumberToken(GeneralValue value, ObjTypeFunda* objType) : m_value(value), m_objType(objType) {}
    GeneralValue m_value;
    ObjTypeFunda* m_objType;
  };
}

/* Declarations needed for declarations of the generated parser */
%code requires
{
  class Driver;
  class ParserExt;
  class AstNode;

  struct RawAstDataDecl;
  struct RawAstDataDef;
}

/** Defines signature for the lex familiy of functions. This signature is
reduntaly defined by YYLEX_SIGNATURE and by %lex-param of bison's input file */
%lex-param { Driver& driver }
%parse-param { Driver& driver } { ParserExt& parserExt } { AstNode*& astRoot } 

%code provides
{
  #include "../parserapiext.h"
}

%locations
%initial-action
{
  @$.initialize(&driver.fileName());
};

/*%define parse.trace*/
%define parse.error verbose

%code
{
  #include "../scanner.h"
  #include "../driver.h"
  #include "../parserext.h"
  #include "../objtype.h"
  using namespace std;
  using namespace yy;
}

%define api.token.prefix {TOK_}
%token
  END_OF_FILE  0  "end of file"
  TOKENLISTSTART "<TOKENLISTSTART>"
  END "end"
  DECL "decl"
  IF "if"
  ELIF "elif"
  ELSE "else"
  WHILE "while"
  FUN "fun"
  VAL "val"
  VAR "var"
  RAW_NEW "raw_new"
  RAW_DELETE "raw_delete"
  NOP "nop"
  RETURN "return"
  EQUAL "="
  DOT_EQUAL ".="
  COLON_EQUAL ":="
  COMMA ","
  SEMICOLON ";"
  DOLLAR "$"
  COLON ":"
  PLUS "+"
  MINUS "-"
  STAR "*"
  CARET "^"
  AMPER "&"
  NOT "not"
  EXCL "!"
  AND "and"
  AMPER_AMPER "&&"
  OR "or"
  PIPE_PIPE "||"
  EQUAL_EQUAL "=="
  SLASH "/"
  G_LPAREN "g("
  LPAREN "("
  RPAREN ")"
  LBRACE "{"
  RBRACE "}"
  ARROW "->"
;

%token <ObjTypeFunda::EType> FUNDAMENTAL_TYPE
%token <std::string> OP_NAME 
%token <std::string> ID "identifier"
%token <NumberToken> NUMBER "number"
%precedence ASSIGNEMENT
%right EQUAL DOT_EQUAL
%left PIPE_PIPE OR
%left AMPER_AMPER AND
%left EQUAL_EQUAL
%left PLUS
      MINUS
%left STAR
      SLASH
%precedence EXCL NOT AMPER CARET
%precedence LPAREN

%token TOKENLISTEND "<TOKENLISTEND>"


%type <AstCtList*> ct_list initializer pure2_standalone_expr_seq
%type <std::list<AstArgDecl*>*> pure_naked_param_ct_list
%type <std::list<AstValue*>*> pure_ct_list
%type <AstValue*> block_expr standalone_expr_seq standalone_expr sub_expr operator_expr primary_expr list_expr naked_if elif opt_else naked_return naked_while
%type <AstArgDecl*> param_decl
%type <ObjType::Qualifiers> valvar
%type <RawAstDataDecl*> naked_data_decl
%type <RawAstDataDef*> naked_data_def
%type <AstFunDecl*> naked_fun_decl
%type <AstFunDef*> naked_fun_def
%type <ObjType*> type opt_colon_type opt_ret_type

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
  : standalone_expr_seq                             { $$ = new AstBlock($1); }
  ;

/* Note that the trailing 'seq_operator' is not really a sequence operator,
i.e. it does not build an expression sequence */
standalone_expr_seq
  : standalone_expr seq_operator                    { std::swap($$,$1); }
  | pure2_standalone_expr_seq seq_operator          { $$ = parserExt.mkOperatorTree(";", $1); }
  ;

pure2_standalone_expr_seq
  : standalone_expr seq_operator standalone_expr           { $$ = new AstCtList($1,$3); }
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
  : ID COLON type                                   { $$ = new AstArgDecl($1, &(($3)->addQualifiers(ObjType::eMutable))); }
  ;
  
type
  : FUNDAMENTAL_TYPE                                { $$ = new ObjTypeFunda($1); }
  | CARET type                                      { $$ = new ObjTypePtr(std::shared_ptr<ObjType>{$2}); }
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
  /* function call and cast */
  : sub_expr         LPAREN ct_list         RPAREN  { $$ = new AstFunCall($1, $3); }
  | OP_NAME          LPAREN ct_list         RPAREN  { $$ = parserExt.mkOperatorTree($1, $3); }
  | FUNDAMENTAL_TYPE LPAREN standalone_expr RPAREN  { $$ = new AstCast($1, $3); }

  /* unary prefix */
  | NOT  sub_expr                                   { $$ = new AstOperator(AstOperator::eNot, $2); }
  | EXCL sub_expr                                   { $$ = new AstOperator(AstOperator::eNot, $2); }
  | CARET sub_expr                                  { $$ = new AstOperator('^', $2); }
  | AMPER sub_expr                                  { $$ = new AstOperator('&', $2); }

  /* binary operators */
  | sub_expr EQUAL       sub_expr                   { $$ = new AstOperator('=', $1, $3); }
  | sub_expr DOT_EQUAL   sub_expr                   { $$ = new AstOperator(".=", $1, $3); }
  | ID       COLON_EQUAL sub_expr %prec ASSIGNEMENT { $$ = new AstDataDef(new AstDataDecl($1, new ObjTypeFunda(ObjTypeFunda::eInt)), $3); }
  | sub_expr OR          sub_expr                   { $$ = new AstOperator(AstOperator::eOr, $1, $3); }
  | sub_expr PIPE_PIPE   sub_expr                   { $$ = new AstOperator(AstOperator::eOr, $1, $3); }
  | sub_expr AND         sub_expr                   { $$ = new AstOperator(AstOperator::eAnd, $1, $3); }
  | sub_expr AMPER_AMPER sub_expr                   { $$ = new AstOperator(AstOperator::eAnd, $1, $3); }
  | sub_expr EQUAL_EQUAL sub_expr                   { $$ = new AstOperator(AstOperator::eEqualTo, $1, $3); }
  | sub_expr PLUS        sub_expr                   { $$ = new AstOperator('+', $1, $3); }
  | sub_expr MINUS       sub_expr                   { $$ = new AstOperator('-', $1, $3); }
  | sub_expr STAR        sub_expr                   { $$ = new AstOperator('*', $1, $3); }
  | sub_expr SLASH       sub_expr                   { $$ = new AstOperator('/', $1, $3); }
  ;

primary_expr
  : list_expr                                       { std::swap($$,$1); }
  | NUMBER                                          { $$ = new AstNumber($1.m_value, $1.m_objType); }
  | G_LPAREN standalone_expr_seq RPAREN             { $$ = $2; }
  | ID                                              { $$ = new AstSymbol($1); }
  | NOP                                             { $$ = new AstNop(); }
  ;
  
list_expr
  : valvar kwao naked_data_def kwac                 { $$ = parserExt.mkDataDef($1, $3); }
  | DECL kwao FUN naked_fun_decl kwac               { $$ = $4; }
  | FUN kwao naked_fun_def kwac                     { $$ = $3; }
  | IF kwao naked_if kwac                           { std::swap($$,$3); }
  | WHILE kwao naked_while kwac                     { std::swap($$,$3); }
  | RETURN kwao naked_return kwac                   { $$ = $3; }
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
  : naked_fun_decl EQUAL block_expr                                  { $$ = parserExt.mkFunDef($1, $3); }
  ;
  
naked_fun_decl
  : ID COLON                                                  opt_ret_type  { $$ = parserExt.mkFunDecl($1, $3); }
  | ID COLON LPAREN                                    RPAREN opt_ret_type  { $$ = parserExt.mkFunDecl($1, $5); }
  | ID COLON LPAREN pure_naked_param_ct_list opt_comma RPAREN opt_ret_type  { $$ = parserExt.mkFunDecl($1, $7, $4); }
  ;

opt_ret_type
  : %empty                                                           { assert(false); } // intended for a future auto type
  | type                                                             { swap($$,$1); }
  ;

initializer
  : EQUAL standalone_expr                                            { $$ = new AstCtList($2); }
  | EQUAL LPAREN ct_list RPAREN                                      { swap($$,$3); }
  ;

valvar
  : VAL                                                              { $$ = ObjType::eNoQualifier; }
  | VAR	                                                             { $$ = ObjType::eMutable; }
  ;

naked_if
  : standalone_expr opt_colon block_expr opt_else                    { $$ = new AstBlock(new AstIf($1, $3, $4)); }
  | standalone_expr opt_colon block_expr elif                        { $$ = new AstBlock(new AstIf($1, $3, $4)); }
  ;

elif
  : ELIF standalone_expr opt_colon block_expr opt_else               { $$ = new AstBlock(new AstIf($2, $4, $5)); }  
  | ELIF standalone_expr opt_colon block_expr elif                   { $$ = new AstBlock(new AstIf($2, $4, $5)); }
  ;  

opt_else
  : %empty                                                           { $$ = NULL; }
  | ELSE block_expr                                                  { $$ = $2; }
  ;

naked_while
  : standalone_expr opt_colon block_expr                             { $$ = new AstBlock(new AstLoop($1, $3)); }
  ;

naked_return
  : %empty                                                           { $$ = new AstReturn(new AstNop()); }
  | standalone_expr                                                  { $$ = new AstReturn($1); }
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
