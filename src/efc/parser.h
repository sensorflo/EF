#pragma once

#include "gensrc/genparser.hpp"

namespace yy {

class Parser : public yy::GenParser {
public:
  Parser(TokenStream& tokenStream_yyarg, Driver& driver_yyarg,
    ParserExt& parserExt_yyarg, std::unique_ptr<AstNode>& astRoot_yyarg)
    : yy::GenParser{
        tokenStream_yyarg, driver_yyarg, parserExt_yyarg, astRoot_yyarg} {}
};
}
