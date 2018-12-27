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

  struct TypeAndStorageDuration {
    AstObjType* m_type;
    StorageDuration m_storageDuration;
  };

  struct FunSignature {
    std::vector<AstDataDef*>* m_paramDefs;
    AstObjType* m_retType;
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
  IF_LPAREN "if("
  IF "if"
  THEN "then"
  ELIF "elif"
  ELSE "else"
  WHILE_LPAREN "while("
  WHILE "while"
  DO_LPAREN "do("
  DO "do"
  FUN_LPAREN "fun("
  FUN "fun"
  VAL_LPAREN "val("
  VAL "val"
  VAR_LPAREN "var("
  VAR "var"
  RAW_NEW_LPAREN "raw_new("
  RAW_NEW "raw_new"
  RAW_DELETE_LPAREN "raw_delete("
  RAW_DELETE "raw_delete"
  NOP "nop"
  RETURN_LPAREN "return("
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
  LPAREN_EQUAL "(="
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


%type <AstCtList*> ct_list ct_list_2plus initializer_arg opt_initializer_arg initializer_special_arg opt_initializer_special_arg naked_initializer
%type <std::vector<AstNode*>*> pure_standalone_node_seq_expr
%type <std::vector<AstDataDef*>*> pure_naked_param_ct_list
%type <std::vector<AstObject*>*> pure_ct_list
%type <AstNode*> standalone_node
%type <AstObject*> block_expr standalone_node_seq_expr standalone_expr sub_expr ct_list_arg operator_expr primary_expr list_expr naked_if elif_chain opt_else naked_return naked_while
%type <AstDataDef*> param_def
%type <ObjType::Qualifiers> valvar opt_valvar valvar_lparen type_qualifier
%type <StorageDuration> storage_duration storage_duration_arg
%type <TypeAndStorageDuration> type_and_or_storage_duration_arg
%type <RawAstDataDef*> naked_data_def
%type <AstFunDef*> naked_fun_def
%type <AstObjType*> type opt_type type_arg opt_ret_type
%type <ConditionActionPair> condition_action_pair_then
%type <std::string> opt_id
%type <FunSignature> fun_signature_arg opt_fun_signature_arg

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
  | pure_ct_list opt_sep                            { $$ = new AstCtList($1); }
  ;

ct_list_2plus
  : ct_list_arg sep pure_ct_list opt_sep            { ($3)->insert(($3)->begin(),$1); $$ = new AstCtList($3); }
  ;

pure_ct_list
  : ct_list_arg                                     { $$ = new std::vector<AstObject*>(); ($$)->push_back($1); }
  | pure_ct_list sep ct_list_arg                    { ($1)->push_back($3); std::swap($$,$1); }
  ;

/* will later be more complex than standalone_node_seq_expr, e.g. by allowing
designated initializer. For now, I like the alias, since it's easier for me to
think about this way. */
ct_list_arg
  : standalone_node_seq_expr                        { swap($$,$1); }
  ;

type
  : FUNDAMENTAL_TYPE                                { $$ = new AstObjTypeSymbol($1); }
  | STAR           opt_newline type                 { $$ = new AstObjTypePtr($3); }
  | type_qualifier opt_newline type                 { $$ = new AstObjTypeQuali($1, $3); }
  | ID                                              { assert(false); /* user defined names not yet supported; but I wanted to have ID already in grammar*/ }
  ;

opt_type
  : %empty                                          { $$ = parserExt.mkDefaultType(); }
  | type                                            { swap($$,$1); }
  ;

type_arg
  : COLON opt_type                                  { swap($$,$2); }
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

type_and_or_storage_duration_arg
  : type_arg                                        { ($$).m_type = $1;
                                                      ($$).m_storageDuration = parserExt.mkDefaultStorageDuration(); }
  | storage_duration_arg                            { ($$).m_type = parserExt.mkDefaultType();
                                                      ($$).m_storageDuration = $1; }
  | type_arg storage_duration_arg                   { ($$).m_type = $1;
                                                      ($$).m_storageDuration = $2; }
  ;

/* generic list separator */
sep
  : COLON
  | COMMA
  ;

opt_sep
  : %empty
  | sep
  ;

opt_colon
  : %empty
  | COLON
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
  /* function call and cast (aka construction of temporary) */
  : sub_expr         LPAREN ct_list RPAREN          { $$ = new AstFunCall($1, $3); }
  | OP_NAME          LPAREN ct_list RPAREN          { $$ = parserExt.mkOperatorTree($1, $3); }
  | FUNDAMENTAL_TYPE LPAREN ct_list RPAREN          { $$ = new AstCast(new AstObjTypeSymbol($1), $3); }

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
  | ID       COLON_EQUAL opt_newline sub_expr %prec ASSIGNEMENT { $$ = new AstDataDef($1, parserExt.mkDefaultType(), parserExt.mkDefaultStorageDuration(), new AstCtList($4)); }
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
  : valvar_lparen     naked_data_def       RPAREN   { $$ = parserExt.mkDataDef($1, $2); }
  | valvar            naked_data_def       kwac     { $$ = parserExt.mkDataDef($1, $2); }
  | FUN_LPAREN        naked_fun_def        RPAREN   { $$ = $2; }
  | FUN               naked_fun_def        kwac     { $$ = $2; }
  | IF_LPAREN         naked_if             RPAREN   { std::swap($$,$2); }
  | IF                naked_if             kwac     { std::swap($$,$2); }
  | WHILE_LPAREN      naked_while          RPAREN   { std::swap($$,$2); }
  | WHILE             naked_while          kwac     { std::swap($$,$2); }
  | RETURN_LPAREN     naked_return         RPAREN   { $$ = $2; }
  | RETURN            naked_return         kwac     { $$ = $2; }
  | RAW_NEW_LPAREN    type initializer_arg RPAREN   { $$ = nullptr; }
  | RAW_NEW           type initializer_arg kwac     { $$ = nullptr; }
  | RAW_DELETE_LPAREN sub_expr             RPAREN   { $$ = nullptr; }
  | RAW_DELETE        sub_expr             kwac     { $$ = nullptr; }
  ;

/* keyword argument list close delimiter */
kwac
  : DOLLAR
  | END
  | ENDOF opt_newline id_or_keyword opt_newline DOLLAR /* or consider end(...) */
  ;

id_or_keyword
  : ID | IF | ELIF | ELSE | FUN | VAL | VAR | END | NOT | AND | OR | FUNDAMENTAL_TYPE
  ;

/* TODO: semantic analyzer must report error if neither type nor initializer is
specified */
naked_data_def
  /* A trailing opt_initializer_arg is only possible if there's at least one
  type or storage duration arg, because else its equal_as_sep conflicts with any
  = in the opt_initializer_special_arg's standalone_expr */
  : opt_id opt_initializer_special_arg type_and_or_storage_duration_arg opt_initializer_arg   { $$ = new RawAstDataDef(parserExt.errorHandler(), $1, $2, $4, ($3).m_type, ($3).m_storageDuration); }

  /* KLUDGE: We want to disallow an empty naked_data_def. The above already
  ensures that at type or storage duration is specified. Now we ensure that
  either id or initializer is specified. */
  | opt_id initializer_special_arg                                                            { $$ = new RawAstDataDef(parserExt.errorHandler(), $1, $2     , nullptr, parserExt.mkDefaultType(), parserExt.mkDefaultStorageDuration()); }
  | ID                                                                                        { $$ = new RawAstDataDef(parserExt.errorHandler(), $1, nullptr, nullptr, parserExt.mkDefaultType(), parserExt.mkDefaultStorageDuration()); }
  ;

naked_fun_def
  : opt_id opt_fun_signature_arg equal_as_sep block_expr                    { $$ = parserExt.mkFunDef($1, ($2).m_paramDefs, ($2).m_retType, $4); }
  ;

fun_signature_arg
  : COLON                                                      opt_ret_type { ($$).m_paramDefs = new std::vector<AstDataDef*>(); ($$).m_retType = $2; }
  | opt_colon LPAREN                                    RPAREN opt_ret_type { ($$).m_paramDefs = new std::vector<AstDataDef*>(); ($$).m_retType = $4; }
  | opt_colon LPAREN pure_naked_param_ct_list opt_comma RPAREN opt_ret_type { ($$).m_paramDefs = $3;                             ($$).m_retType = $6; }
  ;

opt_fun_signature_arg                           
  : %empty                                                                  { ($$).m_paramDefs = new std::vector<AstDataDef*>(); ($$).m_retType = parserExt.mkDefaultType(); }
  | fun_signature_arg                                                       { swap($$,$1); }
  ;

pure_naked_param_ct_list
  : param_def                                                        { $$ = new std::vector<AstDataDef*>(); ($$)->push_back($1); }
  | pure_naked_param_ct_list COMMA param_def                         { ($1)->push_back($3); std::swap($$,$1); }
  ;

param_def
  : opt_valvar naked_data_def                                        { $$ = parserExt.mkDataDef($1, $2); }
  ;

opt_ret_type
  : %empty                                                           { assert(false); } // intended for a future auto type
  | type                                                             { swap($$,$1); }
  ;

initializer_arg
  : equal_as_sep        naked_initializer                            { swap($$,$2); }
  ;

opt_initializer_arg
  : %empty                                                           { $$ = nullptr;  }
  | initializer_arg                                                  { swap($$,$1); }
  ;

/*
TODO: semantic analizer must report error when noinit is used for function
parameters in a function definition

TODO: semantic analizer must report error when invalid storage duration is used
for function parameters in in a function definition.

1) If that is an parenthesized expression, say "val a = (42)$", then the resulting
   initializer in the AST is two nested AstSeq, i.e. (;;42) (the complete AST is
   ":;data(a int (;;42))"). One is because ct_list_arg is ultimatively an
   standalone_node_seq_expr, and one is because here "(...)" is a primary_expr,
   whose content is again a standalone_node_seq_expr. */
naked_initializer
  : LPAREN               RPAREN                                      { $$ = new AstCtList(); }
  | LPAREN ct_list_2plus RPAREN                                      { swap($$,$2); }
  | LPAREN NOINIT        RPAREN                                      { $$ = new AstCtList(AstDataDef::noInit); }
  | ct_list_arg /* 1) */                                             { auto objs = new std::vector<AstObject*>(); objs->push_back($1); $$ = new AstCtList(objs); }
  | NOINIT                                                           { $$ = new AstCtList(AstDataDef::noInit); }
  ;

initializer_special_arg
  : initializer_arg                                                  { swap($$,$1); }
  | lparen_as_sep ct_list RPAREN                                     { swap($$,$2); }
  | lparen_as_sep NOINIT  RPAREN                                     { $$ = new AstCtList(AstDataDef::noInit); }
  ;

opt_initializer_special_arg
  : %empty                                                           { $$ = nullptr;  }
  | initializer_special_arg                                          { swap($$,$1); }
  ;

valvar
  : VAL                                                              { $$ = ObjType::eNoQualifier; }
  | VAR                                                              { $$ = ObjType::eMutable; }
  ;

opt_valvar
  : %empty                                                           { $$ = parserExt.mkDefaultObjectTypeQualifier(); }
  | valvar                                                           { swap($$,$1); }
  ; 

valvar_lparen
  : VAL_LPAREN                                                       { $$ = ObjType::eNoQualifier; }
  | VAR_LPAREN                                                       { $$ = ObjType::eMutable; }
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
  /* not ct_list_arg because do_sep allows newline as sep, which in ct_list_arg
  are seq_operator */
  : standalone_expr do_sep block_expr                                { $$ = new AstBlock(new AstLoop($1, $3)); }
  ;

condition_action_pair_then
  /* see naked_while why it's standalone_expr and not  seq_operator */
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

opt_id
  : %empty                                                           { $$ = std::string(); }
  | ID                                                               { swap($$,$1); }
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
