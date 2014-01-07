/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file   HexGrammar.tcc
 * @author Peter Schüller
 * @date   Wed Jul  8 14:00:48 CEST 2009
 * 
 * @brief  Implementation of HexGrammar.h
 */

/**
 * @file   HexGrammar.tcc
 * @author Peter Schüller
 * 
 * @brief  Grammar for parsing HEX using boost::spirit
 */

#ifndef DLVHEX_HEX_GRAMMAR_TCC_INCLUDED
#define DLVHEX_HEX_GRAMMAR_TCC_INCLUDED

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ExtSourceProperties.h"

#include <algorithm>

#include <boost/spirit/include/qi.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>


DLVHEX_NAMESPACE_BEGIN

/////////////////////////////////////////////////////////////////
// Skipper //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
template<typename Iterator>
HexParserSkipperGrammar<Iterator>::HexParserSkipperGrammar():
  HexParserSkipperGrammar::base_type(ws)
{
  using namespace boost::spirit;
  ws
    = ascii::space
    | qi::lexeme[ qi::char_('%') > *(qi::char_ - qi::eol) ];

  #ifdef BOOST_SPIRIT_DEBUG_WS
  BOOST_SPIRIT_DEBUG_NODE(ws);
  #endif
}

/////////////////////////////////////////////////////////////////
// HexGrammarBase semantic processors ///////////////////////////
/////////////////////////////////////////////////////////////////
template<>
struct sem<HexGrammarSemantics::termFromCIdent>
{
  void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target)
  {
    assert(!source.empty() && islower(source[0]));
    target = mgr.ctx.registry()->terms.getIDByString(source);
    if( target == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
      target = mgr.ctx.registry()->terms.storeAndGetID(term);
    }
  }
};

template<>
struct sem<HexGrammarSemantics::termFromFunctionTerm>
{
  void operator()(HexGrammarSemantics& mgr, const boost::fusion::vector2<const std::string, boost::optional<boost::optional<std::vector<ID> > > >& source, ID& target)
  {
    Term functionSymbol(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, boost::fusion::at_c<0>(source));
    ID fid = mgr.ctx.registry()->terms.getIDByString(functionSymbol.symbol);
    if (fid == ID_FAIL) fid = mgr.ctx.registry()->terms.storeAndGetID(functionSymbol);

    std::vector<ID> args;
    args.push_back(fid);
    if (!!boost::fusion::at_c<1>(source) && !!boost::fusion::at_c<1>(source).get() ){
      BOOST_FOREACH (ID id, boost::fusion::at_c<1>(source).get().get()) args.push_back(id);
    }

    Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_NESTED, args, mgr.ctx.registry());
    target = mgr.ctx.registry()->terms.getIDByString(term.symbol);
    if (target == ID_FAIL) target = mgr.ctx.registry()->terms.storeAndGetID(term);
  }
};

template<>
struct sem<HexGrammarSemantics::termFromInteger>
{
  void operator()(HexGrammarSemantics& mgr, unsigned int source, ID& target)
  {
    target = ID::termFromInteger(source);
  }
};


template<>
struct sem<HexGrammarSemantics::termFromString>
{
  void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target)
  {
    assert(!source.empty() && source[0] == '"' && source[source.size()-1] == '"');
    target = mgr.ctx.registry()->terms.getIDByString(source);
    if( target == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
      target = mgr.ctx.registry()->terms.storeAndGetID(term);
    }
  }
};


template<>
struct sem<HexGrammarSemantics::termFromVariable>
{
  void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target)
  {
    assert(!source.empty() && ((source[0] == '_' && source.size() == 1) || isupper(source[0])));
    // special handling of anonymous variables
    IDKind addFlags = 0;
    if( source == "_" )
    {
      addFlags |= ID::PROPERTY_VAR_ANONYMOUS;
    }
    // regular handling + flags
    target = mgr.ctx.registry()->terms.getIDByString(source);
    if( target == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE | addFlags, source);
      target = mgr.ctx.registry()->terms.storeAndGetID(term);
    }
  }
};


// helper method to prefix and store predicates
void storePredicate(const std::string& oriPredName, int predArity, HexGrammarSemantics& mgr, ID& target){
    std::string newPredName;
//		if ( mgr.mlpMode == 0 )
//			{ // ordinary encoding
				newPredName = oriPredName;
//			}	
//		else
//			{	// mlp encoding
				newPredName = mgr.ctx.registry()->moduleTable.getModuleName( mgr.ctx.registry()->moduleTable.getSize()-1 ) + MODULEPREFIXSEPARATOR + oriPredName;
//			}

    target = mgr.ctx.registry()->preds.getIDByString(newPredName);
    if( target == ID_FAIL )
      {   
        Predicate predicate(ID::MAINKIND_TERM | ID::SUBKIND_TERM_PREDICATE, newPredName, predArity);
        target = mgr.ctx.registry()->preds.storeAndGetID(predicate);
        DBGLOG(DBG, "Preds stored: " << predicate << " got id: " << target);
      } 
    else 
      {
        DBGLOG(DBG, "Preds previously stored: " << newPredName << "/" << predArity << " got id: " << target);
      }
}



template<>
struct sem<HexGrammarSemantics::predFromPredDecl>
{
  void operator()(
    HexGrammarSemantics& mgr, 
    const boost::fusion::vector2<const std::string&, unsigned int>& source,
    ID& target)
  {

    const std::string& oriPredName = boost::fusion::at_c<0>(source);
    assert(!oriPredName.empty() && islower(oriPredName[0]));

    unsigned int predArity = boost::fusion::at_c<1>(source);

    std::string newPredName;
		newPredName = mgr.currentModuleName + MODULEPREFIXSEPARATOR + oriPredName;

    target = mgr.ctx.registry()->preds.getIDByString(newPredName);
    if( target == ID_FAIL )
      {   
        Predicate predicate(ID::MAINKIND_TERM | ID::SUBKIND_TERM_PREDICATE, newPredName, predArity);
        target = mgr.ctx.registry()->preds.storeAndGetID(predicate);
        DBGLOG(DBG, "Preds stored: " << predicate << " got id: " << target);
      } 
    else 
      {
        DBGLOG(DBG, "Preds previously stored: " << newPredName << "/" << predArity << " got id: " << target);
      }

  }
};



