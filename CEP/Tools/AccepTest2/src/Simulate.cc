#include <Common/LofarLogger.h>
#include <tinyCEP/SimulatorParseClass.h>
#include <AH_AccepTest.h>

using namespace LOFAR;

int main (int argc, const char** argv)
{
  //INIT_LOGGER("AccepTest2.properties");

  try {
    AH_AccepTest simulator;
    simulator.setarg (argc, argv);
    simulator.baseDefine();
    simulator.basePrerun();
    simulator.baseRun(1);
    //simulator.baseDump();
    simulator.baseQuit();

  } catch (...) {
    cout << "Unexpected exception" << endl;
  }
}
