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

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"

#include <boost/spirit/include/qi.hpp>

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

  #ifdef BOOST_SPIRIT_DEBUG
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


template<>
struct sem<HexGrammarSemantics::predFromNameArity>
{
  void operator()(
    HexGrammarSemantics& mgr, 
    const boost::fusion::vector2<const std::string&, unsigned int>& source,
    ID& target)
  {

    const std::string& predName = boost::fusion::at_c<0>(source);
    unsigned int predArity = boost::fusion::at_c<1>(source);

    assert(!predName.empty() && islower(predName[0]));
    target = mgr.ctx.registry()->preds.getIDByString(predName);
    if( target == ID_FAIL )
      {   
        Predicate predicate(ID::MAINKIND_TERM | ID::SUBKIND_TERM_PREDICATE, predName, predArity);
        target = mgr.ctx.registry()->preds.storeAndGetID(predicate);
        DBGLOG(DBG, "Preds stored: " << predicate << " got id: " << target);
      } 
    else 
      {
        DBGLOG(DBG, "Preds previously stored: " << predName << "/" << predArity << " got id: " << target);
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

    const std::string& predName = source;
    int predArity = -1;

    assert(!predName.empty() && islower(predName[0]));
    target = mgr.ctx.registry()->preds.getIDByString(predName);
    if( target == ID_FAIL )
      {   
        Predicate predicate(ID::MAINKIND_TERM | ID::SUBKIND_TERM_PREDICATE, predName, predArity);
        target = mgr.ctx.registry()->preds.storeAndGetID(predicate);
        DBGLOG(DBG, "Preds stored: " << predicate << " got id: " << target);
      } 
    else 
      {
        DBGLOG(DBG, "Preds previously stored: " << predName << "/" << predArity << " got id: " << target);
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

    const std::string& predName = source;
    int predArity = -1;

    target = mgr.ctx.registry()->preds.getIDByString(predName);
    if( target == ID_FAIL )
      {   
        Predicate predicate(ID::MAINKIND_TERM | ID::SUBKIND_TERM_PREDICATE, predName, predArity);
        target = mgr.ctx.registry()->preds.storeAndGetID(predicate);
        DBGLOG(DBG, "Preds stored: " << predicate << " got id: " << target);
      } 
    else 
      {
        DBGLOG(DBG, "Preds previously stored: " << predName << "/" << predArity << " got id: " << target);
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
    BOOST_FOREACH(const ID& id, atom.tuple)
    {
      kind |= id.kind;
      // make this sure to make the groundness check work
      // (if we add "builtin constant terms" like #supremum we might have to change the above statement)
      assert((id.kind & ID::SUBKIND_MASK) != ID::SUBKIND_TERM_BUILTIN);
    }
    const bool ground = !(kind & ID::SUBKIND_TERM_VARIABLE);
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
      mgr.ctx.registry()->preds.setArity(idpred, tuple.size());
    } else {
      mgr.ctx.registry()->preds.setArity(idpred, 0);
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
    Tuple& aggBody = aatom.atoms;
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
  void operator()(
    HexGrammarSemantics& mgr,
    const boost::fusion::vector3<
      ID,
      boost::optional<boost::optional<std::vector<ID> > >,
      boost::optional<boost::optional<std::vector<ID> > >
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

    DBGLOG(DBG,"storing external atom " << atom);
    target = mgr.ctx.registry()->eatoms.storeAndGetID(atom);
    DBGLOG(DBG,"external atom " << atom << " got id " << target);
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
      target = reg->rules.storeAndGetID(r);
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
        target = reg->rules.storeAndGetID(r);
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
    target = mgr.ctx.registry()->rules.storeAndGetID(r);
    DBGLOG(DBG,"created constraint " << r << " with id " << target);
  }
};


template<>
struct sem<HexGrammarSemantics::moduleHeader>
{
  void operator()(
    HexGrammarSemantics& mgr,
//    const std::string& source,
    const boost::fusion::vector2<
      const std::string&,
      boost::optional< boost::optional<std::vector<dlvhex::ID> > >
    >& source,
    const boost::spirit::unused_type& target)
  {
		// take care module name
		const std::string& mlpModuleName = boost::fusion::at_c<0>(source);
	  Module module(mlpModuleName, mgr.ctx.registry()->inputList.size(), mgr.ctx.edbList.size(), mgr.ctx.idbList.size());
		mgr.ctx.registry()->moduleTable.storeAndGetAddress(module);		    

		// get and insert input list
		// resize +1 to handle if the input list is empty (because it's optional)
    mgr.ctx.registry()->inputList.resize(mgr.ctx.registry()->inputList.size()+1);
		mgr.ctx.registry()->inputList.back() = boost::fusion::at_c<1>(source).get().get() ;

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
    if( source.isAtom() )
    {
      // fact -> put into EDB
      if( !source.isOrdinaryGroundAtom() )
        throw SyntaxError(
          "fact '"+reg->ogatoms.getByID(source).text+"' not safe!");

		  if ( mgr.ctx.registry()->moduleTable.getSize() == 0 )		    
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
		  if ( mgr.ctx.registry()->moduleTable.getSize() == 0 )		    
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
    = cident     [ Sem::termFromCIdent(sem) ]
    | string     [ Sem::termFromString(sem) ]
    | variable   [ Sem::termFromVariable(sem) ]
    | posinteger [ Sem::termFromInteger(sem) ]
    | termExt;
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
        -(qi::lit('(') > -terms >> qi::lit(')')) > qi::eps 
      ) [ Sem::externalAtom(sem) ];
  mlpModuleAtomPredicate
    = cident [ Sem::termFromCIdent(sem) ];
  mlpModuleAtom
    = (
        qi::lit('@') > mlpModuleAtomPredicate >  // for module predicate
        -(qi::lit('[') > -preds >> qi::lit(']')) > qi::eps >  // for input
	qi::lit(':') > qi::lit(':') > classicalAtom > qi::eps // for output
      ) [ Sem::mlpModuleAtom(sem) ];


  predDecl
    = (cident > qi::lit('/') > qi::ulong_)[ Sem::predFromNameArity(sem) ];
  predList
    = (predDecl > qi::eps) % qi::lit(',');

  mlpModuleHeader 
    = ( qi::lit("#module") > // #module
        qi::lit('(') > cident > qi::lit(',') > 		// module name
        -(qi::lit('[') > -predList >> qi::lit(']')) 	// predicate list
        >> qi::lit(')') > qi::eps > qi::lit('.') > qi::eps
      ) [ Sem::moduleHeader(sem) ];

  bodyAtom
    = classicalAtom
    | externalAtom
    | mlpModuleAtom
    | builtinAtom
    | aggregateAtom
    | bodyAtomExt;
  bodyLiteral
    = (
        -qi::lexeme[qi::string("not") >> qi::omit[ascii::space]] >> bodyAtom
      ) [ Sem::bodyLiteral(sem) ];
  headAtom
    = classicalAtom
    | headAtomExt;
  rule
    = (
        (headAtom % qi::no_skip[qi::char_('v') >> ascii::space]) >>
       -(
          qi::lit(":-") >
          (bodyLiteral % qi::char_(','))
        ) >>
        qi::lit('.')
      ) [ Sem::rule(sem) ];
  constraint
    = (
        qi::lit(":-") >>
        (bodyLiteral % qi::char_(',')) >>
        qi::lit('.')
      ) [ Sem::constraint(sem) ];
  toplevelBuiltin
    = (qi::lit("#maxint") > qi::lit('=') > qi::ulong_ >> qi::lit('.') > qi::eps)
        [ Sem::maxint(sem) ];
  toplevel
    = (rule > qi::eps)
        [ Sem::add(sem) ]
    | (constraint > qi::eps)
        [ Sem::add(sem) ]
    | (mlpModuleHeader > qi::eps)
    | (toplevelBuiltin > qi::eps)
    | (toplevelExt > qi::eps)
        [ Sem::ignoreAndWarnIfNotFail(sem) ];
  // the root rule
  start
    = *(toplevel);

  // TODO will weak constraints go into toplevelExt?
  // TODO queries go into toplevelExt
  // TODO namespaces go into toplevelExt
  toplevelExt
    = qi::eps(false);
  // TODO strong negation goes into bodyAtomExt
  bodyAtomExt
    = qi::eps(false);
  // TODO strong negation goes into HeadAtomExt
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
