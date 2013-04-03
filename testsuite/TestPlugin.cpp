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

#include "dlvhex2/ExternalLearningHelper.h"
#include "dlvhex2/ComfortPluginInterface.h"
#include "dlvhex2/Term.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"

#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>

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
	for (int i = 0; i < str.length(); i++){
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
	for (int i = 0; i < str.length(); i++){
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
    setOutputArity(1);

    prop.functional = true;
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	const std::string& str1 = registry->terms.getByID(query.input[0]).getUnquotedString();
	const std::string& str2 = registry->terms.getByID(query.input[1]).getUnquotedString();

	std::string merged;

	std::stringstream element1, element2;
	int c1old = 0;
	int c2old = 0;
	int c1 = 0;
	int c2 = 0;
	while (true){
		c1old = c1;
		c2old = c2;
	
		// get next element from list 1
		if (str1[c1] == ';') c1++;
		element1.str("");
		while (str1[c1] != ';' && str1[c1] != '\0'){
			element1 << str1[c1];
			c1++;
		}

		// get next element from list 2
		if (str2[c2] == ';') c2++;
		element2.str("");
		while (str2[c2] != ';' && str2[c2] != '\0'){
			element2 << str2[c2];
			c2++;
		}

		if (element1.str().length() == 0 && element2.str().length() == 0) break;

		// take the smaller element
		if (merged.length() > 0) merged += ";";
		
		if (element2.str().length() == 0){
			merged += element1.str();
			c2 = c2old;
		}else if (element1.str().length() == 0){
			merged += element2.str();
			c1 = c1old;
		}else if(element1.str().compare(element2.str()) < 0){
			merged += element1.str();
			c2 = c2old;
		}else{
			merged += element2.str();
			c1 = c1old;
		}
	}
	
    Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "\"" + merged + "\"");
    Tuple tu;
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

class TestSetMinusNogoodBasedLearningAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestSetMinusNogoodBasedLearningAtom():
    PluginAtom("testSetMinusNogoodBasedLearning", false) // monotonic, and no predicate inputs anyway
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
    addInputPredicate();
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	assert(false);	// this method should never be called

	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (int i = 1; i < atom.tuple.size(); ++i){
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
		for (int i = 1; i < atom.tuple.size(); ++i){
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

			// Test: Learning based on direct definition of nogoods
			if (nogoods != NogoodContainerPtr()){
				if (query.ctx->config.getOption("ExternalLearningUser")){
					// learn that presence of t in query.input[0] and absence in query.input[1] implies presence in output
					OrdinaryAtom at1(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					at1.tuple.push_back(query.input[0]);
					for (int i = 0; i < t.size(); ++i) at1.tuple.push_back(t[i]);
					OrdinaryAtom at2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
					at2.tuple.push_back(query.input[1]);
					for (int i = 0; i < t.size(); ++i) at2.tuple.push_back(t[i]);

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
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
    addInputPredicate();
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	assert(false);	// this method should never be called

	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (int i = 1; i < atom.tuple.size(); ++i){
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
		for (int i = 1; i < atom.tuple.size(); ++i){
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
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
    addInputPredicate();
    addInputPredicate();
    setOutputArity(1);
  }

  virtual void retrieve(const Query& query, Answer& answer)
  {
	assert(false);	// this method should never be called

	// find relevant input
	bm::bvector<>::enumerator en = query.interpretation->getStorage().first();
	bm::bvector<>::enumerator en_end = query.interpretation->getStorage().end();

	std::vector<Tuple> tuples1;
	std::vector<Tuple> tuples2;
	while (en < en_end){

		const OrdinaryAtom& atom = getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		Tuple tu;
		for (int i = 1; i < atom.tuple.size(); ++i){
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
		for (int i = 1; i < atom.tuple.size(); ++i){
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
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
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
		for (int i = 1; i < atom.tuple.size(); ++i){
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
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
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
		for (int i = 1; i < atom.tuple.size(); ++i){
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
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
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

class TestIdcAtom:	// tests user-defined external learning
  public PluginAtom
{
public:
  TestIdcAtom():
    PluginAtom("idc", false) // monotonic, and no predicate inputs anyway
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
    addInputConstant();
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
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
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
    #warning TODO if a plugin atom has only onstant inputs, is it always monotonic? if yes, automate this, at least create a warning
  {
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
		#warning create (or reuse, maybe from potassco?) cmdline option processing facility
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
	  ret.push_back(PluginAtomPtr(new TestListConcatAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListLengthAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListSplitAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestListMergeAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSubstrAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSmallerThanAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestFirstAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestPushAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestMoveAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestStrlenAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusNogoodBasedLearningAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusNongroundNogoodBasedLearningAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestSetMinusRuleBasedLearningAtom(&ctx), PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestNonmonAtom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestNonmon2Atom, PluginPtrDeleter<PluginAtom>()));
	  ret.push_back(PluginAtomPtr(new TestIdAtom, PluginPtrDeleter<PluginAtom>()));
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
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theTestPlugin);
}

/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
