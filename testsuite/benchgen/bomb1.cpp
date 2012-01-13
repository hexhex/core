/**
 * N-bomb defusing, see bomb_defusing.txt
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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
  unsigned n;
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

  const unsigned finalt = 2*config.n;

  for(unsigned b = 0; b < config.n; ++b)
  {
    // create 2 timepoints
    o << "timepoint(" << (2*b) << ")." << std::endl;
    o << "timepoint(" << (2*b+1) << ")." << std::endl;
    // create bomb
    o << "bomb(" << b << ")." << std::endl;
    // create time ordering
    if( b != 0 )
      o << "succ(" << (2*b-1) << "," << (2*b) << ")." << std::endl;
    o << "succ(" << (2*b) << "," << (2*b+1) << ")." << std::endl;
    // create initial situation
    if( random.getBool() )
      o << "disarmed(" << b << ",0)." << std::endl;

    // guess actions
    o << "toggleArmed(" << b << ",T) v look(" << b << ",T) v nop(" << b << ",T) :- succ(T,_)." << std::endl;

    // cause exploded after disarmed and toggle
    o << "exploded(" << b << ",TS) :- toggleArmed(" << b << ",T), disarmed(" << b << ",T), succ(T,TS)." << std::endl;

    // cause disarmed after not disarmed and toggle
    o << "disarmed(" << b << ",TS) :- toggleArmed(" << b << ",T), not disarmed(" << b << ",T), succ(T,TS)." << std::endl;

    // inertia for exploded
    o << "exploded(" << b << ",TS) :- not toggleArmed(" << b << ",T), exploded(" << b << ",T), succ(T,TS)." << std::endl;

    // inertia for disarmed
    o << "disarmed(" << b << ",TS) :- not toggleArmed(" << b << ",T), disarmed(" << b << ",T), succ(T,TS)." << std::endl;

    // inertia for knowThatNotArmed
    o << "knowThatNotArmed(" << b << ",TS) :- not toggleArmed(" << b << ",T), knowThatNotArmed(" << b << ",T), succ(T,TS)." << std::endl;

    // agent does not disarm if it knows that bomb is not armed
    o << ":- toggleArmed(" << b << ",TS), knowThatNotArmed(" << b << ",T), succ(T,TS)." << std::endl;

    // agent learns knowThatNotArmed(b,TS) if look(b,T) and &senseNotArmed1[disarmed,look,B,T]()
    // (external atom is true iff disarmed(B,T) and look(B,T))
    o << "knowThatNotArmed(" << b << ",TS) :- &senseNotArmed1[disarmed,look," << b << ",T](), look(" << b << ",T), succ(T,TS)." << std::endl;
    
    // goal: not exploded
    o << ":- exploded(" << b << ",_)." << std::endl;
    // goal: disarmed
    o << ":- not disarmed(" << b << "," << finalt << ")." << std::endl;
  }
  // final timepoint
  o << "timepoint(" << finalt << ")." << std::endl;
  o << "succ(" << (finalt-1) << "," << finalt << ")." << std::endl;

  // no concurrent look and toggle
  o << "lookaction(T) :- look(_,T)." << std::endl;
  o << "toggleaction(T) :- toggleArmed(_,T)." << std::endl;
  o << ":- lookaction(T), toggleaction(T)." << std::endl;

  o << "equal(X,X) :- bomb(X)." << std::endl;
  o << "equal(X,X) :- timepoint(X)." << std::endl;

  // no concurrent look
  o << ":- look(B1,T1), look(B2,T2), equal(T1,T2), not equal(B1,B2)." << std::endl;
  // no concurrent toggle
  o << ":- toggleArmed(B1,T1), toggleArmed(B2,T2), equal(T1,T2), not equal(B1,B2)." << std::endl;

  return 0;

  }
  catch(const std::exception& e)
  {
    std::cerr << "exception: " << e.what() << std::endl;
    return -1;
  }
}
