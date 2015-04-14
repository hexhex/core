/**
 * @file   MLPSyntaxChecker.h
 * @author Tri Kurniawan Wijaya
 * @date   Fri 02 Sep 2011 03:29:05 PM CEST
 *
 * @brief  Checking syntax for modular logic programs
 */

#if !defined(_DLVHEX_MLPSYNTAXCHECKER_H)
#define _DLVHEX_MLPSYNTAXCHECKER_H

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
//#include "dlvhex2/ModuleTable.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <iostream>
#include <iterator>
#include <string>

DLVHEX_NAMESPACE_BEGIN

/** \brief The complete class to perform syntactic checking on the modular logic programs. */
class DLVHEX_EXPORT MLPSyntaxChecker
{
    private:
        /** \brief ProgramCtx. */
        ProgramCtx ctx;
        /** \brief Detects the arity of a predicate.
         * @param predName Predicate name.
         * @return Arity of \p predName. */
        int getArity(std::string predName);
        /** \brief Detects the arity of a predicate.
         * @param predName Predicate ID.
         * @return Arity of \p predName. */
        int getArity(ID idp);
        /** \brief Extracts the module prefix from a string.
         * @param s String.
         * @return Module prefix of \p s. */
        std::string getStringBeforeSeparator(const std::string& s);
        /** \brief Strips off the module prefix from a string.
         * @param s String.
         * @return String \p s without module prefix. */
        std::string getStringAfterSeparator(const std::string& s);
        /** \brief Checks the input arities of modules.
         * @param module Program module.
         * @param tuple Input tuple.
         * @return True if \p tuple is a valid input to \p module and false otherwise. */
        bool verifyPredInputsArityModuleCall(ID module, Tuple tuple);
        /** \brief Checks the output arities of modules.
         * @param module Program module.
         * @param outputpredicate Output predicate.
         * @return True if \p outputpredicate is a output from \p module and false otherwise. */
        bool verifyPredOutputArityModuleCall(ID module, ID outputpredicate);
        /** \brief Overall validation.
         * @return True if the modular program is syntactically valid. */
        bool verifyAllModuleCalls();

    public:
        /** \brief Constructor.
         * @param ctx1 ProgramCtx. */
        MLPSyntaxChecker(ProgramCtx& ctx1);
        /** \brief Overall validation.
         * @return True if the modular program is syntactically valid. */
        bool verifySyntax();
};

DLVHEX_NAMESPACE_END
#endif                           /* _DLVHEX_MLPSYNTAXCHECKER_H */

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
