#pragma once
#include "../tokenstream.h"

#include <memory>
#include <vector>

class LiteralTokenStream : public TokenStream {
public:
  LiteralTokenStream(const std::vector<Parser::token_type>& tokens);
  Parser::symbol_type pop() override;

private:
  size_t m_frontPos;
  const size_t m_size;
  std::unique_ptr<Parser::symbol_type[]> const m_stream;
};
