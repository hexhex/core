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
	public:

	/**
	 * As a container for the rules of a program, a std::set is used. This set
	 * stores only pointers to rules.
	 */
	typedef std::set<const Rule*, boost::indirect_fun<std::less<Rule> > > ruleset_t;

	/**
	 * Custom iterator class, such that we can treat the class program similar
	 * to a container.
	 */
	class const_iterator
	{
		ruleset_t::const_iterator it;

		public:

		const_iterator()
		{
			//assert(0);
		}

		const_iterator(const ruleset_t::const_iterator &it1)
		: it(it1)
		{ }

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
	const_iterator
	begin() const
	{
		return const_iterator(rules.begin());
	}

	/**
	 * Last rule of the program.
	 *
	 * \sa Program::begin()
	 */
	const_iterator
	end() const
	{
		return const_iterator(rules.end());
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
	addRule(const Rule*);

	/**
	 * Adds a weak constraint to the program.
	 *
	 * The weak constraint is added as a pointer. See also Program::addRule()
	 * how to use this.
	 */
	void
	addWeakConstraint(const WeakConstraint*);

	/**
	 * Returns a list of all weak constraints in the program.
	 */
	const std::vector<const WeakConstraint*>&
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
	std::vector<const WeakConstraint*> weakConstraints;

	/**
	 * All external atoms.
	 */
	std::vector<ExternalAtom*> externalAtoms;
};

#endif /* _PROGRAM_H */

/* vim: set noet sw=4 ts=4 tw=80: */
