/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @author Peter Schller
 * @date   Wed Jul  8 14:00:48 CEST 2009
 *
 * @brief  Implementation of HexGrammar.h
 */

/**
 * @file   HexGrammar.tcc
 * @author Peter Schller
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
struct sem<HexGrammarSemantics::termId>
{
    void operator()(HexGrammarSemantics& mgr, const ID& source, ID& target) {
        target = source;
    }
};

template<>
struct sem<HexGrammarSemantics::termFromCIdent>
{
    void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target) {
        assert(!source.empty() && islower(source[0]));
        target = mgr.ctx.registry()->terms.getIDByString(source);
        if( target == ID_FAIL ) {
            Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
            target = mgr.ctx.registry()->terms.storeAndGetID(term);
        }
    }
};

template<>
struct sem<HexGrammarSemantics::termFromFunctionTerm>
{
    void operator()(HexGrammarSemantics& mgr, const boost::fusion::vector2<const std::string, boost::optional<boost::optional<std::vector<ID> > > >& source, ID& target) {
        Term functionSymbol(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, boost::fusion::at_c<0>(source));
        ID fid = mgr.ctx.registry()->terms.getIDByString(functionSymbol.symbol);
        if (fid == ID_FAIL) fid = mgr.ctx.registry()->terms.storeAndGetID(functionSymbol);

        std::vector<ID> args;
        args.push_back(fid);
        if (!!boost::fusion::at_c<1>(source) && !!boost::fusion::at_c<1>(source).get() ) {
            BOOST_FOREACH (ID id, boost::fusion::at_c<1>(source).get().get()) args.push_back(id);
        }

        Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_NESTED, args, mgr.ctx.registry());
        target = mgr.ctx.registry()->terms.getIDByString(term.symbol);
        if (target == ID_FAIL) target = mgr.ctx.registry()->terms.storeAndGetID(term);
    }
};

template<>
struct sem<HexGrammarSemantics::termFromRange>
{
    void operator()(HexGrammarSemantics& mgr, const boost::fusion::vector2<ID, ID>& source, ID& target) {
        std::vector<ID> args;
        Term functionSymbol(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, "range");
        ID fid = mgr.ctx.registry()->terms.getIDByString(functionSymbol.symbol);
        if (fid == ID_FAIL) fid = mgr.ctx.registry()->terms.storeAndGetID(functionSymbol);
        args.push_back(fid);
        args.push_back(boost::fusion::at_c<0>(source));
        args.push_back(boost::fusion::at_c<1>(source));
        Term rangeTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_NESTED | ID::SUBKIND_TERM_RANGE, args, mgr.ctx.registry());
        target = mgr.ctx.registry()->terms.getIDByString(rangeTerm.symbol);
        if (target == ID_FAIL) target = mgr.ctx.registry()->terms.storeAndGetID(rangeTerm);
    }
};

template<>
struct sem<HexGrammarSemantics::termFromInteger>
{
    void operator()(HexGrammarSemantics& mgr, unsigned int source, ID& target) {
        target = ID::termFromInteger(source);
        if (source > mgr.ctx.maxint) mgr.ctx.maxint = source; // by default, set maxint to the largest number in the input
    }
};

template<>
struct sem<HexGrammarSemantics::termFromString>
{
    void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target) {
        assert(!source.empty() && source[0] == '"' && source[source.size()-1] == '"');
        target = mgr.ctx.registry()->terms.getIDByString(source);
        if( target == ID_FAIL ) {
            Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
            target = mgr.ctx.registry()->terms.storeAndGetID(term);
        }
    }
};

template<>
struct sem<HexGrammarSemantics::termFromVariable>
{
    void operator()(HexGrammarSemantics& mgr, const std::string& source, ID& target) {
        assert(!source.empty() && ((source[0] == '_' && source.size() == 1) || isupper(source[0])));
        // special handling of anonymous variables
        IDKind addFlags = 0;
        if( source == "_" ) {
            addFlags |= ID::PROPERTY_VAR_ANONYMOUS;
        }
        // regular handling + flags
        target = mgr.ctx.registry()->terms.getIDByString(source);
        if( target == ID_FAIL ) {
            Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE | addFlags, source);
            target = mgr.ctx.registry()->terms.storeAndGetID(term);
        }
    }
};

// helper method to prefix and store predicates
void storePredicate(const std::string& oriPredName, int predArity, HexGrammarSemantics& mgr, ID& target)
{
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
    if( target == ID_FAIL ) {
        Predicate predicate(ID::MAINKIND_TERM | ID::SUBKIND_TERM_PREDICATE, newPredName, predArity);
        target = mgr.ctx.registry()->preds.storeAndGetID(predicate);
        DBGLOG(DBG, "Preds stored: " << predicate << " got id: " << target);
    }
    else {
        DBGLOG(DBG, "Preds previously stored: " << newPredName << "/" << predArity << " got id: " << target);
    }
}


template<>
struct sem<HexGrammarSemantics::predFromPredDecl>
{
    void operator()(
        HexGrammarSemantics& mgr,
        const boost::fusion::vector2<const std::string&, unsigned int>& source,
    ID& target) {

        const std::string& oriPredName = boost::fusion::at_c<0>(source);
        assert(!oriPredName.empty() && islower(oriPredName[0]));

        unsigned int predArity = boost::fusion::at_c<1>(source);

        std::string newPredName;
        newPredName = mgr.currentModuleName + MODULEPREFIXSEPARATOR + oriPredName;

        target = mgr.ctx.registry()->preds.getIDByString(newPredName);
        if( target == ID_FAIL ) {
            Predicate predicate(ID::MAINKIND_TERM | ID::SUBKIND_TERM_PREDICATE, newPredName, predArity);
            target = mgr.ctx.registry()->preds.storeAndGetID(predicate);
            DBGLOG(DBG, "Preds stored: " << predicate << " got id: " << target);
        }
        else {
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
    ID& target) {

        assert(!source.empty() && islower(source[0]));
        if (mgr.mlpMode == 1) {
                                 // mlp encoding
            int predArity = -1;
            storePredicate(source, predArity, mgr, target);
        }
        else {
                                 // ordinary encoding
            target = mgr.ctx.registry()->terms.getIDByString(source);
            if( target == ID_FAIL ) {
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
    ID& target) {

        assert(!source.empty() && source[0] == '"' && source[source.size()-1] == '"');

        if (mgr.mlpMode == 1) {
                                 // mlp encoding
            const std::string& oriPredName = source;
            int predArity = -1;
            storePredicate(oriPredName, predArity, mgr, target);
        }
        else {
                                 // ordinary encoding
            target = mgr.ctx.registry()->terms.getIDByString(source);
            if( target == ID_FAIL ) {
                Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, source);
                target = mgr.ctx.registry()->terms.storeAndGetID(term);
            }
        }

    }
};

template<>
struct sem<HexGrammarSemantics::classicalAtomFromPrefix>
{
    void createAtom(RegistryPtr reg, OrdinaryAtom& atom, ID& target) {
        // groundness
        DBGLOG(DBG,"checking groundness of tuple " << printrange(atom.tuple));
        IDKind kind = 0;
        std::set<ID> var;
        BOOST_FOREACH(const ID& id, atom.tuple) {
            reg->getVariablesInID(id, var);
            kind |= id.kind;
            // make this sure to make the groundness check work
            // (if we add "builtin constant terms" like #supremum we might have to change the above statement)
            assert((id.kind & ID::SUBKIND_MASK) != ID::SUBKIND_TERM_BUILTIN);
        }
        const bool ground = !(kind & ID::SUBKIND_TERM_VARIABLE) && var.size() == 0;
        if( ground ) {
            atom.kind |= ID::SUBKIND_ATOM_ORDINARYG;
            target = reg->storeOrdinaryGAtom(atom);
        }
        else {
            atom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
            target = reg->storeOrdinaryNAtom(atom);
        }
        DBGLOG(DBG,"stored atom " << atom << " which got id " << target);
    }

    void operator()(
        HexGrammarSemantics& mgr,
        const boost::fusion::vector2<ID, boost::optional<boost::optional<std::vector<ID> > > >& source,
    ID& target) {
        RegistryPtr reg = mgr.ctx.registry();
        OrdinaryAtom atom(ID::MAINKIND_ATOM);

        // predicate
        const ID& idpred = boost::fusion::at_c<0>(source);
        atom.tuple.push_back(idpred);

        // arguments
        if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) ) {
            const Tuple& tuple = boost::fusion::at_c<1>(source).get().get();
            atom.tuple.insert(atom.tuple.end(), tuple.begin(), tuple.end());
            if ( mgr.mlpMode==1 ) mgr.ctx.registry()->preds.setArity(idpred, tuple.size());
        }
        else {
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
    ID& target) {
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
    ID& target) {
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
    ID& target) {
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
    ID& target) {
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
    ID& target) {
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
    ID& target) {
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
                                 // left term and left comparison operator
        boost::optional<boost::fusion::vector2<ID, ID> >,
        boost::fusion::vector2<ID,// aggregate function
        std::vector<             // set of symbolic sets
                                 // variables and literals of the current symbolic set
        boost::fusion::vector2<std::vector<ID>, boost::optional<std::vector<ID> > >
        >
        >,
                                 // right comparison operator and right term
        boost::optional<boost::fusion::vector2<ID, ID> >
        >& source,
    ID& target) {
        AggregateAtom aatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE);
        ID& leftTerm = aatom.tuple[0];
        ID& leftComp = aatom.tuple[1];
        ID& aggFunc = aatom.tuple[2];
        ID& rightComp = aatom.tuple[3];
        ID& rightTerm = aatom.tuple[4];

        // left term + operator
        if( !!boost::fusion::at_c<0>(source) ) {
            leftTerm = boost::fusion::at_c<0>(
                boost::fusion::at_c<0>(source).get());
            leftComp = boost::fusion::at_c<1>(
                boost::fusion::at_c<0>(source).get());
        }

        // right term + operator
        if( !!boost::fusion::at_c<2>(source) ) {
            rightComp = boost::fusion::at_c<0>(
                boost::fusion::at_c<2>(source).get());
            rightTerm = boost::fusion::at_c<1>(
                boost::fusion::at_c<2>(source).get());
        }

        WARNING("TODO throw iterator in syntax error and display it nicely (like expectation failure)")
            if( leftTerm == ID_FAIL && rightTerm == ID_FAIL )
            throw SyntaxError("aggregate needs at least one term + comparison operator");

        // aggregation
        aggFunc = boost::fusion::at_c<0>(boost::fusion::at_c<1>(source));

        // symbolic set
        const std::vector<boost::fusion::vector2<std::vector<ID>, boost::optional<std::vector<ID> > > >& symbolicSets = boost::fusion::at_c<1>(boost::fusion::at_c<1>(source));
        for (int currentSymbolicSet = 0; currentSymbolicSet < symbolicSets.size(); ++currentSymbolicSet) {
            const boost::fusion::vector2<std::vector<ID>, boost::optional<std::vector<ID> > >& symbolicSet = symbolicSets[currentSymbolicSet];
            Tuple aggVariables = boost::fusion::at_c<0>(symbolicSet);
            Tuple aggBody = (!!boost::fusion::at_c<1>(symbolicSet) ? boost::fusion::at_c<1>(symbolicSet).get() : Tuple());
            if (symbolicSets.size() > 1) {
                aatom.mvariables.push_back(aggVariables);
                aatom.mliterals.push_back(aggBody);
            }
            else {
                aatom.variables = aggVariables;
                aatom.literals = aggBody;
            }
        }

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
        const boost::fusion::vector4<
        ID,
        boost::optional<boost::optional<std::vector<ID> > >,
        boost::optional<boost::optional<std::vector<ID> > >,
        boost::optional<boost::optional<std::vector<std::vector<std::string> > > >
        >& source,
    ID& target) {
        ExternalAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);

        // predicate
        atom.predicate = boost::fusion::at_c<0>(source);

        // inputs
        if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) ) {
            atom.inputs = boost::fusion::at_c<1>(source).get().get();
        }

        // outputs
        if( (!!boost::fusion::at_c<2>(source)) &&
        (!!(boost::fusion::at_c<2>(source).get())) ) {
            atom.tuple = boost::fusion::at_c<2>(source).get().get();
        }

        // properties
        if( (!!boost::fusion::at_c<3>(source)) &&
        (!!(boost::fusion::at_c<3>(source).get())) ) {
            atom.prop.interpretProperties(mgr.ctx.registry(), atom, boost::fusion::at_c<3>(source).get().get());
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
        boost::optional<boost::optional<std::vector<std::string> > > >& source,
    std::vector<std::string>& target) {
        target.push_back(boost::fusion::at_c<0>(source));
        if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) ) {
            target.insert(target.end(), boost::fusion::at_c<1>(source).get().get().begin(), boost::fusion::at_c<1>(source).get().get().end());
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
    ID& target) {
        ModuleAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_MODULE);

        // predicate
        atom.predicate = boost::fusion::at_c<0>(source);

        // get actual module name
        const std::string& predName = mgr.ctx.registry()->preds.getByID(atom.predicate).symbol;
        int n = predName.find(MODULEPREFIXSEPARATOR);
        atom.actualModuleName = predName.substr(n+2, predName.length());

        // inputs
        if( (!!boost::fusion::at_c<1>(source)) &&
        (!!(boost::fusion::at_c<1>(source).get())) ) {
            atom.inputs = boost::fusion::at_c<1>(source).get().get();
        }

        // output
        atom.outputAtom = boost::fusion::at_c<2>(source);

        ID atomNewID = mgr.ctx.registry()->matoms.getIDByElement(atom.predicate, atom.inputs, atom.outputAtom);
        if ( atomNewID == ID_FAIL ) {
            DBGLOG(DBG,"storing mlp Module atom " << atom);
            target = mgr.ctx.registry()->matoms.storeAndGetID(atom);
            DBGLOG(DBG,"mlp Module atom " << atom << " got id " << target);
        }
        else {
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
    ID& target) {
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
    ID& target) {
        RegistryPtr reg = mgr.ctx.registry();
        const Tuple& head = boost::fusion::at_c<0>(source);
        bool hasBody = !!boost::fusion::at_c<1>(source);

        if( hasBody ) {
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
        else {
            if( head.size() > 1 ) {
                // disjunctive fact -> create rule
                Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ,
                    head, Tuple());
                mgr.markExternalPropertyIfExternalBody(reg, r);
                mgr.markModulePropertyIfModuleBody(reg, r);
                target = reg->storeRule(r);
            }
            else {
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
    ID& target) {
        RegistryPtr reg = mgr.ctx.registry();
        Tuple head;
        head.push_back(boost::fusion::at_c<0>(source));
        const Tuple& headGuard = boost::fusion::at_c<1>(source);
        bool hasBody = !!boost::fusion::at_c<2>(source);

        if( hasBody ) {
            // rule -> put into IDB
            Tuple body = boost::fusion::at_c<2>(source).get();
            //      body.insert(body.end(), headGuard.begin(), headGuard.end());

            Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, head, body, headGuard);
            if (r.headGuard.size() > 0) r.kind |= ID::PROPERTY_RULE_HEADGUARD;
            mgr.markExternalPropertyIfExternalBody(reg, r);
            mgr.markModulePropertyIfModuleBody(reg, r);
            // mark as disjunctive if required
            if( r.head.size() > 1 )
                r.kind |= ID::PROPERTY_RULE_DISJ;
            target = reg->storeRule(r);
        }
        else {
            // in order to process the head guard we need to create a rule
            Tuple body;
            //      body.insert(body.end(), headGuard.begin(), headGuard.end());
            Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ,
                head, body, headGuard);
            if (r.headGuard.size() > 0) r.kind |= ID::PROPERTY_RULE_HEADGUARD;
            mgr.markExternalPropertyIfExternalBody(reg, r);
            mgr.markModulePropertyIfModuleBody(reg, r);
            target = reg->storeRule(r);
        }
    }
};

template<>
struct sem<HexGrammarSemantics::constraint>
{
    void operator()(
        HexGrammarSemantics& mgr,
        const std::vector<dlvhex::ID>& source,
    ID& target) {
        Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
        r.body = source;
        mgr.markExternalPropertyIfExternalBody(mgr.ctx.registry(), r);
        mgr.markModulePropertyIfModuleBody(mgr.ctx.registry(), r);
        ID existing = mgr.ctx.registry()->rules.getIDByElement(r);
        if( existing == ID_FAIL ) {
            target = mgr.ctx.registry()->storeRule(r);
            DBGLOG(DBG,"created constraint " << r << " with id " << target);
        }
        else
            target = existing;   // ID_FAIL;
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
    ID& target) {
        Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT);
        r.body = boost::fusion::at_c<0>(source);
        r.weakconstraintVector.push_back(ID_FAIL); // DLV-style
        if (!!boost::fusion::at_c<1>(source)) {
            r.weight = boost::fusion::at_c<0>(boost::fusion::at_c<1>(source).get());
            r.level = boost::fusion::at_c<1>(boost::fusion::at_c<1>(source).get());
        }
        else {
            r.weight = ID::termFromInteger(1);
            r.level = ID::termFromInteger(1);
        }
        mgr.markExternalPropertyIfExternalBody(mgr.ctx.registry(), r);
        mgr.markModulePropertyIfModuleBody(mgr.ctx.registry(), r);
        ID existing = mgr.ctx.registry()->rules.getIDByElement(r);
        if( existing == ID_FAIL ) {
            target = mgr.ctx.registry()->storeRule(r);
            DBGLOG(DBG,"created weak constraint " << r << " with id " << target);
        }
        else
            target = existing;   // ID_FAIL;
    }
};

template<>
struct sem<HexGrammarSemantics::weakconstraintaspcore2>
{
    void operator()(
        HexGrammarSemantics& mgr,
        const boost::fusion::vector4<
            const std::vector<dlvhex::ID>&,
            const ID&,
            const boost::optional<ID>&,
            const boost::optional<std::vector<ID> > >& source,
    ID& target) {

        Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT);
        r.body = boost::fusion::at_c<0>(source);
        r.weight = boost::fusion::at_c<1>(source);
        if (!!boost::fusion::at_c<2>(source)){
            r.level = boost::fusion::at_c<2>(source).get();
        }else{
            r.level = ID::termFromInteger(1);
        }

        // ASP-Core-2-style
        if (!!boost::fusion::at_c<3>(source)) {
            r.weakconstraintVector = boost::fusion::at_c<3>(source).get();
        }

        mgr.markExternalPropertyIfExternalBody(mgr.ctx.registry(), r);
        mgr.markModulePropertyIfModuleBody(mgr.ctx.registry(), r);
        ID existing = mgr.ctx.registry()->rules.getIDByElement(r);
        if( existing == ID_FAIL ) {
            target = mgr.ctx.registry()->storeRule(r);
            DBGLOG(DBG,"created weak constraint " << r << " with id " << target);
        }
        else
            target = existing;   // ID_FAIL;
    }
};

template<>
struct sem<HexGrammarSemantics::addMLPModuleName>
{
    void operator()(
        HexGrammarSemantics& mgr,
        const std::string& source,
    std::string& target) {
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
    const boost::spirit::unused_type& target) {
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
        (!!(boost::fusion::at_c<1>(source).get())) ) {
            mgr.ctx.registry()->inputList.back() = boost::fusion::at_c<1>(source).get().get();
        }

        // extend edbList, idbList for the mlp module body
        mgr.ctx.edbList.resize(mgr.ctx.edbList.size()+1);
        mgr.ctx.edbList.back().reset(new Interpretation(mgr.ctx.registry()));
        mgr.ctx.idbList.resize(mgr.ctx.idbList.size()+1);
    }
};

WARNING("look at spirit mailing list 'optimizing parsing of large input'")

template<>
struct sem<HexGrammarSemantics::add>
{
    void operator()(
        HexGrammarSemantics& mgr,
        const dlvhex::ID& source,
    const boost::spirit::unused_type& target) {
        RegistryPtr reg = mgr.ctx.registry();
        assert(source != ID_FAIL);
        if( source.isAtom() ) {
            // fact -> put into EDB
            if( !source.isOrdinaryGroundAtom() )
                throw SyntaxError(
                    "fact '"+reg->onatoms.getByID(source).text+"' not safe!");

            if ( mgr.mlpMode == 0 ) {
                                 // ordinary encoding
                mgr.ctx.edb->setFact(source.address);
            }
            else {
                                 // mlp encoding
                mgr.ctx.edbList.back()->setFact(source.address);
            }
            DBGLOG(DBG,"added fact with id " << source << " to edb");
        }
        else if( source.isRule() ) {
            if ( mgr.mlpMode == 0 ) {
                                 // ordinary encoding
                mgr.ctx.idb.push_back(source);
            }
            else {
                                 // mlp encoding
                mgr.ctx.idbList.back().push_back(source);
            }
            DBGLOG(DBG,"added rule with id " << source << " to idb");
        }
        else {
            // something bad happened if we get no rule and no atom here
            assert(false);
        }
    }
};

template<>
struct sem<HexGrammarSemantics::ignoreAndWarnIfNotFail>
{
    void operator()(
        HexGrammarSemantics&,    // mgr,
        const dlvhex::ID& source,
                                 // target)
    const boost::spirit::unused_type&) {
        if( source != ID_FAIL ) {
            LOG(WARNING,"ignoring ID " << source);
        }
    }
};

template<>
struct sem<HexGrammarSemantics::maxint>
{
    void operator()(
        HexGrammarSemantics& mgr,
        uint32_t source,
                                 // target)
    const boost::spirit::unused_type& ) {
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
        = qi::lexeme[ qi::char_('"') >>
        *( qi::string("\\\"")
        | qi::as_string[qi::char_ - (qi::char_('"') | qi::eol)]
        ) >> qi::char_('"') ];
    variable
        = qi::string("_")        // this can be qi::char_('_') in boost 1.44 ... boost 1.46
        | qi::lexeme[ ascii::upper >> *(ascii::alnum | qi::char_('_')) ];
    posinteger
        = qi::ulong_;
    primitiveTerm
        = cident     [ Sem::termFromCIdent(sem) ]
        | string     [ Sem::termFromString(sem) ]
        | variable   [ Sem::termFromVariable(sem) ]
        | posinteger [ Sem::termFromInteger(sem) ];
    term
        = termExt                                                            [ Sem::termId(sem) ]   // termId is a workaround: for some reason the value of the subexpression must be explicitly copied, otherwise term evaluates to ID_FAIL
        | ( cident >> qi::lit('(') >> -terms >> qi::lit(')') > qi::eps )     [ Sem::termFromFunctionTerm(sem) ]
        | (primitiveTerm >> qi::lit("..") >> primitiveTerm)                  [ Sem::termFromRange(sem) ]
        | primitiveTerm                                                      [ Sem::termId(sem) ];

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
                                 // module for higher order adds a variable here
        | string [ Sem::predFromString(sem) ];

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
    symbolicSet
        = (terms > -(qi::lit(':') > (bodyLiteral % (qi::char_(',') | qi::char_(';')))) > qi::eps);
    aggregateTerm
        = builtinOpsAgg > qi::lit('{') > (symbolicSet % qi::lit(';')) > qi::lit('}');
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

    externalAtomPropertyString
        = qi::lexeme[ (ascii::alnum >> *(ascii::alnum) > qi::eps) ];

    externalAtomProperty
        = (externalAtomPropertyString > -(externalAtomPropertyString % qi::eps) > qi::eps) [ Sem::extSourceProperty(sem) ];

    externalAtomProperties
        = (externalAtomProperty > qi::eps) % qi::lit(',');

    mlpModuleAtomPredicate
        = cident [ Sem::predFromNameOnly(sem) ];

    mlpModuleAtom
        = (
                                 // for module predicate
        qi::lit('@') > mlpModuleAtomPredicate >
                                 // for input
        -(qi::lit('[') > -preds >> qi::lit(']')) > qi::eps >
                                 // for output
        qi::lit(':') > qi::lit(':') > classicalAtom > qi::eps
        ) [ Sem::mlpModuleAtom(sem) ];

    predDecl
        = (cident > qi::lit('/') > qi::ulong_)[ Sem::predFromPredDecl(sem) ];

    predList
        = (predDecl > qi::eps) % qi::lit(',');

    mlpModuleName
        = cident [ Sem::addMLPModuleName(sem) ];

    mlpModuleHeader
        = ( qi::lit("#module") >>// #module
                                 // module name
        qi::lit('(') > mlpModuleName >
                                 // predicate list
        -( qi::lit(',') > qi::lit('[') > -predList >> qi::lit(']') ) > qi::eps >
        qi::lit(')') > qi::eps > qi::lit('.') > qi::eps
        ) [ Sem::addMLPModuleHeader(sem) ];

    bodyAtom
        = bodyAtomExt
        | classicalAtom
        | externalAtom
        | mlpModuleAtom
        | builtinAtom
        | aggregateAtom;

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
        = headAtomExt
        | classicalAtom;

    rule
        = (
        (headAtom % qi::no_skip[*ascii::space >> qi::char_('v') >> ascii::space]) >>
        -(
        qi::lit(":-") >
        (bodyLiteral % (qi::char_(',') | qi::char_(';')))
        ) >>
        qi::lit('.')
        ) [ Sem::rule(sem) ]
        | (
        headAtom >> qi::lit(':') >> (bodyLiteral % qi::char_(',')) >>
        -(
        qi::lit(":-") >
        (bodyLiteral % (qi::char_(',') | qi::char_(';')))
        ) >>
        qi::lit('.')
        ) [ Sem::ruleVariableDisjunction(sem) ];
    constraint
        = (
        qi::lit(":-") >>
        (bodyLiteral % (qi::char_(',') | qi::char_(';'))) >>
        qi::lit('.')
        ) [ Sem::constraint(sem) ];
    weakconstraint
        = (
        qi::lit(":~") >>
        (bodyLiteral % (qi::char_(',') | qi::char_(';'))) >>
        qi::lit('.') >>
        qi::lit("[") >> term >> -(qi::lit("@") >> term) >> -(qi::lit(',') >> (term % qi::char_(','))) >> qi::lit("]")
        ) [ Sem::weakconstraintaspcore2(sem) ]
        | (
        qi::lit(":~") >>
        (bodyLiteral % (qi::char_(',') | qi::char_(';'))) >>
        qi::lit('.') >>
        -(qi::lit("[") >> term >> qi::lit(":") >> term >> qi::lit("]"))
        ) [ Sem::weakconstraint(sem) ];
    toplevelBuiltin
        = (qi::lit("#maxint") > qi::lit('=') > qi::ulong_ >> qi::lit('.') > qi::eps)
        [ Sem::maxint(sem) ];
    toplevel
        = (toplevelExt > qi::eps)
        [ Sem::ignoreAndWarnIfNotFail(sem) ]
        | (rule > qi::eps)
        [ Sem::add(sem) ]
        | (constraint > qi::eps)
        [ Sem::add(sem) ]
        | (weakconstraint > qi::eps)
        [ Sem::add(sem) ]
        | (mlpModuleHeader > qi::eps)
        | (toplevelBuiltin > qi::eps);
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
    BOOST_SPIRIT_DEBUG_NODE(primitiveTerm);
    BOOST_SPIRIT_DEBUG_NODE(externalAtom);
    BOOST_SPIRIT_DEBUG_NODE(externalAtomPredicate);
    BOOST_SPIRIT_DEBUG_NODE(externalAtomPropertyString);
    BOOST_SPIRIT_DEBUG_NODE(externalAtomProperty);
    BOOST_SPIRIT_DEBUG_NODE(externalAtomProperties);
    BOOST_SPIRIT_DEBUG_NODE(mlpModuleAtom);
    BOOST_SPIRIT_DEBUG_NODE(mlpModuleAtomPredicate);
    BOOST_SPIRIT_DEBUG_NODE(classicalAtomPredicate);
    BOOST_SPIRIT_DEBUG_NODE(classicalAtom);
    BOOST_SPIRIT_DEBUG_NODE(builtinAtom);
    BOOST_SPIRIT_DEBUG_NODE(symbolicSet);
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


WARNING("TODO more efficient than " rule = rule.copy() | *module " could be something else (see comments below)")
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
#endif                           // DLVHEX_HEX_GRAMMAR_TCC_INCLUDED


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
