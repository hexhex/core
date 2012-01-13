/**
 * external atom &above[predicate/1,constant](X)
 * returns { X | predicate(X), constant < X } in X
 * 
 * parameters:
 * * t towers, each 1 stratum, each
 * ** g guesses
 * ** ic constraints a 3 body atoms over guesses
 * ** ec constraints a 3 body atoms over guesses, where one body atom is external
 * * gic constraints a 3 body atoms over all towers' guesses
 * * gec constraints a 3 body atoms over all towers' guesses, where all are external
 *
 * overall 2*g symbols
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
  unsigned t, g, ic, ec, gic, gec;
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
      "'towers' = cluster of symbols including guess")
    ("guesses,g", po::value<unsigned>(&config.g)->required(),
      "guesses in each tower")
    ("ic", po::value<unsigned>(&config.ic)->required(),
      "number of constraints over guesses in each tower")
    ("ec", po::value<unsigned>(&config.ec)->required(),
      "number of constraints over guesses in each tower, including one external body")
    ("gic", po::value<unsigned>(&config.gic)->required(),
      "number of constraints over all towers")
    ("gec", po::value<unsigned>(&config.gec)->required(),
      "number of constraints over all towers, all external")
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

  const unsigned nsymbols = config.g*2;

  // create symbols (shared across strata)
  StringVector symbols;
  symbols.reserve(nsymbols);
  for(unsigned u = 0; u < nsymbols; ++u)
    symbols.push_back(provider.getNextSymbol("c"));

  for(unsigned tow = 0; tow < config.t; ++tow)
  {
      StringVector guessSyms(symbols);
      randomizeRange(random, guessSyms);
      for(unsigned g = 0; g < config.g; ++g)
      {
        o << in(tow,guessSyms[2*g]) << " :- not " << in(tow,guessSyms[2*g+1]) << "." << std::endl;
        o << in(tow,guessSyms[2*g+1]) << " :- not " << in(tow,guessSyms[2*g]) << "." << std::endl;
      }
  }

  // ic
  for(unsigned tow = 0; tow < config.t; ++tow)
  {
      // choose build ic distinct constraints
      // (without distinct we may not kill enough,
      // this way we have more fine-grained control over the number of models)
      std::set< std::set<unsigned> > constraints;
      while(constraints.size() != config.ic)
      {
        // choose 2 distinct symbols to exclude
        // (without distinct we may kill a lot with few constraints,
        // this way we have more fine-grained control over the number of models)

        unsigned s1 = random.getInRange(0, nsymbols-1);
        unsigned s2 = s1;
        while( s2 == s1 )
          s2 = random.getInRange(0, nsymbols-1);
        unsigned s3 = s1;
        while( s3 == s1 || s3 == s2 )
          s3 = random.getInRange(0, nsymbols-1);

        std::set<unsigned> cstr;
        cstr.insert(s1);
        cstr.insert(s2);
        cstr.insert(s3);
        constraints.insert(cstr);
      }

      // build ic constraints
      for(std::set< std::set<unsigned> >::const_iterator it = constraints.begin();
          it != constraints.end(); ++it)
      {
        o << ":- ";
        std::set<unsigned>::const_iterator its = it->begin();
        o << in(tow,symbols[*its]) << ",";
        its++;
        assert(its != it->end());
        o << in(tow,symbols[*its]) << ",";
        its++;
        assert(its != it->end());
        o << in(tow,symbols[*its]) << "." << std::endl;
        its++;
        assert(its == it->end());
      }
  }

  // ec
  for(unsigned tow = 0; tow < config.t; ++tow)
  {
      // choose build ec distinct constraints
      // (without distinct we may not kill enough,
      // this way we have more fine-grained control over the number of models)
      std::set< std::set<unsigned> > constraints;
      while(constraints.size() != config.ec)
      {
        // choose 2 distinct symbols to exclude
        // (without distinct we may kill a lot with few constraints,
        // this way we have more fine-grained control over the number of models)

        unsigned s1 = random.getInRange(0, nsymbols-1);
        unsigned s2 = s1;
        while( s2 == s1 )
          s2 = random.getInRange(0, nsymbols-1);

        std::set<unsigned> cstr;
        cstr.insert(s1);
        cstr.insert(s2);
        constraints.insert(cstr);
      }

      // build ic constraints
      for(std::set< std::set<unsigned> >::const_iterator it = constraints.begin();
          it != constraints.end(); ++it)
      {
        o << ":- ";
        o <<
          "&above[" << inPred(tow) << "," << symbols[random.getInRange(0, nsymbols-1)] << "]" <<
          "(" << symbols[random.getInRange(0, nsymbols-1)] << "),";
        std::set<unsigned>::const_iterator its = it->begin();
        assert(its != it->end());
        o << in(tow,symbols[*its]) << ",";
        its++;
        assert(its != it->end());
        o << in(tow,symbols[*its]) << "." << std::endl;
        its++;
        assert(its == it->end());
      }
  }
  
  // gic
  {
      if( config.gic != 0 && config.t < 3 )
        throw std::runtime_error("for gic != 0, t must be >= 3");

      // choose gic distinct constraints
      // (without distinct we may not kill enough,
      // this way we have more fine-grained control over the number of models)
      std::set< std::vector<unsigned> > constraints;
      while(constraints.size() != config.gic)
      {
        // choose 3 distinct symbols in 3 distinct towers to exclude
        // (without distinct we may kill a lot with few constraints,
        // this way we have more fine-grained control over the number of models)

        unsigned t1 = random.getInRange(0, config.t-1);
        unsigned t2 = t1;
        while( t2 == t1 )
          t2 = random.getInRange(0, config.t-1);
        unsigned t3 = t1;
        while( t3 == t1 || t3 == t2 )
          t3 = random.getInRange(0, config.t-1);

        unsigned s1 = random.getInRange(0, nsymbols-1);
        unsigned s2 = s1;
        while( s2 == s1 )
          s2 = random.getInRange(0, nsymbols-1);
        unsigned s3 = s1;
        while( s3 == s1 || s3 == s2 )
          s3 = random.getInRange(0, nsymbols-1);

        std::vector<unsigned> cstr;
        cstr.push_back(t1);
        cstr.push_back(s1);
        cstr.push_back(t2);
        cstr.push_back(s2);
        cstr.push_back(t3);
        cstr.push_back(s3);
        constraints.insert(cstr);
      }

      // build gic constraints
      for(std::set< std::vector<unsigned> >::const_iterator it = constraints.begin();
          it != constraints.end(); ++it)
      {
        const std::vector<unsigned>& vec = *it;
        assert(vec.size() == 6);
        o << ":- ";
        o << in(vec[0],symbols[vec[1]]) << ",";
        o << in(vec[2],symbols[vec[3]]) << ",";
        o << in(vec[4],symbols[vec[5]]) << "." << std::endl;
      }
  }

  // gec
  {
      if( config.gec != 0 && config.t < 3 )
        throw std::runtime_error("for gec != 0, t must be >= 3");

      // choose gec distinct constraints
      // (without distinct we may not kill enough,
      // this way we have more fine-grained control over the number of models)
      std::set< std::vector<unsigned> > constraints;
      while(constraints.size() != config.gec)
      {
        // choose 3 distinct symbols in 3 distinct towers to exclude
        // (without distinct we may kill a lot with few constraints,
        // this way we have more fine-grained control over the number of models)

        unsigned t1 = random.getInRange(0, config.t-1);
        unsigned t2 = t1;
        while( t2 == t1 )
          t2 = random.getInRange(0, config.t-1);
        unsigned t3 = t1;
        while( t3 == t1 || t3 == t2 )
          t3 = random.getInRange(0, config.t-1);

        unsigned s1 = random.getInRange(0, nsymbols-1);
        unsigned s2 = s1;
        while( s2 == s1 )
          s2 = random.getInRange(0, nsymbols-1);
        unsigned s3 = s1;
        while( s3 == s1 || s3 == s2 )
          s3 = random.getInRange(0, nsymbols-1);

        std::vector<unsigned> cstr;
        cstr.push_back(t1);
        cstr.push_back(s1);
        cstr.push_back(t2);
        cstr.push_back(s2);
        cstr.push_back(t3);
        cstr.push_back(s3);
        constraints.insert(cstr);
      }

      // build gec constraints
      for(std::set< std::vector<unsigned> >::const_iterator it = constraints.begin();
          it != constraints.end(); ++it)
      {
        const std::vector<unsigned>& vec = *it;
        assert(vec.size() == 6);
        o << ":- ";
        o <<
          "&above[" << inPred(vec[0]) << "," << symbols[vec[1]] << "]" <<
          "(" << symbols[vec[1]] << "),";
        o <<
          "&above[" << inPred(vec[2]) << "," << symbols[vec[3]] << "]" <<
          "(" << symbols[vec[3]] << "),";
        o <<
          "&above[" << inPred(vec[4]) << "," << symbols[vec[5]] << "]" <<
          "(" << symbols[vec[5]] << ")." << std::endl;
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
