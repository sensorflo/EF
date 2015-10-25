#pragma once

// Note: No trailing semicolon so caller is forced to put a semicolon behind
// the macro call.
#define NEITHER_COPY_NOR_MOVEABLE(name)          \
  name(const name&) = delete;                   \
  name(name&&) = delete;                        \
  const name& operator=(const name&) = delete;  \
  const name& operator=(name&&) = delete
