/* -*- C++ -*- */

/**
 * @file Term.cpp
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Term class.
 *
 *
 */


#include <assert.h>

#include "dlvhex/Term.h"


Term::Term()
    : type(NullConst), constantString(names.end()), variableString("")
{
}


Term::Term(const Term &term2)
    : type(term2.type)
{
    if (this != &term2)
    {
        if (!term2.isVariable())
        {
            if( term2.isString() || term2.isSymbol() )
                constantString = term2.constantString;
            else
                constantInteger = term2.constantInteger;
        }
        else
            variableString = term2.variableString;
    }
}


Term::Term(const std::string name, bool isString)
{
    if (name[0] == '\"')
    {
        //constantString = names.insert(names.end(), name);
        constantString = names.insert(name.substr(1, name.length() - 2));
        //constantString = name;
        type = String;
    }
    else
    {
        if (isString)
        {
            //constantString = "\"" + name + "\"";
            //constantString = names.insert(names.end(), "\"" + name + "\"");
            constantString = names.insert(name);
            type = String;
        }
        else
        {
            //
            // Var or Const?
            //
            if (isupper(name[0]))
            {
                variableString = name;
                type = Variable;
            }
            else
            {
                //constantString = name;
                constantString = names.insert(name);
                type = Symbol;
            }
        }
    }
}

Term::Term(const char* name, bool isString)
{
    if (name[0] == '\"')
    {
        //constantString = (std::string)name;
        //constantString = names.insert(names.end(), (std::string)name);
        std::string n(name);
        constantString = names.insert(n.substr(1, n.length() - 2));
        type = String;
    }
    else
    {
        if (isString)
        {
            //constantString = "\"" + (std::string)name + "\"";
            //constantString = names.insert(names.end(), "\"" + (std::string)name + "\"");
            constantString = names.insert((std::string)name);
            type = String;
        }
        else
        {
            //
            // Var or Const?
            //
            if (isupper(name[0]))
            {
                variableString = name;
                type = Variable;
            }
            else
            {
                //constantString = name;
                constantString = names.insert((std::string)name);
                type = Symbol;
            }
        }
    }
}

Term::Term(const int &num)
    : type(Integer), constantInteger(num)
{
}

Term::Type Term::getType() const
{
    return type;
} 

bool
Term::isInt() const
{
    return type == Integer;
}

bool
Term::isString() const
{
    return type == String;
}

bool
Term::isSymbol() const
{
    return type == Symbol;
}

bool
Term::isVariable() const
{
    return type == Variable;
}

std::string
Term::getString() const
{
    assert((type == String) || (type == Symbol));

    assert(constantString != names.end());

    std::string ret(*constantString);

    if (type == String)
    {
        ret = "\"" + (std::string)ret + "\"";
    }
    
    return ret;
}

std::string
Term::getUnquotedString() const
{
    assert((type == String) || (type == Symbol));
    
    assert(constantString != names.end());
    
//    if (type == String)
//        return (*constantString).substr(1, (*constantString).length() - 2);
//    else
        return *constantString;
}

int
Term::getInt() const
{
    assert(type == Integer);

    return constantInteger;
}

std::string
Term::getVariable() const
{
    assert(type == Variable);
    
    return variableString;
}

bool
Term::isNull() const
{
    return type == NullConst;
}


bool
Term::unifiesWith(const Term &term2) const
{
    //
    // If at least one of the is variable, they unify
    //
    if (isVariable() || term2.isVariable())
        return 1;
    
    //
    // if the constants are of different types, they don't unify
    //
    if (type != term2.type)
        return 0;
 
    //
    // now, only equal constant types are left. they unify if they
    // are equal
    //
    if (*this == term2)
        return 1;
    
    //
    // everything else:
    //
    return 0;
}


Term&
Term::operator= (const Term &term2)
{
    if( this != &term2 )
    {
        constantString = term2.constantString;
        constantInteger = term2.constantInteger;
        type = term2.type;
    }

    return *this;
}

int
Term::operator!= (const Term &term2) const
{
    if( type != term2.type )
        return (int)type - (int)term2.type;

    switch (type)
    {
        case Integer:
            return constantInteger - term2.getInt();
        
        case Symbol:
            assert(constantString != names.end());
    
            return (*constantString).compare(term2.getString());
        
        case String:
            assert(constantString != names.end());

            return (*constantString).compare(term2.getUnquotedString());
        
        case Variable:
            //
            // TODO:
            // when are two variables unequal???        
            //
            return 0;
        
        default:
            assert(0);
            return 0;
    }
}

bool
Term::operator== (const Term &term2) const
{
    return ( *this != term2 ) == 0;
}

bool
Term::operator== (const std::string &str) const
{
    Term t2(str);
    
    return *this == t2;
}

bool
Term::operator< (const Term &term2) const
{
    return ( *this != term2 ) < 0;
}

bool
Term::operator<= (const Term &term2) const
{
    return ( *this != term2 ) <= 0;
}

bool
Term::operator> (const Term &term2) const
{
    return ( *this != term2 ) > 0;
}

bool
Term::operator>= (const Term &term2) const
{
    return ( *this != term2 ) >= 0;
}

std::ostream&
operator<< (std::ostream &out, const Term &term)
{
    switch (term.getType())
    {
        case Term::Integer:
            out << term.getInt();
            break;
        
        case Term::Symbol:
            out << term.getString();
            break;
        
        case Term::String:
            out << term.getString();
            break;
        
        case Term::Variable:
            out << term.getVariable();
            break;
        
        case Term::NullConst:
            out << "_";
            break;
        
        default:
            assert(0);
            break;
    }
    
    return out;
}

std::ostream&
operator<< (std::ostream &out, const Tuple &tuple)
{
    for (unsigned i = 0; i < tuple.size(); i++)
    {
        out << tuple[i];
        
        if (i < tuple.size() - 1)
            out << ",";
    }

    return out;
}

std::vector<std::pair<std::string, std::string> > Term::namespaces;

NamesTable<std::string> Term::names;
