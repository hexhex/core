// include above because we must link against this
#include <boost/program_options.hpp>

#define _GLIBCXX_DEBUG // safe iterators where possible (where not already included above)

#include "dlvhex/Logger.hpp"

#if 0
#include <gmp.h>
#include <gmpxx.h>
#endif

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
  unsigned tracks, papers, ureferees, sreferees, uconflicts, sconflicts, noext;
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
      "percentage of unique reviewers with less conflicts")
    ("sconflicts,b", po::value<unsigned>(&config.sconflicts)->required(),
      "percentage of shared reviewers with less conflicts")
    ("noext,n", po::value<unsigned>(&config.noext)->default_value(0),
      "do not output external atoms")
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

  std::stringstream prelude;
  StringVector tracksyms;
  StringVector papersyms;
  StringVector urefereesyms;
  StringVector srefereesyms;
  {
    std::ostream& o = prelude;

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

  #if 0
  mpz_t guessspace;
  {
    mpz_t oneguessspace;
    mpz_init(oneguessspace);
    mpz_bin_uiui(oneguessspace, config.ureferees+config.sreferees, 2); 

    mpz_init_set_ui(guessspace,1);
    for(unsigned u = 0; u < config.papers; ++u)
    {
      mpz_mul(guessspace, guessspace, oneguessspace);
    }
  }
  LOG("guessing space for each track = " << mpz_class(guessspace).get_str());

  mpz_t bestdif;
  mpz_init_set_ui(bestdif, config.models);

  do
  {
    LOG("starting iteration");
    mpz_t models;
    mpz_init_set_ui(models,1);
  #endif

    std::stringstream o;

    // create track-local conflicts
    for(unsigned t = 0; t < config.tracks; ++t)
    {
      // build conflicts for one referee
      for(unsigned r = 0; r < config.ureferees; ++r)
      {
        std::set<unsigned> conflict;
        // create all conflicts except for two papers
        for(unsigned c = 0; c < config.papers; ++c)
        {
          if( c != r && c != ((r+1)%config.papers) )
            conflict.insert(t*config.papers+c);
        }

        // remove some conflicts
        if( (r*100/config.ureferees) < config.uconflicts )
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
        LOG("conflicts in track " << tracksyms[t] << " for referee " << r << ": " << printrange(conflict));

        BOOST_FOREACH(unsigned pap, conflict)
        {
          o << "conflict(" << papersyms[pap] << "," << urefereesyms[t*config.ureferees+r] << ")." << std::endl;
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

      // remove three conflicts (this way we really require global constraint checks!)
      if( (r*100/config.sreferees) < config.sconflicts )
      {
        unsigned removed = 0;
        while(removed < 3 )
        {
          unsigned remove = random.getInRange(0, config.tracks*config.papers-1);
          if( conflict.count(remove) > 0 )
          {
            conflict.erase(remove);
            removed++;
          }
        }
      }
      LOG("conflicts for global referee " << r << ": " << printrange(conflict));

      BOOST_FOREACH(unsigned pap, conflict)
      {
        o << "conflict(" << papersyms[pap] << "," << srefereesyms[r] << ")." << std::endl;
      }
    }

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
      if( config.noext == 1 )
        o << ":- assign(" << tracksyms[t] << ",P,R), conflict(P,R)." << std::endl;
      else
        o << ":- assign(" << tracksyms[t] << ",P,R), &gen2[conflict,P,R]()." << std::endl;

      //mpz_mul(models, models, guessspace);
    }

    // no more than 2 assignments per reviewer (global)
    o << ":- assign(T,P1,R), assign(T,P2,R), assign(T,P3,R), P1 != P2, P1 != P3, P2 != P3." << std::endl;

    // conflicts (global)
    if( config.noext == 1 )
      o << ":- assign(T,P,R), conflict(P,R)." << std::endl;
    else
      o << ":- assign(T,P,R), &gen2[conflict,P,R]()." << std::endl;

    //LOG("models so far = " << mpz_class(models).get_str());

    // add conflicts



    #if 0
    // calculate difference to reference model count
    mpz_t dif;
    mpz_init(dif);
    mpz_sub_ui(dif, models, config.models);
    mpz_abs(dif, dif);
    LOG("difference here is " << mpz_class(dif).get_str() << "!");

    if( mpz_cmp(dif, bestdif) < 0 )
    {
      mpz_set(bestdif, dif);
      LOG("this is new best dif! " << mpz_class(dif).get_str());

      if( mpz_cmp_ui(bestdif, config.tolerance) < 0 )
      {
        LOG("this is below tolerance!");
        #endif
        std::cout << prelude.str();
        std::cout << o.str();
        #if 0
        break;
      }
    }
  }
  while(true);
  #endif












  #if 0

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


  #endif

  return 0;

  }
  catch(const std::exception& e)
  {
    std::cerr << "exception: " << e.what() << std::endl;
    return -1;
  }
}
