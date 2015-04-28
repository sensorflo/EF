#pragma once

#include "gensrc/parser.hpp"

class Driver;

#define YY_DECL yy::Parser::symbol_type yylex(Driver& driver)
YY_DECL;
