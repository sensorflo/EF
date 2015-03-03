#include "test.h"
#include "../driver.h"
#include "../errorhandler.h"
using namespace testing;
using namespace std;

class TestingDriver : public Driver {
public:
  TestingDriver() : Driver("") {};

  using Driver::m_errorHandler;
  using Driver::m_parserExt;
};

void testSaTransAndIrBuildReportsError(TestingDriver& UUT, AstValue* astRoot,
  Error::No expectedErrorNo, const string& spec = "") {

  // setup
  ENV_ASSERT_TRUE( astRoot!=NULL );
  auto_ptr<AstValue> astRootAp(astRoot);
  bool foreignExceptionThrown = false;
  string excptionwhat;

  // execute
  try {
    UUT.doSemanticAnalysisAndGenIR(astRoot);
  }

  // verify
  catch (BuildError& buildError) {
    // nop
  }
  catch (exception& e) {
    foreignExceptionThrown = true;
    excptionwhat = e.what();
  }
  catch (exception* e) {
    foreignExceptionThrown = true;
    if ( e ) {
      excptionwhat = e->what();
    }
  }
  catch (...) {
    foreignExceptionThrown = true;
  }
  EXPECT_FALSE(foreignExceptionThrown) <<
    "Expecting either no exceptions or BuildError exception\n" <<
    amendSpec(spec) << amendAst(astRoot) << amend(UUT.m_errorHandler) <<
    "\nexceptionwhat: " << excptionwhat;
  const ErrorHandler::Container& errors = UUT.m_errorHandler.errors();
  EXPECT_EQ(1, errors.size()) <<
    "Expecting exactly one error\n" << 
    amendSpec(spec) << amend(UUT.m_errorHandler) << amendAst(astRoot);
  EXPECT_EQ(expectedErrorNo, errors.front()->no()) <<
    amendSpec(spec) << amend(UUT.m_errorHandler) << amendAst(astRoot);
}

#define TEST_SATRANSANDIRBUILD_REPORTS_ERROR(astRoot, expectedErrorNo, spec) \
  {                                                                     \
    SCOPED_TRACE("testSaTransAndIrBuildSucceeds called from here (via TEST_SATRANSANDIRBUILD_SUCCEEDS)"); \
    TestingDriver UUT;                                                  \
    testSaTransAndIrBuildReportsError(UUT, astRoot, expectedErrorNo, spec); \
  }

TEST(DriverSystemTest, MAKE_TEST_NAME(
    a,
    b,
    c)) {
  string spec = "Assigning to literal";
  TEST_SATRANSANDIRBUILD_REPORTS_ERROR(
    new AstOperator('=',
      new AstNumber(42),
      new AstNumber(77)),
    Error::eWriteToReadOnly, spec);
}

TEST(DriverSystemTest, MAKE_TEST_NAME(
    a2,
    b,
    c)) {
  string spec = "Assigning to literal";
  TEST_SATRANSANDIRBUILD_REPORTS_ERROR(
    new AstOperator('=',
      new AstNumber(42),
      new AstNumber(77)),
    Error::eWriteToReadOnly, spec);
}


