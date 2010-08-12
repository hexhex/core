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
 * @date Tue Mar 27 17:28:33 CEST 2007
 *
 * @brief Test-plugin for the dlvhex-testsuite.
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex/PluginInterface.h"

#include <string>
#include <sstream>
#include <iostream>

#include <cstdio>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN

class TestAAtom : public PluginAtom
{
public:
	TestAAtom()
	{
		addInputPredicate();
		setOutputArity(1);
	}

	virtual void
	retrieve(const Query& query, Answer& answer) throw(PluginError)
	{
		std::vector<Tuple> out;

		Term t1("foo");
		Term t2("bar");
		Tuple tu1;

		if (query.getInterpretation().size() == 0)
			tu1.push_back(t1);

		if (query.getInterpretation().size() > 0)
			tu1.push_back(t2);

		out.push_back(tu1);

		answer.addTuples(out);
	}
};


class TestBAtom : public PluginAtom
{
public:
	TestBAtom()
	{
		addInputPredicate();
		addInputPredicate();
		setOutputArity(1);
	}

	virtual void
	retrieve(const Query& query, Answer& answer) throw(PluginError)
	{
		std::vector<Tuple> out;

		Term t1("bar");
		Term t2("foo");
		Tuple tu1;

		if (query.getInterpretation().size() <= 1)
		{
			tu1.push_back(t1);
			out.push_back(tu1);
		}

		if (query.getInterpretation().size() > 1)
		{
			tu1.push_back(t2);
			out.push_back(tu1);
		}

		answer.addTuples(out);
	}
};




class TestCAtom : public PluginAtom
{
public:
  TestCAtom()
  {
    addInputPredicate();
    setOutputArity(1);
  }
  
  virtual void
  retrieve(const Query& query, Answer& answer) throw(PluginError)
  {
    std::vector<Tuple> out;
    
    const Tuple& in = query.getInputTuple();
    
    std::string t = "-" + in[0].getUnquotedString();
    
    AtomSet negm;
    query.getInterpretation().matchPredicate(t, negm);

    for (AtomSet::const_iterator it = negm.begin();
	 it != negm.end(); ++it)
      {
	out.push_back(it->getArguments());
      }
    
    answer.addTuples(out);
  }
};



class TestZeroArityAtom : public PluginAtom
{
protected:
	bool succeed;
public:
  TestZeroArityAtom(bool succeed): succeed(succeed)
  {
    setOutputArity(0);
  }
  
  virtual void
  retrieve(const Query&, Answer& answer) throw(PluginError)
  {
		if( succeed )
		{
			// succeed by returning an empty tuple
			answer.addTuple(Tuple());
		}
		else
		{
			// fail by returning no tuple
		}
  }
};



class TestConcatAtom : public PluginAtom
{
public:

    TestConcatAtom()
    {
        addInputConstant();
        addInputConstant();
        setOutputArity(1);
    }

    virtual void
    retrieve(const Query& query, Answer& answer) throw (PluginError)
    {

        std::stringstream in1, in2;

        Term s1 = query.getInputTuple()[0];
        Term s2 = query.getInputTuple()[1];

        if (s1.isInt())
            in1 << s1.getInt();
        else if (s1.isString())
            in1 << s1.getUnquotedString();
        else
            throw PluginError("Wrong input argument type");
        
        if (s2.isInt())
            in2 << s2.getInt();
        else if (s2.isString())
            in2 << s2.getUnquotedString();
        else
            throw PluginError("Wrong input argument type");
        
        Tuple out;

        //
        // call Term::Term with second argument true to get a quoted string!
        //
        out.push_back(Term(std::string(in1.str() + in2.str()), 1));

        answer.addTuple(out);
    }
};

class TestMinusOneAtom : public PluginAtom
{
public:

    TestMinusOneAtom():
      PluginAtom()
    {
        addInputConstant();
        setOutputArity(1);
    }

    virtual void
    retrieve(const Query& query, Answer& answer) throw (PluginError)
    {
      const Tuple& in = query.getInputTuple();
    
      const std::string& s1 = in[0].getUnquotedString();
      
      std::stringstream s;
      s << s1;
      int i;
      s >> i;
      if( i > 0 )
        i--;

      Tuple t;
      t.push_back(Term(i));
      answer.addTuple(t);
    }
};


