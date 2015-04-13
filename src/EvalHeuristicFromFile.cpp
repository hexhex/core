/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
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
 * @file EvalHeuristicFromFile.cpp
 * @author Peter Schller
 *
 * @brief Implementation of an evaluation heuristic that simply executes commands from a file.
 *
 * Such files are best created after creating a component graph .dot file using the dlvhex
 * --graphviz=comp option. IDs of the components can be specified in the command file.
 * The command file has lines of the form "collapse <id> <id> <id> ...".
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/EvalHeuristicFromFile.h"
#include "dlvhex2/EvalHeuristicShared.h"
#include "dlvhex2/Logger.h"

#include <boost/unordered_map.hpp>
#include <boost/graph/properties.hpp>

//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/qi.hpp>

#include <fstream>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

using namespace evalheur;

namespace
{

    typedef std::vector<unsigned> IdxVector;

    struct IdxBuildCommand
    {
        // components to collapse to unit
        IdxVector collapse;
        // components to share into unit (constraint components)
        IdxVector share;
    };
    typedef std::vector<IdxBuildCommand> IdxCommandVector;

    void translateIdxBuildCommandsToBuildCommands(
        const IdxCommandVector& icommands,
        const ComponentGraph& cg,
    CommandVector& commands) {
        // get components in order of iteration into vector
        std::vector<Component> indexableComps;
        {
            ComponentGraph::ComponentIterator cit, cit_end;
            for(boost::tie(cit, cit_end) = cg.getComponents();
            cit != cit_end; ++cit) {
                indexableComps.push_back(*cit);
            }
        }

        BOOST_FOREACH(const IdxBuildCommand& ibc, icommands) {
            BuildCommand bc;
            BOOST_FOREACH(unsigned idx, ibc.collapse) {
                assert(idx < indexableComps.size() && "got bad component index");
                bc.collapse.push_back(indexableComps[idx]);
            }
            BOOST_FOREACH(unsigned idx, ibc.share) {
                assert(idx < indexableComps.size() && "got bad component index");
                bc.share.push_back(indexableComps[idx]);
            }
            commands.push_back(bc);
        }
    }

}


EvalHeuristicFromFile::EvalHeuristicFromFile(const std::string& fname):
Base(),
fname(fname)
{
}


EvalHeuristicFromFile::~EvalHeuristicFromFile()
{
}


// for an example command file with only collapsing see examples/extatom3_evalplan.txt
// for an example command file with collapsing and cloning see examples/extatom3_evalplanb.txt
void parseCommandFile(const std::string& fname, IdxCommandVector& commands);

// manual strategy:
// get commands from file
void EvalHeuristicFromFile::build(EvalGraphBuilder& builder)
{
    const ComponentGraph& compgraph = builder.getComponentGraph();

    // get commands from file
    IdxCommandVector icommands;
    parseCommandFile(fname, icommands);

    CommandVector commands;
    translateIdxBuildCommandsToBuildCommands(
        icommands, compgraph, commands);

    executeBuildCommands(commands, builder);

    // create eval units for all components not specified in the file and warn

    // topologically sort all components that are still left at that point
    ComponentContainer sortedleftovers;
    topologicalSortComponents(builder.getComponentGraphRest(), sortedleftovers);

    // create evaluation units
    for(ComponentContainer::const_iterator it = sortedleftovers.begin();
    it != sortedleftovers.end(); ++it) {
        std::list<Component> comps, ccomps;
        comps.push_back(*it);
        EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(comps, ccomps);
        LOG(WARNING,"component " << *it << " was not specified in input file "
            "and became eval unit " << u);
    }
}


namespace
{

    struct sem
    {
        // this is just to create useful compiler/linker errors
        template<typename Attrib, typename Context>
            void operator()(const Attrib& attrib, Context& c, boost::spirit::qi::unused_type) const;

        template<typename Context>
            void operator()(const
            boost::fusion::vector2<
            std::vector<unsigned int>,
            boost::optional<std::vector<unsigned int> >
            >& attrib, Context& c, boost::spirit::qi::unused_type) const
        {
            IdxBuildCommand& target = boost::fusion::at_c<0>(c.attributes);
            const std::vector<unsigned int>& collapse = boost::fusion::at_c<0>(attrib);
            target.collapse = collapse;
            if( !!boost::fusion::at_c<1>(attrib) ) {
                const std::vector<unsigned int>& share = boost::fusion::at_c<1>(attrib).get();
                target.share = share;
            }
        }
    };

}


// for an example command file see examples/extatom3_evalplan.txt
void parseCommandFile(const std::string& fname, IdxCommandVector& commands)
{
    std::string input;
    WARNING("we should directly parse from stream") {
        std::ifstream in(fname.c_str());
        std::ostringstream buf;
        buf << in.rdbuf();
        input = buf.str();
    }
    std::string::const_iterator inp_begin = input.begin(), inp_end = input.end();

    namespace spirit = ::boost::spirit;
    namespace qi = ::boost::spirit::qi;
    qi::rule<std::string::const_iterator, IdxBuildCommand(), spirit::ascii::space_type> cmd;
    cmd = (qi::lit("collapse") >> +(qi::uint_) >> -(qi::lit("share") >> +(qi::uint_))) [ sem() ];
    qi::rule<std::string::const_iterator, IdxCommandVector(), spirit::ascii::space_type> root;
    root = *( cmd );
    #ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_NODE(root);
    #endif
    bool success =
        qi::phrase_parse(
        inp_begin, inp_end,
        root >> qi::eoi,
        spirit::ascii::space, commands);
    if (!success || inp_begin != inp_end) {
        LOG(ERROR,"success=" << success << " (begin!=end)=" << (inp_begin != inp_end));
        throw SyntaxError("Could not parse complete collapse command file!");
    }
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
