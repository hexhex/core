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
 * @file Term.h
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Term class.
 *
 * Contains the declaration of the class Term as well as the typedef of Tuple
 * and the serializing operators for both Term and Tuple.
 */



#if !defined(_DLVHEX_TERM_H)
#define _DLVHEX_TERM_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/NamesTable.h"

#include <iosfwd>
#include <string>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

/**
 * Class representing a term.
 *
 * A Term can be a variable, constant or null constant. A constant is either
 * a number, a symbol (alphanumeric character sequence), or a string (= quoted symbol).
 * A null constant is a "don't care" term.
 * The type is determined by the constructor when a Term object is created (see
 * Term::TermType).
 *
 * Examples of predicates with different term types:
 *
 * <table style="border: 0;"> 
 * <tr><td>p(4)</td><td>4 is a number term.</td></tr>
 * <tr><td>q("42")</td><td>	"42" is a string term.</td></tr>
 * <tr><td>foo("$% \"a\\"")</td><td>"$% \"a\\"" is a string term.</td></tr>
 * <tr><td>bar(name)</td><td>name is a constant term.</td></tr>
 * <tr><td>f(Name,s)</td><td>Name is a variable term, s is a constant
 * term.</td></tr>
 * <tr><td>g(X,Y,_)</td><td>X and Y are variable terms, _ is a null
 * constant.</td></tr>
 * </table>
 */
class DLVHEX_EXPORT Term
{
public:
	
	/**
	 * @brief Type of the term.
	 *
	 * An INTEGER is a number. A SYMBOL is a string containing only [a-zA-Z_0-9]
	 * and starting with a lowercase letter. A STRING is a double-quoted array of
	 * characters, within the quotes everything is permitted. A VARIABLE is
	 * defined like a SYMBOL, except for beginning with an uppercase letter. A
	 * NULLCONST is an anonymous term.
	 */
	typedef enum { INTEGER, SYMBOL, STRING, VARIABLE, NULLCONST } TermType;

	/**
	 * @brief Default Constructor.
	 *
	 * The default constructor creates a term of type NULLCONST.
	 */
	Term();

	/**
	 * @brief Creates a constant string term.
	 *
	 * @param symbolname create a Term using passed parameter
	 * @param addQuotes If true, then the string will be quoted, but only if it
	 * is not already a quoted string.
	 *
	 * If the passed string is quoted or the flag addQuotes is true, then the
	 * type of the term will be STRING.  Otherwise it is of type SYMBOL, if the
	 * first character is lowercase, or of type VARIABLE if uppercase. In order
	 * to create a null constant term, use the default constructor Term().
	 */
        explicit
	Term(const std::string& symbolname, bool addQuotes = false); 

	/**
	 * @brief Same as the Term constructor with std::string.
	 */
        explicit
	Term(const char*, bool addQuotes = false); 

	/**
	 * @brief Creates a constant integer term. Type will be 'INTEGER'.
	 */
	explicit
	Term(int);

	/**
	 * Copy constructor.
	 *
	 * The copy constructor behaves as expected, creating an exact copy of a
	 * term.
	 */
	Term(const Term&);


	/**
	 * @brief Assignment operator.
	 */
	Term&
	operator= (const Term&);


	/**
	 * @brief Returns the Type of the term.
	 */
	TermType
	getType() const;

	/**
	 * @brief Returns 1 if the term is a constant integer.
	 */
	bool
	isInt() const;  

	/**
	 * @brief Returns 1 if the term is a constant quoted string.
	 */
	bool
	isString() const; 

	/**
	 * @brief Returns 1 if the term is a constant symbol.
	 */
	bool
	isSymbol() const;

	/**
	 * @brief Returns 1 if the term is a Variable.
	 */
	bool
	isVariable() const;

	/**
	 * @brief Returns 1 if the term is anonymous.
	 */
	bool
	isAnon() const;

	/**
	 * Returns the string of the term.
	 *
	 * Returns the symbol string, if the constant is of type SYMBOL.
	 * In case of a STRING constant, the quoted string is returned.
	 * Other term types will raise a failed assertion.
	 */
	const std::string&
	getString() const; 

	/**
	 * Returns a string without quotes.
	 *
	 * The term needs to be of type CONSTANT or STRING. A CONSTANT is returned
	 * as is, a STRING is returned with stripped quotes.
	 */
	inline std::string
	getUnquotedString() const
	{
		return type == STRING ?
			std::string(getString().c_str() + 1, getString().length() - 2) :
			getString();
	}

	/**
	 * Returns the constant integer.
	 *
	 * If the term is not of type INTEGER, an assertion fails.
	 */
	int
	getInt() const;

	/**
	 * Returns the variable identifier the term was constructed with.
	 *
	 * If the term is not of type VARIABLE, an assertion fails.
	 */
	const std::string&
	getVariable() const;

	/**
	 * Returns 1 if the term is if type NULLCONST.
	 */
	bool
	isNull() const; 
	
	/**
	 * Tests for unification with another term.
	 *
	 * Two variables unify, as well as one variable and one constant.
	 * Two constants or strings unify, if they are equal and of same type.
	 * A null constant unifies with every other term.
	 */
	bool
	unifiesWith(const Term&) const;

