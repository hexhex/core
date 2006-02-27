/* -*- C++ -*- */

/**
 * @file SafetyChecker.cpp
 * @author Roman Schindlauer
 * @date Mon Feb 27 15:08:46 CET 2006
 *
 * @brief Class for checking rule and program safety.
 *
 *
 */

#include <sstream>

#include "dlvhex/SafetyChecker.h"
#include "dlvhex/globals.h"


SafetyChecker::SafetyChecker(const Program& program,
                             const DependencyGraph* dg)
{
    if (global::optionVerbose)
        std::cout << std::endl << "@@@ Checking Safety @@@" << std::endl << std::endl;

    testRules(program);

    testStrongSafety();
}


void
SafetyChecker::testRules(const Program& program) const
{
    Program::const_iterator progb = program.begin(), proge = program.end();

    while (progb != proge)
    {
        //
        // testing for simple rule safety:
        // * Each variable occurs in a positive ordinary atom.
        // * A variable occurs in the output list of an external atom and all
        //   input variables occur in a positive ordinary atom.
        //
        // -> 1) get all ordinary body atoms -> safeset
        //    2) look at extatoms: each input var must be in safeset
        //    3) if all is ok: add ext-atom arguments to safeset
        //    4) test if all head vars are in safeset
        //

        const RuleHead_t head = (*progb)->getHead();
        const RuleBody_t body = (*progb)->getBody();

        //
        // set of all variables in non-ext body atoms
        //
        std::set<Term> bodyvars;

        RuleBody_t::const_iterator bb = body.begin(), be = body.end();

        //
        // going through the rule body
        //
        while (bb != be)
        {
            Tuple bodyarg = (*(bb++))->getAtom()->getArguments();

            Tuple::const_iterator bodytupb = bodyarg.begin(), bodytupe = bodyarg.end();

            while (bodytupb != bodytupe)
                if ((*bodytupb).isVariable())
                    bodyvars.insert(*(bodytupb++));
        }
        


        RuleHead_t::const_iterator hb = head.begin(), he = head.end();

        //
        // going through the rule head
        //
        while (hb != he)
        {
            Tuple headarg = (*(hb++))->getArguments();

            Tuple::const_iterator tupb = headarg.begin(), tube = headarg.end();

            //
            // for each head atom: going through its arguments
            //
            while (tupb != tube)
            {
                Term t(*(tupb++));

                //
                // does this variable occur in any positive body atom?
                //
                if (t.isVariable())
                {
                    if (find(bodyvars.begin(), bodyvars.end(), t) == bodyvars.end())
                    {
                        throw InputError((*progb)->getFile(),
                                         (*progb)->getLine(),
                                         "rule not safe");
                    }
                }
            }
        }

        std::vector<ExternalAtom*>::const_iterator extb = (*progb)->getExternalAtoms().begin();
        std::vector<ExternalAtom*>::const_iterator exte = (*progb)->getExternalAtoms().end();

        //
        // going through the external atoms
        //
        while (extb != exte)
        {
            extb++;
        }

        //
        // next rule
        //
        ++progb;
    }
}


void
SafetyChecker::testStrongSafety() const
{
}
