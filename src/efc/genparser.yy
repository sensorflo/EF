/* Bison declaration section
----------------------------------------------------------------------*/
%skeleton "lalr1.cc"
%token-table
%require "3.0.2"
%expect 0
%defines
%define parser_class_name { GenParser }
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define api.location.type { Location }

%code requires
{
  #include "../location.h"

  /* If api.location.type is not %define-ed, then the generated header files
  which define a location type also define YY_NULLPTR. I think this is a bug in
  Bison. The generated parser should care himself about defining YY_NULLPTR,
  since _he_ want's to use it internally. */
  #ifndef YY_NULLPTR
  # if defined __cplusplus && 201103L <= __cplusplus
  #  define YY_NULLPTR nullptr
  # else
  #  define YY_NULLPTR 0
  # endif
  #endif

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
  #include <string>
  class GenParserExt;
  class TokenStream;
}

// see declaration of yylex
%lex-param { TokenStream& tokenStream } 
%parse-param { TokenStream& tokenStream } { std::string& fileName } { GenParserExt& genParserExt } { std::unique_ptr<AstNode>& astRoot }

%locations
%initial-action
{
  @$.initialize(&fileName);
};

/*%define parse.trace*/
%define parse.error verbose

%code
{
  #include "../genparserext.h"
  #include "../objtype.h"
  #include "../storageduration.h"
  #include "../ast.h"
  #include "../errorhandler.h"
  using namespace std;
  using namespace yy;

  /** Returns the next token. It is called by the generated parser, which
  expects this method to be defined. The argument type is specified by the
  %lex-param directive. The definition is in the epilogue section of this
  file. See also yylex_raw in genscanner.l */
  yy::GenParser::symbol_type yylex(TokenStream& tokenStream);
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
%precedence EXCL NOT AMPER RETURN
%precedence LPAREN

%token TOKENLISTEND "<TOKENLISTEND>"


%type <AstCtList*> ct_list ct_list_2plus initializer_arg opt_initializer_arg initializer_special_arg opt_initializer_special_arg naked_initializer
%type <std::vector<AstNode*>*> pure_node_seq
%type <std::vector<AstDataDef*>*> param_list pure_param_list
%type <std::vector<AstObject*>*> pure_ct_list
%type <AstNode*> node
%type <AstObject*> block node_seq standalone_expr expr ct_list_arg operator_expr primary_expr list_expr naked_if elif_chain opt_else naked_while
%type <AstDataDef*> param_def
%type <ObjType::Qualifiers> valvar opt_valvar valvar_lparen type_qualifier
%type <StorageDuration> storage_duration storage_duration_arg
%type <TypeAndStorageDuration> type_and_or_storage_duration_arg
%type <RawAstDataDef*> naked_data_def
%type <AstFunDef*> naked_fun_def
%type <AstObjType*> type opt_type type_arg
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
  : block END_OF_FILE                               { astRoot = std::unique_ptr<AstNode>($1); }
  ;

block
  : node_seq                                        { $$ = new AstBlock($1, @$); }
  ;

/* Sequence of standalone nodes being itself an expr. The dynamic type of the
last node must be expr, which is verified later by the semantic analizer. That
is also the reason why it makes sense to create a 'sequence' even if there is
only one element; there must be someone verifying that the dynamic type of that
element is expr.

Note that the seq_operator is _not_ the thing that builds / creates the
sequence. It is the context where node_seq is used that defines that an sequence
must be created, also if it only contains one element. */
node_seq
  : pure_node_seq opt_seq_operator                  { $$ = new AstSeq($1, @$); }
  ;

pure_node_seq
  : node                                            { $$ = new std::vector<AstNode*>{$1}; }
  | pure_node_seq seq_operator node                 { ($1)->push_back($3); std::swap($$,$1); }
  ;

/* can later be also e.g. a type definition or import declarative etc. */
node
  : standalone_expr                                 { $$ = $1; }
  ;

standalone_expr
  : expr                                            { std::swap($$,$1); }
  ;

ct_list
  : %empty                                          { $$ = new AstCtList(@$); }
  | pure_ct_list opt_sep                            { $$ = new AstCtList($1, @$); }
  ;

ct_list_2plus
  : ct_list_arg sep pure_ct_list opt_sep            { ($3)->insert(($3)->begin(),$1); $$ = new AstCtList($3, @$); }
  ;

pure_ct_list
  : ct_list_arg                                     { $$ = new std::vector<AstObject*>(); ($$)->push_back($1); }
  | pure_ct_list sep ct_list_arg                    { ($1)->push_back($3); std::swap($$,$1); }
  ;

/* will later be more complex than node_seq, e.g. by allowing designated
initializer. For now, I like the alias, since it's easier for me to think about
this way.

TODO: ct_list_arg can itself be "LPAREN ct_list RPAREN", to construct the arg
with multiple (nested) arguments.

TODO: Note that the standalone_expr within node_seq have their own semantics,
e.g. concerning temporaries. So now we have these rules nested, which is awfully
complex to understand. It might work because the sequence typically only has one
element */
ct_list_arg
  : node_seq                                        { swap($$,$1); }
  ;

type
  : FUNDAMENTAL_TYPE                                { $$ = new AstObjTypeSymbol($1, @$); }
  | STAR           opt_nl type                      { $$ = new AstObjTypePtr($3, @$); }
  | type_qualifier opt_nl type                      { $$ = new AstObjTypeQuali($1, $3, @$); }
  | ID                                              { assert(false); /* user defined names not yet supported; but I wanted to have ID already in grammar*/ }
  ;

opt_type
  : %empty                                          { $$ = genParserExt.mkDefaultType(@$); }
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
                                                      ($$).m_storageDuration = genParserExt.mkDefaultStorageDuration(); }
  | storage_duration_arg                            { ($$).m_type = genParserExt.mkDefaultType(@$);
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
  : opt_nl EQUAL opt_nl
  ;

/* tokenfilter already removes newlines after LPAREN */
lparen_as_sep
  : opt_nl LPAREN
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

expr
  : primary_expr                                    { std::swap($$,$1); }
  | operator_expr                                   { std::swap($$,$1); }
  ;

operator_expr
  /* function call and cast (aka construction of temporary) */
  : expr             LPAREN ct_list RPAREN          { $$ = new AstFunCall($1, $3, @2); }
  | OP_NAME          LPAREN ct_list RPAREN          { $$ = genParserExt.mkOperatorTree($1, $3, @2); }
  | FUNDAMENTAL_TYPE LPAREN ct_list RPAREN          { $$ = new AstCast(new AstObjTypeSymbol($1, @1), $3, @2); }

  /* unary prefix */
  | NOT    opt_nl expr                              { $$ = new AstOperator(AstOperator::eNot, $3, nullptr, @1); }
  | EXCL   opt_nl expr                              { $$ = new AstOperator(AstOperator::eNot, $3, nullptr, @1); }
  | STAR   opt_nl expr                              { $$ = new AstOperator(AstOperator::eDeref, $3, nullptr, @1); }
  | AMPER  opt_nl expr                              { $$ = new AstOperator('&', $3, nullptr, @1); }
  | MINUS  opt_nl expr                              { $$ = new AstOperator('-', $3, nullptr, @1); }
  | PLUS   opt_nl expr                              { $$ = new AstOperator('+', $3, nullptr, @1); }
  | RETURN        expr                              { $$ = new AstReturn($2, @1); }

  /* binary operators */
  | expr EQUAL       opt_nl expr                    { $$ = new AstOperator('=', $1, $4, @2); }
  | expr EQUAL_LESS  opt_nl expr                    { $$ = new AstOperator("=<", $1, $4, @2); }
  | ID   COLON_EQUAL opt_nl expr %prec ASSIGNEMENT  { $$ = new AstDataDef($1, genParserExt.mkDefaultType(@2), genParserExt.mkDefaultStorageDuration(), new AstCtList(@2, $4), @2); }
  | expr OR          opt_nl expr                    { $$ = new AstOperator(AstOperator::eOr, $1, $4, @2); }
  | expr PIPE_PIPE   opt_nl expr                    { $$ = new AstOperator(AstOperator::eOr, $1, $4, @2); }
  | expr AND         opt_nl expr                    { $$ = new AstOperator(AstOperator::eAnd, $1, $4, @2); }
  | expr AMPER_AMPER opt_nl expr                    { $$ = new AstOperator(AstOperator::eAnd, $1, $4, @2); }
  | expr EQUAL_EQUAL opt_nl expr                    { $$ = new AstOperator(AstOperator::eEqualTo, $1, $4, @2); }
  | expr PLUS        opt_nl expr                    { $$ = new AstOperator('+', $1, $4, @2); }
  | expr MINUS       opt_nl expr                    { $$ = new AstOperator('-', $1, $4, @2); }
  | expr STAR        opt_nl expr                    { $$ = new AstOperator('*', $1, $4, @2); }
  | expr SLASH       opt_nl expr                    { $$ = new AstOperator('/', $1, $4, @2); }
  ;

primary_expr
  : list_expr                                       { std::swap($$,$1); }
  | NUMBER                                          { $$ = new AstNumber($1.m_value, $1.m_objType, @1); }
  | LPAREN node_seq RPAREN                          { $$ = $2; }
  | LPAREN RPAREN                                   { $$ = new AstNop(@$); }
  | ID                                              { $$ = new AstSymbol($1, @1); }
  | NOP                                             { $$ = new AstNop(@1); }
  ;

list_expr
  : valvar_lparen     naked_data_def       RPAREN   { $$ = genParserExt.mkDataDef($1, $2, @$); }
  | valvar            naked_data_def       kwac     { $$ = genParserExt.mkDataDef($1, $2, @$); }
  | FUN_LPAREN        naked_fun_def        RPAREN   { $$ = $2; }
  | FUN               naked_fun_def        kwac     { $$ = $2; }
  | IF_LPAREN         naked_if             RPAREN   { std::swap($$,$2); }
  | IF                naked_if             kwac     { std::swap($$,$2); }
  | WHILE_LPAREN      naked_while          RPAREN   { std::swap($$,$2); }
  | WHILE             naked_while          kwac     { std::swap($$,$2); }
  | RAW_NEW_LPAREN    type initializer_arg RPAREN   { $$ = nullptr; }
  | RAW_NEW           type initializer_arg kwac     { $$ = nullptr; }
  | RAW_DELETE_LPAREN expr                 RPAREN   { $$ = nullptr; }
  | RAW_DELETE        expr                 kwac     { $$ = nullptr; }
  | RETURN_LPAREN     ct_list              RPAREN   { $$ = new AstReturn($2, @$); }
  ;

/* keyword argument list close delimiter */
kwac
  : DOLLAR
  | END
  | ENDOF opt_nl id_or_keyword opt_nl DOLLAR /* or consider end(...) */
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
  : opt_id opt_initializer_special_arg type_and_or_storage_duration_arg opt_initializer_arg   { $$ = new RawAstDataDef(genParserExt.errorHandler(), $1, $2, $4, ($3).m_type, ($3).m_storageDuration, @1); }

  /* KLUDGE: We want to disallow an empty naked_data_def. The above already
  ensures that at type or storage duration is specified. Now we ensure that
  either id or initializer is specified. */
  | opt_id initializer_special_arg                                                            { $$ = new RawAstDataDef(genParserExt.errorHandler(), $1, $2     , nullptr, genParserExt.mkDefaultType(@1), genParserExt.mkDefaultStorageDuration(), @1); }
  | ID                                                                                        { $$ = new RawAstDataDef(genParserExt.errorHandler(), $1, nullptr, nullptr, genParserExt.mkDefaultType(@1), genParserExt.mkDefaultStorageDuration(), @1); }
  ;

naked_fun_def
  : opt_id opt_fun_signature_arg equal_as_sep block                  { $$ = genParserExt.mkFunDef($1, ($2).m_paramDefs, ($2).m_retType, $4, @$); }
  ;

fun_signature_arg
  : COLON                              opt_type                      { ($$).m_paramDefs = new std::vector<AstDataDef*>(); ($$).m_retType = $2; }
  | opt_colon LPAREN param_list RPAREN opt_type                      { ($$).m_paramDefs = $3;                             ($$).m_retType = $5; }
  ;

opt_fun_signature_arg                           
  : %empty                                                           { ($$).m_paramDefs = new std::vector<AstDataDef*>(); ($$).m_retType = genParserExt.mkDefaultType(@$); }
  | fun_signature_arg                                                { swap($$,$1); }
  ;

param_list
  : %empty                                                           { $$ = new std::vector<AstDataDef*>(); }
  | pure_param_list opt_comma                                        { swap($$,$1); }
  ;

pure_param_list
  : param_def                                                        { $$ = new std::vector<AstDataDef*>(); ($$)->push_back($1); }
  | pure_param_list COMMA param_def                                  { ($1)->push_back($3); std::swap($$,$1); }
  ;

param_def
  : opt_valvar naked_data_def                                        { $$ = genParserExt.mkDataDef($1, $2, @$); }
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

TODO: semantic analizer must report error/warning when noinit is used in return.

1) If that is an parenthesized expression, say "val a = (42)$", then the
   resulting initializer in the AST is two nested AstSeq, i.e. (;;42) (the
   complete AST is ":;data(a int (;;42))"). One is because ct_list_arg is
   ultimatively an node_seq, and one is because here "(...)" is a primary_expr,
   whose content is again a node_seq. */
