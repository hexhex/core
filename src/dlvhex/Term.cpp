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
    : type(NULLCONST), constantString(names.end()), variableString("")
{
}


Term::Term(const Term& term2)
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


Term::Term(const std::string& name, bool isString)
{
    if (name[0] == '\"')
    {
        //constantString = names.insert(name.substr(1, name.length() - 2));
        constantString = names.insert(name);
        type = STRING;
    }
    else
    {
        if (isString)
        {
            constantString = names.insert("\"" + name + "\"");
            type = STRING;
        }
        else
        {
            //
            // Var or Const?
            //
            if (isupper(name[0]))
            {
                variableString = name;
                type = VARIABLE;
            }
            else
            {
                constantString = names.insert(name);
                type = SYMBOL;
            }
        }
    }
}


Term::Term(const char* name, bool isString)
{
    if (name[0] == '\"')
    {
        //std::string n(name);
        //constantString = names.insert(n.substr(1, n.length() - 2));
        constantString = names.insert(name);
        type = STRING;
    }
    else
    {
        if (isString)
        {
            //constantString = names.insert((std::string)name);
            constantString = names.insert("\"" + (std::string)name + "\"");
            type = STRING;
        }
        else
        {
            //
            // Var or Const?
            //
            if (isupper(name[0]))
            {
                variableString = name;
                type = VARIABLE;
            }
            else
            {
                constantString = names.insert((std::string)name);
                type = SYMBOL;
            }
        }
    }
}


Term::Term(const int& num)
    : type(INTEGER), constantInteger(num)
{
//    std::cout << "created number term: " << num << std::endl;
}


Term::Type Term::getType() const
{
    return type;
} 


bool
Term::isInt() const
{
    return type == INTEGER;
}


bool
Term::isString() const
{
    return type == STRING;
}


bool
Term::isSymbol() const
{
    return type == SYMBOL;
}


bool
Term::isVariable() const
{
    return type == VARIABLE;
}


const std::string&
Term::getString() const
{
    assert((type == STRING) || (type == SYMBOL));

    assert(constantString != names.end());

    /*
    std::string ret(*constantString);

    if (type == STRING)
    {
        ret = "\"" + (std::string)ret + "\"";
    }
    
    return ret;
    */

    return *constantString;
}


std::string
Term::getUnquotedString() const
{
    assert((type == STRING) || (type == SYMBOL));
    
    assert(constantString != names.end());

    //
    // trim quotes from strings
    //
    if (type == STRING)    
        return (*constantString).substr(1, (*constantString).length() - 2);

    //
    // otherwise just return symbol string
    //
    return *constantString;
}


int
Term::getInt() const
{
    assert(type == INTEGER);

    return constantInteger;
}


const std::string&
Term::getVariable() const
{
    assert(type == VARIABLE);
    
    return variableString;
}


bool
Term::isNull() const
{
    return type == NULLCONST;
}


bool
Term::unifiesWith(const Term& term2) const
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
Term::operator= (const Term& term2)
{
  //  if (this != &term2)
  //  {
        constantString = term2.constantString;
        constantInteger = term2.constantInteger;
        variableString = term2.variableString;
        type = term2.type;
  //  }

    return *this;
}


int
Term::operator!= (const Term& term2) const
{
    if( type != term2.type )
        return (int)type - (int)term2.type;


    switch (type)
    {
        case INTEGER:
            return constantInteger - term2.getInt();
        
        case SYMBOL:
        case STRING:
            assert(constantString != names.end());
    
            //return (*constantString).compare(term2.getString());
            return constantString.cmp(term2.constantString);
        
        case VARIABLE:
            //
            // two variables are unequal if their strings are unequal.
            //
            return (variableString).compare(term2.getVariable());
        
        case NULLCONST:

            return 0;

        default:
            assert(0);
            return 0;
    }
}


bool
Term::operator== (const Term& term2) const
{
    return ( *this != term2 ) == 0;
}


bool
Term::operator== (const std::string& str) const
{
    Term t2(str);
    
    return *this == t2;
}


bool
Term::operator< (const Term& term2) const
{
    return ( *this != term2 ) < 0;
}


bool
Term::operator<= (const Term& term2) const
{
    return ( *this != term2 ) <= 0;
}


bool
Term::operator> (const Term& term2) const
{
    return ( *this != term2 ) > 0;
}


bool
Term::operator>= (const Term& term2) const
{
    return ( *this != term2 ) >= 0;
}


void
Term::registerAuxiliaryName(const std::string& auxname)
{
    auxnames.insert(auxname);
}


const NamesTable<std::string>&
Term::getAuxiliaryNames()
{
    return auxnames;
}


std::ostream&
operator<< (std::ostream& out, const Term& term)
{
    switch (term.getType())
    {
        case Term::INTEGER:
            out << term.getInt();
            break;
        
        case Term::SYMBOL:
            out << term.getString();
            break;
        
        case Term::STRING:
            out << term.getString();
            break;
        
        case Term::VARIABLE:
            out << term.getVariable();
            break;
        
        case Term::NULLCONST:
            out << "_";
            break;
        
        default:
            assert(0);
            break;
    }
    
    return out;
}


std::ostream&
operator<< (std::ostream& out, const Tuple& tuple)
{
    for (unsigned i = 0; i < tuple.size(); i++)
    {
        out << tuple[i];
        
        if (i < tuple.size() - 1)
            out << ",";
    }

    return out;
}


//
// initializing static members
//
std::vector<std::pair<std::string, std::string> > Term::namespaces;

NamesTable<std::string> Term::names;

NamesTable<std::string> Term::auxnames;

