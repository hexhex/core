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
 * @file SafetyChecker.h
 * @author Roman Schindlauer
 * @author Peter Schueller
 * @date Mon Feb 27 15:09:49 CET 2006
 *
 * @brief Class for checking rule and program safety.
 *
 *
 */


#ifndef _DLVHEX_SAFETYCHECKER_H
#define _DLVHEX_SAFETYCHECKER_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Error.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Abstract base class for the SaftyCheckers
 */
class DLVHEX_EXPORT SafetyCheckerBase
{
protected:
  const ProgramCtx& ctx;
  
public:
  SafetyCheckerBase(const ProgramCtx& ctx);
  virtual ~SafetyCheckerBase();

public:
  
  /// operator() does the safety check
  virtual void
  operator() () const throw (SyntaxError) = 0;
};


/**
 * @brief Safety checker class.
 */
class DLVHEX_EXPORT SafetyChecker : public SafetyCheckerBase
{
public:
  SafetyChecker(const ProgramCtx& ctx);
  virtual ~SafetyChecker();

  virtual void
  operator() () const throw (SyntaxError);

  Tuple checkSafety(bool throwOnUnsafeVariables) const throw (SyntaxError);
};


/**
 * @brief Strong safety checker class.
 */
class DLVHEX_EXPORT StrongSafetyChecker : public SafetyCheckerBase
{
public:
  StrongSafetyChecker(const ProgramCtx& ctx);
  virtual ~StrongSafetyChecker();
  
  virtual void
  operator() () const throw (SyntaxError);
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_SAFETYCHECKER_H_ */


// Local Variables:
// mode: C++
// End:
