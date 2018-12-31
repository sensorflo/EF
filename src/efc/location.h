#pragma once

#include "position.h"

/** A losition in the EF source file. */
class Location {
public:
  /** Initializes to a valid location pointing to the beginning of the
  input. I.e. does _not_ initialize to the 'null location'.  */
  Location() : Location{Position{}} {}
  Location(const Position& b, const Position& e) : begin{b}, end{e} {}

  explicit Location(const Position& p) : begin{p}, end{p} {}

  explicit Location(std::string* f, unsigned int l = 1u, unsigned int c = 1u)
    : begin{f, l, c}, end{f, l, c} {}

  void initialize(
    std::string* f = nullptr, unsigned int l = 1u, unsigned int c = 1u) {
    begin.initialize(f, l, c);
    end = begin;
  }

public:
  /** Reset initial location to final location. */
  void step() { begin = end; }

  /** Extend the current location to the COUNT next columns. */
  void columns(int count = 1) { end += count; }

  /** Extend the current location to the COUNT next lines. */
  void lines(int count = 1) { end.lines(count); }

  /** True if this location compares equal to s_nullLoc */
  bool isNull() const { return begin.isNull() || end.isNull(); }

public:
  /** Beginning of the located region. */
  Position begin;
  /** End of the located region. */
  Position end;
};

/** Join two locations, in place. */
inline Location& operator+=(Location& res, const Location& end) {
  res.end = end.end;
  return res;
}

/** Join two locations. */
inline Location operator+(Location res, const Location& end) {
  return res += end;
}

/** Add \a width columns to the end position, in place. */
inline Location& operator+=(Location& res, int width) {
  res.columns(width);
  return res;
}

/** Add \a width columns to the end position. */
inline Location operator+(Location res, int width) {
  return res += width;
}

/** Subtract \a width columns to the end position, in place. */
inline Location& operator-=(Location& res, int width) {
  return res += -width;
}

/** Subtract \a width columns to the end position. */
inline Location operator-(Location res, int width) {
  return res -= width;
}

inline bool operator==(const Location& loc1, const Location& loc2) {
  return loc1.begin == loc2.begin && loc1.end == loc2.end;
}

inline bool operator!=(const Location& loc1, const Location& loc2) {
  return !(loc1 == loc2);
}

template<typename YYChar>
inline std::basic_ostream<YYChar>& operator<<(
  std::basic_ostream<YYChar>& ostr, const Location& loc) {
  unsigned int end_col = 0 < loc.end.m_column ? loc.end.m_column - 1 : 0;
  ostr << loc.begin;
  if (loc.end.m_fileName &&
    (!loc.begin.m_fileName || *loc.begin.m_fileName != *loc.end.m_fileName))
    ostr << '-' << loc.end.m_fileName << ':' << loc.end.m_line << '.'
         << end_col;
  else if (loc.begin.m_line < loc.end.m_line)
    ostr << '-' << loc.end.m_line << '.' << end_col;
  else if (loc.begin.m_column < end_col)
    ostr << '-' << end_col;
  return ostr;
}

extern const Location s_nullLoc;
