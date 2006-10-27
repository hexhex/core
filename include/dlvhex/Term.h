/* -*- C++ -*- */

/**
 * @file Term.h
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Term class.
 *
 *
 */


#ifndef _TERM_H
#define _TERM_H


#include <iostream>
#include <string>
#include <vector>
//#include <set>
//#include <map>

#include "dlvhex/NamesTable.h"


/**
 * Term is the most basic one of 
 * A Term can be a variable, constant or null constant. A constant is either
 * a number, a symbol (alphanumeric character sequence) or a string (= quoted symbol).
 */
class Term
{
public:
    
    /**
     * @brief Type of the term.
     *
     * An INTEGER is a number. A SYMBOL is a string containing only [a-zA-Z_0-9]
     * and strarting with a lowercase letter. A STRING is a double-quoted array of
     * characters, whithin the quotes everything is permitted. A VARIABLE is
     * defined like a constant, except for beginning with an uppercase letter. A
     * NULLCONST is an anonymous term.
     */
    typedef enum { INTEGER, SYMBOL, STRING, VARIABLE, NULLCONST } Type;

    /**
     * @brief Default Constructor.
     */
    Term();

    /**
     * Copy constructor.
     */
    Term(const Term&);

    /**
     * @brief Creates a constant string term.
     *
     * @param addQuotes If true, then the string will be quoted, but only if it
     * is not already a quoted string.
     *
     * If the passed string is quoted or the flag addQuotes is true, then the
     * type of the term will be STRING.  Otherwise it is of type SYMBOL, if the
     * first character is lowercase, or of type VARIABLE if uppercase.
     */
    Term(const std::string&, bool addQuotes = false); 

    /**
     * @brief Same as the Term constructor with std::string.
     */
    Term(const char*, bool addQuotes = false); 

    /**
     * @brief Creates a constant integer term. Type will be 'INTEGER'.
     */
    Term(const int&);

    /**
     * @brief Returns the Type of the term.
     */
    Type
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
     * other term types cause an assertion to fail.
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
     * Returns the original variable identifier the term was constructed with.
     *
     * If the term is not of type VARIABLE, an assertion fails.
     */
    const std::string&
    getVariable() const;

    /**
     * @todo do we need this?
     */
    bool
    isNull() const; 
    
    /**
     * Tests for unification with another term.
     *
     * Two variables unify, as well as one variable and one constant.
     * Two constants or strings unify, if they are equal and of same type.
     */
    bool
    unifiesWith(const Term&) const;

    /**
     * @brief Assignment operator.
     */
    Term
    &operator= (const Term&);

    /**
     * Comparison function for two terms.
     *
     * The passed term2 is compared with *this. If *this and term2 have
     * different types, a value != 0 is returned.  In case both terms are
     * INTEGER, *this - term2 is returned. If both are STRING or CONSTANT, 0 is
     * returned if the strings are equal and a value != 0 otherwise
     * (lexicographical comparison through std::string::compare() with term2 as
     * argument). If both
     * terms are VARIABLE, the result of the std::string::cmp() function is
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
     * Equality operator, compares two terms. Returns the negation of !=.
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
     * @brief Greater-than operator (see Less-than operator).
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
     */
    static const NamesTable<std::string>&
    getAuxiliaryNames();

    /**
     * @brief Table of all constant names of a program.
     */
    static NamesTable<std::string> names;

    /**
     * @brief List of namespaces.
     *
     * Each entry in the list contains the namespace string and the abbreviation
     * string.
     */
    static std::vector<std::pair<std::string, std::string> > namespaces;

private:
    
    /**
     * Type of the Term.
     */
    Type type;

    /**
     * Reference to the constant in the global names table if the Term is a
     * constant.
     *
     * Since the UNA is valid for us, we store all constants in a global
     * (static) table and let each constant term only refer to the respective
     * table entry.
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
     * @brief Additional list for auxiliary predicate names, to be removed
     * before final result output.
     */
    static NamesTable<std::string> auxnames;

};


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
 * A Tuple is a std::vector of terms.
 */
typedef std::vector<Term> Tuple;


/**
 * Serializes a tuple, separating the tuple's terms with commas.
 */
std::ostream&
operator<< (std::ostream&, const Tuple&);


#endif /* _TERM_H */