template<>
struct sem<HexGrammarSemantics::predFromNameOnly>
{
  void operator()(
    HexGrammarSemantics& mgr, 
    const std::string& source,
    ID& target)
  {

    assert(!source.empty() && islower(source[0]));
		if (mgr.mlpMode == 1) 
			{ // mlp encoding
		    int predArity = -1;
				storePredicate(source, predArity, mgr, target);
			}
		else
			{ // ordinary encoding
    		target = mgr.ctx.registry()->terms.getIDByString(source);
		    if( target == ID_FAIL )
    			{
   				  Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
      			target = mgr.ctx.registry()->terms.storeAndGetID(term);
    			}
			}

  }
};



template<>
struct sem<HexGrammarSemantics::predFromString>
{
  void operator()(
    HexGrammarSemantics& mgr, 
    const std::string& source,
    ID& target)
  {

    assert(!source.empty() && source[0] == '"' && source[source.size()-1] == '"');

		if (mgr.mlpMode == 1)
			{ // mlp encoding
		    const std::string& oriPredName = source;
    		int predArity = -1;
				storePredicate(oriPredName, predArity, mgr, target);
			}
		else
			{ // ordinary encoding
    		target = mgr.ctx.registry()->terms.getIDByString(source);
		    if( target == ID_FAIL )
			    {
			      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
			      target = mgr.ctx.registry()->terms.storeAndGetID(term);
			    }		
			}

  }
};



template<>
struct sem<HexGrammarSemantics::classicalAtomFromPrefix>
{
  void createAtom(RegistryPtr reg, OrdinaryAtom& atom, ID& target)
  {
    // groundness
    DBGLOG(DBG,"checking groundness of tuple " << printrange(atom.tuple));
    IDKind kind = 0;
    std::set<ID> var;
    BOOST_FOREACH(const ID& id, atom.tuple)
    {
      reg->getVariablesInID(id, var);
      kind |= id.kind;
      // make this sure to make the groundness check work
      // (if we add "builtin constant terms" like #supremum we might have to change the above statement)
      assert((id.kind & ID::SUBKIND_MASK) != ID::SUBKIND_TERM_BUILTIN);
    }
    const bool ground = !(kind & ID::SUBKIND_TERM_VARIABLE) && var.size() == 0;
    if( ground )
    {
      atom.kind |= ID::SUBKIND_ATOM_ORDINARYG;
      target = reg->storeOrdinaryGAtom(atom);
    }
    else
    {
      atom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
      target = reg->storeOrdinaryNAtom(atom);
    }
    DBGLOG(DBG,"stored atom " << atom << " which got id " << target);
  }

  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<ID, boost::optional<boost::optional<std::vector<ID> > > >& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    OrdinaryAtom atom(ID::MAINKIND_ATOM);

    // predicate
    const ID& idpred = boost::fusion::at_c<0>(source);
    atom.tuple.push_back(idpred);

    // arguments
    if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) )
    {
      const Tuple& tuple = boost::fusion::at_c<1>(source).get().get();
      atom.tuple.insert(atom.tuple.end(), tuple.begin(), tuple.end());
			if ( mgr.mlpMode==1 ) mgr.ctx.registry()->preds.setArity(idpred, tuple.size());
    } else {
      if ( mgr.mlpMode==1 ) mgr.ctx.registry()->preds.setArity(idpred, 0);
    }

    createAtom(reg, atom, target);
  }
};


template<>
struct sem<HexGrammarSemantics::classicalAtomFromTuple>:
  private sem<HexGrammarSemantics::classicalAtomFromPrefix>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<ID, std::vector<ID> >& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    OrdinaryAtom atom(ID::MAINKIND_ATOM);

    // predicate
    atom.tuple.push_back(boost::fusion::at_c<0>(source));
    // arguments
    const Tuple& tuple = boost::fusion::at_c<1>(source);
    atom.tuple.insert(atom.tuple.end(), tuple.begin(), tuple.end());

    createAtom(reg, atom, target);
  }
};


template<>
struct sem<HexGrammarSemantics::builtinTernaryInfix>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector4<
      ID, ID, ID, ID
    >& source,
    ID& target)
  {
    BuiltinAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
    atom.tuple.push_back(boost::fusion::at_c<2>(source));
    atom.tuple.push_back(boost::fusion::at_c<1>(source));
    atom.tuple.push_back(boost::fusion::at_c<3>(source));
    atom.tuple.push_back(boost::fusion::at_c<0>(source));

    DBGLOG(DBG,"storing builtin atom " << atom);
    target = mgr.ctx.registry()->batoms.storeAndGetID(atom);
    DBGLOG(DBG,"builtin atom " << atom << " got id " << target);
  }
};


template<>
struct sem<HexGrammarSemantics::builtinBinaryInfix>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector3<
      ID, ID, ID
    >& source,
    ID& target)
  {
    BuiltinAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
    atom.tuple.push_back(boost::fusion::at_c<1>(source));
    atom.tuple.push_back(boost::fusion::at_c<0>(source));
    atom.tuple.push_back(boost::fusion::at_c<2>(source));

    DBGLOG(DBG,"storing builtin atom " << atom);
    target = mgr.ctx.registry()->batoms.storeAndGetID(atom);
    DBGLOG(DBG,"builtin atom " << atom << " got id " << target);
  }
};


template<>
struct sem<HexGrammarSemantics::builtinUnaryPrefix>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<
      ID, ID
    >& source,
    ID& target)
  {
    BuiltinAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
    atom.tuple.push_back(boost::fusion::at_c<0>(source));
    atom.tuple.push_back(boost::fusion::at_c<1>(source));

    DBGLOG(DBG,"storing builtin atom " << atom);
    target = mgr.ctx.registry()->batoms.storeAndGetID(atom);
    DBGLOG(DBG,"builtin atom " << atom << " got id " << target);
  }
};


template<>
struct sem<HexGrammarSemantics::builtinBinaryPrefix>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector3<
      ID, ID, ID
    >& source,
    ID& target)
  {
    BuiltinAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
    atom.tuple.push_back(boost::fusion::at_c<0>(source));
    atom.tuple.push_back(boost::fusion::at_c<1>(source));
    atom.tuple.push_back(boost::fusion::at_c<2>(source));

    DBGLOG(DBG,"storing builtin atom " << atom);
    target = mgr.ctx.registry()->batoms.storeAndGetID(atom);
    DBGLOG(DBG,"builtin atom " << atom << " got id " << target);
  }
};