naked_initializer
  /* note that "LPAREN RPAREN" is already handled within ct_list_arg by "LPAREN RPAREN" being a primary_expr*/
  : LPAREN ct_list_2plus RPAREN                                      { swap($$,$2); }
  | LPAREN NOINIT        RPAREN                                      { $$ = new AstCtList(@$, AstDataDef::noInit); }
  | ct_list_arg /* 1) */                                             { auto objs = new std::vector<AstObject*>(); objs->push_back($1); $$ = new AstCtList(objs, @$); }
  | NOINIT                                                           { $$ = new AstCtList(@$, AstDataDef::noInit); }
  ;

initializer_special_arg
  : initializer_arg                                                  { swap($$,$1); }
  | lparen_as_sep ct_list RPAREN                                     { swap($$,$2); }
  | lparen_as_sep NOINIT  RPAREN                                     { $$ = new AstCtList(@$, AstDataDef::noInit); }
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
  : %empty                                                           { $$ = genParserExt.mkDefaultObjectTypeQualifier(); }
  | valvar                                                           { swap($$,$1); }
  ; 

valvar_lparen
  : VAL_LPAREN                                                       { $$ = ObjType::eNoQualifier; }
  | VAR_LPAREN                                                       { $$ = ObjType::eMutable; }
  ;

