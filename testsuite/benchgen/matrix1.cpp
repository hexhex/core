/**
 * external atom &above[predicate/1,constant](X)
 * returns { X | predicate(X), constant < X } in X
 * 
 * parameters:
 * * t towers, each
 * * s strata, each
 * ** g guesses
 * ** c constraints a 3 (pos) body atoms over guesses (neg would not make a search space difference)
 * ** ea dependencies to strata above via external and nonexternal atoms
 * ** es dependencies to strata sideways and above via external and nonexternal atoms
 *
 * overall 3*g symbols
 */

// include above because we must link against this
#include <boost/program_options.hpp>

#define _GLIBCXX_DEBUG // safe iterators where possible (where not already included above)

#include "dlvhex/Logger.hpp"
#include "dlvhex/Printhelpers.hpp"

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
  unsigned t, s, g, c, ea, es;
};

class SymbolProvider
{
public:
  SymbolProvider():
    at(0)
  {
  }

  std::string getNextSymbol(const std::string& prefix="")
  {
    std::stringstream s;
    s << prefix << std::hex << at;
    at++;
    return s.str();
  }

protected:
  unsigned at;
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

std::string inPred(unsigned tower, unsigned stratum)
{
  std::stringstream s;
  s << "in" << tower << "_" << stratum;
  return s.str();
}

std::string in(unsigned tower, unsigned stratum, const std::string& sym)
{
  std::stringstream s;
  s << inPred(tower,stratum) << "(" << sym << ")";
  return s.str();
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
    ("towers,t", po::value<unsigned>(&config.t)->required(),
      "'towers' containing strata")
    ("strata,s", po::value<unsigned>(&config.s)->required(),
      "number of strata in each tower")
    ("guesses,g", po::value<unsigned>(&config.g)->required(),
      "guesses in each stratum")
    ("constraints,c", po::value<unsigned>(&config.c)->required(),
      "number of constraints over guesses (a 3 body atoms) in each stratum")
    ("eabove,a", po::value<unsigned>(&config.ea)->required(),
      "number of external dependencies to stratum above")
    ("esideways,w", po::value<unsigned>(&config.es)->required(),
      "number of external dependencies to strata above and sideways")
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

  SymbolProvider provider;

  typedef std::set<std::string> StringSet;
  typedef std::list<std::string> StringList;
  typedef std::vector<std::string> StringVector;
  typedef std::vector<StringVector> StrataStringVector;

  const unsigned nsymbols = config.g*3;

  // create symbols (shared across strata)
  StringVector symbols;
  symbols.reserve(nsymbols);
  for(unsigned u = 0; u < nsymbols; ++u)
    symbols.push_back(provider.getNextSymbol("c"));

  for(unsigned tow = 0; tow < config.t; ++tow)
  {
    for(unsigned strat = 0; strat < config.s; ++strat)
    {
      LOG(DBG,"guesses for cluster[stratum " << strat << " in tower " << tow << "]");
      StringVector guessSyms(symbols);
      randomizeRange(random, guessSyms);
      for(unsigned g = 0; g < config.g; ++g)
      {
        o << in(tow,strat,guessSyms[2*g]) << " :- not " << in(tow,strat,guessSyms[2*g+1]) << "." << std::endl;
        o << in(tow,strat,guessSyms[2*g+1]) << " :- not " << in(tow,strat,guessSyms[2*g]) << "." << std::endl;
      }

      LOG(DBG,"constraints for cluster[stratum " << strat << " in tower " << tow << "]");
      // choose c distinct constrints
      // (without distinct we may not kill enough,
      // this way we have more fine-grained control over the number of models)
      std::set< std::set<unsigned> > constraints;
      while(constraints.size() != config.c)
      {
        // choose 2 distinct symbols to exclude
        // (without distinct we may kill a lot with few constraints,
        // this way we have more fine-grained control over the number of models)

        unsigned s1 = random.getInRange(0, config.g-1);
        unsigned s2 = s1;
        while( s2 == s1 )
          s2 = random.getInRange(0, config.g-1);
        unsigned s3 = s1;
        while( s3 == s1 || s3 == s2 )
          s3 = random.getInRange(0, nsymbols-1);

        std::set<unsigned> cstr;
        cstr.insert(s1);
        cstr.insert(s2);
        cstr.insert(s3);
        constraints.insert(cstr);
      }

      if( strat == 0 )
        continue;

      // build constraints
      for(std::set< std::set<unsigned> >::const_iterator it = constraints.begin();
          it != constraints.end(); ++it)
      {
        o << ":- ";
        std::set<unsigned>::const_iterator its = it->begin();
        o << in(tow,strat,guessSyms[*its]) << ",";
        its++;
        assert(its != it->end());
        o << in(tow,strat,guessSyms[*its]) << ",";
        its++;
        assert(its != it->end());
        o << in(tow,strat,guessSyms[*its]) << "." << std::endl;
        its++;
        assert(its == it->end());
      }
    }
  }

  for(unsigned tow = 0; tow < config.t; ++tow)
  {
    // start at stratum 1!
    for(unsigned strat = 1; strat < config.s; ++strat)
    {
      // above
      LOG(DBG,"above connections for cluster[stratum " << strat << " in tower " << tow << "]");
      for(unsigned u = 0; u < config.ea; ++u)
      {
        std::string naf1;
        if( random.getBool() )
          naf1 = "not ";
        std::string naf2;
        if( random.getBool() )
          naf2 = "not ";
        o << in(tow,strat,symbols[random.getInRange(0, nsymbols-1)]) << " :- " <<
          naf1 << in(tow,strat-1,symbols[random.getInRange(0, nsymbols-1)]) << "," <<
          naf2 <<
          "&above[" << inPred(tow,strat-1) << "," << symbols[random.getInRange(0, nsymbols-1)] << "]" <<
          "(" << symbols[random.getInRange(0, nsymbols-1)] << ")." << std::endl;
      }

      // sideways
      LOG(DBG,"sideways connections for cluster[stratum " << strat << " in tower " << tow << "]");
      for(unsigned u = 0; u < config.es; ++u)
      {
        unsigned tower1 = tow;
        while(tower1 == tow)
          tower1 = random.getInRange(0,config.t-1);

        unsigned tower2 = tow;
        while(tower2 == tow)
          tower2 = random.getInRange(0,config.t-1);

        std::string naf1;
        if( random.getBool() )
          naf1 = "not ";
        std::string naf2;
        if( random.getBool() )
          naf2 = "not ";
        o << in(tow,strat,symbols[random.getInRange(0, nsymbols-1)]) << " :- " <<
          naf1 << in(tower1,strat-1,symbols[random.getInRange(0, nsymbols-1)]) << "," <<
          naf2 <<
          "&above[" << inPred(tower2,strat-1) << "," << symbols[random.getInRange(0, nsymbols-1)] << "]" <<
          "(" << symbols[random.getInRange(0, nsymbols-1)] << ")." << std::endl;
      }
    }
  }
  return 0;

  }
  catch(const std::exception& e)
  {
    std::cerr << "exception: " << e.what() << std::endl;
    return -1;
  }
}
