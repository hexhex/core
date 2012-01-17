/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * @file TestPlugin.cpp
 * @author Roman Schindlauer
 * @author Peter Schueller
 * @date Tue Mar 27 17:28:33 CEST 2007
 *
 * @brief Test-plugin for the dlvhex-testsuite.
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/ComfortPluginInterface.hpp"
#include "dlvhex2/Term.hpp"
#include "dlvhex2/Registry.hpp"

#include <boost/foreach.hpp>

#include <string>
#include <sstream>
#include <iostream>

#include <cstdio>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN

class TestAAtom:
  public ComfortPluginAtom
{
public:
	TestAAtom():
    ComfortPluginAtom("testA")
	{
		addInputPredicate();
		setOutputArity(1);
	}

	virtual void retrieve(const ComfortQuery& query, ComfortAnswer& answer)
	{
		ComfortTuple tu;
		if( query.interpretation.empty() )
    {
			tu.push_back(ComfortTerm::createConstant("foo"));
    }
    else
    {
			tu.push_back(ComfortTerm::createConstant("bar"));
    }

		answer.insert(tu);
	}
};

class TestBAtom: 
  public ComfortPluginAtom
{
public:
	TestBAtom():
    ComfortPluginAtom("testB")
	{
		addInputPredicate();
		addInputPredicate();
		setOutputArity(1);
	}

	virtual void retrieve(const ComfortQuery& query, ComfortAnswer& answer)
	{
		if( query.interpretation.size() <= 1 )
		{
      ComfortTuple tu;
      tu.push_back(ComfortTerm::createConstant("bar"));
      answer.insert(tu);
		}
    else if( query.interpretation.size() > 1 )
		{
      ComfortTuple tu;
      tu.push_back(ComfortTerm::createConstant("foo"));
      answer.insert(tu);
		}
	}
};

class TestCAtom:
  public ComfortPluginAtom
{
public:
  TestCAtom():
    ComfortPluginAtom("testC")
  {
    addInputPredicate();
    setOutputArity(1);
  }
  
  virtual void retrieve(const ComfortQuery& query, ComfortAnswer& answer)
  {
    assert(query.input.size() > 0);
    assert(query.input[0].isConstant());

    // was: std::string t = "-" + query.input[0].strval;
    std::string t = query.input[0].strval;
    
    ComfortInterpretation proj;
    query.interpretation.matchPredicate(t, proj);

    for(ComfortInterpretation::const_iterator it = proj.begin();
        it != proj.end(); ++it)
    {
      const ComfortAtom& at = *it;
      ComfortTuple::const_iterator itt = at.tuple.begin();
      assert(itt != at.tuple.end());
      // skip predicate
      itt++;
      while( itt != at.tuple.end() )
      {
        // add each constant of the atom as separate output tuple
        // so foo(a,b,c) will end up as three tuples [a], [b], and [c]
        ComfortTuple tu;
        tu.push_back(*itt);
        answer.insert(tu);
        itt++;
      }
    }
  }
};

// this is no comfortplugin atom as we don't need comfort here
class TestZeroArityAtom:
  public PluginAtom
{
protected:
	bool succeed;
public:
  TestZeroArityAtom(const std::string& name, bool succeed):
    PluginAtom(name, true),
    succeed(succeed)
  {
    // no inputs
    setOutputArity(0);
  }
  
  virtual void retrieve(const Query&, Answer& answer)
  {
		if( succeed )
		{
			// succeed by returning an empty tuple
			answer.get().push_back(Tuple());
		}
		else
		{
			// fail by returning no tuple (but mark answer as set)
			answer.use();
		}
  }
};

# if 0
// comfort implementation
class TestConcatAtom:
  public ComfortPluginAtom
{
public:
  TestConcatAtom():
    ComfortPluginAtom("testConcat", true) // monotonic, and no predicate inputs anyway
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
    addInputTuple();
    setOutputArity(1);
  }

  virtual void retrieve(const ComfortQuery& query, ComfortAnswer& answer)
  {
    std::stringstream s;

    BOOST_FOREACH(const ComfortTerm& t, query.input)
    {
      if( t.isInteger() )
        s << t.intval;
      else if( t.isConstant() )
        s << t.strval;
      else
        throw PluginError("encountered unknown term type!");
    }
    
    ComfortTuple tu;
    tu.push_back(ComfortTerm::createConstant(s.str()));
    answer.insert(tu);
  }
};
#else
// non-comfort implementation
class TestConcatAtom:
  public PluginAtom
{
public:
  TestConcatAtom():
    PluginAtom("testConcat", true) // monotonic, and no predicate inputs anyway
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
    addInputTuple();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    std::stringstream s;

		bool hasStrings = false;
    BOOST_FOREACH(ID tid, query.input)
    {
			assert(tid.isTerm());
      if( tid.isIntegerTerm() )
        s << tid.address;
      else if( tid.isConstantTerm() )
			{
				const std::string& str = registry->getTermStringByID(tid);
				if( str[0] == '"' )
				{
					hasStrings = true;
					s << str.substr(1,str.size()-2);
				}
				else
				{
					s << str;
				}
			}
      else
        throw PluginError("encountered unknown term type!");
    }
    
		Term resultterm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, s.str());
		if( hasStrings )
			resultterm.symbol = "\"" + resultterm.symbol + "\"";
    Tuple tu;
    tu.push_back(registry->storeTerm(resultterm));
		// the next line would also work and be more efficient, but the above line tests more
    //tu.push_back(registry->storeConstOrVarTerm(resultterm));
    answer.get().push_back(tu);
  }
};
#endif

