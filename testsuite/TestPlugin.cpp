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

#if defined(HAVE_OWLCPP)
#include "owlcpp/rdf/triple_store.hpp"
#include "owlcpp/io/input.hpp"
#include "owlcpp/io/catalog.hpp"
#include "owlcpp/terms/node_tags_owl.hpp"
#endif 


#include "dlvhex2/ExternalLearningHelper.h"
#include "dlvhex2/ComfortPluginInterface.h"
#include "dlvhex2/Term.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printer.h"

#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/program_options.hpp>
#include <boost/range.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <map>

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
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
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
    PluginAtom("testConcat", true) // monotonic, as there is no predicate input anyway
  {
    addInputTuple();
    setOutputArity(1);

    prop.functional = true;
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

    // check if the result is an integer
    try
    {
        int intval = boost::lexical_cast<short>(s.str());
				Tuple tu;
				tu.push_back(ID::termFromInteger(intval));
				answer.get().push_back(tu);
    }
    catch(const boost::bad_lexical_cast &)
    {
			// not an integer
			Term resultterm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, s.str());
			if( hasStrings )
				resultterm.symbol = "\"" + resultterm.symbol + "\"";
		  Tuple tu;
		  tu.push_back(registry->storeTerm(resultterm));
			// the next line would also work and be more efficient, but the above line tests more
		  //tu.push_back(registry->storeConstOrVarTerm(resultterm));
		  answer.get().push_back(tu);
    }
  }
};
#endif

class TestConcatAllAtom:
  public PluginAtom
{
public:
  TestConcatAllAtom():
    PluginAtom("testConcatAll", false) // monotonic, as there is no predicate input anyway
  {
    addInputPredicate();
    setOutputArity(1);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    std::stringstream s;

//		RawPrinter printer(s, registry);
    bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
    bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
    while (en < en_end){
//			printer.print(registry->ogatoms.getIDByAddress(*en));
			s << *en << ";";
      en++;
    }

    // check if the result is an integer
    try
    {
        int intval = boost::lexical_cast<short>(s.str());
				Tuple tu;
				tu.push_back(ID::termFromInteger(intval));
				answer.get().push_back(tu);
    }
    catch(const boost::bad_lexical_cast &)
    {
			// not an integer
			Term resultterm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + s.str() + "\"");
		  Tuple tu;
		  tu.push_back(registry->storeTerm(resultterm));
			// the next line would also work and be more efficient, but the above line tests more
		  //tu.push_back(registry->storeConstOrVarTerm(resultterm));
		  answer.get().push_back(tu);
    }
	}
};

class TestListDomainAtom:
  public PluginAtom
{
public:
  TestListDomainAtom():
    PluginAtom("testListDomain", true) // monotonic, as there is no predicate input anyway
  {
    addInputTuple();
    setOutputArity(1);

    prop.functional = true;
  }

  std::vector<std::string> permute(std::vector<std::string> input){
  
  	if (input.size() > 0){
 		std::vector<std::string> res;
	  	for (uint32_t i = 0; i < input.size(); ++i){
	  		DBGLOG(DBG, "Choosing " << i);
	  		std::vector<std::string> i2 = input;
  			i2.erase(i2.begin() + i);
  			BOOST_FOREACH (std::string subperm, permute(i2)){
			  	std::stringstream ss;
		  		ss << input[i] << (subperm.length() > 0 ? ";" : "") << subperm;
		  		DBGLOG(DBG, "Permutation: " << ss.str());
  				res.push_back(ss.str());

				ss.str("");
  		  		ss << subperm;
		  		DBGLOG(DBG, "Permutation: " << ss.str());
  				res.push_back(ss.str());
			}
  		}
		return res;
	}else{
		input.push_back("");
		return input;
	}
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	const std::string& str = registry->terms.getByID(query.input[0]).getUnquotedString();

	// extract the list elements
	DBGLOG(DBG, "Computing elements in " << str);
	std::vector<std::string> elements;
	std::stringstream element;
	for (uint32_t i = 0; i <= str.length(); i++){
		if (str[i] == ';' || str[i] == '\0'){
			DBGLOG(DBG, "Delimiter detected; element: " << element.str());
			if (element.str().length() > 0) elements.push_back(element.str());
			element.str("");
		}else{
			DBGLOG(DBG, "Consuming character " << str[i]);
			element << str[i];
		}
	}

	// compute all permutations and return them
	DBGLOG(DBG, "Computing permutations over " << elements.size() << " elements");
	BOOST_FOREACH (std::string perm, permute(elements)){
      Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + perm + "\"");
      Tuple tu;
      tu.push_back(registry->storeTerm(t));
      answer.get().push_back(tu);
	}
  }
};

class TestListConcatAtom:
  public PluginAtom
{
public:
  TestListConcatAtom():
    PluginAtom("testListConcat", true) // monotonic, as there is no predicate input anyway
  {
    addInputTuple();
    setOutputArity(1);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    std::stringstream ss;

    BOOST_FOREACH(ID tid, query.input)
    {
			assert(tid.isTerm());
      if( tid.isIntegerTerm() )
        ss << tid.address;
      else if( tid.isConstantTerm() )
			{
				const std::string& str = registry->terms.getByID(tid).getUnquotedString();
				if (ss.str().length() > 0) ss << ";";
				ss << str;
			}
      else
        throw PluginError("encountered unknown term type!");
    }
    
		Term resultterm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + ss.str() + "\"");
    Tuple tu;
    tu.push_back(registry->storeTerm(resultterm));

    answer.get().push_back(tu);
  }
};

class TestListLengthAtom:
  public PluginAtom
{
public:
  TestListLengthAtom():
    PluginAtom("testListLength", true)
  {
    addInputConstant();
    addInputConstant();
    setOutputArity(1);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	const std::string& str = registry->terms.getByID(query.input[0]).getUnquotedString();
	int len = 0;
	if (str.length() > 0) len++;
	for (uint32_t i = 0; i < str.length(); i++){
		if (str[i] == ';') len++;
	}

    Tuple tu;
    tu.push_back(ID::termFromInteger(len / query.input[1].address));
    answer.get().push_back(tu);
  }
};


class TestListSplitAtom:
  public PluginAtom
{
public:
  TestListSplitAtom():
    PluginAtom("testListSplit", true) // monotonic, as there is no predicate input anyway
  {
    addInputConstant();
    addInputConstant();
    setOutputArity(2);

    prop.functional = true;
    prop.wellorderingStrlen.insert(std::pair<int, int>(0, 0));
    prop.wellorderingStrlen.insert(std::pair<int, int>(0, 1));
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	const std::string& str = registry->terms.getByID(query.input[0]).getUnquotedString();
	int cnt = query.input[1].address;

	std::stringstream substr1, substr2;
	int nr = 0;
	for (uint32_t i = 0; i < str.length(); i++){
		if (str[i] == ';'){
			nr++;
			if (nr == cnt) continue;
		}
		if (nr >= cnt) substr2 << str[i];
		else substr1 << str[i];
	}

    Term t1(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + substr1.str() + "\"");
    Term t2(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + substr2.str() + "\"");
    Tuple tu;
    tu.push_back(registry->storeTerm(t1));
    tu.push_back(registry->storeTerm(t2));
    answer.get().push_back(tu);
  }
};


class TestListSplitHalfAtom:
  public PluginAtom
{
public:
  TestListSplitHalfAtom():
    PluginAtom("testListSplitHalf", true) // monotonic, as there is no predicate input anyway
  {
    addInputConstant();
    setOutputArity(2);

    prop.functional = true;
    prop.wellorderingStrlen.insert(std::pair<int, int>(0, 0));
    prop.wellorderingStrlen.insert(std::pair<int, int>(0, 1));
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	const std::string& str = registry->terms.getByID(query.input[0]).getUnquotedString();

	int len = 0;
	if (str.length() > 0) len++;
	for (uint32_t i = 0; i < str.length(); i++){
		if (str[i] == ';') len++;
	}

	int cnt = len / 2;

	std::stringstream substr1, substr2;
	int nr = 0;
	for (uint32_t i = 0; i < str.length(); i++){
		if (str[i] == ';'){
			nr++;
			if (nr == cnt) continue;
		}
		if (nr >= cnt) substr2 << str[i];
		else substr1 << str[i];
	}

    Term t1(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + substr1.str() + "\"");
    Term t2(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + substr2.str() + "\"");
    Tuple tu;
    tu.push_back(registry->storeTerm(t1));
    tu.push_back(registry->storeTerm(t2));
    answer.get().push_back(tu);
  }
};

class TestListMergeAtom:
  public PluginAtom
{
public:
  TestListMergeAtom():
    PluginAtom("testListMerge", true) // monotonic, as there is no predicate input anyway
  {
    addInputConstant();
    addInputConstant();
    addInputConstant();
    setOutputArity(2);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	const std::string& str1 = registry->terms.getByID(query.input[1]).getUnquotedString();
	const std::string& str2 = registry->terms.getByID(query.input[2]).getUnquotedString();

	std::stringstream element1, element2, merged;
	uint32_t c1 = 0;
	uint32_t c2 = 0;
	
	std::vector<std::string> list1, list2;
	
	// extract list 1
	while(str1[c1] != '\0'){
		if (str1[c1] == ';') c1++;
		element1.str("");
		while (str1[c1] != ';' && str1[c1] != '\0'){
			element1 << str1[c1];
			c1++;
		}
		list1.push_back(element1.str());
	}
	
	// extract list 2
	while(str2[c2] != '\0'){
		if (str2[c2] == ';') c2++;
		element2.str("");
		while (str2[c2] != ';' && str2[c2] != '\0'){
			element2 << str2[c2];
			c2++;
		}
		list2.push_back(element2.str());
	}
	
	// merge
	c1 = 0;
	c2 = 0;
	while (c1 < list1.size() || c2 < list2.size()){
		if (c1 > 0 || c2 > 0) merged << ";";
		if (c1 == list1.size()){
			merged << list2[c2];
			c2++;
		}
		else if (c2 == list2.size()){
			merged << list1[c1];
			c1++;
		}
		else if (list1[c1].compare(list2[c2]) < 0){
			merged << list1[c1];
			c1++;
		}
		else{
			assert(list1[c1].compare(list2[c2]) >= 0);
			merged << list2[c2];
			c2++;
		}
	}

	Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + merged.str() + "\"");
    Tuple tu;
    tu.push_back(query.input[0]);
    tu.push_back(registry->storeTerm(t));
    answer.get().push_back(tu);
  }
};

class TestSubstrAtom:
  public PluginAtom
{
public:
  TestSubstrAtom():
    PluginAtom("testSubstr", true) // monotonic, as there is no predicate input anyway
  {
    addInputConstant();
    addInputConstant();
    addInputConstant();
    setOutputArity(1);

    prop.functional = true;
    prop.wellorderingStrlen.insert(std::pair<int, int>(0, 0));
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    if (!query.input[1].isIntegerTerm()) throw GeneralError("testSubstr expects an integer as its second argument");
    if (!query.input[2].isIntegerTerm()) throw GeneralError("testSubstr expects an integer as its third argument");

    try{
      int start = query.input[1].address;
      int len = query.input[2].address;
      std::string str = registry->terms.getByID(query.input[0]).getUnquotedString();
      int clen = str.length() - start;
      if (clen < len) len = clen;
      std::string substring = str.substr(start, len);
      if (registry->terms.getByID(query.input[0]).isQuotedString()) substring = "\"" + substring + "\"";
      ID resultterm = registry->storeConstantTerm(substring);
      Tuple tu;
      tu.push_back(resultterm);
      answer.get().push_back(tu);
    }catch(...){
      // specified substring is out of bounds
      // return nothing
    }
  }
};

class TestSmallerThanAtom:
  public PluginAtom
{
public:
  TestSmallerThanAtom():
    PluginAtom("testSmallerThan", true) // monotonic, as there is no predicate input anyway
  {
    addInputConstant();
    addInputConstant();
    setOutputArity(0);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    if (query.input[0].isIntegerTerm() && query.input[1].isIntegerTerm()){
      // integer comparison
      if (query.input[0].address < query.input[1].address){
        Tuple tu;
        answer.get().push_back(tu);
      }
    }else{
      // string comparison
      std::string str1 = registry->terms.getByID(query.input[0]).getUnquotedString();
      std::string str2 = registry->terms.getByID(query.input[1]).getUnquotedString();
      if (str1.compare(str2) < 0){
        Tuple tu;
        answer.get().push_back(tu);
      }
    }
  }
};

class TestFirstAtom:
  public PluginAtom
{
public:
  TestFirstAtom():
    PluginAtom("testFirst", true) // monotonic, as there is no predicate input anyway
  {
    addInputConstant();
    setOutputArity(2);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    std::string str = registry->terms.getByID(query.input[0]).getUnquotedString();

    Tuple tu;
    tu.push_back(registry->storeConstantTerm("\"" + (str.length() >= 1 ? str.substr(0, 1) : "") + "\"", true));
    tu.push_back(registry->storeConstantTerm("\"" + (str.length() >= 1 ? str.substr(1) : "") + "\"", true));
    answer.get().push_back(tu);
  }
};

class TestPushAtom:
  public PluginAtom
{
public:
  TestPushAtom():
    PluginAtom("testPush", true) // monotonic, as there is no predicate input anyway
  {
    addInputConstant();
    addInputConstant();
    setOutputArity(1);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    std::string str1 = registry->terms.getByID(query.input[0]).getUnquotedString();
    std::string str2 = registry->terms.getByID(query.input[1]).getUnquotedString();

    Tuple tu;
    tu.push_back(registry->storeConstantTerm("\"" + str1 + str2 + "\""));
    answer.get().push_back(tu);
  }
};

class TestMoveAtom:
  public PluginAtom
{
public:
  TestMoveAtom():
    PluginAtom("testMove", true) // monotonic, as there is no predicate input anyway
  {
    addInputPredicate();
    addInputConstant();
    addInputConstant();
    addInputConstant();
    setOutputArity(2);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    ID state = query.input[1];
    std::string ichar = registry->terms.getByID(query.input[2]).getUnquotedString();
    std::string schar = registry->terms.getByID(query.input[3]).getUnquotedString();

    // go through the tuples of the accessability relation
    bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
    bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
    while (en < en_end){
      const OrdinaryAtom& oatom = registry->ogatoms.getByAddress(*en);
      // match?
      if (oatom.tuple[1] == state &&
          registry->terms.getByID(oatom.tuple[2]).getUnquotedString() == ichar &&
          registry->terms.getByID(oatom.tuple[3]).getUnquotedString() == schar){

          // go to this state
          Tuple tu;
          tu.push_back(oatom.tuple[4]);
          tu.push_back(oatom.tuple[5]);
          answer.get().push_back(tu);
      }
      en++;
    }
  }
};


class TestStrlenAtom:
  public PluginAtom
{
public:
  TestStrlenAtom():
    PluginAtom("testStrlen", true) // monotonic, as there is no predicate input anyway
  {
    addInputConstant();
    setOutputArity(1);

    prop.functional = true;
    prop.finiteFiber = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
    Tuple tu;
    tu.push_back(ID::termFromInteger(registry->terms.getByID(query.input[0]).getUnquotedString().length()));
    answer.get().push_back(tu);
  }
};

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
    prop.monotonicInputPredicates.insert(0);
    prop.antimonotonicInputPredicates.insert(1);
    prop.finiteOutputDomain.insert(0);
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

  class EAHeuristics : public ExternalAtomEvaluationHeuristics{
  public:
    EAHeuristics(RegistryPtr reg) : ExternalAtomEvaluationHeuristics(reg) {}
    bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed) { return true; }
  };

  class EAHeuristicsFactory : public ExternalAtomEvaluationHeuristicsFactory{
  public:
    ExternalAtomEvaluationHeuristicsPtr createHeuristics(RegistryPtr reg){ return ExternalAtomEvaluationHeuristicsPtr(new EAHeuristics(reg)); }
  };

  bool providesCustomExternalAtomEvaluationHeuristicsFactory() const { return true; }

  ExternalAtomEvaluationHeuristicsFactoryPtr getCustomExternalAtomEvaluationHeuristicsFactory() const
	{ return ExternalAtomEvaluationHeuristicsFactoryPtr(new EAHeuristicsFactory()); }

};

class TestSetMinusNonmonotonicAtom:
  public ComfortPluginAtom
{
public:
  TestSetMinusNonmonotonicAtom():
    // this nonmonotonicity is very important,
    // because this atom is definitively nonmonotonic
    // and there are testcases that fail if this is set to true!
    ComfortPluginAtom("testSetMinusNonmonotonic", false)
  {
    addInputPredicate();
    addInputPredicate();
    setOutputArity(1);
    prop.finiteOutputDomain.insert(0);
  }

  virtual void retrieve(const ComfortQuery& query, ComfortAnswer& answer)
  {
    assert(query.input.size() == 2);
    if( !query.input[0].isConstant() || !query.input[1].isConstant() )
      throw PluginError("need constant predicates as input to testSetMinusNonmonotonic!");

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
                                        s << "can only process atoms of arity 2 with testSetMinusNonmonotonic" <<
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

class TestSetMinusNonComfortAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestSetMinusNonComfortAtom():
    PluginAtom("testSetMinusNonComfort", false) // monotonic, and no predicate inputs anyway
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputPredicate();
    prop.monotonicInputPredicates.insert(0);
    prop.antimonotonicInputPredicates.insert(1);
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	static std::map<std::string, ID> ruleIDs;

	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
			tu.push_back(atom.tuple[i]);
		}
		if (atom.tuple[0] == query.input[0]){
			tuples1.push_back(tu);
		}
		if (atom.tuple[0] == query.input[1]){
			tuples2.push_back(tu);
		}
		en++;
	}

	// Learning of the nogoods
	BOOST_FOREACH (Tuple t, tuples1){
		if (std::find(tuples2.begin(), tuples2.end(), t) == tuples2.end()){
			answer.get().push_back(t);
		}
	}
  }
};

class TestSetMinusPartialAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestSetMinusPartialAtom():
    PluginAtom("testSetMinusPartial", false) // monotonic, and no predicate inputs anyway
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputPredicate();
    prop.monotonicInputPredicates.insert(0);
    prop.antimonotonicInputPredicates.insert(1);
		prop.setProvidesPartialAnswer(true);
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	static std::map<std::string, ID> ruleIDs;

	// find relevant input
	bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
	bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();

	std::vector<Tuple> tuples1true;
	std::vector<Tuple> tuples1unknown;
	std::vector<Tuple> tuples2true;
	std::vector<Tuple> tuples2unknown;
	while (en < en_end){
		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
			tu.push_back(atom.tuple[i]);
		}
		if (!query.assigned || query.assigned->getFact(*en) ) {
			// assigned
			if (query.interpretation->getFact(*en) ){
				// assigned to true?
				if (atom.tuple[0] == query.input[0]){
					tuples1true.push_back(tu);
				}
				if (atom.tuple[0] == query.input[1]){
					tuples2true.push_back(tu);
				}
			}
		}else{
			// not assigned
			if (atom.tuple[0] == query.input[0]){
				tuples1unknown.push_back(tu);
			}
			if (atom.tuple[0] == query.input[1]){
				tuples2unknown.push_back(tu);
			}
		}
		en++;
	}

	// Learning of the nogoods
	BOOST_FOREACH (Tuple t, tuples1true){
		if (std::find(tuples2true.begin(), tuples2true.end(), t) != tuples2true.end()){
			// true in first predicate, true in second --> false in the result
		}else if (std::find(tuples2unknown.begin(), tuples2unknown.end(), t) != tuples2unknown.end()){
			// true in first predicate, unknown in second --> unknown in the result
			answer.getUnknown().push_back(t);
		}else{
			// true in first predicate, false in second --> true in the result
			answer.get().push_back(t);
		}
	}
	BOOST_FOREACH (Tuple t, tuples1unknown){
		if (std::find(tuples2true.begin(), tuples2true.end(), t) == tuples2true.end()){
			// unknown in first predicate, false or unknown in second --> unknown in the result
			answer.getUnknown().push_back(t);
		}
	}
	// false in the first predicate --> false in the result
  }
};

class TestSetMinusPartialNonmonotonicAtom:  // tests user-defined external learning
  public PluginAtom
{
public:
  TestSetMinusPartialNonmonotonicAtom():
    PluginAtom("testSetMinusPartialNonmonotonic", false) // monotonic, and no predicate inputs anyway
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputPredicate();
    prop.setProvidesPartialAnswer(true);
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
        static std::map<std::string, ID> ruleIDs;

        // find relevant input
        bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
        bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();

        std::vector<Tuple> tuples1true;
        std::vector<Tuple> tuples1unknown;
        std::vector<Tuple> tuples2true;
        std::vector<Tuple> tuples2unknown;
        while (en < en_end){
                const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
                Tuple tu;
                for (uint32_t i = 1; i < atom.tuple.size(); ++i){
                        tu.push_back(atom.tuple[i]);
                }
                if (!query.assigned || query.assigned->getFact(*en) ) {
                        // assigned
                        if (query.interpretation->getFact(*en) ){
                                // assigned to true?
                                if (atom.tuple[0] == query.input[0]){
                                        tuples1true.push_back(tu);
                                }
                                if (atom.tuple[0] == query.input[1]){
                                        tuples2true.push_back(tu);
                                }
                        }
                }else{
                        // not assigned
                        if (atom.tuple[0] == query.input[0]){
                                tuples1unknown.push_back(tu);
                        }
                        if (atom.tuple[0] == query.input[1]){
                                tuples2unknown.push_back(tu);
                        }
                }
                en++;
        }

        // Learning of the nogoods
        BOOST_FOREACH (Tuple t, tuples1true){
                if (std::find(tuples2true.begin(), tuples2true.end(), t) != tuples2true.end()){
                        // true in first predicate, true in second --> false in the result
                }else if (std::find(tuples2unknown.begin(), tuples2unknown.end(), t) != tuples2unknown.end()){
                        // true in first predicate, unknown in second --> unknown in the result
                        answer.getUnknown().push_back(t);
                }else{
                        // true in first predicate, false in second --> true in the result
                        answer.get().push_back(t);
                }
        }
        BOOST_FOREACH (Tuple t, tuples1unknown){
                if (std::find(tuples2true.begin(), tuples2true.end(), t) == tuples2true.end()){
                        // unknown in first predicate, false or unknown in second --> unknown in the result
                        answer.getUnknown().push_back(t);
                }
        }
        // false in the first predicate --> false in the result
  }
};

class TestSetMinusNogoodBasedLearningAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestSetMinusNogoodBasedLearningAtom():
    PluginAtom("testSetMinusNogoodBasedLearning", false) // monotonic, and no predicate inputs anyway
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
  {
	static std::map<std::string, ID> ruleIDs;

	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
			tu.push_back(atom.tuple[i]);
		}
		if (atom.tuple[0] == query.input[0]){
			tuples1.push_back(tu);
		}
		if (atom.tuple[0] == query.input[1]){
			tuples2.push_back(tu);
		}
		en++;
	}

	// Learning of the nogoods
	BOOST_FOREACH (Tuple t, tuples1){
		if (std::find(tuples2.begin(), tuples2.end(), t) == tuples2.end()){
			answer.get().push_back(t);

			// Test: Learning based on direct definition of nogoods
			if (nogoods != NogoodContainerPtr()){
				if (query.ctx->config.getOption("ExternalLearningUser")){
					// learn that presence of t in query.input[0] and absence in query.input[1] implies presence in output
					OrdinaryAtom at1(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					at1.tuple.push_back(query.input[0]);
					for (uint32_t i = 0; i < t.size(); ++i) at1.tuple.push_back(t[i]);
					OrdinaryAtom at2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					at2.tuple.push_back(query.input[1]);
					for (uint32_t i = 0; i < t.size(); ++i) at2.tuple.push_back(t[i]);

					Nogood nogood;
					nogood.insert(NogoodContainer::createLiteral(getRegistry()->storeOrdinaryGAtom(at1).address, true));
					nogood.insert(NogoodContainer::createLiteral(getRegistry()->storeOrdinaryGAtom(at2).address, false));
					nogood.insert(ExternalLearningHelper::getOutputAtom(query, t, false));
					nogoods->addNogood(nogood);

					DBGLOG(DBG, "Learned user-defined nogood: " << nogood);
				}else{
					DBGLOG(DBG, "No user-defined learning");
				}
			}else{
				DBGLOG(DBG, "No user-defined learning");
			}
		}
	}
  }
};

class TestSetMinusNongroundNogoodBasedLearningAtom:	// tests user-defined external learning
  public PluginAtom
{

public:

  TestSetMinusNongroundNogoodBasedLearningAtom():
    PluginAtom("testSetMinusNongroundNogoodBasedLearning", false) // monotonic, and no predicate inputs anyway
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputPredicate();
    prop.monotonicInputPredicates.insert(0);
    prop.antimonotonicInputPredicates.insert(1);
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
  {
	static std::map<std::string, ID> ruleIDs;

	int arity = -1;

	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		arity = atom.tuple.size() - 1;
		Tuple tu;
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
			tu.push_back(atom.tuple[i]);
		}
		if (atom.tuple[0] == query.input[0]){
			tuples1.push_back(tu);
		}
		if (atom.tuple[0] == query.input[1]){
			tuples2.push_back(tu);
		}
		en++;
	}

	BOOST_FOREACH (Tuple t, tuples1){
		if (std::find(tuples2.begin(), tuples2.end(), t) == tuples2.end()){
			answer.get().push_back(t);
		}
	}

	// Test: Learning based on direct definition of nogoods
	if (nogoods != NogoodContainerPtr() && arity > -1){
		if (query.ctx->config.getOption("ExternalLearningUser")){
			// learn that presence of t in query.input[0] and absence in query.input[1] implies presence in output
			OrdinaryAtom at1(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
			OrdinaryAtom at2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
			at1.tuple.push_back(query.input[0]);
			at2.tuple.push_back(query.input[1]);
			Tuple t;
			for (int i = 0; i < arity; ++i){
				std::stringstream var;
				var << "X" << i;
				at1.tuple.push_back(getRegistry()->storeVariableTerm(var.str()));
				at2.tuple.push_back(getRegistry()->storeVariableTerm(var.str()));
				t.push_back(getRegistry()->storeVariableTerm(var.str()));
			}

			Nogood nogood;
			nogood.insert(NogoodContainer::createLiteral(getRegistry()->storeOrdinaryNAtom(at1).address, true, false));
			nogood.insert(NogoodContainer::createLiteral(getRegistry()->storeOrdinaryNAtom(at2).address, false, false));
			nogood.insert(NogoodContainer::createLiteral(ExternalLearningHelper::getOutputAtom(query, t, false).address, true, false));
			nogoods->addNogood(nogood);

			DBGLOG(DBG, "Learned user-defined nogood: " << nogood);
		}else{
			DBGLOG(DBG, "No user-defined learning");
		}
	}else{
		DBGLOG(DBG, "No user-defined learning");
	}
  }
};

class TestSetMinusRuleBasedLearningAtom:	// tests user-defined external learning
  public PluginAtom
{
private:
  ProgramCtx* ctx;

public:
  TestSetMinusRuleBasedLearningAtom(ProgramCtx* ctx):
    ctx(ctx),
    PluginAtom("testSetMinusRuleBasedLearning", false) // monotonic, and no predicate inputs anyway
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
  {
	static std::map<std::string, ID> ruleIDs;

	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
			tu.push_back(atom.tuple[i]);
		}
		if (atom.tuple[0] == query.input[0]){
			tuples1.push_back(tu);
		}
		if (atom.tuple[0] == query.input[1]){
			tuples2.push_back(tu);
		}
		en++;
	}

	// Test: Rule-based learning
	if (nogoods != NogoodContainerPtr()){
		if (ctx->config.getOption("ExternalLearningUser")){
			std::string rule = "out(X) :- in1(X), not in2(X).";

			if (ruleIDs.find(rule) == ruleIDs.end()){
				ruleIDs[rule] = ExternalLearningHelper::getIDOfLearningRule(ctx, rule);
			}
			ID rid = ruleIDs[rule];
			if (rid == ID_FAIL){
				DBGLOG(DBG, "Could not learn from rule because parsing failed");
				exit(0);
			}else{
				ExternalLearningHelper::learnFromRule(query, rid, ctx, nogoods);
			}
		}
	}

	BOOST_FOREACH (Tuple t, tuples1){
		if (std::find(tuples2.begin(), tuples2.end(), t) == tuples2.end()){
			answer.get().push_back(t);
		}
	}
  }
};

class TestNonmonAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestNonmonAtom():
    PluginAtom("testNonmon", false) // monotonic, and no predicate inputs anyway
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    setOutputArity(1);

    prop.finiteOutputDomain.insert(0);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
			tu.push_back(atom.tuple[i]);
		}
		if (tu.size() != 1) throw PluginError("TestNonmonAtom can only process input predicates with arity 1!");
		tuples.push_back(tu);
		en++;
	}

	Tuple t1, t2;
	t1.push_back(ID::termFromInteger(1));
	t2.push_back(ID::termFromInteger(2));

	// {} -> {2}, {1} -> {1}, {2} -> {1}, {1,2} -> {1,2}
	if (std::find(tuples.begin(), tuples.end(), t1) == tuples.end() && std::find(tuples.begin(), tuples.end(), t2) == tuples.end()){
		answer.get().push_back(t2);
	}
	if (std::find(tuples.begin(), tuples.end(), t1) != tuples.end() && std::find(tuples.begin(), tuples.end(), t2) == tuples.end()){
		answer.get().push_back(t1);
	}
	if (std::find(tuples.begin(), tuples.end(), t1) == tuples.end() && std::find(tuples.begin(), tuples.end(), t2) != tuples.end()){
		answer.get().push_back(t1);
	}
	if (std::find(tuples.begin(), tuples.end(), t1) != tuples.end() && std::find(tuples.begin(), tuples.end(), t2) != tuples.end()){
		answer.get().push_back(t1);
		answer.get().push_back(t2);
	}
  }
};

class TestNonmon2Atom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestNonmon2Atom():
    PluginAtom("testNonmon2", false) // monotonic, and no predicate inputs anyway
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
			tu.push_back(atom.tuple[i]);
		}
		if (tu.size() != 1) throw PluginError("TestNonmon2Atom can only process input predicates with arity 1!");
		tuples.push_back(tu);
		en++;
	}

	Tuple t1, t2;
	t1.push_back(ID::termFromInteger(1));
	t2.push_back(ID::termFromInteger(2));

	// {} -> {2}, {1} -> {2}, {2} -> {}, {1,2} -> {1,2}
	if (std::find(tuples.begin(), tuples.end(), t1) == tuples.end() && std::find(tuples.begin(), tuples.end(), t2) == tuples.end()){
		answer.get().push_back(t2);
	}
	if (std::find(tuples.begin(), tuples.end(), t1) != tuples.end() && std::find(tuples.begin(), tuples.end(), t2) == tuples.end()){
		answer.get().push_back(t2);
	}
	if (std::find(tuples.begin(), tuples.end(), t1) == tuples.end() && std::find(tuples.begin(), tuples.end(), t2) != tuples.end()){
	}
	if (std::find(tuples.begin(), tuples.end(), t1) != tuples.end() && std::find(tuples.begin(), tuples.end(), t2) != tuples.end()){
		answer.get().push_back(t1);
		answer.get().push_back(t2);
	}
  }
};

class TestIdAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestIdAtom():
    PluginAtom("id", false) // monotonic
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (atom.tuple.size() != 2) throw PluginError("TestIdAtom can only process input predicates with arity 1!");
		Tuple tu;
		tu.push_back(atom.tuple[1]);
		answer.get().push_back(tu);
		en++;
	}
  }
};

class TestIdpAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestIdpAtom():
    PluginAtom("idp", false) // monotonic
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    setOutputArity(1);

    prop.setProvidesPartialAnswer(true);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {

	// find relevant input
	bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
	bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();

	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (atom.tuple.size() != 2) throw PluginError("TestIdpAtom can only process input predicates with arity 1!");

		Tuple tu;
		tu.push_back(atom.tuple[1]);
        if (query.interpretation->getFact(*en)) answer.get().push_back(tu);
        else answer.getUnknown().push_back(tu);
		en++;
	}
  }
};

class TestIdcAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestIdcAtom():
    PluginAtom("idc", false) // monotonic, and no predicate inputs anyway
  {
    addInputConstant();
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	Tuple tu;
	tu.push_back(query.input[0]);
	answer.get().push_back(tu);
  }
};

class TestNegAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestNegAtom():
    PluginAtom("neg", false)
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputConstant();
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (atom.tuple.size() != 2) throw PluginError("TestNegAtom can only process input predicates with arity 1!");
		if (atom.tuple[1] == query.input[0]){
			return;
		}
		en++;
	}
	Tuple tu;
	tu.push_back(query.input[0]);
	answer.get().push_back(tu);
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

class TestLessThanAtom:
	public PluginAtom
{
public:
  TestLessThanAtom():
		PluginAtom("testLessThan", false)
  {
	addInputPredicate();
	addInputPredicate();
	setOutputArity(0);

	prop.antimonotonicInputPredicates.insert(0);
  }

  virtual void
  retrieve(const Query& query, Answer& answer)
  {
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	int a = 0;
	int b = 0;
	while (en < en_end){
		
		if (getRegistry()->ogatoms.getByAddress(*en).tuple[0] == query.input[0]){
			a++;
		}else{
			b++;
		}
		en++;
	}

	if (a < b){
		// succeed by returning an empty tuple
		Tuple t;
		answer.get().push_back(t);
	}else{
		// fail by returning no tuple
	}
  }
};

class TestEqualAtom:
	public PluginAtom
{
public:
  TestEqualAtom():
		PluginAtom("testEqual", false)
  {
	addInputPredicate();
	addInputPredicate();
	setOutputArity(0);

	prop.antimonotonicInputPredicates.insert(0);
  }

  virtual void
  retrieve(const Query& query, Answer& answer)
  {
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	int a = 0;
	int b = 0;
	while (en < en_end){
		
		if (getRegistry()->ogatoms.getByAddress(*en).tuple[0] == query.input[0]){
			a++;
		}else{
			b++;
		}
		en++;
	}

	if (a == b){
		// succeed by returning an empty tuple
		Tuple t;
		answer.get().push_back(t);
	}else{
		// fail by returning no tuple
	}
  }
};

class TestTransitiveClosureAtom:
	public PluginAtom
{
public:
	TestTransitiveClosureAtom():
		// monotonic, as only constant inputs
		PluginAtom("testTransitiveClosure", true)
	{
			addInputPredicate();
			setOutputArity(2);

			prop.monotonicInputPredicates.insert(0);
	}

	virtual void
	retrieve(const Query& query, Answer& answer)
	{
		assert(query.input.size() == 1);

		std::set<ID> nodes;
		std::set<std::pair<ID, ID> > edges;

		bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
		bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
		while (en < en_end){
			const OrdinaryAtom& ogatom = getRegistry()->ogatoms.getByAddress(*en);

			nodes.insert(ogatom.tuple[1]);
			nodes.insert(ogatom.tuple[2]);
			edges.insert(std::pair<ID, ID>(ogatom.tuple[1], ogatom.tuple[2]));
			en++;
		}

		BOOST_FOREACH (ID n, nodes){
			BOOST_FOREACH (ID m, nodes){
				BOOST_FOREACH (ID o, nodes){
					if (std::find(edges.begin(), edges.end(), std::pair<ID, ID>(n, m)) != edges.end() &&
					    std::find(edges.begin(), edges.end(), std::pair<ID, ID>(m, o)) != edges.end()){
						Tuple t;
						t.push_back(n);
						t.push_back(o);
						answer.get().push_back(t);
					}
				}
			}
		}
	}
};

class TestCycleAtom:
	public PluginAtom
{
public:
	TestCycleAtom():
		// monotonic, as only constant inputs
		PluginAtom("testCycle", true)
	{
			addInputPredicate();
			addInputConstant();
			setOutputArity(0);

			prop.monotonicInputPredicates.insert(0);
	}

	bool dfscycle(bool directed, ID parent, ID node, std::map<ID, std::set<ID> >& outedges, std::map<ID, bool>& visited, std::set<std::pair<ID, ID> >& cycle){

		// if the node was already visited in the dfs search, then we have a cycle
		if (visited[node]) return true;

		// otherwise: visit the node
		visited[node] = true;

		// visit all child nodes
		BOOST_FOREACH (ID child, outedges[node]){
			cycle.insert(std::pair<ID, ID>(node, child));
			if (directed || child != parent){
				if (dfscycle(directed, node, child, outedges, visited, cycle)) return true;
			}
			cycle.erase(std::pair<ID, ID>(node, child));
		}

		visited[node] = false;
		return false;
	}

	virtual void
	retrieve(const Query& query, Answer& answer)
	{
		assert(query.input.size() == 1);

		Term dir(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "directed");
		bool directed = query.input[1] == getRegistry()->storeTerm(dir);

		std::set<ID> nodes;
		std::map<ID, std::set<ID> > outedges;
		std::map<ID, bool> visited;
		std::set<std::pair<ID, ID> > cycle;

		bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
		bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
		while (en < en_end){
			const OrdinaryAtom& ogatom = getRegistry()->ogatoms.getByAddress(*en);

			nodes.insert(ogatom.tuple[1]);
			nodes.insert(ogatom.tuple[2]);
			outedges[ogatom.tuple[1]].insert(ogatom.tuple[2]);
			if (!directed) outedges[ogatom.tuple[2]].insert(ogatom.tuple[1]);
			en++;
		}

		BOOST_FOREACH (ID n, nodes){
			if (dfscycle(directed, ID_FAIL, n, outedges, visited, cycle)){
				Tuple t;
				answer.get().push_back(t);
			}
		}
	}
};

class TestAppendAtom:
	public PluginAtom
{
public:
  TestAppendAtom():
		PluginAtom("testAppend", true)
  {
	addInputPredicate();
	addInputConstant();
	setOutputArity(1);

	prop.antimonotonicInputPredicates.insert(0);
  }

  virtual void
  retrieve(const Query& query, Answer& answer)
  {
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	while (en < en_end){

		std::string str = getRegistry()->terms.getByID(getRegistry()->ogatoms.getByAddress(*en).tuple[1]).getUnquotedString();
		str = str + getRegistry()->terms.getByID(query.input[1]).getUnquotedString();
		Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, str);
		ID idout = getRegistry()->storeTerm(term);
		Tuple t;
		t.push_back(idout);
		answer.get().push_back(t);

		en++;
	}
  }
};

class TestDisjAtom:
  public PluginAtom
{
public:
  TestDisjAtom():
    PluginAtom("testDisj", false)
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();	// interpretation i
    addInputPredicate();	// positive p
    addInputPredicate();	// negative n
    // The external atom implements the following disjunction:
    // (bigvee_{\vec{t} \in ext(p)} i(\vec{t})) \vee (bigvee_{\vec{t} \in ext(n)} \naf i(\vec{t}))
    // Example: &testDisj[i, p, n]() with p(1), p(2), n(0) is true iff i(1) \vee i(2) \vee \naf i(0) holds
    setOutputArity(0);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (atom.tuple[0] == query.input[1]){
			OrdinaryAtom iatom = atom;
			iatom.tuple[0] = query.input[0];
			if (query.interpretation->getFact(getRegistry()->storeOrdinaryGAtom(iatom).address)){
				Tuple t;
				answer.get().push_back(t);
				return;
			}
		}
		if (atom.tuple[0] == query.input[2]){
			OrdinaryAtom iatom = atom;
			iatom.tuple[0] = query.input[0];
			if (!query.interpretation->getFact(getRegistry()->storeOrdinaryGAtom(iatom).address)){
				Tuple t;
				answer.get().push_back(t);
				return;
			}
		}
		en++;
	}
  }
};

class TestHashAtom:
  public PluginAtom
{
public:
  TestHashAtom():
    PluginAtom("testHash", false)
  {
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::size_t hashValue = 0;

	while (en < en_end){
		const OrdinaryAtom& ogatom = getRegistry()->ogatoms.getByAddress(*en);
		BOOST_FOREACH (ID t, ogatom.tuple){
			boost::hash_combine(hashValue, t.address);
		}
		en++;
	}

	std::stringstream ss;
	ss << "h" << hashValue;
	Tuple t;
	t.push_back(getRegistry()->storeConstantTerm(ss.str()));
	answer.get().push_back(t);
  }
};

// just always true and takes 5 constant inputs
class TestTrueMultiInpAtom:
  public PluginAtom
{
public:
  TestTrueMultiInpAtom():
    PluginAtom("testTrueMultiInp", true)
  {
    addInputConstant();
    addInputConstant();
    addInputConstant();
    addInputConstant();
    addInputConstant();
    setOutputArity(0);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
		Tuple t;
		answer.get().push_back(t);
  }
};

// takes 5 constant inputs, just always returns integer 1 in output
class TestTrueMultiInpAtom2:
  public PluginAtom
{
public:
  TestTrueMultiInpAtom2():
    PluginAtom("testTrueMultiInp2", true)
  {
    addInputConstant();
    addInputConstant();
    addInputConstant();
    addInputConstant();
    addInputConstant();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
		Tuple t;
		t.push_back(ID::termFromInteger(1));
		answer.get().push_back(t);
  }
};

class TestReachableAtom:
  public PluginAtom
{
public:
  TestReachableAtom():
    PluginAtom("testReachable", true)
  {
    addInputPredicate();
    addInputConstant();
    setOutputArity(1);
    
    prop.relativeFiniteOutputDomain.insert(std::pair<int, int>(0, 0));
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
		bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
		bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

		while (en < en_end){
			const OrdinaryAtom& ogatom = getRegistry()->ogatoms.getByAddress(*en);
			if (ogatom.tuple[1] == query.input[1]){
				Tuple t;
				t.push_back(ogatom.tuple[2]);
				answer.get().push_back(t);
			}
			en++;
		}
  }
};



class TestDLSimulatorAtom:
  public PluginAtom
{
public:
  TestDLSimulatorAtom():
    PluginAtom("testDLSimulator", false)
  {
    addInputConstant();		// mode: 1=concept retrieval, 0=consistency check
    addInputConstant();		// domain size: all even domain elements are non-Fliers, all odd domain elements are Fliers
    addInputPredicate();	// plus-concept Flier
    setOutputArity(1);		// \neg Flier
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
		if (query.input[0].address == 0){
			// consistency check: check if an odd element is in plus-concept; if yes, then we have inconsistency
			bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
			bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

			while (en < en_end){
				const OrdinaryAtom& ogatom = getRegistry()->ogatoms.getByAddress(*en);
				if (ogatom.tuple[1].address % 2 == 0) return;	// inconsistent
				en++;
			}
			Tuple t;
			t.push_back(ID::termFromInteger(0));
			answer.get().push_back(t);	// consistent
		}else{
			// concept \neg C query

			// add all even elements up to the specified size; if inconsistent, then add also all odd elements
			bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
			bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

			bool inc = false;
			while (en < en_end){
				const OrdinaryAtom& ogatom = getRegistry()->ogatoms.getByAddress(*en);
				if (ogatom.tuple[1].address % 2 == 0) inc = true;	// inconsistent
				en++;
			}

			for (uint32_t i = 0; i <= query.input[1].address; ++i){
				if (i % 2 == 0 || inc){
					Tuple t;
					t.push_back(ID::termFromInteger(i));
					answer.get().push_back(t);
				}
			}
		}
  }
};




