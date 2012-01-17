/**
 * external atom &above[predicate/1,constant](X)
 * returns { X | predicate(X), constant < X } in X
 * 
 * parameters:
 * * n strata, each
 * ** g guesses
 * ** s atoms
 * ** c calculations from guesses to atoms within stratum
 * ** i non-external connections from stratum to its guesses
 * ** access k strata above (k > 0)
 * ** k*l non-external connections to above strata
 * ** k*e external connections to above strata
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

// include above because we must link against this
#include <boost/program_options.hpp>

#define _GLIBCXX_DEBUG // safe iterators where possible (where not already included above)

#include "dlvhex2/Logger.hpp"
#include "dlvhex2/Printhelpers.hpp"

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
  unsigned n, g, s, c, i, k, l, e;
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

int main(int ac,char** av)
{
  try
  {

  Config config;
  /*
  config.n = 4;
  config.g = 3;
  config.s = 2;
  config.c = 5;
  config.i = 4;
  config.k = 2;
  config.l = 3;
  config.e = 3;
  */

  po::options_description desc("program options");
  unsigned seed;
  desc.add_options()
    ("help", "produce help message")
    ("seed", po::value<unsigned>(&seed)->default_value(0),
      "random seed")
    ("strata,n", po::value<unsigned>(&config.n)->required(),
      "number of strata")
    ("guesses,g", po::value<unsigned>(&config.g)->required(),
      "guesses in each stratum")
    ("symbols,s", po::value<unsigned>(&config.s)->required(),
      "additional non-guessed symbols in each stratum")
    ("calcs,c", po::value<unsigned>(&config.c)->required(),
      "number of rules calculating symbols from guesses in each stratum")
    ("icalcs,i", po::value<unsigned>(&config.i)->required(),
      "number of constraints on calculated symbols in each stratum")
    ("kabove,k", po::value<unsigned>(&config.k)->required(),
      "access k strata above")
    ("links,l", po::value<unsigned>(&config.l)->required(),
      "k*l random rule non-external connections to above strata")
    ("elinks,e", po::value<unsigned>(&config.e)->required(),
      "k*e random rule external connections to above strata")
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

  const std::string stratumprefix("in_");

  typedef std::set<std::string> StringSet;
  typedef std::list<std::string> StringList;
  typedef std::vector<std::string> StringVector;
  typedef std::vector<StringVector> StrataStringVector;

  StrataStringVector guessSymbols(config.n, StringVector());
  StrataStringVector calcSymbols(config.n, StringVector());
  for(unsigned atStratum = 0; atStratum < config.n; ++atStratum)
  {
    // build stratum
    LOG(DBG,"building stratum " << atStratum);
    std::stringstream stratumpreds;
    stratumpreds << stratumprefix << atStratum;
    const std::string& stratumpred = stratumpreds.str();

    // "guessing" part

    // build symbols
    StringVector guessSymsHere;
    for(unsigned var = 0; var < (2*config.g); ++var)
      guessSymsHere.push_back(provider.getNextSymbol("gv_"));
    // store
    guessSymbols[atStratum] = guessSymsHere;
    LOG(DBG,"guessSymsHere  " << printrange(guessSymsHere));
    // randomize and build guesses
    randomizeRange(random, guessSymsHere);
    LOG(DBG,"guessSymsHere* " << printrange(guessSymsHere));
    for(StringVector::iterator it = guessSymsHere.begin();
        it != guessSymsHere.end(); ++it)
    {
      const std::string syma = *it;
      it++;
      assert(it != guessSymsHere.end());
      const std::string symb = *it;
      o << stratumpred << "(" << syma << ") :- not " << stratumpred << "(" << symb << ")." << std::endl;
      o << stratumpred << "(" << symb << ") :- not " << stratumpred << "(" << syma << ")." << std::endl;
    }

    //
    // "calculation" part
    //
    StringVector calcSymsHere;
    for(unsigned var = 0; var < config.s; ++var)
      calcSymsHere.push_back(provider.getNextSymbol("cv_"));
    LOG(DBG,"calcSymsHere " << printrange(calcSymsHere));
    calcSymbols[atStratum] = calcSymsHere;

    // derive stratum from guesses
    for(unsigned u = 0; u < config.s; ++u)
    {
      const std::string& csym = calcSymsHere[random.getInRange(0, config.s-1)];
      const std::string& gsym = guessSymsHere[random.getInRange(0, 2*config.g-1)];
      std::string naf;
      if( random.getBool() )
        naf = "not ";
      o << stratumpred << "(" << csym << ") :- " << naf << stratumpred << "(" << gsym << ")." << std::endl;
    }

    // derive calculated stratum non-externally from strata above
    if( atStratum != 0 )
    {
      for(unsigned u = 0; u < (config.k*config.l); ++u)
      {
        unsigned strat1 = random.getInRange(
            std::max<int>(0, static_cast<int>(atStratum)-static_cast<int>(config.k)),
            atStratum-1);
        const std::string& ssym1 = calcSymbols[strat1][random.getInRange(0, config.s-1)];
        std::string naf1;
        if( random.getBool() )
          naf1 = "not ";

        unsigned strat2 = random.getInRange(
            std::max<int>(0, static_cast<int>(atStratum)-static_cast<int>(config.k)),
            atStratum-1);
        const std::string& ssym2 = calcSymbols[strat2][random.getInRange(0, config.s-1)];
        std::string naf2;
        if( random.getBool() )
          naf2 = "not ";

        const std::string& tsym = calcSymsHere[random.getInRange(0, config.s-1)];

        o << stratumpred << "(" << tsym << ") :- " <<
          naf1 << stratumprefix << strat1 << "(" << ssym1 << ")," <<
          naf2 << stratumprefix << strat2 << "(" << ssym2 << ")." << std::endl;
      }
    }

    // derive calculated stratum externally from strata above
    if( atStratum != 0 )
    {
      for(unsigned u = 0; u < (config.k*config.e); ++u)
      {
        unsigned strat1 = random.getInRange(
            std::max<int>(0, static_cast<int>(atStratum)-static_cast<int>(config.k)),
            atStratum-1);
        const std::string& ssym1a = calcSymbols[strat1][random.getInRange(0, config.s-1)];
        const std::string& ssym1b = calcSymbols[strat1][random.getInRange(0, config.s-1)];
        std::string naf1;
        if( random.getBool() )
          naf1 = "not ";

        unsigned strat2 = random.getInRange(
            std::max<int>(0, static_cast<int>(atStratum)-static_cast<int>(config.k)),
            atStratum-1);
        const std::string& ssym2 = calcSymbols[strat2][random.getInRange(0, config.s-1)];
        std::string naf2;
        if( random.getBool() )
          naf2 = "not ";

        const std::string& tsym = calcSymsHere[random.getInRange(0, config.s-1)];

        o << stratumpred << "(" << tsym << ") :- " <<
          naf2 << stratumpred << "(" << ssym2 << ")," <<
          naf1 <<
          "&above[" << stratumprefix << strat1 << "," << ssym1a << "]" <<
          "(" << ssym1b << ")." << std::endl;
      }
    }

    // constrain guessed and calculated stratum
    for(unsigned u = 0; u < config.i; ++u)
    {
      const std::string& csym1 = calcSymsHere[random.getInRange(0, config.s-1)];
      const std::string& gsym1 = guessSymsHere[random.getInRange(0, 2*config.g-1)];
      std::string naf1;
      if( random.getBool() )
        naf1 = "not ";
      std::string sym1 = csym1;
      if( random.getBool() )
        sym1 = gsym1;

      const std::string& csym2 = calcSymsHere[random.getInRange(0, config.s-1)];
      const std::string& gsym2 = guessSymsHere[random.getInRange(0, 2*config.g-1)];
      std::string naf2;
      if( random.getBool() )
        naf2 = "not ";
      std::string sym2 = csym2;
      if( random.getBool() )
        sym2 = gsym2;

      const std::string& csym3 = calcSymsHere[random.getInRange(0, config.s-1)];
      const std::string& gsym3 = guessSymsHere[random.getInRange(0, 2*config.g-1)];
      std::string naf3;
      if( random.getBool() )
        naf3 = "not ";
      std::string sym3 = csym3;
      if( random.getBool() )
        sym3 = gsym3;

      o << ":- " <<
        naf1 << stratumpred << "(" << sym1 << ")," <<
        naf2 << stratumpred << "(" << sym2 << ")," <<
        naf3 << stratumpred << "(" << sym3 << ")." << std::endl;
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