	/**
	 * Comparison function for two terms.
	 *
	 * The passed term2 is compared with *this. If *this and term2 have
	 * different types, a value != 0 is returned.  In case both terms are
	 * INTEGER, *this - term2 is returned. If both are STRING or CONSTANT, 0 is
	 * returned if the strings are equal and a value != 0 otherwise
	 * (lexicographical comparison through std::string::compare() with term2 as
	 * argument). If both
	 * terms are VARIABLE, the result of the std::string::compare() function is
	 * returned, with term2 as argument, i.e. the variable identifiers are
	 * lexicographically compared.
	 */
	int
	compare(const Term& term2) const;

	/**
	 * Inequality operator.
	 *
	 * Two terms are not equal, if they are of different type, or if their
	 * constants (numbers, strings, variables or symbols) differ.
	 *
	 * @see Term::compare()
	 */
	bool
	operator!= (const Term&) const;  

	/**
	 * Equality operator, compares two terms. Returns the negation of
	 * Term::operator!=.
	 *
	 * @see Term::compare()
	 */
	bool
	operator== (const Term&) const; 

	/**
	 * Another equality operator, which first constructs a term of a given string
	 * and then compares the two terms.
	 *
	 * @see Term::Term(const std::string&, bool addQuotes = false), Term::compare()
	 */
	bool
	operator== (const std::string&) const; 

	/**
	 * Less-than operator.
	 *
	 * If the terms are of different type, the operator returns true. For two
	 * integer terms, it works as expected. Strings, symbols and variables are
	 * compared lexicographically.
	 *
	 * @see Term::compare()
	 */
	bool
	operator< (const Term&) const;

	/**
	 * @brief Less-or-equal operator.
	 */
	bool
	operator<= (const Term&) const;

	/**
	 * @brief Greater-than operator.
	 */
	bool
	operator> (const Term&) const;

	/**
	 * @brief Greater-or-equal operator.
	 */
	bool
	operator>= (const Term&) const; 

	/**
	 * @brief Insert a name into the list of auxiliary predicates.
	 *
	 * Predicates that are generated internally and registered here will be
	 * filtered out from the final result.
	 */
	static void
	registerAuxiliaryName(const std::string&);

	/**
	 * @brief Returns the list of auxiliary predicates.
	 *
	 * The list is given in terms of a NamesTable.
	 */
	static const NamesTable<std::string>&
	getAuxiliaryNames();

	/**
	 * @brief Returns the list of auxiliary predicates.
	 *
	 * The list is given in terms of a NamesTable.
	 */
	static NamesTable<std::string>&
	getNames();

	/**
	 * @brief Returns the list of auxiliary predicates.
	 *
	 * The list is given in terms of a NamesTable.
	 */
	static std::vector<std::pair<std::string, std::string> >&
	getNameSpaces();

private:
	
	/**
	 * Type of the Term.
	 */
	TermType type;

	/**
	 * Reference to the constant in the global names table if the Term is a
	 * constant.
	 *
	 * @see Term::names
	 */
	NamesTable<std::string>::const_iterator constantString;

	/**
	 * Integer value if this Term is of type INTEGER.
	 */
	int constantInteger;

	/**
	 * Variable identifier if term is of type VARIABLE.
	 */
	std::string variableString;

	/**
	 * @brief initialize singleton member vars, used in getNames(), getNameSpaces(), getAuxiliaryNames()
	 */
	static void
        initTables();

	/**
	 * @brief Table of all constant names of a program.
	 *
	 * Since the UNA is valid for us, we store all constants in a global
	 * (static) table and let each constant term only refer to the respective
	 * table entry. NamesTable is a kind of orered hash map for unique items (we use
	 * strings). As a reference to entries in NamesTable, we use iterators.
	 */
	static NamesTable<std::string>* names;

	/**
	 * @brief List of namespaces.
	 *
	 * Each entry in the list contains the namespace string and the abbreviation
	 * string. The list of namespaces is administered as a static member within
	 * Term, since namespaces have a direct effect on the constant terms in a
	 * program.
	 */
	static std::vector<std::pair<std::string, std::string> >* namespaces;

	/**
	 * @brief Additional list for auxiliary predicate names, to be removed
	 * before final result output.
	 */
	static NamesTable<std::string>* auxnames;

};


/**
 * A Tuple is a std::vector of terms.
 * \ingroup dlvhextypes
 * 
 * Tuple is not a separate class, instead we use a vector of Term, since a tuple
 * is just an ordered list of terms. Hence, all features of a vector can be used
 * with tuples, like push_back, access by the [] operator, iterators, etc.
 */
typedef std::vector<Term> Tuple;


/**
 * Serializes a term.
 *
 * For a variable term, the original variable symbol is used.
 * A symbol, string and integer term is serialized as expected. A NULLCONST
 * (anonymous variable) is serialized as '_'.
 */
std::ostream&
operator<< (std::ostream&, const Term&);


/**
 * Serializes a tuple, separating the tuple's terms with commas.
 *
 * This operator can be used for debugging and verbosity purposes. However, the
 * actual serializing of a Term within an Atom happens in the descendants of the
 * BaseVisitor class.
 */
std::ostream&
operator<< (std::ostream&, const Tuple&);

DLVHEX_NAMESPACE_END


#endif /* _DLVHEX_TERM_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