// Common base class for cautious and brave queries.
// The only difference between answering cautious and brave queries
// concerns the aggregation of the answer sets of the subprogram,
// thus almost everything is implemented in this class.
class TestASPQueryAtom:
  public PluginAtom
{
private:
	ProgramCtx& ctx;

public:
  TestASPQueryAtom(ProgramCtx& ctx, std::string atomName):
    ctx(ctx), PluginAtom(atomName, false /* not monotonic */)
  {
    addInputConstant();		// program file
    addInputPredicate();	// input interpretation
    addInputConstant();		// query predicate
    setOutputArity(0);

		prop.variableOutputArity = true;					// the output arity of this external atom depends on the arity of the query predicate
		prop.supportSets = true;									// we provide support sets
                prop.onlySafeSupportSets = true;
		prop.completePositiveSupportSets = true;	// we even provide (positive) complete support sets
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
		assert(false);
	}


  virtual void learnSupportSets(const Query& query, NogoodContainerPtr nogoods)
  {
    Answer ans;
    retrieveOrLearnSupportSets(query, ans, nogoods, true);
  }

  virtual void retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
  {
    retrieveOrLearnSupportSets(query, answer, nogoods, false);
  }

  virtual void retrieveOrLearnSupportSets(const Query& query, Answer& answer, NogoodContainerPtr nogoods, bool learnSupportSets)
  {
		RegistryPtr reg = getRegistry();

		// input parameters to external atom &testCautiousQuery["prog", p, q](x):
		//	query.input[0] (i.e. "prog"): filename of the program P over which we do query answering
		//	query.input[1] (i.e. p): a predicate name; the set F of all atoms over this predicate are added to P as facts before evaluation
		//	query.input[2] (i.e. q): name of the query predicate; the external atom will be true for all output vectors x such that q(x) is true in every answer set of P \cup F

		// read the subprogram from the file
		InputProviderPtr ip(new InputProvider());
		ip->addFileInput(getRegistry()->terms.getByID(query.input[0]).getUnquotedString());

		// prepare data structures for the subprogram P
		ProgramCtx pc = ctx;
		pc.idb.clear();
		pc.edb = InterpretationPtr(new Interpretation(reg));
		pc.currentOptimum.clear();
		pc.config.setOption("NumberOfModels",0);
                pc.config.setOption("TransUnitLearning",0);;
		pc.inputProvider = ip;
		ip.reset();

		// add already assigned facts F to the EDB of P
                if (!query.assigned) pc.edb->getStorage() |= query.interpretation->getStorage();
                else pc.edb->getStorage() |= (query.interpretation->getStorage() & query.assigned->getStorage());

                // for yet unassigned facts, add a guess
                if (!!query.assigned) {
                    bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
                    bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
                    while (en < en_end) {
                        if (!query.assigned->getFact(*en)) {
                            Rule guess(ID::MAINKIND_RULE | ID::PROPERTY_RULE_DISJ);
                            guess.head.push_back(registry->ogatoms.getIDByAddress(*en));
                            guess.head.push_back(registry->getAuxiliaryAtom('x', guess.head[0]));
                            pc.idb.push_back(registry->storeRule(guess));
                        }
                        en++;
                    }
                }

		// compute all answer sets of P \cup F
		std::vector<InterpretationPtr> answersets = ctx.evaluateSubprogram(pc, true);

		// learn support sets (only if --supportsets option is specified on the command line)
		if (learnSupportSets && !!nogoods && query.ctx->config.getOption("SupportSets")){
			SimpleNogoodContainerPtr preparedNogoods = SimpleNogoodContainerPtr(new SimpleNogoodContainer());

			// for all rules r of P
			BOOST_FOREACH (ID ruleID, pc.idb){
				const Rule& rule = reg->rules.getByID(ruleID);

				// Check if r is a rule of form
				//			hatom :- B,
				// where hatom is a single atom and B contains only positive atoms.
				bool posBody = true;
				BOOST_FOREACH (ID b, rule.body){
					if (b.isNaf()) posBody = false;
				}
				if (rule.head.size() == 1 /*&& posBody*/){
					// We learn the following (nonground) nogoods: { T b | b \in B } \cup { F hatom }.
					Nogood nogood;

					// add all (positive) body atoms
					BOOST_FOREACH (ID blit, rule.body) nogood.insert(NogoodContainer::createLiteral(blit));

					// add the negated head atom
					nogood.insert(NogoodContainer::createLiteral(rule.head[0] | ID(ID::NAF_MASK, 0)));

					// actually learn this nogood
					DBGLOG(DBG, "Learn prepared nogood " << nogood.getStringRepresentation(reg));
					preparedNogoods->addNogood(nogood);
				}
			}

			// exhaustively generate all resolvents of the prepared nogoods
			DBGLOG(DBG, "Computing resolvents of prepared nogoods up to size " << (query.interpretation->getStorage().count() + 1));
			preparedNogoods->addAllResolvents(reg, query.interpretation->getStorage().count() + 1);

			// all nogoods of form
			//		{ T b | b \in B } \cup { F q(X) }
			// containing only atoms over p and q are transformed into support sets of form
			//		{ T b | b \in B } \cup { F e_{&testCautiousQuery["prog", p, q]}(X) }
			// This is because if all body atoms are in the input (atoms over predicate p), then q(X) is true in every answer set of P \cup F.
			// But then, since q is the query predicate, also &testCautiousQuery["prog", p, q](X) is true.
			DBGLOG(DBG, "Extracting support sets from prepared nogoods");
			for (int i = 0; i < preparedNogoods->getNogoodCount(); i++){
				const Nogood& ng = preparedNogoods->getNogood(i);
				bool isSupportSet = true;
				Nogood supportSet;
				BOOST_FOREACH (ID id, ng){
					ID pred = reg->lookupOrdinaryAtom(id).tuple[0];
					if (pred == query.input[1]){
						supportSet.insert(id);
					}else if (pred == query.input[2]){
						const OrdinaryAtom& hatom = reg->lookupOrdinaryAtom(id);
						// add e_{&testCautiousQuery["prog", p, q]}(X) using a helper function	
						supportSet.insert(NogoodContainer::createLiteral(
																ExternalLearningHelper::getOutputAtom(
																		query,	// this parameter is always the same
																		Tuple(hatom.tuple.begin() + 1, hatom.tuple.end()),	// hatom.tuple[0]=q and hatom.tuple[i] for i >= 1 stores the elements of X;
																																												// here we need only the X and use hatom.tuple.begin() + 1 to eliminate the predicate q
																		!id.isNaf() /* technical detail, is set to true almost always */).address,
																true,																								// sign of the literal e_{&testCautiousQuery["prog", p, q]}(X) in the nogood
																id.isOrdinaryGroundAtom()															/* specify if this literal is ground or nonground (the same as the head atom) */ ));
					}else{
						isSupportSet = false;
						break;
					}
				}
				if (isSupportSet){
					DBGLOG(DBG, "Learn support set: " << supportSet.getStringRepresentation(reg));
					nogoods->addNogood(supportSet);
				}
			}
		}

		// create a mask for the query predicate, i.e., retrieve all atoms over the query predicate
		PredicateMaskPtr pm = PredicateMaskPtr(new PredicateMask());
		pm->setRegistry(reg);
		pm->addPredicate(query.input[2]);
		pm->updateMask();

		// now since we know all answer sets, we can answer the query
		answerQuery(pm, answersets, query, answer);
  }

  // define an abstract method for aggregating the answer sets (this part is specific for cautious and brave queries)
	virtual void answerQuery(PredicateMaskPtr pm, std::vector<InterpretationPtr>& answersets, const Query& query, Answer& answer) = 0;
};

class TestCautiousQueryAtom:
  public TestASPQueryAtom
{
public:
  TestCautiousQueryAtom(ProgramCtx& ctx):
    TestASPQueryAtom(ctx, "testCautiousQuery")
  {
      prop.providesPartialAnswer = true;
  }

	// implement the specific part
	void answerQuery(PredicateMaskPtr pm, std::vector<InterpretationPtr>& answersets, const Query& query, Answer& answer){

		RegistryPtr reg = getRegistry();

		// special case: if there are no answer sets, cautious ground queries are trivially true, but cautious non-ground queries are always false for all ground substituions (by definition)
		if (answersets.size() == 0){
			if (query.pattern.size() == 0){
				// return the empty tuple
				Tuple t;
				answer.get().push_back(t);
			}
		}else{

			InterpretationPtr out = InterpretationPtr(new Interpretation(reg));
                        InterpretationPtr outU = InterpretationPtr(new Interpretation(reg));
			out->add(*pm->mask());

			// get the set of atoms over the query predicate which are true in all answer sets
			BOOST_FOREACH (InterpretationPtr intr, answersets) {
				out->getStorage() &= intr->getStorage();
			}

                        // all other atoms, which are true in at least one answer set, might be true in all answer sets at the end
                        if (!!query.assigned && query.assigned->getStorage().count() < query.predicateInputMask->getStorage().count()) {
                            BOOST_FOREACH (InterpretationPtr intr, answersets) {
                                outU->getStorage() |= (intr->getStorage() & pm->mask()->getStorage());
                            }
                        }
                        outU->getStorage() -= out->getStorage();

			// retrieve all output atoms oatom=q(c)
			bm::bvector<>::enumerator en = out->getStorage().first();
			bm::bvector<>::enumerator en_end = out->getStorage().end();
			while (en < en_end) {
				const OrdinaryAtom& oatom = reg->ogatoms.getByAddress(*en);

				// add c to the output
				answer.get().push_back(Tuple(oatom.tuple.begin() + 1, oatom.tuple.end()));
				en++;
			}
                        en = outU->getStorage().first();
                        en_end = outU->getStorage().end();
                        while (en < en_end) {
                                const OrdinaryAtom& oatom = reg->ogatoms.getByAddress(*en);

                                // add c to the output
                                answer.getUnknown().push_back(Tuple(oatom.tuple.begin() + 1, oatom.tuple.end()));
                                en++;
                        }
		}
	}
};