template<>
struct sem<HexGrammarSemantics::builtinTernaryPrefix>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector4<
      ID, ID, ID, ID
    >& source,
    ID& target)
  {
    BuiltinAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
    atom.tuple.push_back(boost::fusion::at_c<0>(source));
    atom.tuple.push_back(boost::fusion::at_c<1>(source));
    atom.tuple.push_back(boost::fusion::at_c<2>(source));
    atom.tuple.push_back(boost::fusion::at_c<3>(source));

    DBGLOG(DBG,"storing builtin atom " << atom);
    target = mgr.ctx.registry()->batoms.storeAndGetID(atom);
    DBGLOG(DBG,"builtin atom " << atom << " got id " << target);
  }
};


template<>
struct sem<HexGrammarSemantics::aggregateAtom>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector3<
      boost::optional<boost::fusion::vector2<ID, ID> >,
      boost::fusion::vector3<ID, std::vector<ID>, std::vector<ID> >,
      boost::optional<boost::fusion::vector2<ID, ID> >
    >& source,
    ID& target)
  {
    AggregateAtom aatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE);
    ID& leftTerm = aatom.tuple[0];
    ID& leftComp = aatom.tuple[1];
    ID& aggFunc = aatom.tuple[2];
    Tuple& aggVariables = aatom.variables;
    Tuple& aggBody = aatom.literals;
    ID& rightComp = aatom.tuple[3];
    ID& rightTerm = aatom.tuple[4];

    // left term + operator
    if( !!boost::fusion::at_c<0>(source) )
    {
      leftTerm = boost::fusion::at_c<0>(
        boost::fusion::at_c<0>(source).get());
      leftComp = boost::fusion::at_c<1>(
        boost::fusion::at_c<0>(source).get());
    }

    // right term + operator
    if( !!boost::fusion::at_c<2>(source) )
    {
      rightComp = boost::fusion::at_c<0>(
        boost::fusion::at_c<2>(source).get());
      rightTerm = boost::fusion::at_c<1>(
        boost::fusion::at_c<2>(source).get());
    }

    #warning TODO throw iterator in syntax error and display it nicely (like expectation failure)
    if( leftTerm == ID_FAIL && rightTerm == ID_FAIL )
      throw SyntaxError("aggregate needs at least one term + comparison operator");

    // aggregation + symbolic set
    aggFunc = boost::fusion::at_c<0>(boost::fusion::at_c<1>(source));
    aggVariables = boost::fusion::at_c<1>(boost::fusion::at_c<1>(source));
    aggBody = boost::fusion::at_c<2>(boost::fusion::at_c<1>(source));

    DBGLOG(DBG,"storing aggregate atom " << aatom);
    target = mgr.ctx.registry()->aatoms.storeAndGetID(aatom);
    DBGLOG(DBG,"stored aggregate atom " << aatom << " which got id " << target);
  }
};

template<>
struct sem<HexGrammarSemantics::externalAtom>
{

