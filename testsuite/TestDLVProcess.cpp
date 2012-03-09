#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/DLVProcess.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "TestDLVProcess"
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>


DLVHEX_NAMESPACE_USE

class SimpleEvaluator
{
public:
  SimpleEvaluator(const std::string& in)
    : input(in)
  { }

  void
  solve()
  {
    proc = new DLVProcess;
    proc->setPath(DLVPATH);
    proc->addOption("-silent");
    proc->addOption("-facts");
    proc->addOption("--");
    proc->spawn();

    std::ostream& programStream = proc->getOutput();
    programStream << input;
    programStream.flush();
    proc->endoffile();

    std::cerr << "input = " << std::endl << input << std::endl;

    std::istream& is = proc->getInput();

    std::cerr << "Getting results: " << std::endl;
    std::size_t count = 0;

    std::cerr << "Ans " << ++count;
    std::string res;
    std::getline(is, res);
    std::cerr << ":  ";
    if (res.empty())
      {
	std::cerr << "Break now! res.empty()" << std::endl;
	//break;
      }
    else if (is.bad())
      {
	std::cerr << "Break now! is.bad()" << std::endl;
	//break;
      }
    std::cerr << res << std::endl;
    
    std::cerr << std::endl;
    delete proc;
    proc = 0;
  }

private:
  DLVProcess* proc;
  std::string input;
};

BOOST_AUTO_TEST_CASE ( testDLVProcess )
{
  std::string line1 = "a v b :- c.\n";
  std::string line2 = "c v d :- e.\n";
  std::string line3 = "e v f :- g.\n";
  std::string line4 = "g v h :- i.\n";
  std::string line5 = "i v j.\n";

  std::string program = "";
  program = program + line1 + line2 + line3 + line4 + line5;

  SimpleEvaluator se(program);

  se.solve();
  se.solve();
  se.solve();
  se.solve();
  se.solve();
  se.solve();
  se.solve();
  se.solve();
}

// Local Variables:
// mode: C++
// End:
