/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
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
 * @file ASPSolver.cpp
 * @author Thomas Krennwallner
 * @author Peter Schueller
 * @date Tue Jun 16 14:34:00 CEST 2009
 *
 * @brief ASP Solvers
 *
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/AnswerSet.h"

#ifdef HAVE_LIBDLV
# include "dlv.h"
#endif

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <list>

DLVHEX_NAMESPACE_BEGIN

namespace ASPSolver
{

    //
    // DLVLibSoftware (currently not in working condition, libdlv is not released)
    //
    #ifdef HAVE_LIBDLV

    // if this does not work, maybe the old other branch helps
    // (it was not fully working back then either, but maybe there are hints)
    // https://dlvhex.svn.sourceforge.net/svnroot/dlvhex/dlvhex/branches/dlvhex-libdlv-integration@2879

    struct DLVLibSoftware::Delegate::Impl
    {
        Options options;
        PROGRAM_HANDLER *ph;
        RegistryPtr reg;

        Impl(const Options& options):
        options(options),
        ph(create_program_handler()) {
            if( options.includeFacts )
                ph->setOption(INCLUDE_FACTS, 1);
            else
                ph->setOption(INCLUDE_FACTS, 0);
            BOOST_FOREACH(const std::string& arg, options.arguments) {
                if( arg == "-silent" ) {
                    // do nothing?
                }
                else
                    throw std::runtime_error("dlv-lib commandline option not implemented: " + arg);
            }
        }

        ~Impl() {
            destroy_program_handler(ph);
        }
    };

    DLVLibSoftware::Delegate::Delegate(const Options& options):
    pimpl(new Impl(options)) {
    }

    DLVLibSoftware::Delegate::~Delegate() {
    }

    void
    DLVLibSoftware::Delegate::useASTInput(const OrdinaryASPProgram& program) {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVLibSoftware::Delegate::useASTInput");

        // TODO HO checks

        try
        {
            pimpl->reg = program.registry;
            assert(pimpl->reg);
            if( program.maxint != 0 )
                pimpl->ph->setMaxInt(program.maxint);

            pimpl->ph->clearProgram();

            // output program
            std::stringstream programStream;
            RawPrinter printer(programStream, program.registry);
            // TODO HO stuff

            if( program.edb != 0 ) {
                // print edb interpretation as facts
                program.edb->printAsFacts(programStream);
                programStream << "\n";
                programStream.flush();
            }

            printer.printmany(program.idb, "\n");
            programStream.flush();

            DBGLOG(DBG,"sending program to dlv-lib:===");
            DBGLOG(DBG,programStream.str());
            DBGLOG(DBG,"==============================");
            pimpl->ph->Parse(programStream);
            pimpl->ph->ResolveProgram(SYNCRONOUSLY);
        }
        catch(const std::exception& e) {
            LOG(ERROR,"EXCEPTION: " << e.what());
            throw;
        }
    }

    // reuse DLVResults class from above

