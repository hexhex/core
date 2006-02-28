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

    testStrongSafety(dg);
}


void
SafetyChecker::testRules(const Program& program) const
{
    Program::const_iterator ruleit = program.begin();

    while (ruleit != program.end())
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

        const RuleHead_t head = (*ruleit)->getHead();
        const RuleBody_t body = (*ruleit)->getBody();

        //
        // set of all variables in non-ext body atoms
        //
        std::set<Term> safevars;

        //
        // 1)
        // going through the rule body
        //

        RuleBody_t::const_iterator bb = body.begin(), be = body.end();

        while (bb != be)
        {
            //
            // only look at ordinary atoms
            //
            if (typeid(*((*bb)->getAtom())) == typeid(Atom))
            {
                Tuple bodyarg = (*bb)->getAtom()->getArguments();

                Tuple::const_iterator ordterm = bodyarg.begin();

                while (ordterm != bodyarg.end())
                {
                    if ((*ordterm).isVariable())
                        safevars.insert(*ordterm);

                    ordterm++;
                }
            }

            bb++;
        }
        

        //
        // 2)
        // going through the external atoms
        //
        
        const std::vector<ExternalAtom*> extatoms = (*ruleit)->getExternalAtoms();
        
        std::vector<ExternalAtom*>::const_iterator extit = extatoms.begin();

        while (extit != extatoms.end())
        {
            Tuple inp = (*extit)->getInputTerms();

            Tuple::const_iterator inpterm = inp.begin();

            while (inpterm != inp.end())
                if (safevars.find(*inpterm++) == safevars.end())
                    throw InputError((*ruleit)->getFile(), (*ruleit)->getLine(), "rule not safe");

            //
            // 3)
            // this ext-atom is safe - we can add its arguments to the safe set
            //
            Tuple extarg = (*extit)->getArguments();

            Tuple::const_iterator extterm = extarg.begin();

            while (extterm != extarg.end())
                if ((*extterm).isVariable())
                    safevars.insert(*extterm++);

            extit++;
        }


        RuleHead_t::const_iterator hb = head.begin();

        //
        // 4)
        // going through the rule head
        //
        while (hb != head.end())
        {
            Tuple headarg = (*(hb++))->getArguments();

            Tuple::const_iterator headterm = headarg.begin();

            //
            // for each head atom: going through its arguments
            //
            while (headterm != headarg.end())
            {
                Term t(*(headterm++));

                //
                // does this variable occur in any positive body atom?
                //
                if (t.isVariable())
                {
                    if (find(safevars.begin(), safevars.end(), t) == safevars.end())
                    {
                        throw InputError((*ruleit)->getFile(),
                                         (*ruleit)->getLine(),
                                         "rule not safe");
                    }
                }
            }
        }

        if (global::optionVerbose)
            std::cout << "Rule in line " << (*ruleit)->getLine() << " is safe." << std::endl;
        //
        // next rule
        //
        ++ruleit;
    }
}


void
SafetyChecker::testStrongSafety(const DependencyGraph* dg) const
{
    //
    // testing for strong safety:
    //
    // A rule is strongly safe, if
    // * it is safe and
    // * if an external atom in the rule is part of a cycle, each variable in
    //   its output list occurs in a positive atom in the body, which does not
    //   belong to the cycle.
    //

    //
    // go through all program components
    // (a ProgramComponent is a SCC with external atom!)
    //
    const std::vector<Component*> components = dg->getComponents();

    std::vector<Component*>::const_iterator compit = components.begin();

    while (compit != components.end())
    {
        if (typeid(**compit) == typeid(ProgramComponent))
        {
            //
            // go through all rules of this component
            //
            ProgramComponent* progcomp = dynamic_cast<ProgramComponent*>(*compit);

            const Program rules = progcomp->getBottom();

            for (Program::const_iterator ruleit = rules.begin();
                 ruleit != rules.end();
                 ++ruleit)
            {
                const RuleHead_t head = (*ruleit)->getHead();
                const RuleBody_t body = (*ruleit)->getBody();

                //
                // for this rule: go through all ext-atoms
                //
                for (std::vector<ExternalAtom*>::const_iterator extit = (*ruleit)->getExternalAtoms().begin();
                     extit != (*ruleit)->getExternalAtoms().end();
                     ++extit)
                {
                    //
                    // is this atom also in the component?
                    // (not all the atoms in the bottom of a component are also
                    // in the component themselves!)
                    //
                    if (!progcomp->isInComponent(*extit))
                        continue;

                    //
                    // ok, this external atom is in the cycle of the component:
                    // now we have to check if each of its output arguments is
                    // strongly safe, i.e., if it occurs in another atom in the
                    // body, which is NOT part of the cycle
                    //
                    Tuple output = (*extit)->getArguments();

                    //
                    // look at all terms in its output list
                    //
                    for (Tuple::const_iterator outterm = output.begin();
                         outterm != output.end();
                         ++outterm)
                    {
                        bool argIsUnsafe = true;

                        RuleBody_t::const_iterator bodylit = body.begin();

                        while (bodylit != body.end())
                        {
                            //
                            // only look at atoms that are not part of the
                            // component!
                            // and only look at ordinary or external atoms
                            // builtins do not make a variable safe!
                            //
                            if ((typeid(*(*bodylit)->getAtom()) == typeid(Atom)) ||
                                (typeid(*(*bodylit)->getAtom()) == typeid(ExternalAtom)))
                            {
                                if (!(*compit)->isInComponent((*bodylit)->getAtom()))
                                {
                                    //
                                    // the arguments of this atom are safe
                                    //
                                    Tuple safeargs = (*bodylit)->getAtom()->getArguments();

                                    //
                                    // now see if the current
                                    // extatom-output-argument is one of those
                                    // safe vars
                                    //
                                    for (Tuple::const_iterator safeterm = safeargs.begin();
                                        safeterm != safeargs.end();
                                        ++safeterm)
                                    {
                                        if ((*safeterm).isVariable())
                                        {
                                            if (*safeterm == *outterm)
                                                argIsUnsafe = false;
                                        }
                                    }
                                }
                            }

                            bodylit++;
                        }

                        if (argIsUnsafe)
                            throw InputError((*ruleit)->getFile(),
                                             (*ruleit)->getLine(),
                                             "rule not safe");
                    }
                }
            }
        }

        compit++;
    }
}