class TestSetMinusAtom:
  public ComfortPluginAtom
{
public:
  TestSetMinusAtom():
    // this nonmonotonicity is very important,
    // because this atom is definitively nonmonotonic
    // and there are testcases that fail if this is set to true!
    ComfortPluginAtom("testSetMinus", false)
  {
    addInputPredicate();
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const ComfortQuery& query, ComfortAnswer& answer)
  {
    assert(query.input.size() == 2);
    if( !query.input[0].isConstant() || !query.input[1].isConstant() )
      throw PluginError("need constant predicates as input to testSetMinus!");

    // extract predicates from input
    std::vector<ComfortInterpretation> psets;
    psets.resize(2); // allocate exactly two sets
    query.interpretation.matchPredicate(query.input[0].strval, psets[0]);
    query.interpretation.matchPredicate(query.input[1].strval, psets[1]);
    
    // extract terms from predicate sets
    std::vector<std::set<ComfortTerm> > tsets;
    BOOST_FOREACH(const ComfortInterpretation& pset, psets)
    {
      // put result into termsets
      tsets.push_back(std::set<ComfortTerm>());
      std::set<ComfortTerm>& tset = tsets.back();

      // extract (assume unary predicates)
      BOOST_FOREACH(const ComfortAtom& pred, pset)
      {
        if( pred.tuple.size() != 2 )
				{
					std::stringstream s;
					s << "can only process atoms of arity 2 with testSetMinus" <<
						"(got " << printrange(pred.tuple) << ")";
          throw PluginError(s.str());
				}
        // simply insert the argument into the set
        tset.insert(pred.tuple[1]);
      }
    }

    // do set difference between tsets
    std::set<ComfortTerm> result;
    std::insert_iterator<std::set<ComfortTerm> > 
      iit(result, result.begin());
    std::set_difference(
        tsets[0].begin(), tsets[0].end(),
        tsets[1].begin(), tsets[1].end(),
        iit);
    BOOST_FOREACH(const ComfortTerm& t, result)
    {
      ComfortTuple tu;
      tu.push_back(t);
      answer.insert(tu);
    }
  }
};


class TestMinusOneAtom:
	public ComfortPluginAtom
{
public:
	TestMinusOneAtom():
		// monotonic, as only constant inputs
		ComfortPluginAtom("testMinusOne", true)
	{
			addInputConstant();
			setOutputArity(1);
	}

	virtual void
	retrieve(const ComfortQuery& query, ComfortAnswer& answer)
	{
		assert(query.input.size() == 1);
		if( !query.input[0].isInteger() )
			throw PluginError("TestMinusOneAtom can only process integer inputs!");

		int i = query.input[0].intval;
		if( i > 0 )
			i--;

		ComfortTuple t;
		t.push_back(ComfortTerm::createInteger(i));
		answer.insert(t);
	}
};


class TestEvenAtom:
	public ComfortPluginAtom
{
public:
  TestEvenAtom():
		ComfortPluginAtom("testEven", false)
  {
    addInputPredicate();
    addInputPredicate();
    setOutputArity(0);
  }
 
  virtual void
  retrieve(const ComfortQuery& query, ComfortAnswer& answer)
  {
    // Even is true, iff input predicates hold for even individuals
 
    if (query.interpretation.size() % 2 == 0)
		{
			// succeed by returning an empty tuple
			answer.insert(ComfortTuple());
		}
    else
		{
			// fail by returning no tuple
		}
  }
};
 
class TestOddAtom:
	public ComfortPluginAtom
{
public:
  TestOddAtom():
		ComfortPluginAtom("testOdd", false)
  {
    addInputPredicate();
    addInputPredicate();
    setOutputArity(0);
  }

  virtual void
  retrieve(const ComfortQuery& query, ComfortAnswer& answer)
  {
    if (query.interpretation.size() % 2 != 0)
		{
			// succeed by returning an empty tuple
			answer.insert(ComfortTuple());
		}
    else
		{
			// fail by returning no tuple
		}
  }
};


class TestPlugin:
  public PluginInterface
{
public:
  TestPlugin():
    PluginInterface()
  {
    setNameVersion("dlvhex-testplugin", 0, 0, 1);
  }

  virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx&) const
  {
    std::vector<PluginAtomPtr> ret;

		// return smart pointer with deleter (i.e., delete code compiled into this plugin)
    ret.push_back(PluginAtomPtr(new TestAAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestBAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestCAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestZeroArityAtom("testZeroArity0", false), PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestZeroArityAtom("testZeroArity1", true), PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestConcatAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestMinusOneAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestEvenAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestOddAtom, PluginPtrDeleter<PluginAtom>()));

    return ret;
	}
};

TestPlugin theTestPlugin;

DLVHEX_NAMESPACE_END

IMPLEMENT_PLUGINVERSIONFUNCTION(DLVHEX_VERSION_MAJOR,DLVHEX_VERSION_MINOR,DLVHEX_VERSION_MICRO);

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theTestPlugin);
}

/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