class TestBraveQueryAtom:
  public TestASPQueryAtom
{
public:
  TestBraveQueryAtom(ProgramCtx& ctx):
    TestASPQueryAtom(ctx, "testCautiousBrave")
  { }

	// implement the specific part
	virtual void answerQuery(PredicateMaskPtr pm, std::vector<InterpretationPtr>& answersets, const Query& query, Answer& answer){

		RegistryPtr reg = getRegistry();

		InterpretationPtr out = InterpretationPtr(new Interpretation(reg));

		// get the set of atoms over the query predicate which are true in all answer sets
		BOOST_FOREACH (InterpretationPtr intr, answersets){
			out->getStorage() |= (pm->mask()->getStorage() & intr->getStorage());
		}

		// retrieve all output atoms oatom=q(c)
		bm::bvector<>::enumerator en = out->getStorage().first();
		bm::bvector<>::enumerator en_end = out->getStorage().end();
		while (en < en_end){
			const OrdinaryAtom& oatom = reg->ogatoms.getByAddress(*en);

			// add c to the output
			answer.get().push_back(Tuple(oatom.tuple.begin() + 1, oatom.tuple.end()));
			en++;
		}
	}
};

class TestFinalCallback:
	public FinalCallback
{
public:
	TestFinalCallback(ProgramCtx& ctx):
		ctx(ctx),
		first(true)
	{
	}

  virtual void operator()()
	{
		std::cout << "TestFinalCallback::operator()()" << std::endl;
		if( first )
		{
			// repeat
			ctx.config.setOption("RepeatEvaluation",1);
		}
		else
		{
			// don't repeat again
		}
		first = false;
	}

private:
	ProgramCtx& ctx;
	bool first;
};

