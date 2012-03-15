#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

// include above because we must link against this
#include <boost/program_options.hpp>

#define _GLIBCXX_DEBUG // safe iterators where possible (where not already included above)

#include "dlvhex2/Logger.h"

#include <boost/foreach.hpp>

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
  unsigned tracks, papers, referees;
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

std::string inPred(unsigned tower)
{
  std::stringstream s;
  s << "in" << tower;
  return s.str();
}

std::string in(unsigned tower, const std::string& sym)
{
  std::stringstream s;
  s << inPred(tower) << "(" << sym << ")";
  return s.str();
}

typedef std::set<std::string> StringSet;
typedef std::list<std::string> StringList;
typedef std::vector<std::string> StringVector;

void genSyms(const std::string& prefix, unsigned count, StringVector& into)
{
  for(unsigned u = 0; u < count; ++u)
  {
    std::stringstream s;
    s << prefix << u;
    into.push_back(s.str());
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
    ("tracks,t", po::value<unsigned>(&config.tracks)->required(),
      "number of conference tracks")
    ("papers,p", po::value<unsigned>(&config.papers)->required(),
      "number of papers=referees per track")
  ;
  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);

  if( !vm["help"].empty() )
  {
    std::cerr << desc << std::endl;
    return -1;
  }

  po::notify(vm);

  config.referees = config.papers;

  RandomNumbers random;

  std::ostream& o = std::cout;

  StringVector tracksyms;
  StringVector papersyms;
  StringVector refereesyms;
  {
    // create symbols (shared across strata)
    genSyms("track", config.tracks, tracksyms);
    genSyms("paper", config.tracks*config.papers, papersyms);
    genSyms("ref", config.tracks*config.referees, refereesyms);

    for(unsigned p = 0; p < config.tracks*config.papers; ++p)
    {
      o << "paper(" << papersyms[p] << ")." << std::endl;
    }
    for(unsigned r = 0; r < config.tracks*config.referees; ++r)
    {
      o << "referee(" << refereesyms[r] << ")." << std::endl;
    }
    for(unsigned t = 0; t < config.tracks; ++t)
    {
      o << "track(" << tracksyms[t] << ")." << std::endl;
      for(unsigned p = 0; p < config.papers; ++p)
      {
        o << "track_paper(" << tracksyms[t] << "," << papersyms[t*config.papers+p] << ")." << std::endl;
      }
      for(unsigned r = 0; r < config.referees; ++r)
      {
        o << "track_referee(" << tracksyms[t] << "," << refereesyms[t*config.referees+r] << ")." << std::endl;
      }
    }
  }

  // create conflicts
  {
    for(unsigned t = 0; t < config.tracks; ++t)
    {
      // build conflicts for one referee
      // a referee conflicts with all papers except
      // * the paper with the same number as the referee
      // * the paper with the next higher number as the referee (modulo number of papers)
      //
      // the following conflicts are special (external)
      // * for referee 1: paper 0 (this does not give more solutions yet)
      // * for the last referee: paper 2 (this conflict permits one additional solution)
      // (this way we have exactly two solutions)
      for(unsigned r = 0; r < config.referees; ++r)
      {
        std::set<unsigned> conflict;
        // create all conflicts except for two papers
        for(unsigned c = 0; c < config.papers; ++c)
        {
          if( c != r && c != ((r+1)%config.papers) )
          {
            if( (r == 1 && c == 0) ||
                (r == (config.referees-1) && c == 2) )
            {
              // this is an external conflict
              o << "conflict(" << papersyms[t*config.papers+c] << "," << refereesyms[t*config.referees+r] << ")." << std::endl;
            }
            else
            {
              // this is an internal conflict
              o << "iconflict(" << papersyms[t*config.papers+c] << "," << refereesyms[t*config.referees+r] << ")." << std::endl;
            }
          }
        }
      }
    }
  }

  // create rules
  {
    for(unsigned t = 0; t < config.tracks; ++t)
    {
      o << "assign(" << tracksyms[t] << ",P,R) v nassign(" << tracksyms[t] << ",P,R) :- track_paper(" << tracksyms[t] << ",P), track_referee(" << tracksyms[t] << ",R)." << std::endl;

      // no more than 2 assignments per paper
      o << ":- assign(" << tracksyms[t] << ",P,R1), assign(" << tracksyms[t] << ",P,R2), assign(" << tracksyms[t] << ",P,R3), R1 != R2, R1 != R3, R2 != R3." << std::endl;
      // no less than 2 assignments per paper
      o << "ok(" << tracksyms[t] << ",P) :- assign(" << tracksyms[t] << ",P,R1), assign(" << tracksyms[t] << ",P,R2), R1 != R2." << std::endl;
      o << ":- not ok(" << tracksyms[t] << ",P), track_paper(" << tracksyms[t] << ",P)." << std::endl;

      // no more than 2 assignments per reviewer (local)
      o << ":- assign(" << tracksyms[t] << ",P1,R), assign(" << tracksyms[t] << ",P2,R), assign(" << tracksyms[t] << ",P3,R), P1 != P2, P1 != P3, P2 != P3." << std::endl;

      // conflicts (local)
      o << ":- assign(" << tracksyms[t] << ",P,R), iconflict(P,R)." << std::endl;
      o << ":- assign(" << tracksyms[t] << ",P,R), conflict(P,R). % REMOVEFORHEX" << std::endl;
      o << ":- assign(" << tracksyms[t] << ",P,R), &gen2[conflict,P,R](). % ONLYFORHEX" << std::endl;
    }

    // no more than 2 assignments per reviewer (global)
    o << ":- assign(T,P1,R), assign(T,P2,R), assign(T,P3,R), P1 != P2, P1 != P3, P2 != P3." << std::endl;

    // conflicts (global)
    o << ":- assign(T,P,R), iconflict(P,R)." << std::endl;
    o << ":- assign(T,P,R), conflict(P,R). % REMOVEFORHEX" << std::endl;
    o << ":- assign(T,P,R), &gen2[conflict,P,R](). % ONLYFORHEX" << std::endl;
  }

  return 0;

  }
  catch(const std::exception& e)
  {
    std::cerr << "exception: " << e.what() << std::endl;
    return -1;
  }
}
