#pragma once
#include "../parser.h"

#include <string>
#include <vector>

#define EXPECT_TOK_EQ(expectedTokenWONamespace, actualSymbol) \
  EXPECT_EQ(Parser::token::expectedTokenWONamespace, actualSymbol.token())

// token initializer list
// clang-format off
#define TOKIL0() {}
#define TOKIL1(tok1) { Parser::token::tok1 }
#define TOKIL2(tok1, tok2) { Parser::token::tok1, Parser::token::tok2 }
#define TOKIL3(tok1, tok2, tok3) { Parser::token::tok1, Parser::token::tok2, Parser::token::tok3 }
#define TOKIL4(tok1, tok2, tok3, tok4) { Parser::token::tok1, Parser::token::tok2, Parser::token::tok3, Parser::token::tok4 }
#define TOKIL5(tok1, tok2, tok3, tok4, tok5) { Parser::token::tok1, Parser::token::tok2, Parser::token::tok3, Parser::token::tok4, Parser::token::tok5 }
// clang-format on

std::string amend(const std::vector<Parser::token_type>& inputTokens,
  const std::vector<Parser::token_type>& expectedTokens,
  int currentExpectedPos);
