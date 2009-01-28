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
 * @file   Literal.h
 * @author Roman Schindlauer
 * @date   Sun Sep 4 12:39:40 2005
 * 
 * @brief  Literal class.
 * 
 * 
 */

#if !defined(_DLVHEX_LITERAL_H)
#define _DLVHEX_LITERAL_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Atom.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/Repository.h"

#include <set>

#include <boost/ptr_container/indirect_fun.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Literal class.
 *
 * A literal is the constituting part of a rule body. It can be an atom or a
 * weakly negated Atom. The atom of a literal can both be an ordinary as well as
 * an external atom.
 */
class DLVHEX_EXPORT Literal : public ProgramObject
{
public:

	/**
	 * Destructor.
	 */
	~Literal();


	/**
	 * @brief Construct a literal containing the specified atom, possibly weakly
	 * negated.
	 *
	 * \sa AtomPtr
	 */
        explicit
	Literal(const AtomPtr, bool naf = false);

	/**
	 * @brief Assignment operator.
	 */
	Literal&
	operator=(const Literal&);


	/**
	 * @brief Returns a pointer to the atom of the literal.
	 *
	 * \sa AtomPtr
	 */
	const AtomPtr
	getAtom() const;


	/**
	 * @brief returns true if the literal's atom is weakly negated, otherwise false.
	 */
	bool
	isNAF() const;


	/**
	 * @brief Test for equality.
	 *
	 * Two Literals are equal, if they contain the same atom and neither or both
	 * are weakly negated.
	 */
	bool
	operator== (const Literal& lit2) const;

	/**
	 * Inequality test.
	 *
	 * \sa Literal::operator==
	 */
	bool
	operator!= (const Literal& lit2) const;

	/**
	 * Less-than operator.
	 *
	 * A Literal is "smaller" than another, if the first is not weakly negated
	 * but the second is. If none or both are weakly negated, then their atoms
	 * are compared.
	 *
	 * \sa Atom::operator<
	 */
	bool
	operator< (const Literal& lit2) const;

	/**
	 * @brief Accepts a visitor.
	 *
	 * According to the visitor pattern, accept simply calls the respective
	 * visitor with the Literal itself as parameter.
	 *
	 * \sa http://en.wikipedia.org/wiki/Visitor_pattern
	 */
	virtual void
	accept(BaseVisitor&);

	friend std::ostream&
	operator<<(std::ostream&, const Literal&);

private:

	/**
	 * No default constructed void Literals.
	 */
	Literal();

	/**
	 * Atom of the Literal.
	 */
	const AtomPtr atom;

	/**
	 * Weak negation flag.
	 */
	bool isWeaklyNegated;
};

/**
 * A rule body is a conjunction of literals.
 *
 * Since the literals within a body are unordered, we use a std::set here. This
 * container provides the usual methods insert(), begin(), end(), the
 * corresponding iterators, etc.
 *
 * \sa http://www.sgi.com/tech/stl/set.html
 */
typedef std::set<Literal*, boost::indirect_fun<std::less<Literal> > > RuleBody_t;

/**
 * Comparison operator for rule bodies.
 *
 * Used for comparing rules.
 */
bool
operator< (const RuleBody_t& body1, const RuleBody_t& body2);


/**
 * Direct serialization of a Literal using a RawPrintVisitor.
 *
 * Should be used for debugging or verbose only.
 */
std::ostream&
operator<<(std::ostream&, const Literal&);

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_LITERAL_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
