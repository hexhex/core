/**
 * N-bomb defusing, see bomb_defusing.txt
 */

// include above because we must link against this
#include <boost/program_options.hpp>

#define _GLIBCXX_DEBUG // safe iterators where possible (where not already included above)

#include "dlvhex/Logger.hpp"

#include <sstream>
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <sys/time.h>

namespace po = boost::program_options;

struct Config
{
  unsigned n, l;
};

class RandomNumbers
{
public:
  RandomNumbers(unsigned seed=0)
  {
    if( seed == 0 )
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      srand(tv.tv_sec + tv.tv_usec);
    }
    else
    {
      srand(seed);
    }
  }

  unsigned getInRange(unsigned lowest, unsigned highest)
  {
    assert(lowest <= highest);
    return lowest + rand() % (highest + 1 - lowest);
  }

  bool getBool()
  {
    return rand() % 2 == 0;
  }
};

template<typename Range>
void randomizeRange(RandomNumbers& rn, Range& l)
{
  std::set<typename Range::value_type> tmp;
  tmp.insert(l.begin(), l.end());
  unsigned size = l.size();
  l.clear();
  // now l is empty

  typename std::set<typename Range::value_type>::const_iterator it;
  while(!tmp.empty())
  {
    // select random element
    unsigned step = rn.getInRange(0,size-1);
    it = tmp.begin();
    for(unsigned u = 0; u < step; ++u)
    {
      assert(it != tmp.end());
      it++;
    }
    assert(it != tmp.end());

    // take out this element and push_back into l
    l.push_back(*it);
    tmp.erase(it);
    size--;
  }
}

int main(int ac,char** av)
{
  try
  {

  Config config;

  po::options_description desc("program options");
  unsigned seed;
  desc.add_options()
    ("help", "produce help message")
    ("seed", po::value<unsigned>(&seed)->default_value(0),
      "random seed")
    ("bombs,n", po::value<unsigned>(&config.n)->required(),
      "number of bombs")
    ("maxlength,l", po::value<unsigned>(&config.l)->required(),
      "length of plan")
  ;
  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);

  if( !vm["help"].empty() )
  {
    std::cerr << desc << std::endl;
    return -1;
  }

  po::notify(vm);

  RandomNumbers random;

  std::ostream& o = std::cout;

  // final timepoint = l
  const unsigned finalt = config.l;

  // create timepoints (l+1)
  for(unsigned t = 0; t <= finalt; ++t)
  {
    o << "timepoint(" << t << ")." << std::endl;
    // create time ordering
    if( t != 0 )
      o << "succ(" << (t-1) << "," << (t) << ")." << std::endl;
  }

  for(unsigned b = 0; b < config.n; ++b)
  {
    // create bomb
    o << "bomb(" << b << ")." << std::endl;

    // create initial situation
    if( random.getBool() )
      o << "disarmed" << b << "(0)." << std::endl;

    // guess actions
    o << "toggleArmed" << b << "(T) v look" << b << "(T) v nop" << b << "(T) :- succ(T,X)." << std::endl;

    // cause exploded after disarmed and toggle
    o << "exploded" << b << "(TS) :- toggleArmed" << b << "(T), disarmed" << b << "(T), succ(T,TS)." << std::endl;

    // cause disarmed after not disarmed and toggle
    o << "disarmed" << b << "(TS) :- toggleArmed" << b << "(T), not disarmed" << b << "(T), succ(T,TS)." << std::endl;

    // inertia for exploded
    o << "exploded" << b << "(TS) :- not toggleArmed" << b << "(T), exploded" << b << "(T), succ(T,TS)." << std::endl;

    // inertia for disarmed
    o << "disarmed" << b << "(TS) :- not toggleArmed" << b << "(T), disarmed" << b << "(T), succ(T,TS)." << std::endl;

    // inertia for knowThatNotArmed
    o << "knowThatNotArmed" << b << "(TS) :- not toggleArmed" << b << "(T), knowThatNotArmed" << b << "(T), succ(T,TS)." << std::endl;

    // agent does not disarm if it knows that bomb is not armed
    o << ":- toggleArmed" << b << "(TS), knowThatNotArmed" << b << "(T), succ(T,TS)." << std::endl;

    // agent learns knowThatNotArmedb(TS) if lookb(T) and &senseNotArmed2[disarmedB,lookB,T]()
    // (external atom is true iff disarmedB(T) and lookB(T))
    o << "knowThatNotArmed" << b << "(TS) :- &senseNotArmed2[disarmed" << b << ",look" << b << ",T](), look" << b << "(T), succ(T,TS)." << std::endl;

    // for nonconcurrency
    //o << "lookaction(T) :- look" << b << "(T)." << std::endl;
    //o << "toggleaction(T) :- toggleArmed" << b << "(T)." << std::endl;

    // no concurrent look
    //for(unsigned b2 = b+1; b2 < config.n; ++b2)
    //  o << ":- look" << b << "(T), look" << b2 <<"(T)." << std::endl;
    // no concurrent toggle
    //for(unsigned b2 = b+1; b2 < config.n; ++b2)
    //  o << ":- toggleArmed" << b << "(T), toggleArmed" << b2 << "(T)." << std::endl;
    
    // dead if exploded
    o << "dead(X) :- exploded" << b << "(X)." << std::endl;
    // armed if something is not disarmed
    o << "armed(X) :- not disarmed" << b << "(X), timepoint(X)." << std::endl;

    // simulate constraint duplication by duplicating goals below:
    // goal = forbidden: dead from here at any timepoint
    o << ":- exploded" << b << "(X), timepoint(T)." << std::endl;
    // goal = forbidden: armed here at the end
    o << ":- not disarmed" << b << "(" << finalt << ")." << std::endl;
  }

  // goal = forbidden: dead at any timepoint
  o << ":- dead(T), timepoint(T)." << std::endl;

  // goal = forbidden: armed at the end
  o << ":- armed(" << finalt << ")." << std::endl;

  // no concurrent look and toggle
  //o << ":- lookaction(T), toggleaction(T)." << std::endl;

  //o << "equal(X,X) :- bomb(X)." << std::endl;
  //o << "equal(X,X) :- timepoint(X)." << std::endl;

  return 0;

  }
  catch(const std::exception& e)
  {
    std::cerr << "exception: " << e.what() << std::endl;
    return -1;
  }
}
