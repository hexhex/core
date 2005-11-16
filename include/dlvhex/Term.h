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
#include <set>
#include <map>


template <class NameType>
class NamesTable
{
    typedef std::map<size_t, NameType> lookup_t;

    lookup_t lookup;

    size_t indexcount;

public:

    
    class const_iterator
    {
        typename lookup_t::const_iterator it;

    public:

        const_iterator()
        {
            //assert(0);
        }

        const_iterator(const typename lookup_t::const_iterator &it1)
            : it(it1)
        { }

        size_t
        getIndex() const
        {
            return (*it).first;
        }

        const NameType&
        operator *() const
        {
            return (*it).second;
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
    
    NamesTable()
        : indexcount(0)
    {
    }

    const_iterator
    find(NameType name) const
    {
        for (typename lookup_t::const_iterator i = lookup.begin();
             i != lookup.end();
             i++)
        {
            if ((*i).second == name)
                return const_iterator(i);
        }

        return const_iterator(lookup.end());
    }

    const_iterator
    insert(NameType name)
    {
        const_iterator i = find(name);
        
        if (i == const_iterator(lookup.end()))
        {
            indexcount++;

            std::pair<size_t, NameType> ins(indexcount, name);
            std::pair<typename lookup_t::iterator, bool> res;
            res = lookup.insert(ins);
            return const_iterator(res.first);
        }

        return i;
    }

    void
    modify(const_iterator i, NameType name)
    {
        lookup[i.getIndex()] = name;
    }

    const_iterator
    begin() const
    {
        return const_iterator(lookup.begin());
    }

    const_iterator
    end() const
    {
        return const_iterator(lookup.end());
    }
};


/**
 * A Term can be a variable, constant or null constant. A constant is either
 * a number, a symbol (alphanumeric character sequence) or a string (= quoted symbol).
 */
class Term
{
public:
    
    /**
     * @brief Type of the term.
     */
    enum Type { Integer, Symbol, String, Variable, NullConst };

    /**
     * @brief Default Constructor.
     */
    Term();

    /**
     * Copy constructor.
     */
    Term(const Term &);

    /**
     * @brief Creates a constant string term.
     *
     * If the second Parameter is true, the
     * string will be quoted (if not already quoted) and the type of the term
     * will be 'String'. Otherwise it is a 'Symbol', if the first character
     * is lowercase, or a 'Variable' if uppercase.
     */
    Term(const std::string name, bool isString = false); 

    /**
     * @brief Same as Term(const string name, bool isString = false).
     */
    Term(const char *name, bool isString = false); 

    /**
     * @brief Creates a constant integer term. Type will be 'Integer'.
     */
    Term(const int &num);

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
     * Returns the symbol string, if the constant is of type 'Symbol'.
     * In case of a 'String' constant, the quoted string is returned.
     * other term types cause an assertion to fail.
     */
    std::string
    getString() const; 

    /**
     * Returns a string without quotes. The term needs to be of type 'Constant'
     * or 'String'.
     */
    std::string
    getUnquotedString() const; 

    /**
     * Returns the constant integer. If the term is not of type 'Integer', an
     * assertion fails.
     */
    int
    getInt() const;

    /**
     * Returns the original variable identifier the term was constructed with.
     * If the term is not of type 'Variable', an assertion fails.
     */
    std::string
    getVariable() const;

    /**
     * @brief TODO: do we need this?
     */
    bool
    isNull() const; 
    
    /**
     * Tests for unification with another term.
     * Two variables unify, as well as one variable and one constant.
     * Two constants unify, if they are equal.
     */
    bool
    unifiesWith(const Term &term2) const;

    /**
     * @brief Assignment operator.
     */
    Term
    &operator= (const Term &term2);

    /**
     * Inequality operator.
     * Two terms are not equal, if they are of different type, or if their
     * constants (numbers, strings or symbols) differ. If both strings are
     * variables, they are considered to be equal (TODO: is this the right
     * behaviour??).
     */
    int
    operator!= (const Term &term2) const;  

    /**
     * Equality operator, compares two terms. Returns the negation of !=.
     */
    bool
    operator== (const Term &term2) const; 

    /**
     * Another equality operator, which first constructs a term of a given string
     * (see constructor) and then compares the two terms.
     */
    bool
    operator== (const std::string &term2) const; 

    /**
     * Less-than operator. If the terms are of different type, the operator retuns
     * true. For two integer terms, it works as expected. Strings and symbols are
     * compared lexicographically. Variables are always equal (TODO: is this the right
     * behaviour??)
     */
    bool
    operator< (const Term &term2) const;

    /**
     * @brief Less-or-equal operator.
     */
    bool
    operator<= (const Term &term2) const;

    /**
     * @brief Greater-than operator (see Less-than operator).
     */
    bool
    operator> (const Term &term2) const;

    /**
     * @brief Greater-or-equal operator.
     */
    bool
    operator>= (const Term &term2) const; 

    /**
     * @brief Provides access to the static names table.
     */
    static NamesTable<std::string>&
    getNamesTable();

    static std::vector<std::pair<std::string, std::string> > namespaces;

private:
    
    Type type;

    /**
     * @brief Table of all constant names of a program.
     */
    static NamesTable<std::string> names;

    NamesTable<std::string>::const_iterator constantString;

    int constantInteger;

    std::string variableString;
};


/**
 * Serializes a term. For a variable term, the original variable Symbol is used.
 * A symbol, string and variable term is serialized as expected.
 */
std::ostream&
operator<< (std::ostream &out, const Term &term);


/**
 * A Tuple is a vector of terms.
 */
typedef std::vector<Term> Tuple;


std::ostream&
operator<< (std::ostream &out, const Tuple &tuple);

#endif /* _TERM_H */
