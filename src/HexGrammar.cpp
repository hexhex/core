/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file   HexGrammar.cpp
 * @author Peter Schller
 *
 * @brief  Template Instantiations for HexGrammar.h
 *
 * This file is intended to contain mainly template instantiations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

// spirit parser node debugging
//#define BOOST_SPIRIT_DEBUG
// spirit parser node debugging for whitespace parser (comments and spaces)
//#define BOOST_SPIRIT_DEBUG_WS

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/HexGrammar.h"
#include "dlvhex2/HexGrammar.tcc"

DLVHEX_NAMESPACE_BEGIN

// instantiate skip parser grammar
template struct HexParserSkipperGrammar<HexParserIterator>;

// instantiate HEX parser grammar base
template struct HexGrammarBase<HexParserIterator, HexParserSkipper>;

// instantiate HEX parser grammar
template struct HexGrammar<HexParserIterator, HexParserSkipper>;

// HEX grammar semantic action processor
HexGrammarSemantics::HexGrammarSemantics(ProgramCtx& ctx):
ctx(ctx)
{
    mlpMode=0;
}


void HexGrammarSemantics::markExternalPropertyIfExternalBody(
RegistryPtr registry, Rule& r)
{
    Tuple eatoms;
    registry->getExternalAtomsInTuple(r.body, eatoms);
    if( !eatoms.empty() )
        r.kind |= ID::PROPERTY_RULE_EXTATOMS;
}


void HexGrammarSemantics::markModulePropertyIfModuleBody(
RegistryPtr registry, Rule& r)
{
    for(Tuple::const_iterator itt = r.body.begin(); itt != r.body.end(); ++itt) {
        if( itt->isModuleAtom() ) {
            r.kind |= ID::PROPERTY_RULE_MODATOMS;
            return;
        }
    }
}


DLVHEX_NAMESPACE_END