class TestSetMinusAtom : public PluginAtom
{
public:

    TestSetMinusAtom():
      PluginAtom(false)
    {
        addInputPredicate();
        addInputPredicate();
        setOutputArity(1);
    }

    virtual void
    retrieve(const Query& query, Answer& answer) throw (PluginError)
    {
      const Tuple& in = query.getInputTuple();
    
      const std::string& s1 = in[0].getUnquotedString();
      const std::string& s2 = in[1].getUnquotedString();
      
      AtomSet set1;
      AtomSet set2;

      query.getInterpretation().matchPredicate(s1, set1);
      query.getInterpretation().matchPredicate(s2, set2);

      std::set<std::string> terms1;
      for(AtomSet::const_iterator it = set1.begin();
          it != set1.end(); ++it)
      {
        const Tuple& args = it->getArguments();
        assert(args.size() == 1);
        assert(args[0].isSymbol());
        terms1.insert(args[0].getString());
        //std::cerr << "inset1 " << args[0].getString() << std::endl;
      }

      std::set<std::string> terms2;
      for(AtomSet::const_iterator it = set2.begin();
          it != set2.end(); ++it)
      {
        const Tuple& args = it->getArguments();
        assert(args.size() == 1);
        assert(args[0].isSymbol());
        terms2.insert(args[0].getString());
        //std::cerr << "inset2 " << args[0].getString() << std::endl;
      }

      std::vector<Tuple> out;
      std::set<std::string> result;
      std::insert_iterator<std::set<std::string> > 
        iit(result, result.begin());
      std::set_difference(
          terms1.begin(), terms1.end(),
          terms2.begin(), terms2.end(),
          iit);
      for(std::set<std::string>::const_iterator it = result.begin();
          it != result.end(); ++it)
      {
        Tuple t;
        t.push_back(Term(*it));
        out.push_back(t);
        //std::cerr << "inresult " << *it << std::endl;
      }
    
      answer.addTuples(out);
    }
};


class TestPlugin : public PluginInterface
{
public:
	/*
		virtual PluginRewriter*
		createRewriter(istream& i, ostream& o)
		{ return new MyRewriter(i,o); }
		*/

	virtual void
	getAtoms(AtomFunctionMap& a)
	{
	  boost::shared_ptr<PluginAtom> testA(new TestAAtom);
	  boost::shared_ptr<PluginAtom> testB(new TestBAtom);
	  boost::shared_ptr<PluginAtom> testC(new TestCAtom);
	  boost::shared_ptr<PluginAtom> testZeroArity0(new TestZeroArityAtom(false));
	  boost::shared_ptr<PluginAtom> testZeroArity1(new TestZeroArityAtom(true));
	  boost::shared_ptr<PluginAtom> testConcat(new TestConcatAtom);
	  boost::shared_ptr<PluginAtom> testSetMinus(new TestSetMinusAtom);
	  boost::shared_ptr<PluginAtom> testMinusOne(new TestMinusOneAtom);

	  a["testA"] = testA;
	  a["testB"] = testB;
	  a["testC"] = testC;
	  a["testZeroArity0"] = testZeroArity0;
	  a["testZeroArity1"] = testZeroArity1;
	  a["testConcat"] = testConcat;
	  a["testSetMinus"] = testSetMinus;
	  a["testMinusOne"] = testMinusOne;
	}

	virtual void
	setOptions(bool /* doHelp */, std::vector<std::string>& /* argv */, std::ostream& /* out */)
	{
		//
		// no options yet
		//
		return;
	}
};

TestPlugin theTestPlugin;

DLVHEX_NAMESPACE_END

extern "C"
DLVHEX_NAMESPACE TestPlugin*
PLUGINIMPORTFUNCTION()
{
	DLVHEX_NAMESPACE theTestPlugin.setPluginName("dlvhex-testplugin");
	DLVHEX_NAMESPACE theTestPlugin.setVersion(0,0,1);
	return & DLVHEX_NAMESPACE theTestPlugin;
}

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
