/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file   Body.h
 * @author Thomas Krennwallner
 * @date   Tue Jul 15 12:01:21 2008
 * 
 * @brief  A rule body.
 * 
 * 
 */

#if !defined(_DLVHEX_BODY_H)
#define _DLVHEX_BODY_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Literal.h"

#include <list>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * A rule body is a conjunction of literals.
 *
 * Since the user should be able to rewrite the literals within a
 * body, we use a std::list here.
 *
 * \sa http://www.sgi.com/tech/stl/List.html
 */
typedef std::list<LiteralPtr> Body;

typedef boost::shared_ptr<Body> BodyPtr;

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_BODY_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
