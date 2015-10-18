#include "test.h"
#include "../symboltableentry.h"
using namespace testing;
using namespace std;

TEST(ObjectTest, MAKE_TEST_NAME1(
    toStr)) {
  auto ot = make_shared<ObjTypeFunda>(ObjTypeFunda::eInt);
  EXPECT_EQ( SymbolTableEntry(ot, StorageDuration::eLocal).toStr(), "int" );
  EXPECT_EQ( SymbolTableEntry(ot, StorageDuration::eStatic).toStr(), "static/int" );
}