naked_if
  : condition_action_pair_then opt_else                              { $$ = new AstBlock(new AstIf(($1).m_condition, ($1).m_action, $2, @1), @1); }
  | condition_action_pair_then elif_chain                            { $$ = new AstBlock(new AstIf(($1).m_condition, ($1).m_action, $2, @1), @1); }
  ;

elif_chain
  : ELIF condition_action_pair_then opt_else                         { $$ = new AstBlock(new AstIf(($2).m_condition, ($2).m_action, $3, @1), @1); }
  | ELIF condition_action_pair_then elif_chain                       { $$ = new AstBlock(new AstIf(($2).m_condition, ($2).m_action, $3, @1), @1); }
  ;

opt_else
  : %empty                                                           { $$ = nullptr; }
  | else_sep block                                                   { $$ = $2; }
  ;

naked_while
  /* not ct_list_arg because do_sep allows newline as sep, which in ct_list_arg
  are seq_operator */
  : standalone_expr do_sep block                                     { $$ = new AstBlock(new AstLoop($1, $3, @1), @1); }
  ;

condition_action_pair_then
  /* see naked_while why it's standalone_expr and not  seq_operator */
  : standalone_expr then_sep block                                   { $$ = ConditionActionPair{ $1, $3}; }
  ;

opt_nl
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

void yy::GenParser::error(const location_type& loc, const std::string& msg)
{
  Error::throwError(genParserExt.errorHandler(), Error::eScanOrParseFailed);
}

// See declaration at the top of this file
yy::GenParser::symbol_type yylex(TokenStream& tokenStream) {
  return tokenStream.pop();
}
