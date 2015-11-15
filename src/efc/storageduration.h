#pragma once

enum class StorageDuration {
  /** For example for objects pointed to by pointers. Never used for declared
  (whether implicitely or explicitely) objects. */
  eUnknown,
  /** Also applies to abstract objects which are not really stored. Also
  applies to objects which are optimized to live only as SSA value */
  eLocal,
  eStatic,
  eMember
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  StorageDuration sd);

