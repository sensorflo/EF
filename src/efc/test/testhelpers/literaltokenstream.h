#pragma once
#include "../tokenstream.h"
#include <vector>
#include <memory>

class LiteralTokenStream : public TokenStream {
public:
  LiteralTokenStream(const std::vector<yy::Parser::token_type>& tokens);
  yy::Parser::symbol_type& front() override;
  yy::Parser::symbol_type pop() override;

private:
  size_t m_front;
  const size_t m_size;
  std::unique_ptr<yy::Parser::symbol_type[]> const m_stream;
};

