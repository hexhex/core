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
 * @file   MLPSolver.h
 * @author Tri Kurniawan Wijaya
 * @date   Fri 02 Sep 2011 02:18:37 PM CEST
 *
 * @brief  Checking syntax for modular logic programs
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/MLPSyntaxChecker.h"

DLVHEX_NAMESPACE_BEGIN

MLPSyntaxChecker::MLPSyntaxChecker(ProgramCtx& ctx1)
{
    ctx = ctx1;
}


// get the arity of the predicate
int MLPSyntaxChecker::getArity(std::string predName)
{
    return ctx.registry()->preds.getByString(predName).arity;
}


// get the arity of predicate idp
int MLPSyntaxChecker::getArity(ID idp)
{
    if (idp.isTerm()==false) {
        return -2;
    }
    return ctx.registry()->preds.getByID(idp).arity;
}


// s = "p1.p2" will return "p1"
std::string MLPSyntaxChecker::getStringBeforeSeparator(const std::string& s)
{
    int n=s.find(MODULEPREFIXSEPARATOR);
    return s.substr(0, n);
}


// s = "p1.p2" will return "p2"
std::string MLPSyntaxChecker::getStringAfterSeparator(const std::string& s)
{
    int n=s.find(MODULEPREFIXSEPARATOR);
    return s.substr(n+2, s.length());
}


// for example:
// module = p1.p2
// tuple = (q1)
// moduleFullName = p1.p2
// moduleToCall = p2
bool MLPSyntaxChecker::verifyPredInputsArityModuleCall(ID module, Tuple tuple)
{
    // get the module to call
    std::string moduleFullName = ctx.registry()->preds.getByID(module).symbol;
    std::string moduleToCall = getStringAfterSeparator(moduleFullName);

    // get the module that is called
    const Module& moduleCalled = ctx.registry()->moduleTable.getModuleByName(moduleToCall);
    if ( moduleCalled == MODULE_FAIL ) {
        DBGLOG(ERROR,"[MLPSyntaxChecker::verifyPredInputsArityModuleCall] Error: Module '" << moduleToCall << "' not found");
        return false;
    }

    // get the predicate inputs of the module that is being called
    // ModuleHeaderTable::PredSetIndexBySequenced& predSetIndex = predInputs.get<ModuleHeaderTable::bySequenced>();
    // ModuleHeaderTable::PredSetIteratorBySequenced itp = predSetIndex.begin();
    Tuple inputList = ctx.registry()->inputList.at(moduleCalled.inputList);
    Tuple::const_iterator itp = inputList.begin();

    // predArity1 = for predicate arity in module call input
    // predArity2 = for predicate arity in module header that is being called
    int predArity1;

    Tuple::const_iterator it = tuple.begin();
    while ( it != tuple.end() ) {
        predArity1 = getArity(*it);
        if ( predArity1 != -1 ) {
            if (itp==inputList.end()) {
                DBGLOG(ERROR,"[MLPSyntaxChecker::verifyPredInputsArityModuleCall] Error: Too many predicate inputs in '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "'"<< std::endl);
                return false;
            }

            if (predArity1 != ctx.registry()->preds.getByID(*itp).arity) {
                DBGLOG(ERROR,"[MLPSyntaxChecker::verifyPredInputsArityModuleCall] Error: Mismatch predicate inputs arity '" << getStringAfterSeparator(ctx.registry()->preds.getByID(*it).symbol) << "' when calling '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' " << std::endl);
                return false;
            }
        }
        it++;
        itp++;
    }
    if (itp!=inputList.end()) {
        DBGLOG(ERROR,"[MLPSyntaxChecker::verifyPredInputsArityModuleCall] Error: Need more predicate inputs in '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' " << std::endl);
        return false;
    }

    DBGLOG(INFO,"[MLPSyntaxChecker::verifyPredInputsArityModuleCall] Verifying predicate inputs in module call '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' succeeded");
    return true;

}


bool MLPSyntaxChecker::verifyPredOutputArityModuleCall(ID module, ID outputAtom)
{
    // get the module to call
    std::string moduleFullName = ctx.registry()->preds.getByID(module).symbol;
    std::string moduleToCall = getStringAfterSeparator(moduleFullName);

    // get the arity of the outputAtom in the module Call
    OrdinaryAtom oa = ctx.registry()->lookupOrdinaryAtom(outputAtom);
    int arity1 = oa.tuple.size()-1;

    std::string predFullName = ctx.registry()->preds.getByID(oa.tuple.front()).symbol;
    std::string predName = getStringAfterSeparator(predFullName);
    std::string predNewName = moduleToCall + MODULEPREFIXSEPARATOR + predName;
    int arity2 = getArity(ctx.registry()->preds.getIDByString(predNewName));
    //  std::string predName = ctx.registry()->preds.getByID(oa.tuple.front()).symbol;
    //  int arity2 = getArity(ctx.registry()->preds.getIDByString(predName));

    if (arity1 == arity2) {
        DBGLOG(INFO,"[MLPSyntaxChecker::verifyPredInputsArityModuleCall] Verifying predicate output of module call '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' succeeded");
        return true;
    }
    else {
        DBGLOG(ERROR,"[MLPSyntaxChecker::verifyPredInputsArityModuleCall] Error: Verifying predicate output '" << predName << "' of module call '@" << getStringAfterSeparator(moduleFullName) << "' in module '" << getStringBeforeSeparator(moduleFullName) << "' failed" << std::endl);
        return false;
    }
}


bool MLPSyntaxChecker::verifyAllModuleCalls()
{
    ModuleAtomTable::AddressIterator it, it_end;
    boost::tie(it, it_end) = ctx.registry()->matoms.getAllByAddress();
    while (it!=it_end) {
        ModuleAtom ma = *it;
        // Verifying pred Inputs
        if (verifyPredInputsArityModuleCall(ma.predicate, ma.inputs) == false) {
            DBGLOG(ERROR,"[MLPSyntaxChecker::verifyAllModuleCall] Error: Verifying predicates input and output for all module calls failed in " << ma << std::endl);
            return false;
        }
        // Verifying pred Ouput
        if (verifyPredOutputArityModuleCall(ma.predicate, ma.outputAtom) == false) {
            DBGLOG(ERROR,"[MLPSyntaxChecker::verifyAllModuleCall] Error: Verifying predicates input and output for all module calls failed in " << ma << std::endl);
            return false;
        }
        it++;
    }
    DBGLOG(INFO,"[MLPSyntaxChecker::verifyAllModuleCall] Verifying predicates input and output for all module calls succeeded");
    return true;
}


bool MLPSyntaxChecker::verifySyntax()
{
    bool result = verifyAllModuleCalls();
    // successful verification?
    if( result ==  false )
        throw FatalError("MLP syntax error");
    return result;
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
