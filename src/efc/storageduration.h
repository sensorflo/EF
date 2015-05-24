#pragma once

enum class StorageDuration {
  eUnknown,
  /** Also applies to abstract objects which are not really stored. Also
  applies to objects which are optimized to live only as SSA value */
  eLocal,
  eStatic
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  StorageDuration sd);

