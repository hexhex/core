#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

// include above because we must link against this
#include <boost/program_options.hpp>

#define _GLIBCXX_DEBUG // safe iterators where possible (where not already included above)

#include "dlvhex2/Logger.hpp"
#include "dlvhex2/Printhelpers.hpp"

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

const Logger::Levels DBG = 0x2;

struct Config
{
  unsigned tracks, papers, ureferees, sreferees, uconflicts, sconflicts, noext, globalnoconf;
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
      "number of papers per track")
    ("ureferees,u", po::value<unsigned>(&config.ureferees)->required(),
      "number of track-unique referees")
    ("sreferees,s", po::value<unsigned>(&config.sreferees)->required(),
      "number of shared referees")
    ("uconflicts,a", po::value<unsigned>(&config.uconflicts)->required(),
      "percentage of tracks where unique reviewers have less conflicts")
    ("sconflicts,b", po::value<unsigned>(&config.sconflicts)->required(),
      "percentage of shared reviewers with less conflicts")
    ("noext,n", po::value<unsigned>(&config.noext)->required(),
      "number of external conflicts for local referees")
    ("globalnoconf,g", po::value<unsigned>(&config.globalnoconf)->default_value(0),
      "number of non-conflicts for global referees")
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

  StringVector tracksyms;
  StringVector papersyms;
  StringVector urefereesyms;
  StringVector srefereesyms;
  {
    // create symbols (shared across strata)
    genSyms("track", config.tracks, tracksyms);
    genSyms("paper", config.tracks*config.papers, papersyms);
    genSyms("ref", config.tracks*config.ureferees, urefereesyms);
    genSyms("sref", config.sreferees, srefereesyms);

    for(unsigned p = 0; p < config.tracks*config.papers; ++p)
    {
      o << "paper(" << papersyms[p] << ")." << std::endl;
    }
    for(unsigned r = 0; r < config.tracks*config.ureferees; ++r)
    {
      o << "referee(" << urefereesyms[r] << ")." << std::endl;
    }
    for(unsigned r = 0; r < config.sreferees; ++r)
    {
      o << "referee(" << srefereesyms[r] << ")." << std::endl;
    }
    for(unsigned t = 0; t < config.tracks; ++t)
    {
      o << "track(" << tracksyms[t] << ")." << std::endl;
      for(unsigned p = 0; p < config.papers; ++p)
      {
        o << "track_paper(" << tracksyms[t] << "," << papersyms[t*config.papers+p] << ")." << std::endl;
      }
      for(unsigned r = 0; r < config.ureferees; ++r)
      {
        o << "track_referee(" << tracksyms[t] << "," << urefereesyms[t*config.ureferees+r] << ")." << std::endl;
      }
      for(unsigned r = 0; r < config.sreferees; ++r)
      {
        o << "track_referee(" << tracksyms[t] << "," << srefereesyms[r] << ")." << std::endl;
      }
    }
  }

  // create conflicts
  {
    // only a small amount of external conflicts for local referees per track
    /*
    if( config.noext > config.tracks )
    {
      std::cerr << "reduced config.noext to config.tracks" << std::endl;
      config.noext = config.tracks;
    }
    unsigned u = 0;
    */

    // create track-local conflicts
    for(unsigned t = 0; t < config.tracks; ++t)
    {
      // build conflicts for one referee
      unsigned count = 0;
      for(unsigned r = 0; r < config.ureferees; ++r)
      {
        std::set<unsigned> conflict;
        // create all conflicts except for two papers
        // do not create conflicts for the last referee in the track (this is automatically)
        if( (r+1) != config.ureferees )
        {
          for(unsigned c = 0; c < config.papers; ++c)
          {
            //if( c != r && c != ((r+1)%config.papers) )
            if( c != r && c != (r+1) )
              conflict.insert(t*config.papers+c);
          }
        }

        // remove some conflicts
        if( (t*100/config.tracks) < config.uconflicts )
        {
          while(true)
          {
            unsigned remove = random.getInRange(t*config.papers, (t+1)*config.papers-1);
            if( conflict.count(remove) > 0 )
            {
              conflict.erase(remove);
              break;
            }
          }
        }
        LOG(DBG,"conflicts in track " << tracksyms[t] << " for referee " << r << ": " << printrange(conflict));

        // for referee r, conflict number r may be external
        BOOST_FOREACH(unsigned pap, conflict)
        {
          if( (r > count) && (count < config.noext) )
          {
            o << "conflict(" << papersyms[pap] << "," << urefereesyms[t*config.ureferees+r] << ")." << std::endl;
            //u++;
            count++;
          }
          else
          {
            o << "iconflict(" << papersyms[pap] << "," << urefereesyms[t*config.ureferees+r] << ")." << std::endl;
          }
        }
      }
    }

    // create global conflicts
    for(unsigned r = 0; r < config.sreferees; ++r)
    {
      std::set<unsigned> conflict;
      // create all conflicts
      for(unsigned c = 0; c < config.tracks*config.papers; ++c)
      {
        conflict.insert(c);
      }

      // remove one conflict for each track
      // to get global models we remove for the first sreferee the largest conflict in the first track,
      // for the second sreferee the largest conflict in the second track, ...
      if( (r*100/config.sreferees) < config.sconflicts )
      {
        if( config.globalnoconf > config.tracks )
        {
          std::cerr << "reduced config.globalnoconf to config.tracks" << std::endl;
          config.globalnoconf = config.tracks;
        }
        for(unsigned t = 0; t < config.tracks && t < config.globalnoconf; ++t)
        {
          unsigned remove = t*config.papers + config.papers - 1 - r;
          while(true)
          {
            if( conflict.count(remove) > 0 )
            {
              conflict.erase(remove);
              break;
            }
            // if this is not possible (e.g. if not enough papers are there), do a real guessing
            remove = t*config.papers + random.getInRange(0, config.papers-1);
          }
        }
      }
      LOG(DBG,"conflicts for global referee " << r << ": " << printrange(conflict));

      // only internal global conflicts
      //bool first = true;
      BOOST_FOREACH(unsigned pap, conflict)
      {
        if( true ) //first )
        {
          // first conflict is external
          o << "conflict(" << papersyms[pap] << "," << srefereesyms[r] << ")." << std::endl;
        }
        else
        {
          o << "iconflict(" << papersyms[pap] << "," << srefereesyms[r] << ")." << std::endl;
          //first = false;
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
