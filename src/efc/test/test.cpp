#include "test.h"
using namespace std;
using namespace testing;

string amendSpec(const string& spec) {
  string amendedSpec;
  if (spec.empty()) {
    /*nop*/
  } else {
    amendedSpec += "\nsub-specification or example description:\n";
    amendedSpec += spec;
    if ( spec[spec.length()-1]!='\n' ) {
      amendedSpec += '\n';
    }
  }
  return amendedSpec;
}

