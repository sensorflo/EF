#include "test.h"
#include "driverontmpfile.h"
#include "../driver.h"
#include "../errorhandler.h"
#include "../ast.h"
#include <memory>
using namespace testing;
using namespace std;

void testSaTransAndIrBuildReportsError(TestingDriver& UUT, AstObject* astRoot,
  Error::No expectedErrorNo, const string& spec = "") {
  // setup
  ENV_ASSERT_TRUE(astRoot != nullptr);
  unique_ptr<AstObject> dummy(astRoot);
  bool foreignExceptionThrown = false;
  string excptionwhat;

  // execute
  try {
    UUT.doSemanticAnalysis(*astRoot);
    UUT.generateIr(*astRoot);
  }

  // verify
  catch (BuildError&) {
    // nop
  }
  catch (exception& e) {
    foreignExceptionThrown = true;
    excptionwhat = e.what();
  }
  catch (exception* e) {
    foreignExceptionThrown = true;
    if (e) { excptionwhat = e->what(); }
  }
  catch (...) {
    foreignExceptionThrown = true;
  }
  EXPECT_FALSE(foreignExceptionThrown)
    << "Expecting either no exceptions or Error exception\n"
    << amendSpec(spec) << amendAst(astRoot) << amend(*UUT.m_errorHandler)
    << "\nexceptionwhat: " << excptionwhat;
  const ErrorHandler::Container& errors = UUT.m_errorHandler->errors();
  EXPECT_EQ(1U, errors.size())
    << "Expecting exactly one error\n"
    << amendSpec(spec) << amend(*UUT.m_errorHandler) << amendAst(astRoot);
  if (!errors.empty()) {
    EXPECT_EQ(expectedErrorNo, errors.front()->no())
      << amendSpec(spec) << amend(*UUT.m_errorHandler) << amendAst(astRoot)
      << amend(*UUT.m_env);
  }
}

#define TEST_SATRANSANDIRBUILD_REPORTS_ERROR(astRoot, expectedErrorNo, spec) \
  {                                                                          \
    SCOPED_TRACE(                                                            \
      "testSaTransAndIrBuildSucceeds called from here (via "                 \
      "TEST_SATRANSANDIRBUILD_SUCCEEDS)");                                   \
    TestingDriver UUT;                                                       \
    testSaTransAndIrBuildReportsError(UUT, astRoot, expectedErrorNo, spec);  \
  }

TEST(DriverSystemTest, MAKE_TEST_NAME(
    a,
    b,
    c)) {
  string spec = "Assigning to literal";
  TEST_SATRANSANDIRBUILD_REPORTS_ERROR(
    new AstOperator('=', new AstNumber(42), new AstNumber(77)),
    Error::eWriteToImmutable, spec);
}

TEST(DriverSystemTest, MAKE_TEST_NAME(
    a2,
    b,
    c)) {
  string spec = "Assigning to literal";
  TEST_SATRANSANDIRBUILD_REPORTS_ERROR(
    new AstOperator('=', new AstNumber(42), new AstNumber(77)),
    Error::eWriteToImmutable, spec);
}

TEST(DriverSystemTest, MAKE_TEST_NAME(
    an_well_formed_EF_program,
    compile,
    writes_nothing_to_its_error_ostream)) {
  // setup
  string well_formed_ef_program = "42";
  stringstream errorMsgFromDriver;
  DriverOnTmpFile driverOnTmpFile(well_formed_ef_program, &errorMsgFromDriver);
  TestingDriver& UUT = driverOnTmpFile;

  // execute
  UUT.compile();

  // verify
  EXPECT_EQ(0U, errorMsgFromDriver.str().length())
    << "\n"
    << "errorMsgFromDriver: \"" << errorMsgFromDriver.str() << "\"\n"
    << "EF program: \"" << well_formed_ef_program << "\"\n"
    << amend(*UUT.m_env) << amendAst(UUT.m_astRootFromParser);
}

TEST(DriverSystemTest, MAKE_TEST_NAME(
    an_EF_program_containing_an_error,
    compile,
    writes_an_error_to_its_error_ostream)) {
  // setup
  string ef_program_with_error = "42 = 77";
  stringstream errorMsgFromDriver;
  DriverOnTmpFile driverOnTmpFile(ef_program_with_error, &errorMsgFromDriver);
  Driver& UUT = driverOnTmpFile;

  // execute
  UUT.compile();

  // verify
  EXPECT_EQ(0U, errorMsgFromDriver.str().find("Build error(s):"))
    << "\n"
    << "errorMsgFromDriver: \"" << errorMsgFromDriver.str() << "\"\n"
    << "EF program: \"" << ef_program_with_error << "\"\n";
}