  void interpretProperties(HexGrammarSemantics& mgr, ExternalAtom& atom, std::vector<ExtSourceProperty>& props){

    BOOST_FOREACH (ExtSourceProperty prop, props){
      switch (prop.type){
        case ExtSourceProperty::FUNCTIONAL:
	 if (prop.param1 != ID_FAIL || prop.param2 != ID_FAIL) throw GeneralError("Property \"functional\" expects no parameters");
         DBGLOG(DBG, "External Atom is functional");
          atom.prop.functional = true;
          break;
        case ExtSourceProperty::MONOTONIC:
	  if (prop.param2 != ID_FAIL) throw GeneralError("Property \"monotonic\" expects less than two parameters");
          if (prop.param1 == ID_FAIL){
            DBGLOG(DBG, "External Atom is monotonic in all input parameters");
            for (int i = 0; i < atom.inputs.size(); ++i){
              atom.prop.monotonicInputPredicates.insert(i);
            }
          }else{
            bool found = false;
            for (int i = 0; i < atom.inputs.size(); ++i){
              if (atom.inputs[i] == prop.param1){
                DBGLOG(DBG, "External Atom is monotonic in parameter " << i);
                atom.prop.monotonicInputPredicates.insert(i);
                found = true;
                break;
              }
            }
            if (!found) throw SyntaxError("Property refers to invalid input parameter");
          }
          break;
        case ExtSourceProperty::ANTIMONOTONIC:
	  if (prop.param2 != ID_FAIL) throw GeneralError("Property \"antimonotonic\" expects less than two parameters");
          if (prop.param1 == ID_FAIL){
            DBGLOG(DBG, "External Atom is antimonotonic in all input parameters");
            for (int i = 0; i < atom.inputs.size(); ++i){
              atom.prop.antimonotonicInputPredicates.insert(i);
            }
          }else{
            bool found = false;
            for (int i = 0; i < atom.inputs.size(); ++i){
              if (atom.inputs[i] == prop.param1){
                DBGLOG(DBG, "External Atom is antimonotonic in parameter " << i);
                atom.prop.antimonotonicInputPredicates.insert(i);
                found = true;
                break;
              }
            }
            if (!found) throw SyntaxError("Property refers to invalid input parameter");
          }
          break;
        case ExtSourceProperty::ATOMLEVELLINEAR:
	  if (prop.param1 != ID_FAIL || prop.param2 != ID_FAIL) throw GeneralError("Property \"atomlevellinear\" expects no parameters");
          DBGLOG(DBG, "External Atom is linear on atom level");
          atom.prop.atomlevellinear = true;
          break;
        case ExtSourceProperty::TUPLELEVELLINEAR:
	  if (prop.param1 != ID_FAIL || prop.param2 != ID_FAIL) throw GeneralError("Property \"tuplelevellinear\" expects no parameters");
          DBGLOG(DBG, "External Atom is linear on tuple level");
          atom.prop.tuplelevellinear = true;
          break;
        case ExtSourceProperty::USES_ENVIRONMENT:
	  if (prop.param1 != ID_FAIL || prop.param2 != ID_FAIL) throw GeneralError("Property \"usesenvironment\" expects no parameters");
          DBGLOG(DBG, "External Atom uses environment");
          atom.prop.usesEnvironment = true;
          break;
        case ExtSourceProperty::FINITEDOMAIN:
	  if (prop.param2 != ID_FAIL) throw GeneralError("Property \"finitedomain\" expects less than two parameters");
          if (prop.param1 == ID_FAIL){
            DBGLOG(DBG, "External Atom has a finite domain in all output positions");
            for (int i = 0; i < atom.tuple.size(); ++i){
              atom.prop.finiteOutputDomain.insert(i);
            }
          }else{
            bool found = false;
	    if (!prop.param1.isIntegerTerm()) throw GeneralError("The parameter of property \"finitedomain\" must be an integer");
            atom.prop.finiteOutputDomain.insert(prop.param1.address);
          }
          break;
        case ExtSourceProperty::RELATIVEFINITEDOMAIN:
        	{
	  if (prop.param1 == ID_FAIL || prop.param2 == ID_FAIL) throw GeneralError("Property \"relativefinitedomain\" expects two parameters");
	  		int wrt;
            bool found = false;
            for (int i = 0; i < atom.inputs.size(); ++i){
              if (atom.inputs[i] == prop.param2){
				wrt = i;
                found = true;
                break;
              }
            }
            if (!found) throw SyntaxError("Property refers to invalid input parameter");
           	if (!prop.param1.isIntegerTerm()) throw GeneralError("The first parameter of property \"relativefinitedomain\" must be an integer");
            atom.prop.relativeFiniteOutputDomain.insert(std::pair<int, int>(prop.param1.address, wrt));
            }
          break;
        case ExtSourceProperty::FINITEFIBER:
	  if (prop.param1 != ID_FAIL || prop.param2 != ID_FAIL) throw GeneralError("Property \"finitefiber\" expects no parameters");
          DBGLOG(DBG, "External Atom has a finite fiber");
          atom.prop.finiteFiber = true;
          break;
        case ExtSourceProperty::WELLORDERINGSTRLEN:
	  if (prop.param1 == ID_FAIL || prop.param2 == ID_FAIL) throw GeneralError("Property \"wellordering\" expects two parameters");
          DBGLOG(DBG, "External Atom has a wellordering using strlen");
          atom.prop.wellorderingStrlen.insert(std::pair<int, int>(prop.param1.address, prop.param2.address));
          break;
        case ExtSourceProperty::WELLORDERINGNATURAL:
	  if (prop.param1 == ID_FAIL || prop.param2 == ID_FAIL) throw GeneralError("Property \"wellordering\" expects two parameters");
          DBGLOG(DBG, "External Atom has a wellordering using natural");
          atom.prop.wellorderingNatural.insert(std::pair<int, int>(prop.param1.address, prop.param2.address));
          break;
        case ExtSourceProperty::PROVIDES_SUPPORTSETS:
	  if (prop.param1 != ID_FAIL || prop.param2 != ID_FAIL) throw GeneralError("Property \"providessupportsets\" expects no parameters");
          DBGLOG(DBG, "External Atom has a wellordering using natural");
          atom.prop.providesSupportSets = true;
          break;
        case ExtSourceProperty::PROVIDES_COMPLETEPOSITIVESUPPORTSETS:
	  if (prop.param1 != ID_FAIL || prop.param2 != ID_FAIL) throw GeneralError("Property \"providescompletepositivesupportsets\" expects no parameters");
          DBGLOG(DBG, "External Atom has a wellordering using natural");
          atom.prop.providesSupportSets = true;
          atom.prop.providesCompletePositiveSupportSets = true;
          break;
        case ExtSourceProperty::PROVIDES_COMPLETENEGATIVESUPPORTSETS:
	  if (prop.param1 != ID_FAIL || prop.param2 != ID_FAIL) throw GeneralError("Property \"providescompletepositivesupportsets\" expects no parameters");
          DBGLOG(DBG, "External Atom has a wellordering using natural");
          atom.prop.providesSupportSets = true;
          atom.prop.providesCompleteNegativeSupportSets = true;
          break;
      }
    }
  }

  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector4<
      ID,
      boost::optional<boost::optional<std::vector<ID> > >,
      boost::optional<boost::optional<std::vector<ID> > >,
      boost::optional<boost::optional<std::vector<ExtSourceProperty> > >
    >& source,
    ID& target)
  {
    ExternalAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);

    // predicate
    atom.predicate = boost::fusion::at_c<0>(source);

    // inputs
    if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) )
    {
      atom.inputs = boost::fusion::at_c<1>(source).get().get();
    }

    // outputs
    if( (!!boost::fusion::at_c<2>(source)) &&
        (!!(boost::fusion::at_c<2>(source).get())) )
    {
      atom.tuple = boost::fusion::at_c<2>(source).get().get();
    }

    // properties
    if( (!!boost::fusion::at_c<3>(source)) &&
        (!!(boost::fusion::at_c<3>(source).get())) )
    {
	std::vector<ExtSourceProperty> p = boost::fusion::at_c<3>(source).get().get();
	interpretProperties(mgr, atom, p);
    }

    DBGLOG(DBG,"storing external atom " << atom);
    target = mgr.ctx.registry()->eatoms.storeAndGetID(atom);
    DBGLOG(DBG,"external atom " << atom << " got id " << target);
  }
};

