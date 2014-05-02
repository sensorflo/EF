/* Bison declaration section
----------------------------------------------------------------------*/
%skeleton "lalr1.cc"
%require "3.0.2"
%defines
%define parser_class_name {Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires
{
  class Driver;
}

%param { Driver& driver }

%locations
%initial-action
{
  @$.begin.filename = @$.end.filename = &driver.m_fileName;
};

/*%define parse.trace*/
%define parse.error verbose

%code
{
  #include "../driver.h"
}

%define api.token.prefix {TOK_}
%token
  END_OF_FILE  0  "end of file"
  IF "if"
  ELSE "else"
  ID "identifier"
;

/* Grammar rules section
----------------------------------------------------------------------*/
%%

%start program;

program
  : END_OF_FILE
  ;

/* Epilogue section
----------------------------------------------------------------------*/
%%

void yy::Parser::error(const location_type& loc, const std::string& msg)
{
  driver.error(loc, msg);
}