    ASPSolverManager::ResultsPtr
    DLVLibSoftware::Delegate::getResults() {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVLibSoftware::Delegate::getResults");

        //DBGLOG(DBG,"getting results");
        try
        {
            // for now, we parse all results and store them into the result container
            // later we should do kind of an online processing here

            boost::shared_ptr<DLVResults> ret(new DLVResults);

            // TODO really do incremental model fetching
            const std::vector<MODEL> *models = pimpl->ph->getAllModels();
            std::vector<MODEL>::const_iterator itm;
            // iterate over models
            for(itm = models->begin();
            itm != models->end(); ++itm) {
                AnswerSet::Ptr as(new AnswerSet(pimpl->reg));

                // iterate over atoms
                for(MODEL_ATOMS::const_iterator ita = itm->begin();
                ita != itm->end(); ++ita) {
                    const char* pname = ita->getName();
                    assert(pname);

                    // i have a hunch this might be the encoding
                    assert(pname[0] != '-');

                    typedef std::list<const char*> ParmList;
                    ParmList parms;
                    //DBGLOG(DBG,"creating predicate with first term '" << pname << "'");
                    parms.push_back(pname);

                    // TODO HO stuff
                    // TODO integer terms

                    for(MODEL_TERMS::const_iterator itt = ita->getParams().begin();
                    itt != ita->getParams().end(); ++itt) {
                        switch(itt->type) {
                            case 1:
                                // string terms
                                //std::cerr << "creating string term '" << itt->data.item << "'" << std::endl;
                                parms.push_back(itt->data.item);
                                break;
                            case 2:
                                // int terms
                                //std::cerr << "creating int term '" << itt->data.number << "'" << std::endl;
                                assert(false);
                                //ptuple.push_back(new dlvhex::Term(itt->data.number));
                                break;
                            default:
                                throw std::runtime_error("unknown term type!");
                        }
                    }

                    // for each param in parms: find id and put into tuple
                    WARNING("TODO create something like inline ID TermTable::getByStringOrRegister(const std::string& symbol, IDKind kind)")
                        Tuple ptuple;
                    ptuple.reserve(parms.size());
                    assert(pimpl->reg);
                    for(ParmList::const_iterator itp = parms.begin();
                    itp != parms.end(); ++itp) {
                        // constant term
                        ID id = pimpl->reg->terms.getIDByString(*itp);
                        if( id == ID_FAIL ) {
                            Term t(ID::MAINKIND_TERM  | ID::SUBKIND_TERM_CONSTANT, *itp);
                            id = pimpl->reg->terms.storeAndGetID(t);
                        }
                        assert(id != ID_FAIL);
                        DBGLOG(DBG,"got term " << *itp << " with id " << id);
                        ptuple.push_back(id);
                    }

                    // ogatom
                    ID fid = pimpl->reg->ogatoms.getIDByTuple(ptuple);
                    if( fid == ID_FAIL ) {
                        OrdinaryAtom a(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
                        a.tuple.swap(ptuple);
                        {
                            WARNING("parsing efficiency problem see HexGrammarPTToASTConverter")
                                std::stringstream ss;
                            RawPrinter printer(ss, pimpl->reg);
                            Tuple::const_iterator it = ptuple.begin();
                            printer.print(*it);
                            it++;
                            if( it != ptuple.end() ) {
                                ss << "(";
                                printer.print(*it);
                                it++;
                                while(it != ptuple.end()) {
                                    ss << ",";
                                    printer.print(*it);
                                    it++;
                                }
                                ss << ")";
                            }
                            a.text = ss.str();
                        }
                        fid = pimpl->reg->ogatoms.storeAndGetID(a);
                        DBGLOG(DBG,"added fact " << a << " with id " << fid);
                    }
                    DBGLOG(DBG,"got fact with id " << fid);
                    assert(fid != ID_FAIL);
                    as->interpretation->setFact(fid.address);
                }

                ret->add(as);
            }

            // TODO: is this necessary?
            // delete models;

            ASPSolverManager::ResultsPtr baseret(ret);
            return baseret;
        }
        catch(const std::exception& e) {
            LOG(ERROR,"EXCEPTION: " << e.what());
            throw;
        }
    }
    #endif                       // HAVE_LIBDLV

    WARNING("TODO reactivate dlvdb")

        #if 0
        #if defined(HAVE_DLVDB)
        DLVDBSoftware::Options::Options():
    DLVSoftware::Options(),
    typFile() {
    }

    DLVDBSoftware::Options::~Options() {
    }

    DLVDBSoftware::Delegate::Delegate(const Options& opt):
    DLVSoftware::Delegate(opt),
    options(opt) {
    }

    DLVDBSoftware::Delegate::~Delegate() {
    }

    void DLVDBSoftware::Delegate::setupProcess() {
        DLVSoftware::Delegate::setupProcess();

        proc.setPath(DLVDBPATH);
                                 // turn on database support
        proc.addOption("-DBSupport");
        proc.addOption("-ORdr-");// turn on rewriting of false body rules
        if( !options.typFile.empty() )
            proc.addOption(options.typFile);

    }
    #endif                       // defined(HAVE_DLVDB)
    #endif

}                                // namespace ASPSolver


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