template<>
struct sem<HexGrammarSemantics::extSourceProperty>
{
  void operator()(
	HexGrammarSemantics& mgr,
	const boost::fusion::vector2<
		std::string,
		boost::optional<
			boost::fusion::vector2<
				ID,
				boost::optional<ID>
			>
		> >& source,
	ExtSourceProperty& target)
  {

	ID p1 = ID_FAIL;
	ID p2 = ID_FAIL;
	if (!!boost::fusion::at_c<1>(source)){
		p1 = boost::fusion::at_c<0>(boost::fusion::at_c<1>(source).get());
		if (boost::fusion::at_c<1>(boost::fusion::at_c<1>(source).get())){
			p2 = boost::fusion::at_c<1>(boost::fusion::at_c<1>(source).get()).get();
		}
	}
        if (boost::fusion::at_c<0>(source) == "functional"){
		target = ExtSourceProperty(ExtSourceProperty::FUNCTIONAL, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "monotonic"){
		target = ExtSourceProperty(ExtSourceProperty::MONOTONIC, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "antimonotonic"){
		target = ExtSourceProperty(ExtSourceProperty::ANTIMONOTONIC, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "fullylinear" || boost::fusion::at_c<0>(source) == "atomlevellinear"){
		target = ExtSourceProperty(ExtSourceProperty::ATOMLEVELLINEAR, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "tuplelevellinear"){
		target = ExtSourceProperty(ExtSourceProperty::TUPLELEVELLINEAR, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "usesenvironment"){
		target = ExtSourceProperty(ExtSourceProperty::USES_ENVIRONMENT, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "finitedomain"){
		target = ExtSourceProperty(ExtSourceProperty::FINITEDOMAIN, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "relativefinitedomain"){
		target = ExtSourceProperty(ExtSourceProperty::RELATIVEFINITEDOMAIN, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "finitefiber"){
		target = ExtSourceProperty(ExtSourceProperty::FINITEFIBER, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "wellorderingstrlen"){
		target = ExtSourceProperty(ExtSourceProperty::WELLORDERINGSTRLEN, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "providessupportsets"){
		target = ExtSourceProperty(ExtSourceProperty::PROVIDES_SUPPORTSETS, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "providescompletepositivesupportsets"){
		target = ExtSourceProperty(ExtSourceProperty::PROVIDES_COMPLETEPOSITIVESUPPORTSETS, p1, p2);
        }else if (boost::fusion::at_c<0>(source) == "providescompletenegativesupportsets"){
		target = ExtSourceProperty(ExtSourceProperty::PROVIDES_COMPLETENEGATIVESUPPORTSETS, p1, p2);
	}else{
		throw SyntaxError("Property \"" + boost::fusion::at_c<0>(source) + "\" unrecognized");
	}
  }
};

template<>
struct sem<HexGrammarSemantics::mlpModuleAtom>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector3<
      ID,
      boost::optional<boost::optional<std::vector<ID> > >,
      ID
    >& source,
    ID& target)
  {
    ModuleAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_MODULE);

    // predicate
    atom.predicate = boost::fusion::at_c<0>(source);

    // get actual module name	
    const std::string& predName = mgr.ctx.registry()->preds.getByID(atom.predicate).symbol;
    int n = predName.find(MODULEPREFIXSEPARATOR);
    atom.actualModuleName = predName.substr(n+2, predName.length());

    // inputs
    if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) )
    {
      atom.inputs = boost::fusion::at_c<1>(source).get().get();
    }

    // output
    atom.outputAtom = boost::fusion::at_c<2>(source);

    ID atomNewID = mgr.ctx.registry()->matoms.getIDByElement(atom.predicate, atom.inputs, atom.outputAtom);
    if ( atomNewID == ID_FAIL )
      {
    	DBGLOG(DBG,"storing mlp Module atom " << atom);
    	target = mgr.ctx.registry()->matoms.storeAndGetID(atom);
    	DBGLOG(DBG,"mlp Module atom " << atom << " got id " << target);
      }
    else 
      {
    	DBGLOG(DBG,"previously stored mlp Module atom " << atom);
    	target = atomNewID;
    	DBGLOG(DBG,"mlp Module atom " << atom << " got (old) id " << target);
      }		
  }
};


template<>
struct sem<HexGrammarSemantics::bodyLiteral>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<
      boost::optional<std::string>,
      dlvhex::ID
    >& source,
    ID& target)
  {
    bool isNaf = !!boost::fusion::at_c<0>(source);
    assert(boost::fusion::at_c<1>(source).isAtom());
    target = ID::literalFromAtom(boost::fusion::at_c<1>(source), isNaf);
  }
};


template<>
struct sem<HexGrammarSemantics::rule>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<
      std::vector<dlvhex::ID>,
      boost::optional<std::vector<dlvhex::ID> >
    >& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    const Tuple& head = boost::fusion::at_c<0>(source);
    bool hasBody = !!boost::fusion::at_c<1>(source);

    if( hasBody )
    {
      // rule -> put into IDB
      const Tuple& body = boost::fusion::at_c<1>(source).get();

      Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, head, body);
      mgr.markExternalPropertyIfExternalBody(reg, r);
      mgr.markModulePropertyIfModuleBody(reg, r);
      // mark as disjunctive if required
      if( r.head.size() > 1 )
        r.kind |= ID::PROPERTY_RULE_DISJ;
      target = reg->storeRule(r);
    }
    else
    {
      if( head.size() > 1 )
      {
        // disjunctive fact -> create rule
        Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ,
          head, Tuple());
        mgr.markExternalPropertyIfExternalBody(reg, r);
        mgr.markModulePropertyIfModuleBody(reg, r);
        target = reg->storeRule(r);
      }
      else
      {
        assert(head.size() == 1);

        // return ID of fact
        target = *head.begin();
      }
    }
  }
};


template<>
struct sem<HexGrammarSemantics::ruleVariableDisjunction>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector3<
      dlvhex::ID,
      std::vector<dlvhex::ID>,
      boost::optional<std::vector<dlvhex::ID> >
    >& source,
    ID& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    Tuple head;
    head.push_back(boost::fusion::at_c<0>(source));
    const Tuple& headGuard = boost::fusion::at_c<1>(source);
    bool hasBody = !!boost::fusion::at_c<2>(source);

    if( hasBody )
    {
      // rule -> put into IDB
      Tuple body = boost::fusion::at_c<2>(source).get();
      body.insert(body.end(), headGuard.begin(), headGuard.end());

      Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, head, body, headGuard);
      if (r.headGuard.size() > 0) r.kind |= ID::PROPERTY_RULE_HEADGUARD;
      mgr.markExternalPropertyIfExternalBody(reg, r);
      mgr.markModulePropertyIfModuleBody(reg, r);
      // mark as disjunctive if required
      if( r.head.size() > 1 )
        r.kind |= ID::PROPERTY_RULE_DISJ;
      target = reg->storeRule(r);
    }
    else
    {
      if( head.size() > 1 )
      {
        // disjunctive fact -> create rule
        Tuple body;
        body.insert(body.end(), headGuard.begin(), headGuard.end());
        Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ,
          head, body, headGuard);
        if (r.headGuard.size() > 0) r.kind |= ID::PROPERTY_RULE_HEADGUARD;
        mgr.markExternalPropertyIfExternalBody(reg, r);
        mgr.markModulePropertyIfModuleBody(reg, r);
        target = reg->storeRule(r);
      }
      else
      {
        assert(head.size() == 1);

        // return ID of fact
        target = *head.begin();
      }
    }
  }
};


template<>
struct sem<HexGrammarSemantics::constraint>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const std::vector<dlvhex::ID>& source,
    ID& target)
  {
    Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
    r.body = source;
    mgr.markExternalPropertyIfExternalBody(mgr.ctx.registry(), r);
    mgr.markModulePropertyIfModuleBody(mgr.ctx.registry(), r);
    ID existing = mgr.ctx.registry()->rules.getIDByElement(r);
    if( existing == ID_FAIL )
    {
      target = mgr.ctx.registry()->storeRule(r);
      DBGLOG(DBG,"created constraint " << r << " with id " << target);
    }
    else
      target = ID_FAIL;
  }
};

