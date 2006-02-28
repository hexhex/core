/* -*- C++ -*- */

/**
 * @file SafetyChecker.h
 * @author Roman Schindlauer
 * @date Mon Feb 27 15:09:49 CET 2006
 *
 * @brief Class for checking rule and program safety.
 *
 *
 */


#ifndef _SAFETYCHECKER_H
#define _SAFETYCHECKER_H


#include "dlvhex/Rule.h"
#include "dlvhex/DependencyGraph.h"
#include "dlvhex/errorHandling.h"

/**
 * @brief Abstract bae class
 */
class SafetyCheckerBase
{
public:

protected:

    /// Ctor.
    SafetyCheckerBase();

public:

};


/**
 * @brief Safety checker class.
 */
class SafetyChecker : public SafetyCheckerBase
{
public:

    SafetyChecker(const Program&);

    void
    testRules(const Program&) const;
};


/**
 * @brief Strong safety checker class.
 */
class StrongSafetyChecker : public SafetyChecker
{
public:

    StrongSafetyChecker(const Program&, const DependencyGraph*);

    void
    testStrongSafety(const DependencyGraph*) const;
};


#endif /* _SAFETYCHECKER_H_ */
