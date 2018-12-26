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
  /* Declarations and definitions needed to declare semantic value types used in
  tokens produced by scanner. */
  #include "../generalvalue.h"
  #include "../objtype.h"

  enum class StorageDuration: int;

  struct NumberToken {
    NumberToken() : m_value(0), m_objType(ObjTypeFunda::eVoid) {}
    NumberToken(GeneralValue value, ObjTypeFunda::EType objType) : m_value(value), m_objType(objType) {}
    GeneralValue m_value;
    ObjTypeFunda::EType m_objType;
  };


  /* Further declarations and definitions needed to declare semantic value
  types, which however are only used for non-terminal symbols, i.e. which the
  scanner doesn't need to know */
  #include "../astforwards.h"

  class RawAstDataDef;

  struct ConditionActionPair {
    AstObject* m_condition;
    AstObject* m_action;
  };


  /* Further declarations and definitions needed to declare the generated parser */
  class Driver;
  class ParserExt;
  class AstNode;
  class TokenStream;
}

/** \internal Signature of yylex is redundantely defined here by %lex-param
and by declaration of free function yylex */
%lex-param { TokenStream& tokenStream }
%parse-param { TokenStream& tokenStream } { Driver& driver } { ParserExt& parserExt } { std::unique_ptr<AstNode>& astRoot }

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
  #include "../driver.h"
  #include "../parserext.h"
  #include "../objtype.h"
  #include "../storageduration.h"
  #include "../ast.h"
  using namespace std;
  using namespace yy;

  /** \internal Signature is redundantely defined here and by %lex-param of
  bison's .yy input file. The only reason for the existence of the free
  function yylex is that the author doesn't know how to make bison's generated
  parser use tokenStream directly */
  yy::Parser::symbol_type yylex(TokenStream& tokenStream);
}

%define api.token.prefix {TOK_}
%token
  END_OF_FILE  0  "end of file"
  TOKENLISTSTART "<TOKENLISTSTART>"
  END "end"
  ENDOF "endof"
  IF "if"
  THEN "then"
  ELIF "elif"
  ELSE "else"
  WHILE "while"
  DO "do"
  FUN "fun"
  VAL "val"
  VAR "var"
  RAW_NEW "raw_new"
  RAW_DELETE "raw_delete"
  NOP "nop"
  RETURN "return"
  EQUAL "="
  EQUAL_LESS "=<"
  COLON_EQUAL ":="
  COMMA ","
  SEMICOLON ";"
  NEWLINE "newline"
  DOLLAR "$"
  COLON ":"
  PLUS "+"
  MINUS "-"
  STAR "*"
  AMPER "&"
  NOT "not"
  EXCL "!"
  AND "and"
  AMPER_AMPER "&&"
  OR "or"
  PIPE_PIPE "||"
  EQUAL_EQUAL "=="
  COMMA_EQUAL ",="
  LPAREN_EQUAL "(="
  LPAREN_DOLLAR "($"
  SLASH "/"
  LPAREN "("
  RPAREN ")"
  LBRACE "{"
  RBRACE "}"
  ARROW "->"
  MUT "mut"
  IS "is"
  STATIC "static"
  LOCAL "local"
  NOINIT "noinit"
;

%token <ObjTypeFunda::EType> FUNDAMENTAL_TYPE
%token <std::string> OP_NAME
%token <std::string> ID "identifier"
%token <NumberToken> NUMBER "number"
%precedence ASSIGNEMENT
%right EQUAL EQUAL_LESS
%left PIPE_PIPE OR
%left AMPER_AMPER AND
%left EQUAL_EQUAL
%left PLUS
      MINUS
%left STAR
      SLASH
%precedence EXCL NOT AMPER
%precedence LPAREN

%token TOKENLISTEND "<TOKENLISTEND>"