template<>
struct sem<HexGrammarSemantics::weakconstraint>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<
      const std::vector<dlvhex::ID>&,
      const boost::optional<boost::fusion::vector2<ID, ID> >& >& source,
    ID& target)
  {
    Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT);
    r.body = boost::fusion::at_c<0>(source);
    if (!!boost::fusion::at_c<1>(source)){
	r.weight = boost::fusion::at_c<0>(boost::fusion::at_c<1>(source).get());
	r.level = boost::fusion::at_c<1>(boost::fusion::at_c<1>(source).get());
    }else{
        r.weight = ID::termFromInteger(1);
        r.level = ID::termFromInteger(1);
    }
    mgr.markExternalPropertyIfExternalBody(mgr.ctx.registry(), r);
    mgr.markModulePropertyIfModuleBody(mgr.ctx.registry(), r);
    ID existing = mgr.ctx.registry()->rules.getIDByElement(r);
    if( existing == ID_FAIL )
    {
      target = mgr.ctx.registry()->storeRule(r);
      DBGLOG(DBG,"created weak constraint " << r << " with id " << target);
    }
    else
      target = ID_FAIL;
  }
};

template<>
struct sem<HexGrammarSemantics::addMLPModuleName>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const std::string& source,
    std::string& target)
  {
    mgr.mlpMode = 1;
		mgr.currentModuleName = source;		
		target = source;
	}
};


template<>
struct sem<HexGrammarSemantics::addMLPModuleHeader>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector2<
      const std::string&,
      boost::optional<boost::optional<std::vector<ID> > >
    >& source,
    const boost::spirit::unused_type& target)
  {
		// take care module name
		// const std::string& mlpModuleName = boost::fusion::at_c<0>(source);
		const std::string& mlpModuleName = mgr.currentModuleName;
	  Module module(mlpModuleName, mgr.ctx.registry()->inputList.size(), mgr.ctx.edbList.size(), mgr.ctx.idbList.size());
		mgr.ctx.registry()->moduleTable.storeAndGetAddress(module);		    

		// get and insert input list
		// resize +1 to handle if the input list is empty (because it's optional)
    mgr.ctx.registry()->inputList.resize(mgr.ctx.registry()->inputList.size()+1);
    // formal input predicates
    if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) )
    {
			mgr.ctx.registry()->inputList.back() = boost::fusion::at_c<1>(source).get().get();
    }

		// extend edbList, idbList for the mlp module body
    mgr.ctx.edbList.resize(mgr.ctx.edbList.size()+1);
  	mgr.ctx.edbList.back().reset(new Interpretation(mgr.ctx.registry()));
    mgr.ctx.idbList.resize(mgr.ctx.idbList.size()+1);
  }
};


#warning look at spirit mailing list 'optimizing parsing of large input'

template<>
struct sem<HexGrammarSemantics::add>
{
  void operator()(
    HexGrammarSemantics& mgr,
    const dlvhex::ID& source,
    const boost::spirit::unused_type& target)
  {
    RegistryPtr reg = mgr.ctx.registry();
    assert(source != ID_FAIL);
    if( source.isAtom() )
    {
      // fact -> put into EDB
      if( !source.isOrdinaryGroundAtom() )
        throw SyntaxError(
          "fact '"+reg->onatoms.getByID(source).text+"' not safe!");

		  if ( mgr.mlpMode == 0 )		    
				{ // ordinary encoding
      		mgr.ctx.edb->setFact(source.address);
				}
			else
				{ // mlp encoding
      		mgr.ctx.edbList.back()->setFact(source.address);
				}
      DBGLOG(DBG,"added fact with id " << source << " to edb");
    }
    else if( source.isRule() )
    {
		  if ( mgr.mlpMode == 0 )		    
				{ // ordinary encoding
      		mgr.ctx.idb.push_back(source);
				}
			else
				{ // mlp encoding
      		mgr.ctx.idbList.back().push_back(source);
				}
      DBGLOG(DBG,"added rule with id " << source << " to idb");
    }
    else
    {
      // something bad happened if we get no rule and no atom here
      assert(false);
    }
  }
};


template<>
struct sem<HexGrammarSemantics::ignoreAndWarnIfNotFail>
{
  void operator()(
    HexGrammarSemantics&, // mgr,
    const dlvhex::ID& source,
    const boost::spirit::unused_type&) // target)
  {
    if( source != ID_FAIL )
    {
      LOG(WARNING,"ignoring ID " << source);
    }
  }
};


template<>
struct sem<HexGrammarSemantics::maxint>
{
  void operator()(
    HexGrammarSemantics& mgr,
    unsigned int source,
    const boost::spirit::unused_type& )// target)
  {
    mgr.ctx.maxint = source;
  }
};

