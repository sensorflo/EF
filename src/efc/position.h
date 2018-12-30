#pragma once

#include <algorithm>
#include <iostream>
#include <string>

/** A position in the EF source file. */
class Position {
public:
  explicit Position(
    std::string* f = nullptr, unsigned int l = 1u, unsigned int c = 1u)
    : m_fileName{f}, m_line{l}, m_column{c} {}

  void initialize(
    std::string* fn = nullptr, unsigned int l = 1u, unsigned int c = 1u) {
    m_fileName = fn;
    m_line = l;
    m_column = c;
  }

  /** (line related) Advance to the COUNT next lines. */
  void lines(int count = 1) {
    if (count) {
      m_column = 1u;
      m_line = add_(m_line, count, 1);
    }
  }

  /** (column related) Advance to the COUNT next columns. */
  void columns(int count = 1) { m_column = add_(m_column, count, 1); }

  /** File name to which this position refers. */
  std::string* m_fileName;
  /** Current line number. */
  unsigned int m_line;
  /** Current column number. */
  unsigned int m_column;

private:
  /** Compute max(min, lhs+rhs) (provided min <= lhs). */
  static unsigned int add_(unsigned int lhs, int rhs, unsigned int min) {
    return (0 < rhs || -static_cast<unsigned int>(rhs) < lhs ? rhs + lhs : min);
  }
};

/** Add \a width columns, in place. */
inline Position& operator+=(Position& res, int width) {
  res.columns(width);
  return res;
}

/** Add \a width columns. */
inline Position operator+(Position res, int width) {
  return res += width;
}

/** Subtract \a width columns, in place. */
inline Position& operator-=(Position& res, int width) {
  return res += -width;
}

/** Subtract \a width columns. */
inline Position operator-(Position res, int width) {
  return res -= width;
}

inline bool operator==(const Position& pos1, const Position& pos2) {
  return (pos1.m_line == pos2.m_line && pos1.m_column == pos2.m_column &&
    (pos1.m_fileName == pos2.m_fileName ||
      (pos1.m_fileName && pos2.m_fileName &&
        *pos1.m_fileName == *pos2.m_fileName)));
}

inline bool operator!=(const Position& pos1, const Position& pos2) {
  return !(pos1 == pos2);
}

template<typename YYChar>
inline std::basic_ostream<YYChar>& operator<<(
  std::basic_ostream<YYChar>& ostr, const Position& pos) {
  if (pos.m_fileName) ostr << *pos.m_fileName << ':';
  return ostr << pos.m_line << '.' << pos.m_column;
}
