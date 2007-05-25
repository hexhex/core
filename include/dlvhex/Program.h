/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* -*- C++ -*- */

/**
 * @file Program.h
 * @author Roman Schindlauer
 * @date Tue Mar  7 16:48:47 CET 2006
 *
 * @brief Program class.
 *
 */


#ifndef _PROGRAM_H
#define _PROGRAM_H

#include "dlvhex/Rule.h"
#include "dlvhex/PrintVisitor.h"
#include "boost/ptr_container/indirect_fun.hpp"


/**
 * @brief Program class.
 *
 * A program is a set of rules. It does not include facts, they are stored
 * elsewhere as AtomSet.
 */
class Program
{
	/**
	 * As a container for the rules of a program, a std::set is used. This set
	 * stores only pointers to rules.
	 */
	typedef std::set<Rule*, boost::indirect_fun<std::less<Rule> > > ruleset_t;

public:

	class const_iterator;

	/**
	 * Custom iterator class, such that we can treat the class program similar
	 * to a container.
	 */
	class iterator
	{
		/// needed for conversion.
		friend class const_iterator;

		/// needed for access to iterator::it in Program
		friend class Program;

		ruleset_t::iterator it;

	public:

		iterator()
		{ }

		iterator(const ruleset_t::iterator &it1)
			: it(it1)
		{ }

		/**
		 * Dereference operator.
		 */
		Rule*
		operator *() const
		{
			return (*it);
		}

		void
		operator ++()
		{
			it++;
		}

		bool
		operator== (const iterator& i2) const
		{
			return it == i2.it;
		}

		bool
		operator != (const iterator& i2) const
		{
			return (it != i2.it);
		}
	};


	/**
	 * Custom const_iterator class, such that we can treat the class program similar
	 * to a container.
	 */
	class const_iterator
	{
		ruleset_t::const_iterator it;

	public:

		const_iterator()
		{ }

		/**
		 * Conversion from iterator to const_iterator.
		 */
		const_iterator(const iterator &it1)
			: it(it1.it)
		{ }

		/**
		 * Dereference operator.
		 */
		const Rule*
		operator *() const
		{
			return (*it);
		}

		void
		operator ++()
		{
			it++;
		}

		bool
		operator== (const const_iterator& i2) const
		{
			return it == i2.it;
		}

		bool
		operator != (const const_iterator& i2) const
		{
			return (it != i2.it);
		}
	};

	/**
	 * Returns the first rule of the Program.
	 *
	 * Note that the actual order of the rules has nothing to do with the parsed
	 * input. The first one can be any rule of the program.
	 */
	iterator
	begin() const
	{
		return iterator(rules.begin());
	}

	/**
	 * Last rule of the program.
	 *
	 * \sa Program::begin()
	 */
	iterator
	end() const
	{
		return iterator(rules.end());
	}

	/**
	 * Constructor.
	 */
	Program();

	/**
	 * Adds a rule to the program.
	 *
	 * The rule is added as a pointer. If the rule-object should live longer
	 * than only in the local scope, best practice is here to use the central
	 * registry for storing such objects:
	 *
	 * \code
	 * Rule *rp = new Rule(...)
	 * Registry::Instance()->storeObject(rp);
	 * program.addRule(rp);
	 * \endcode
	 *
	 * By using the Registry, it is secured that the object will be deleted at
	 * program termination.
	 */
	void
	addRule(Rule*);

	/**
	 * Deletes a rule from the program.
	 */
	void
	deleteRule(iterator);

	/**
	 * Adds a weak constraint to the program.
	 *
	 * The weak constraint is added as a pointer. See also Program::addRule()
	 * how to use this.
	 */
	void
	addWeakConstraint(WeakConstraint*);

	/**
	 * Returns a list of all weak constraints in the program.
	 */
	const std::vector<WeakConstraint*>&
	getWeakConstraints() const;

	/**
	 * Returns a list of all external atoms in the program.
	 */
	const std::vector<ExternalAtom*>&
	getExternalAtoms() const;

	/**
	 * Only for debugging purposes. The real output functions are implemented
	 * by the ProgramBuilder class!
	 */
	void
	dump(PrintVisitor&) const;

	private:

	/**
	 * Set of rules.
	 */
	ruleset_t rules;

	/**
	 * All weak constraints.
	 */
	std::vector<WeakConstraint*> weakConstraints;

	/**
	 * All external atoms.
	 */
	std::vector<ExternalAtom*> externalAtoms;
};

#endif /* _PROGRAM_H */

/* vim: set noet sw=4 ts=4 tw=80: */
