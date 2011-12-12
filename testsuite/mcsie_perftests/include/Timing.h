#ifndef _DLVHEX_MCSDIAGEXPL_TIMING_H_
#define _DLVHEX_MCSDIAGEXPL_TIMING_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <ostream>
#include <sstream>
#include <map>

namespace dlvhex {
  namespace mcsdiagexpl {

	using namespace boost::posix_time;
	using namespace boost::gregorian;

    class Timing {

      public:
	static Timing* getInstance();
	bool begin();
	bool start(int id);
	bool stop(int id);
	bool end();
	bool stopPostProc();
	void activate();
	bool isActive();
	time_duration getFullPrgDuration() const;
	std::ostream& getAccOutput(std::ostream&) const;

      private:
	static Timing *t;
	Timing();
	Timing( const Timing& );

	bool started, activated;
        ptime prg_start, prg_end, post_end;
        time_duration full_acc_duration, null_duration;

	struct acc_time_info {
		ptime start;
		int count;
		time_duration duration;
	};

	std::map<int, acc_time_info> acc_info_map;
    }; // END class Timing


    static std::ostream&
    operator<< (std::ostream& out, const Timing& t){
	std::string dString;

	out << std::endl;
	out << " --------------------------------------------------------------------- " << std::endl;
    	out << "| time/call summary of Diagnosis and Explanation calculation          |" << std::endl;
	out << "| for Multi Context Systems                                           |" << std::endl;
	out << " ===================================================================== " << std::endl;
	out << "| Context        | Total Time in   | Average Time in | Number of ACC  |" << std::endl;
	out << "| ID             | ACC Function    | ACC Function    | Function calls |" << std::endl;
	out << "|----------------|-----------------|-----------------|----------------|" << std::endl;

	t.getAccOutput(out);
	//end loop for each context


	dString = boost::posix_time::to_simple_string(t.getFullPrgDuration());
	out << dString;
	for (int i = dString.size(); i<15; out << " ", i++);
	out << "                  |" << std::endl;
	out << " --------------------------------------------------------------------- " << std::endl;
	out << std::endl;

    	return out;
    };
    
  }  // END namespace mcsdiagexpl
} // END namespace dlvhex

#endif /*_DLVHEX_MCSDIAGEXPL_TIMING_H_*/
// vim:ts=8:
