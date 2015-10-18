#include "test.h"
#include "../object.h"
using namespace testing;
using namespace std;

TEST(ObjectTest, MAKE_TEST_NAME1(
    toStr)) {
  auto ot = make_shared<ObjTypeFunda>(ObjTypeFunda::eInt);
  EXPECT_EQ( Object(ot, StorageDuration::eLocal).toStr(), "int" );
  EXPECT_EQ( Object(ot, StorageDuration::eStatic).toStr(), "static/int" );
}