/////////////////////////////////////////////////////////////////
// HexGrammarBase ///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
template<typename Iterator, typename Skipper>
HexGrammarBase<Iterator, Skipper>::
HexGrammarBase(HexGrammarSemantics& sem):
  sem(sem)
{
  namespace qi = boost::spirit::qi;
  namespace ascii = boost::spirit::ascii;
  typedef HexGrammarSemantics Sem;

  cident
    = qi::lexeme[ ascii::lower >> *(ascii::alnum | qi::char_('_')) ];
  string
    = qi::lexeme[ qi::char_('"') >> *(qi::char_ - (qi::char_('"') | qi::eol)) >> qi::char_('"') ];
  variable
    = qi::string("_") // this can be qi::char_('_') in boost 1.44 ... boost 1.46
    | qi::lexeme[ ascii::upper >> *(ascii::alnum | qi::char_('_')) ];
  posinteger
    = qi::ulong_;
  term
    = ( cident >> qi::lit('(') >> -terms >> qi::lit(')') > qi::eps )     [ Sem::termFromFunctionTerm(sem) ]
    | cident     [ Sem::termFromCIdent(sem) ]
    | string     [ Sem::termFromString(sem) ]
    | variable   [ Sem::termFromVariable(sem) ]
    | posinteger [ Sem::termFromInteger(sem) ]
    | termExt
    ;
    // allow backtracking over terms (no real need to undo the semantic actions == id registrations)
  terms
    = (term > qi::eps) % qi::lit(',');

  pred
    = cident     [ Sem::predFromNameOnly(sem) ];

  preds
    = (pred > qi::eps) % qi::lit(',');

  // if we have this, we can easily extend this to higher order using a module
  classicalAtomPredicate
    = cident [ Sem::predFromNameOnly(sem) ]
    | string [ Sem::predFromString(sem) ]; // module for higher order adds a variable here

  classicalAtom
    = (
        classicalAtomPredicate >> -(qi::lit('(') > -terms >> qi::lit(')')) > qi::eps
      ) [ Sem::classicalAtomFromPrefix(sem) ]
    | (
        qi::lit('(') > classicalAtomPredicate > qi::lit(',') > terms >> qi::lit(')') > qi::eps
      ) [ Sem::classicalAtomFromTuple(sem) ];
  builtinOpsUnary.add
    ("#int", ID::termFromBuiltin(ID::TERM_BUILTIN_INT));
  builtinOpsBinary.add
    ("=", ID::termFromBuiltin(ID::TERM_BUILTIN_EQ))
    ("==", ID::termFromBuiltin(ID::TERM_BUILTIN_EQ))
    ("!=", ID::termFromBuiltin(ID::TERM_BUILTIN_NE))
    ("<>", ID::termFromBuiltin(ID::TERM_BUILTIN_NE))
    ("<", ID::termFromBuiltin(ID::TERM_BUILTIN_LT))
    ("<=", ID::termFromBuiltin(ID::TERM_BUILTIN_LE))
    (">", ID::termFromBuiltin(ID::TERM_BUILTIN_GT))
    (">=", ID::termFromBuiltin(ID::TERM_BUILTIN_GE))
    ("#succ", ID::termFromBuiltin(ID::TERM_BUILTIN_SUCC));
  builtinOpsTernary.add
    ("*", ID::termFromBuiltin(ID::TERM_BUILTIN_MUL))
    ("+", ID::termFromBuiltin(ID::TERM_BUILTIN_ADD))
    ("-", ID::termFromBuiltin(ID::TERM_BUILTIN_SUB))
    ("/", ID::termFromBuiltin(ID::TERM_BUILTIN_DIV))
    ("#mod", ID::termFromBuiltin(ID::TERM_BUILTIN_MOD));
  builtinOpsAgg.add
    ("#count", ID::termFromBuiltin(ID::TERM_BUILTIN_AGGCOUNT))
    ("#min", ID::termFromBuiltin(ID::TERM_BUILTIN_AGGMIN))
    ("#max", ID::termFromBuiltin(ID::TERM_BUILTIN_AGGMAX))
    ("#sum", ID::termFromBuiltin(ID::TERM_BUILTIN_AGGSUM))
    ("#times", ID::termFromBuiltin(ID::TERM_BUILTIN_AGGTIMES))
    ("#avg", ID::termFromBuiltin(ID::TERM_BUILTIN_AGGAVG))
    ("#any", ID::termFromBuiltin(ID::TERM_BUILTIN_AGGANY));
  builtinAtom
    = (term >> qi::lit('=') >> term >> builtinOpsTernary >> term > qi::eps)
      [ Sem::builtinTernaryInfix(sem) ]
    | (term >> builtinOpsBinary >> term > qi::eps)
      [ Sem::builtinBinaryInfix(sem) ]
    | (builtinOpsUnary >> qi::lit('(') > term > qi::lit(')'))
      [ Sem::builtinUnaryPrefix(sem) ]
    | (builtinOpsBinary >> qi::lit('(') > term > qi::lit(',') > term > qi::lit(')'))
      [ Sem::builtinBinaryPrefix(sem) ]
    | (builtinOpsTernary >> qi::lit('(') > term > qi::lit(',') > term > qi::lit(',') > term > qi::lit(')'))
      [ Sem::builtinTernaryPrefix(sem) ];
  aggregateTerm
    = builtinOpsAgg > qi::lit('{') > terms > qi::lit(':') >
      (bodyLiteral % qi::char_(',')) > qi::lit('}');
  aggregateAtom
    // aggregate range or only left or only right part of it
    // (the semantics handler has to rule out that no binop exists)
    = (
        -(term >> builtinOpsBinary) >>
        aggregateTerm >>
        -(builtinOpsBinary >> term) > qi::eps
      ) [ Sem::aggregateAtom(sem) ];
  externalAtomPredicate
    = cident [ Sem::termFromCIdent(sem) ];

  externalAtom
    = (
        qi::lit('&') > externalAtomPredicate >
        -(qi::lit('[') > -terms >> qi::lit(']')) > qi::eps >
        -(qi::lit('(') > -terms >> qi::lit(')')) > qi::eps >
        -(qi::lit('<') > -externalAtomProperties >> qi::lit('>')) > qi::eps
      ) [ Sem::externalAtom(sem) ];

  externalAtomProperty
    = (cident > -(term > -term) > qi::eps) [ Sem::extSourceProperty(sem) ];

  externalAtomProperties
    = (externalAtomProperty > qi::eps) % qi::lit(',');

  mlpModuleAtomPredicate
    = cident [ Sem::predFromNameOnly(sem) ];

  mlpModuleAtom
    = (
        qi::lit('@') > mlpModuleAtomPredicate >  // for module predicate
        -(qi::lit('[') > -preds >> qi::lit(']')) > qi::eps >  // for input
	qi::lit(':') > qi::lit(':') > classicalAtom > qi::eps // for output
      ) [ Sem::mlpModuleAtom(sem) ];


  predDecl
    = (cident > qi::lit('/') > qi::ulong_)[ Sem::predFromPredDecl(sem) ];

  predList
    = (predDecl > qi::eps) % qi::lit(',');

  mlpModuleName 
		= cident [ Sem::addMLPModuleName(sem) ];

  mlpModuleHeader 
    = ( qi::lit("#module") >> // #module
        qi::lit('(') > mlpModuleName >  // module name
        -( qi::lit(',') > qi::lit('[') > -predList >> qi::lit(']') ) > qi::eps > // predicate list
        qi::lit(')') > qi::eps > qi::lit('.') > qi::eps
      ) [ Sem::addMLPModuleHeader(sem) ];

  bodyAtom
    = classicalAtom
    | externalAtom
    | mlpModuleAtom
    | builtinAtom
    | aggregateAtom
    | bodyAtomExt;

  bodyLiteral
    = (
#if BOOST_VERSION >= 104600
        -qi::hold[ qi::lexeme[qi::string("not") >> qi::omit[ascii::space]] ] >>
#else
        -          qi::lexeme[qi::string("not") >> qi::omit[ascii::space]] >>
#endif
        bodyAtom
      ) [ Sem::bodyLiteral(sem) ];

  headAtom
    = classicalAtom
    | headAtomExt;

  rule
    = (
        (headAtom % qi::no_skip[*ascii::space >> qi::char_('v') >> ascii::space]) >>
       -(
          qi::lit(":-") >
          (bodyLiteral % qi::char_(','))
        ) >>
        qi::lit('.')
      ) [ Sem::rule(sem) ]
    | (
        headAtom >> qi::lit(':') >> (bodyLiteral % qi::char_(',')) >>
       -(
          qi::lit(":-") >
          (bodyLiteral % qi::char_(','))
        ) >>
        qi::lit('.')
      ) [ Sem::ruleVariableDisjunction(sem) ];
  constraint
    = (
        qi::lit(":-") >>
        (bodyLiteral % qi::char_(',')) >>
        qi::lit('.')
      ) [ Sem::constraint(sem) ];
  weakconstraint
    = (
        qi::lit(":~") >>
        (bodyLiteral % qi::char_(',')) >>
        qi::lit('.') >>
        -(qi::lit("[") >> term >> qi::lit(":") >> term >> qi::lit("]"))
      ) [ Sem::weakconstraint(sem) ];
  toplevelBuiltin
    = (qi::lit("#maxint") > qi::lit('=') > qi::ulong_ >> qi::lit('.') > qi::eps)
        [ Sem::maxint(sem) ];
  toplevel
    = (rule > qi::eps)
        [ Sem::add(sem) ]
    | (constraint > qi::eps)
        [ Sem::add(sem) ]
    | (weakconstraint > qi::eps)
        [ Sem::add(sem) ]
    | (mlpModuleHeader > qi::eps)
    | (toplevelBuiltin > qi::eps)
    | (toplevelExt > qi::eps)
        [ Sem::ignoreAndWarnIfNotFail(sem) ];
  // the root rule
  start
    = *(toplevel);

  // TODO will weak constraints go into toplevelExt?
  // TODO namespaces go into toplevelExt
  toplevelExt
    = qi::eps(false);
  bodyAtomExt
    = qi::eps(false);
  // TODO action atoms go into HeadAtomExt
  headAtomExt
    = qi::eps(false);
  termExt
    = qi::eps(false);

  #ifdef BOOST_SPIRIT_DEBUG
  BOOST_SPIRIT_DEBUG_NODE(start);
  BOOST_SPIRIT_DEBUG_NODE(toplevel);
  BOOST_SPIRIT_DEBUG_NODE(toplevelBuiltin);
  BOOST_SPIRIT_DEBUG_NODE(cident);
  BOOST_SPIRIT_DEBUG_NODE(string);
  BOOST_SPIRIT_DEBUG_NODE(variable);
  BOOST_SPIRIT_DEBUG_NODE(posinteger);
  BOOST_SPIRIT_DEBUG_NODE(term);
  BOOST_SPIRIT_DEBUG_NODE(externalAtom);
  BOOST_SPIRIT_DEBUG_NODE(externalAtomPredicate);
  BOOST_SPIRIT_DEBUG_NODE(externalAtomProperty);
  BOOST_SPIRIT_DEBUG_NODE(mlpModuleAtom);
  BOOST_SPIRIT_DEBUG_NODE(mlpModuleAtomPredicate);
  BOOST_SPIRIT_DEBUG_NODE(classicalAtomPredicate);
  BOOST_SPIRIT_DEBUG_NODE(classicalAtom);
  BOOST_SPIRIT_DEBUG_NODE(builtinAtom);
  BOOST_SPIRIT_DEBUG_NODE(aggregateAtom);
  BOOST_SPIRIT_DEBUG_NODE(bodyAtom);
  BOOST_SPIRIT_DEBUG_NODE(bodyLiteral);
  BOOST_SPIRIT_DEBUG_NODE(headAtom);
  BOOST_SPIRIT_DEBUG_NODE(rule);
  BOOST_SPIRIT_DEBUG_NODE(constraint);
  BOOST_SPIRIT_DEBUG_NODE(weakconstraint);
  BOOST_SPIRIT_DEBUG_NODE(terms);
  BOOST_SPIRIT_DEBUG_NODE(aggregateTerm);
  BOOST_SPIRIT_DEBUG_NODE(toplevelExt);
  BOOST_SPIRIT_DEBUG_NODE(bodyAtomExt);
  BOOST_SPIRIT_DEBUG_NODE(headAtomExt);
  BOOST_SPIRIT_DEBUG_NODE(termExt);
  #endif
}