%type <AstCtList*> ct_list initializer_arg initializer_special_arg
%type <std::vector<AstNode*>*> pure_standalone_node_seq_expr
%type <std::vector<AstDataDef*>*> pure_naked_param_ct_list
%type <std::vector<AstObject*>*> pure_ct_list
%type <AstNode*> standalone_node
%type <AstObject*> block_expr standalone_node_seq_expr standalone_expr sub_expr operator_expr primary_expr list_expr naked_if elif_chain opt_else naked_return naked_while
%type <AstDataDef*> param_decl
%type <ObjType::Qualifiers> valvar type_qualifier
%type <StorageDuration> storage_duration storage_duration_arg opt_storage_duration_arg
%type <RawAstDataDef*> naked_data_def data_def_args
%type <AstFunDef*> naked_fun_def
%type <AstObjType*> type opt_type type_arg opt_type_arg opt_ret_type
%type <ConditionActionPair> condition_action_pair_then

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
  : block_expr END_OF_FILE                          { astRoot = std::unique_ptr<AstNode>($1); }
  ;

block_expr
  : standalone_node_seq_expr                        { $$ = new AstBlock($1); }
  ;

/* Sequence of standalone nodes being itself an expr. The dynamic type of the
last node must be expr, which is verified later by the semantic analizer. That
is also the reason why it makes sense to create a 'sequence' even if there is
only one element; there must be someone verifying that the dynamic type of
that element is expr.

Note that the seq_operator is _not_ the thing that builds / creates the
sequence. It is the context where standalone_node_seq_expr is used that
defines that an sequence must be created, also if it only contains one
element. */
standalone_node_seq_expr
  : pure_standalone_node_seq_expr opt_seq_operator              { $$ = new AstSeq($1); }
  ;

pure_standalone_node_seq_expr
  : standalone_node                                             { $$ = new std::vector<AstNode*>{$1}; }
  | pure_standalone_node_seq_expr seq_operator standalone_node  { ($1)->push_back($3); std::swap($$,$1); }
  ;

standalone_node
  : standalone_expr                                 { $$ = $1; }
  ;

standalone_expr
  : sub_expr                                        { std::swap($$,$1); }
  ;

ct_list
  : %empty                                          { $$ = new AstCtList(); }
  | pure_ct_list opt_comma                          { $$ = new AstCtList($1); }
  ;

pure_ct_list
  : standalone_expr                                 { $$ = new std::vector<AstObject*>(); ($$)->push_back($1); }
  | pure_ct_list COMMA standalone_expr              { ($1)->push_back($3); std::swap($$,$1); }
  ;

type
  : FUNDAMENTAL_TYPE                                { $$ = new AstObjTypeSymbol($1); }
  | STAR           opt_newline type                 { $$ = new AstObjTypePtr($3); }
  | type_qualifier opt_newline type                 { $$ = new AstObjTypeQuali($1, $3); }
  | ID                                              { assert(false); /* user defined names not yet supported; but I wanted to have ID already in grammar*/ }
  ;

opt_type
  : %empty                                          { $$ = new AstObjTypeSymbol(ObjTypeFunda::eInt); }
  | type                                            { swap($$,$1); }
  ;

type_arg
  : COLON opt_type                                  { swap($$,$2); }
  ;

opt_type_arg
  : %empty                                          { $$ = new AstObjTypeSymbol(ObjTypeFunda::eInt); }
  | type_arg                                        { swap($$,$1); }
  ;

type_qualifier
  : MUT                                             { $$ = ObjType::eMutable; }
  ;

storage_duration
  : STATIC                                          { $$ = StorageDuration::eStatic; }
  | LOCAL                                           { $$ = StorageDuration::eLocal; }
  ;

storage_duration_arg
  : IS storage_duration                             { swap($$, $2); }
  ;

opt_storage_duration_arg
  : %empty                                          { $$ = StorageDuration::eLocal; }
  | storage_duration_arg                            { swap($$, $1); }
  ;

/* generic list separator */
sep
  : COLON
  | COMMA
  ;

/* generic list separator, inclusive newline */
sep_incl_newline
  : sep
  | NEWLINE
  ;

then_sep
  : THEN
  | sep_incl_newline
  ;

do_sep
  : DO
  | sep_incl_newline
  ;

else_sep
  : ELSE
  | sep
  ;

equal_as_sep
  : opt_newline EQUAL opt_newline
  ;

/* tokenfilter already removes newlines after LPAREN */
lparen_as_sep
  : opt_newline LPAREN
  ;

opt_comma
  : %empty
  | COMMA
  ;

seq_operator
  : NEWLINE
  | SEMICOLON
  ;

opt_seq_operator
  : %empty
  | seq_operator
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
  | NOT   opt_newline sub_expr                      { $$ = new AstOperator(AstOperator::eNot, $3); }
  | EXCL  opt_newline sub_expr                      { $$ = new AstOperator(AstOperator::eNot, $3); }
  | STAR  opt_newline sub_expr                      { $$ = new AstOperator(AstOperator::eDeref, $3); }
  | AMPER opt_newline sub_expr                      { $$ = new AstOperator('&', $3); }
  | MINUS opt_newline sub_expr                      { $$ = new AstOperator('-', $3); }
  | PLUS  opt_newline sub_expr                      { $$ = new AstOperator('+', $3); }

  /* binary operators */
  | sub_expr EQUAL       opt_newline sub_expr                   { $$ = new AstOperator('=', $1, $4); }
  | sub_expr EQUAL_LESS  opt_newline sub_expr                   { $$ = new AstOperator("=<", $1, $4); }
  | ID       COLON_EQUAL opt_newline sub_expr %prec ASSIGNEMENT { $$ = new AstDataDef($1, new AstObjTypeSymbol(ObjTypeFunda::eInt), StorageDuration::eLocal, new AstCtList($4)); }
  | sub_expr OR          opt_newline sub_expr                   { $$ = new AstOperator(AstOperator::eOr, $1, $4); }
  | sub_expr PIPE_PIPE   opt_newline sub_expr                   { $$ = new AstOperator(AstOperator::eOr, $1, $4); }
  | sub_expr AND         opt_newline sub_expr                   { $$ = new AstOperator(AstOperator::eAnd, $1, $4); }
  | sub_expr AMPER_AMPER opt_newline sub_expr                   { $$ = new AstOperator(AstOperator::eAnd, $1, $4); }
  | sub_expr EQUAL_EQUAL opt_newline sub_expr                   { $$ = new AstOperator(AstOperator::eEqualTo, $1, $4); }
  | sub_expr PLUS        opt_newline sub_expr                   { $$ = new AstOperator('+', $1, $4); }
  | sub_expr MINUS       opt_newline sub_expr                   { $$ = new AstOperator('-', $1, $4); }
  | sub_expr STAR        opt_newline sub_expr                   { $$ = new AstOperator('*', $1, $4); }
  | sub_expr SLASH       opt_newline sub_expr                   { $$ = new AstOperator('/', $1, $4); }
  ;

primary_expr
  : list_expr                                       { std::swap($$,$1); }
  | NUMBER                                          { $$ = new AstNumber($1.m_value, $1.m_objType); }
  | LPAREN standalone_node_seq_expr RPAREN          { $$ = $2; }
  | ID                                              { $$ = new AstSymbol($1); }
  | NOP                                             { $$ = new AstNop(); }
  ;

list_expr
  : valvar kwao naked_data_def kwac                 { $$ = parserExt.mkDataDef($1, $3); }
  | FUN kwao naked_fun_def kwac                     { $$ = $3; }
  | IF kwao naked_if kwac                           { std::swap($$,$3); }
  | WHILE kwao naked_while kwac                     { std::swap($$,$3); }
  | RETURN kwao naked_return kwac                   { $$ = $3; }
  | RAW_NEW kwao type initializer_arg kwac          { $$ = nullptr; }
  | RAW_DELETE kwao sub_expr kwac                   { $$ = nullptr; }
  ;

/* keyword argument list open delimiter */
kwao
  : %empty
  | LPAREN_DOLLAR
  ;

/* keyword argument list close delimiter */
kwac
  : DOLLAR
  | RPAREN
  | END
  | ENDOF opt_newline id_or_keyword opt_newline DOLLAR /* or consider end(...) */
  ;

id_or_keyword
  : ID | IF | ELIF | ELSE | FUN | VAL | VAR | END | NOT | AND | OR | FUNDAMENTAL_TYPE
  ;

/* TODO: merge with naked_data_def */
param_decl
  : ID type_arg                                                      { $$ = new AstDataDef($1, $2); }
  ;

naked_data_def
  : ID initializer_special_arg opt_type_arg opt_storage_duration_arg { $$ = new RawAstDataDef(parserExt.errorHandler(), $1, $2, $3, $4); }
  | ID data_def_args                                                 { $2->setName($1); swap($$,$2); }
  ;

data_def_args
  : %empty                                                           { $$ = new RawAstDataDef(parserExt.errorHandler()); }
  | data_def_args initializer_arg                                    { ($1)->setCtorArgs($2); swap($$,$1); }
  | data_def_args type_arg                                           { ($1)->setAstObjType($2); swap($$,$1); }
  | data_def_args storage_duration_arg                               { ($1)->setStorageDuration($2); swap($$,$1); }
  ;

naked_fun_def
  : ID COLON                                                  opt_ret_type COMMA_EQUAL block_expr { $$ = parserExt.mkFunDef($1, $3, $5); }
  | ID COLON LPAREN                                    RPAREN opt_ret_type COMMA_EQUAL block_expr { $$ = parserExt.mkFunDef($1, $5, $7); }
  | ID COLON LPAREN pure_naked_param_ct_list opt_comma RPAREN opt_ret_type COMMA_EQUAL block_expr { $$ = parserExt.mkFunDef($1, $4, $7, $9); }
  ;

pure_naked_param_ct_list
  : param_decl                                                       { $$ = new std::vector<AstDataDef*>(); ($$)->push_back($1); }
  | pure_naked_param_ct_list COMMA param_decl                        { ($1)->push_back($3); std::swap($$,$1); }
  ;

opt_ret_type
  : %empty                                                           { assert(false); } // intended for a future auto type
  | type                                                             { swap($$,$1); }
  ;

initializer_arg
  : COMMA_EQUAL standalone_expr                                      { $$ = new AstCtList($2); }
  | COMMA_EQUAL NOINIT                                               { $$ = new AstCtList(AstDataDef::noInit); }
  | LPAREN_EQUAL ct_list RPAREN                                      { swap($$,$2); }
  ;

initializer_special_arg
  : equal_as_sep standalone_expr                                     { $$ = new AstCtList($2); }
  | equal_as_sep NOINIT                                              { $$ = new AstCtList(AstDataDef::noInit); }
  | lparen_as_sep ct_list RPAREN                                     { swap($$,$2); }
  ;

valvar
  : VAL                                                              { $$ = ObjType::eNoQualifier; }
  | VAR                                                              { $$ = ObjType::eMutable; }
  ;

naked_if
  : condition_action_pair_then opt_else                              { $$ = new AstBlock(new AstIf(($1).m_condition, ($1).m_action, $2)); }
  | condition_action_pair_then elif_chain                            { $$ = new AstBlock(new AstIf(($1).m_condition, ($1).m_action, $2)); }
  ;

elif_chain
  : ELIF condition_action_pair_then opt_else                         { $$ = new AstBlock(new AstIf(($2).m_condition, ($2).m_action, $3)); }
  | ELIF condition_action_pair_then elif_chain                       { $$ = new AstBlock(new AstIf(($2).m_condition, ($2).m_action, $3)); }
  ;

opt_else
  : %empty                                                           { $$ = nullptr; }
  | else_sep block_expr                                              { $$ = $2; }
  ;

naked_while
  : standalone_expr do_sep block_expr                                { $$ = new AstBlock(new AstLoop($1, $3)); }
  ;

condition_action_pair_then
  : standalone_expr then_sep block_expr                              { $$ = ConditionActionPair{ $1, $3}; }
  ;

naked_return
  : %empty                                                           { $$ = new AstReturn(); }
  | standalone_expr                                                  { $$ = new AstReturn($1); }
  ;

opt_newline
  : %empty
  | NEWLINE
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%
#include "../tokenstream.h"

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}

yy::Parser::symbol_type yylex(TokenStream& tokenStream) {
  return tokenStream.pop();
}