class TestPlugin:
  public PluginInterface
{
public:
	struct CtxData:
		public PluginData
	{
	public:
		bool testRepetition;

	public:
		CtxData():
			testRepetition(false) {}
		virtual ~CtxData() {}
	};

public:
  TestPlugin():
    PluginInterface()
  {
    setNameVersion("dlvhex-testplugin", 0, 0, 1);
  }

  virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx& ctx)
	{
		TestPlugin::CtxData& pcd = ctx.getPluginData<TestPlugin>();

		typedef std::list<const char*>::iterator Iterator;
		Iterator it;
		WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
		it = pluginOptions.begin();
		while( it != pluginOptions.end() )
		{
			bool processed = false;
			const std::string str(*it);
			if( str == "--testplugin-test-repetition" )
			{
				pcd.testRepetition = true;
				std::cerr << "going to test repetition" << std::endl;
				processed = true;
			}

			if( processed )
			{
				it = pluginOptions.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

class TestSetUnionAtom:	// tests user-defined external learning
  public PluginAtom
{

public:

    TestSetUnionAtom():
		// testSetUnion is the name of our external atom
    PluginAtom("testSetUnion", true) // monotonic
  {
    WARNING("TODO if a plugin atom has only constant inputs, is it always monotonic? if yes, automate this, at least create a warning")
		DBGLOG(DBG,"Constructor of SetUnion plugi is started!");
    addInputPredicate(); // the first set
    addInputPredicate(); // the second set
    setOutputArity(1); // arity of the output list
  }

// function that evaluates external atom without learning
// input parameters: 
// 1. Query is a class, defined in PluginInterface.h (struct DLVHEX_EXPORT Query)
// 2. Answer is a class, defined in PluginInterface.h (struct DLVHEX_EXPORT Answer)
  virtual void retrieve(const Query& query, Answer& answer)
  {
	// find relevant input
	// Iterators (objects that mark the begin and the end of some structure)
  DBGLOG(DBG,"Retrieve function is started");
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	// go through all atoms using the iterator
	while (en < en_end){
	// extract the current atom
	// *emn is the id of the current atom, to which the iterator points	
		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
	// Iterate over the input elements of the current atom (for p(x,y), we go through x and y)
	// We start with 1 because the position 0 is the predicate itself 
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
	// Get element number i from the input list
			tu.push_back(atom.tuple[i]);
		}
	
		if (atom.tuple[0] == query.input[0]){
			tuples1.push_back(tu);
		}
		if (atom.tuple[0] == query.input[1]){
			tuples2.push_back(tu);
		}
		en++;
	}

	// for each element t of tuples1 add t to the answer 
	BOOST_FOREACH (Tuple t, tuples1){
		answer.get().push_back(t);
	}

	BOOST_FOREACH (Tuple t, tuples2){
		answer.get().push_back(t);
	}
  }


// function that evaluates external atom with learning
// input parameters: 
// 1. Query is a class, defined in PluginInterface.h (struct DLVHEX_EXPORT Query)
// 2. Answer is a class, defined in PluginInterface.h (struct DLVHEX_EXPORT Answer)
// 3. Learnt Nogoods

  virtual void retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
  {
	// find relevant input
	// Iterators (objects that mark the begin and the end of some structure)
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	// go through all atoms using the iterator
	while (en < en_end){
	// extract the current atom
	// *emn is the id of the current atom, to which the iterator points	
		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
	// Iterate over the input elements of the current atom (for p(x,y), we go through x and y)
	// We start with 1 because the position 0 is the predicate itself 
		for (uint32_t i = 1; i < atom.tuple.size(); ++i){
	// Get element number i from the input list
			tu.push_back(atom.tuple[i]);
		}
	
		if (atom.tuple[0] == query.input[0]){
			tuples1.push_back(tu);
		}
		if (atom.tuple[0] == query.input[1]){
			tuples2.push_back(tu);
		}
		en++;
	}
	
	// for each element t of tuples1 add t to the answer 
	BOOST_FOREACH (Tuple t, tuples1){
		answer.get().push_back(t);
		// G in the end stands for ground learning (N for nonground)
		// Create a new object where we store the copy of the first input predictae
		OrdinaryAtom at1(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		// Copy input predicate with the parameters to at1
		at1.tuple.push_back(query.input[0]);
		// arity is always 1 here
		BOOST_FOREACH (ID i, t) {	
			at1.tuple.push_back(i);
		}
		// Start with empty nogood
		Nogood nogood;
		// Add the first literal
		// In case of a nonground nogood, we need to store NAtom (storeOrdinaryNAtom)
		// First true is the sign of the literal
		// Second parameter is true if we create ground nogood
 
		nogood.insert(NogoodContainer::createLiteral(getRegistry()->storeOrdinaryGAtom(at1).address, true, true));

		// ExternalLearningHelper is a function that helps to create an element in a nogood for external atom: call the function for the given output tuple
		// Always the same (add the false output in case if under the input parameters the result is true)
		nogood.insert(NogoodContainer::createLiteral(ExternalLearningHelper::getOutputAtom(query, t, false).address, true, false));
		// add the nogood to the set of all nogoods if nogoods is not zero
		if (!!nogoods)
			nogoods->addNogood(nogood);
    DBGLOG(DBG,"nogood is " << nogood);

	}

	BOOST_FOREACH (Tuple t, tuples2){
		answer.get().push_back(t);
	}
  }
};

class TestGen2Atom:      // tests user-defined external learning
  public PluginAtom
{
public:
  TestGen2Atom(std::string name, int arity):
    PluginAtom(name, false)
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    for (int i = 0; i < arity; ++i) addInputConstant();
    setOutputArity(0);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	OrdinaryAtom myat(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
	myat.tuple = query.input;

        // find relevant input
	bool match = false;
        bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
        bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
        while (en < en_end){

                if (getRegistry()->ogatoms.getByAddress(*en).unifiesWith(myat)){
			match = true;
			break;
		}
		en++;
        }
        Tuple tu;
        if (match) answer.get().push_back(tu);
  }
};

class TestIsEmpty:      // tests user-defined external learning
  public PluginAtom
{
public:
  TestIsEmpty():
    PluginAtom("testIsEmpty", false)
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    setOutputArity(0);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
        OrdinaryAtom myat(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
        myat.tuple = query.input;

        // find relevant input
        bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
        bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();
        while (en < en_end){
		return;	// not empty
        }

	// empty
        Tuple tu;
        answer.get().push_back(tu);
  }
};

class TestNumberOfBalls:      // tests user-defined external learning
  public PluginAtom
{
public:
  TestNumberOfBalls():
    PluginAtom("testNumberOfBalls", false)
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputConstant();
    addInputConstant();
    setOutputArity(0);

    prop.providesPartialAnswer = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
        OrdinaryAtom myat(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
        myat.tuple = query.input;

        // find relevant input
	int tr = 0;
	int fa = 0;
	int un = 0;
        bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
        bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
        while (en < en_end){
		if ((!query.assigned || query.assigned->getFact(*en)) && query.interpretation->getFact(*en)) tr++;
		else if ((!query.assigned || query.assigned->getFact(*en)) && !query.interpretation->getFact(*en)) fa++;
		else un++;
		en++;
        }

	if (tr >= query.input[1].address && (tr + un) <= query.input[2].address){
		// true
	        Tuple tu;
        	answer.get().push_back(tu);
	}else if ((tr + un) >= query.input[1].address && tr <= query.input[2].address){
		// unknwon
		Tuple tu;
		answer.getUnknown().push_back(tu);
	}else{
		// false
	}
  }
};

class TestNumberOfBallsSE:      // tests user-defined external learning
  public PluginAtom
{
public:
  TestNumberOfBallsSE():
    PluginAtom("testNumberOfBallsSE", false)
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputConstant();
    setOutputArity(0);

    prop.providesPartialAnswer = true;
    prop.antimonotonicInputPredicates.insert(0);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
        OrdinaryAtom myat(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
        myat.tuple = query.input;

        // find relevant input
        int tr = 0;
        int fa = 0;
        int un = 0;
        bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
        bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
        while (en < en_end){
                if ((!query.assigned || query.assigned->getFact(*en)) && query.interpretation->getFact(*en)) tr++;
                else if ((!query.assigned || query.assigned->getFact(*en)) && !query.interpretation->getFact(*en)) fa++;
                else un++;
                en++;
        }

        if ((tr + un) <= query.input[1].address){
                // true
                Tuple tu;
                answer.get().push_back(tu);
        }else if (tr <= query.input[1].address){
                // unknwon
                Tuple tu;
                answer.getUnknown().push_back(tu);
        }else{
                // false
        }
  }
};

class TestNumberOfBallsGE:      // tests user-defined external learning
  public PluginAtom
{
public:
  TestNumberOfBallsGE():
    PluginAtom("testNumberOfBallsGE", false)
  {
    WARNING("TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning")
    addInputPredicate();
    addInputConstant();
    setOutputArity(0);

    prop.providesPartialAnswer = true;
    prop.monotonicInputPredicates.insert(0);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
        OrdinaryAtom myat(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN);
        myat.tuple = query.input;

        // find relevant input
        int tr = 0;
        int fa = 0;
        int un = 0;
        bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
        bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
        while (en < en_end){
                if ((!query.assigned || query.assigned->getFact(*en)) && query.interpretation->getFact(*en)) tr++;
                else if ((!query.assigned || query.assigned->getFact(*en)) && !query.interpretation->getFact(*en)) fa++;
                else un++;
                en++;
        }

        if (tr >= query.input[1].address) {
                // true
                Tuple tu;
                answer.get().push_back(tu);
        }else if ((tr + un) >= query.input[1].address) {
                // unknwon
                Tuple tu;
                answer.getUnknown().push_back(tu);
        }else{
                // false
        }
  }
};


    class SumNonZeroAtom : public PluginAtom
    {
        public:
            SumNonZeroAtom() : PluginAtom("sumD0", 1)
            {
                addInputPredicate();
                setOutputArity(1);
            }
      
            virtual void
            retrieve(const Query& query, Answer& answer) throw (PluginError)
            {
                Registry &registry = *getRegistry();
    
                int sum = 0;
                int pos = query.interpretation.get()->getStorage().get_first();
                while (pos != 0){
                    const OrdinaryAtom& oatom = registry.ogatoms.getByAddress(pos);
                    if(oatom.tuple[1].address == 0)
                        sum += oatom.tuple[2].address;
                    else
                        sum -= oatom.tuple[2].address;
                    pos = query.interpretation.get()->getStorage().get_next(pos);
                }
        
                Tuple out;
                out.push_back(ID::termFromInteger(sum == 0 ? 0 : 1));
                answer.get().push_back(out);
            }
    };

    class ProductionRequirementsAtom : public PluginAtom
    {
        public:
            ProductionRequirementsAtom() : PluginAtom("getreq", false)
            {
                addInputPredicate();
                addInputPredicate();
                setOutputArity(1);

                prop.setProvidesPartialAnswer(true);
            }
      
            virtual void
            retrieve(const Query& query, Answer& answer) throw (PluginError)
            {
                Registry &registry = *getRegistry();

                std::set<ID> produced;
                std::set<ID> possiblyproduced;
                std::set<ID> allrequirements;

                ID const_p = registry.storeConstantTerm("p");
                ID const_n = registry.storeConstantTerm("n");

                // extract requirements and production plan
                bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
                bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
                while (en < en_end){
                    ID id = registry.ogatoms.getIDByAddress(*en);
                    const OrdinaryAtom& ogatom = registry.ogatoms.getByAddress(*en);
                    if (ogatom.tuple[0] == query.input[0]){
                        if (!query.assigned || query.assigned->getFact(*en)){
                            if (query.interpretation->getFact(*en)) { produced.insert(ogatom.tuple[1]); }
                        } else { possiblyproduced.insert(ogatom.tuple[1]); }
                    }
                    if (ogatom.tuple[0] == query.input[1]){
                        allrequirements.insert(ogatom.tuple[1]);
                    }
                    en++;
                }

                // decide for each requirement if it is true, false or unknown
                BOOST_FOREACH (ID req, allrequirements) {
                    en = query.predicateInputMask->getStorage().first();
                    en_end = query.predicateInputMask->getStorage().end();
                    while (en < en_end){
                        const OrdinaryAtom& ogatom = registry.ogatoms.getByAddress(*en);
                        if (ogatom.tuple[0] == query.input[1] && ogatom.tuple[1] == req){
                            if (ogatom.tuple[2] != const_p) throw PluginError("requirements specification must be of form req(Name, p, ..., n, ...)");
                            bool cursat = true;
                            bool curviolated = false;
                            bool pos = true;
                            for (int i = 3; i < ogatom.tuple.size(); ++i) {
                                // switch to negative requirements
                                if (ogatom.tuple[i] == const_n) {
                                    pos = false;
                                    continue;
                                }
                                // check for satisfaction of the current product
                                cursat &= (pos && produced.find(ogatom.tuple[i]) != produced.end()) || (!pos && produced.find(ogatom.tuple[i]) == produced.end() && possiblyproduced.find(ogatom.tuple[i]) == possiblyproduced.end());
                                curviolated |= (pos && produced.find(ogatom.tuple[i]) == produced.end() && possiblyproduced.find(ogatom.tuple[i]) == possiblyproduced.end()) || (!pos && produced.find(ogatom.tuple[i]) != produced.end());
                            }
                            assert (!(cursat && curviolated) && "precondition for requirement is satisfied and violated at the same time");
                            // requirement is definitely true
                            if (cursat) {
                                Tuple out;
                                out.push_back(req);
                                answer.get().push_back(out);
                                break;
                            }
                            // requirement could be true
                            if (!curviolated) {
                                Tuple out;
                                out.push_back(req);
                                answer.getUnknown().push_back(out);
                                break;
                            }
                        }
                        en++;
                    }
                }
            }
    };

    class MappingAtom : public PluginAtom
    {
        public:
            MappingAtom() : PluginAtom("mapping", false)
            {
                addInputPredicate();
                addInputPredicate();
                setOutputArity(1);

                prop.setProvidesPartialAnswer(true);
            }

            virtual void
            retrieve(const Query& query, Answer& answer) throw (PluginError)
            {
                Registry &registry = *getRegistry();

                std::set<ID> selected;
                std::set<ID> possiblyselected;
                std::set<ID> alltags;

                ID const_p = registry.storeConstantTerm("p");
                ID const_n = registry.storeConstantTerm("n");

                // extract requirements and production plan
                bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
                bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
                while (en < en_end){
                    ID id = registry.ogatoms.getIDByAddress(*en);
                    const OrdinaryAtom& ogatom = registry.ogatoms.getByAddress(*en);
                    if (ogatom.tuple[0] == query.input[0]){
                        if (!query.assigned || query.assigned->getFact(*en)){
                            if (query.interpretation->getFact(*en)) { selected.insert(ogatom.tuple[1]); }
                        } else { possiblyselected.insert(ogatom.tuple[1]); }
                    }
                    if (ogatom.tuple[0] == query.input[1]){
                        alltags.insert(ogatom.tuple[1]);
                    }
                    en++;
                }

                // decide for each requirement if it is true, false or unknown
                BOOST_FOREACH (ID req, alltags) {
                    en = query.predicateInputMask->getStorage().first();
                    en_end = query.predicateInputMask->getStorage().end();
                    while (en < en_end){
                        const OrdinaryAtom& ogatom = registry.ogatoms.getByAddress(*en);
                        if (ogatom.tuple[0] == query.input[1] && ogatom.tuple[1] == req){
                            if (ogatom.tuple[2] != const_p) throw PluginError("tags specification must be of form tags(t, p, ..., n, ...)");
                            bool cursat = true;
                            bool curviolated = false;
                            bool pos = true;
                            for (int i = 3; i < ogatom.tuple.size(); ++i) {
                                // switch to negative requirements
                                if (ogatom.tuple[i] == const_n) {
                                    pos = false;
                                    continue;
                                }
                                // check for satisfaction of the current product
                                cursat &= (pos && selected.find(ogatom.tuple[i]) != selected.end()) || (!pos && selected.find(ogatom.tuple[i]) == selected.end() && possiblyselected.find(ogatom.tuple[i]) == possiblyselected.end());
                                curviolated |= (pos && selected.find(ogatom.tuple[i]) == selected.end() && possiblyselected.find(ogatom.tuple[i]) == possiblyselected.end()) || (!pos && selected.find(ogatom.tuple[i]) != selected.end());
                            }
                            assert (!(cursat && curviolated) && "precondition for tag is satisfied and violated at the same time");
                            // requirement is definitely true
                            if (cursat) {
                                Tuple out;
                                out.push_back(req);
                                answer.get().push_back(out);
                                break;
                            }
                            // requirement could be true
                            if (!curviolated) {
                                Tuple out;
                                out.push_back(req);
                                answer.getUnknown().push_back(out);
                                break;
                            }
                        }
                        en++;
                    }
                }
            }
    };

    class GetSizesAtom : public PluginAtom
    {
        public:
            GetSizesAtom() : PluginAtom("getSizes", false)
            {
                addInputPredicate();
                setOutputArity(2);

                prop.setProvidesPartialAnswer(true);
            }

            virtual void
            retrieve(const Query& query, Answer& answer) throw (PluginError)
            {
                Registry &registry = *getRegistry();

                // for all input atoms
                std::vector<ID> keys;
                typedef boost::unordered_map<ID, int> map;
                map trueCount, unknownCount;
                bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
                bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
                while (en < en_end) {
                    ID id = registry.ogatoms.getIDByAddress(*en);
                    const OrdinaryAtom& ogatom = registry.ogatoms.getByAddress(*en);
                    if (ogatom.tuple[0] == query.input[0]){
                        if (ogatom.tuple.size() != 3) { throw PluginError("Input must be of arity 2"); }
                        keys.push_back(ogatom.tuple[2]);
                        if ( (!query.assigned || query.assigned->getFact(*en)) && query.interpretation->getFact(*en) ) {
                            trueCount[ogatom.tuple[2]]++;
                        }else if (!!query.assigned && !query.assigned->getFact(*en)){
                            unknownCount[ogatom.tuple[2]]++;
                        }
                    }
                    en++;
                }

                BOOST_FOREACH (ID k, keys) {
                    int min = trueCount[k];
                    int max = min + unknownCount[k];
                    if (min == max) {
                        Tuple t;
                        t.push_back(k);
                        t.push_back(ID::termFromInteger(min));
                        answer.get().push_back(t);
                    }else{
                        for (int i = min; i <= max; i++) {
                            Tuple t;
                            t.push_back(k);
                            t.push_back(ID::termFromInteger(i));
                            answer.getUnknown().push_back(t);
                        }
                    }
                }
            }
    };

    class GetSizesRestrAtom : public PluginAtom
    {
        public:
            GetSizesRestrAtom() : PluginAtom("getSizesRestr", false)
            {
                addInputPredicate();
								addInputPredicate();
                setOutputArity(2);

								prop.monotonicInputPredicates.insert(0);
                prop.setProvidesPartialAnswer(true);
            }

            virtual void
            retrieve(const Query& query, Answer& answer) throw (PluginError)
            {
                Registry &registry = *getRegistry();
								
								bool allPossibleAssigned = true;

                // for all input atoms
                std::vector<ID> keys;
                typedef boost::unordered_map<ID, int> map;
                map trueCount, unknownCount;
								boost::unordered_map<ID, std::vector<ID>> possMap;
                bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
                bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
                while (en < en_end) {
                    ID id = registry.ogatoms.getIDByAddress(*en);
                    const OrdinaryAtom& ogatom = registry.ogatoms.getByAddress(*en);

                    if (ogatom.tuple[0] == query.input[1]){
                        if (ogatom.tuple.size() != 3) { throw PluginError("Input must be of arity 2"); }

                        keys.push_back(ogatom.tuple[2]);
                        if ( (!query.assigned || query.assigned->getFact(*en)) && query.interpretation->getFact(*en) ) {
                            possMap[ogatom.tuple[2]].push_back(ogatom.tuple[1]);
                        }else if (!!query.assigned && !query.assigned->getFact(*en)){
                            allPossibleAssigned = false;
                        }
                    }
                    en++;
                }

                en = query.predicateInputMask->getStorage().first();
                en_end = query.predicateInputMask->getStorage().end();
                while (en < en_end) {
                    ID id = registry.ogatoms.getIDByAddress(*en);
                    const OrdinaryAtom& ogatom = registry.ogatoms.getByAddress(*en);

                    if (ogatom.tuple[0] == query.input[0]){
                        if (ogatom.tuple.size() != 3) { throw PluginError("Input must be of arity 2"); }

                        if ( (allPossibleAssigned && (!query.assigned || query.assigned->getFact(*en)) && query.interpretation->getFact(*en)) ) {
                            trueCount[ogatom.tuple[2]]++;
                        }else if (!allPossibleAssigned || (!!query.assigned && !query.assigned->getFact(*en) && (std::find(possMap[ogatom.tuple[2]].begin(), possMap[ogatom.tuple[2]].end(), ogatom.tuple[1]) != possMap[ogatom.tuple[2]].end()) )){
                            unknownCount[ogatom.tuple[2]]++;
                        }
                    }
                    en++;
                }

                BOOST_FOREACH (ID k, keys) {
                    int min = trueCount[k];
                    int max = min + unknownCount[k];
                    if (min == max) {
                        Tuple t;
                        t.push_back(k);
                        t.push_back(ID::termFromInteger(min));
                        answer.get().push_back(t);
                    }else{
                        for (int i = min; i <= max; i++) {
                            Tuple t;
                            t.push_back(k);
                            t.push_back(ID::termFromInteger(i));
                            answer.getUnknown().push_back(t);
                        }
                    }
                }
            }
    };

    class GetDiagnosesAtom : public PluginAtom
    {
        private:
            ProgramCtx& ctx;

        public:
            GetDiagnosesAtom(ProgramCtx& ctx) : PluginAtom("getDiagnoses", false), ctx(ctx)
            {
                addInputConstant();        // program
                addInputPredicate();       // hypotheses
                addInputPredicate();       // observation
                setOutputArity(2);

                prop.setProvidesPartialAnswer(true);
            }

            virtual void
            retrieve(const Query& query, Answer& answer) throw (PluginError)
            {
                RegistryPtr reg = getRegistry();

                // read the subprogram from the string constant
                InputProviderPtr ip(new InputProvider());
                ip->addStringInput(reg->terms.getByID(query.input[0]).getUnquotedString(), "program");

                ProgramCtx pc = ctx;
                pc.idb.clear();
                pc.edb = InterpretationPtr(new Interpretation(reg));
                pc.currentOptimum.clear();
                pc.config.setOption("NumberOfModels",0);
                pc.config.setOption("TransUnitLEarning",0);
                pc.config.setOption("ForceGC",0);
                pc.inputProvider = ip;
                ip.reset();

                bool allHypAssigned = true;
                bool allObsAssigned = true;

                // add guesses over the hypotheses and constraints over the observations
                bm::bvector<>::enumerator en = query.predicateInputMask->getStorage().first();
                bm::bvector<>::enumerator en_end = query.predicateInputMask->getStorage().end();
                while (en < en_end) {
                    ID id = registry->ogatoms.getIDByAddress(*en);
                    const OrdinaryAtom& ogatom = registry->ogatoms.getByID(id);

                    if (ogatom.tuple[0] == query.input[1]) {
                        // hypotheses must be known, otherwise we cannot tell anything
                        if (!!query.assigned && !query.assigned->getFact(*en)) {
                            allHypAssigned = false;
                        }

                        // hypothesis
                        if (query.interpretation->getFact(*en)) {
                            Rule guess(ID::MAINKIND_RULE | ID::PROPERTY_RULE_DISJ);
                            guess.head.push_back(id);
                            guess.head.push_back(registry->getAuxiliaryAtom('x', id));
                            pc.idb.push_back(registry->storeRule(guess));
                        }
                    }
                    if (ogatom.tuple[0] == query.input[2]) {
                        // observation

                        // already known?
                        if (!query.assigned || query.assigned->getFact(*en)) {
                            if (query.interpretation->getFact(*en)) {
                                Rule cons(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
                                cons.body.push_back(ID::nafLiteralFromAtom(id));
                                pc.idb.push_back(registry->storeRule(cons));
                            }
                        }else{
                            allObsAssigned = false;
                        }
                    }
                    en++;
                }

                // compute all answer sets of P \cup F
                std::vector<InterpretationPtr> answersets = ctx.evaluateSubprogram(pc, true);

                // get hypothesis which are true in all resp. at least one diagnoses
                InterpretationPtr trueInAll(new Interpretation(reg));
                InterpretationPtr trueInOne(new Interpretation(reg));
                if (answersets.size() > 0) { trueInAll->getStorage() |= answersets[0]->getStorage(); }
                BOOST_FOREACH (InterpretationPtr answerset, answersets) {
//                    std::cerr << "     D: " << *answerset << std::endl;
                    trueInAll->getStorage() &= answerset->getStorage();
                    trueInOne->getStorage() |= answerset->getStorage();
                }

                // for all hypotheses
                en = query.predicateInputMask->getStorage().first();
                en_end = query.predicateInputMask->getStorage().end();
                while (en < en_end) {
                    ID id = registry->ogatoms.getIDByAddress(*en);
                    const OrdinaryAtom& ogatom = registry->ogatoms.getByID(id);
                    if (ogatom.tuple[0] == query.input[1]) {
                        // without knowing all hypotheses, we cannot decide anything
                        if (!allHypAssigned) {
                            Tuple t;
                            t.push_back(ogatom.tuple[1]);
                            t.push_back(ID::termFromInteger(0));
                            answer.getUnknown().push_back(t);
                            t[1] = ID::termFromInteger(1);
                            answer.getUnknown().push_back(t);
                            t[1] = ID::termFromInteger(2);
                            answer.getUnknown().push_back(t);
                        }
                        else{
                            if (allObsAssigned) {
                                // if it is true in all diagnoses, it is certainly true
                                if (trueInAll->getFact(id.address) || answersets.size() == 0) {
                                    Tuple t;
                                    t.push_back(ogatom.tuple[1]);
                                    t.push_back(ID::termFromInteger(2));
                                    answer.get().push_back(t);

                                    if (trueInOne->getFact(id.address)) {
                                        t[1] = ID::termFromInteger(1);
                                        answer.get().push_back(t);
                                    }else{
                                        t[1] = ID::termFromInteger(0);
                                        answer.get().push_back(t);
                                    }
                                // otherwise, if it is true in at least one diagnosis, it can be true
                                }else if (trueInOne->getFact(id.address)) {
                                    // otherwise we do not know yet
                                    Tuple t;
                                    t.push_back(ogatom.tuple[1]);
                                    t.push_back(ID::termFromInteger(1));
                                    answer.get().push_back(t);
                                }
                                // otherwise, it is false in all diagnoses and therefore certainly false
                                else {
                                    Tuple t;
                                    t.push_back(ogatom.tuple[1]);
                                    t.push_back(ID::termFromInteger(0));
                                    answer.get().push_back(t);
                                }
                            }else{
                                // if it is true in all diagnoses, it is certainly true
                                if (trueInAll->getFact(id.address) || answersets.size() == 0) {
                                    Tuple t;
                                    t.push_back(ogatom.tuple[1]);
                                    t.push_back(ID::termFromInteger(2));
                                    answer.get().push_back(t);

                                    t[1] = ID::termFromInteger(1);
                                    answer.getUnknown().push_back(t);
                                // otherwise, if it is true in at least one diagnosis, it can be true
                                }else if (trueInOne->getFact(id.address)) {
                                    // otherwise we do not know yet
                                    Tuple t;
                                    t.push_back(ogatom.tuple[1]);
                                    t.push_back(ID::termFromInteger(1));
                                    answer.getUnknown().push_back(t);

                                    t[1] = ID::termFromInteger(2);
                                    answer.getUnknown().push_back(t);
                                }
                                // otherwise, it is false in all diagnoses and therefore certainly false
                                else {
                                    Tuple t;
                                    t.push_back(ogatom.tuple[1]);
                                    t.push_back(ID::termFromInteger(0));
                                    answer.get().push_back(t);

                                    t[1] = ID::termFromInteger(2);
                                    answer.getUnknown().push_back(t);
                                }
                            }
                        }
                    }
                    en++;
                }
            }
    };

  virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx& ctx) const
  {
    std::vector<PluginAtomPtr> ret;

		// return smart pointer with deleter (i.e., delete code compiled into this plugin)
    ret.push_back(PluginAtomPtr(new TestAAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestBAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestCAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestZeroArityAtom("testZeroArity0", false), PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestZeroArityAtom("testZeroArity1", true), PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestConcatAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestConcatAllAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListDomainAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListConcatAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListLengthAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListSplitAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListSplitHalfAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListMergeAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSubstrAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSmallerThanAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestFirstAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestPushAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestMoveAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestStrlenAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusAtom, PluginPtrDeleter<PluginAtom>()));
          ret.push_back(PluginAtomPtr(new TestSetMinusNonmonotonicAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusNogoodBasedLearningAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusNonComfortAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusPartialAtom, PluginPtrDeleter<PluginAtom>()));
          ret.push_back(PluginAtomPtr(new TestSetMinusPartialNonmonotonicAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusNongroundNogoodBasedLearningAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusRuleBasedLearningAtom(&ctx), PluginPtrDeleter<PluginAtom>()));
          ret.push_back(PluginAtomPtr(new TestSetUnionAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestNonmonAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestNonmon2Atom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestIdAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestIdpAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestIdcAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestNegAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestMinusOneAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestEvenAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestOddAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestLessThanAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestEqualAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestTransitiveClosureAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestCycleAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestAppendAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestDisjAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestHashAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestTrueMultiInpAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestTrueMultiInpAtom2, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestReachableAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestDLSimulatorAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestCautiousQueryAtom(ctx), PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestBraveQueryAtom(ctx), PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new TestGen2Atom("gen1", 1), PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new TestGen2Atom("gen2", 2), PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new TestGen2Atom("gen3", 3), PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new TestIsEmpty, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new TestNumberOfBalls, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new TestNumberOfBallsSE, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new TestNumberOfBallsGE, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new SumNonZeroAtom, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new ProductionRequirementsAtom, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new MappingAtom, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new GetSizesAtom, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new GetSizesRestrAtom, PluginPtrDeleter<PluginAtom>()));
    ret.push_back(PluginAtomPtr(new GetDiagnosesAtom(ctx), PluginPtrDeleter<PluginAtom>()));

    return ret;
	}


  virtual void setupProgramCtx(ProgramCtx& ctx)
	{
		TestPlugin::CtxData& pcd = ctx.getPluginData<TestPlugin>();

		if( pcd.testRepetition )
		{
			ctx.finalCallbacks.push_back(
					FinalCallbackPtr(new TestFinalCallback(ctx)));
		}
	}
};

TestPlugin theTestPlugin;

DLVHEX_NAMESPACE_END

IMPLEMENT_PLUGINABIVERSIONFUNCTION

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
DLVHEX_PLUGINEXPORT
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theTestPlugin);
}

/* vim: set noet sw=2 ts=2 tw=80: */