#warning TODO more efficient than "rule = rule.copy() | *module" could be something else (see comments below)
// this could be a separate list for each type and a | b | c | d alternatives (have to be coded for each number of arguments)
// this could be something not yet existing, see spirit-general mailinglist Sat, Jul 9, 2011 Vol 62, Issue 6

//! register module for parsing top level elements of input file
//! (use this to parse queries or other meta or control flow information)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerToplevelModule(
    HexParserModuleGrammarPtr module)
{
  // remember the pointer (own it)
  modules.push_back(module);
  toplevelExt = *module | toplevelExt.copy();
}

//! register module for parsing body elements of rules and constraints
//! (use this to parse predicates in rule bodies)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerBodyAtomModule(
    HexParserModuleGrammarPtr module)
{
  // remember the pointer (own it)
  modules.push_back(module);
  bodyAtomExt = *module | bodyAtomExt.copy();
}

//! register module for parsing head elements of rules
//! (use this to parse predicates in rule heads)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerHeadAtomModule(
    HexParserModuleGrammarPtr module)
{
  // remember the pointer (own it)
  modules.push_back(module);
  headAtomExt = *module | headAtomExt.copy();
}

//! register module for parsing terms
//! (use this to parse terms in any predicates)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerTermModule(
    HexParserModuleGrammarPtr module)
{
  // remember the pointer (own it)
  modules.push_back(module);
  termExt = *module | termExt.copy();
}

DLVHEX_NAMESPACE_END

#endif // DLVHEX_HEX_GRAMMAR_TCC_INCLUDED

// Local Variables:
// mode: C++
// End:
