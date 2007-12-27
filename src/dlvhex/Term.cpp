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


/**
 * @file Term.cpp
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Term class.
 *
 *
 */


#include "dlvhex/Term.h"

#include <cassert>

// include iostream, otherwise we do not have the declarations for all
// the standard operator<<'s, and we will fail to compile our own
// operator<<s disgracefully.
#include <iostream>


DLVHEX_NAMESPACE_BEGIN

//
// operator<<
//

std::ostream&
operator<< (std::ostream& out, const DLVHEX_NAMESPACE Term& term)
{
	switch (term.getType())
	{
		case DLVHEX_NAMESPACE Term::INTEGER:
			out << term.getInt();
			break;
		
		case DLVHEX_NAMESPACE Term::SYMBOL:
			out << term.getString();
			break;
		
		case DLVHEX_NAMESPACE Term::STRING:
			out << term.getString();
			break;
		
		case DLVHEX_NAMESPACE Term::VARIABLE:
			out << term.getVariable();
			break;
		
		case DLVHEX_NAMESPACE Term::NULLCONST:
			out << '_';
			break;
		
		default:
			assert(0);
			break;
	}
	
	return out;
}


std::ostream&
operator<< (std::ostream& out, const DLVHEX_NAMESPACE Tuple& tuple)
{
  if (!tuple.empty())
    {
      for (unsigned i = 0; i < tuple.size() - 1; i++)
	{
	  out << tuple[i] << ',';
	}
      out << tuple.back();
    }

  return out;
}




//
// initializing static members
//
std::vector<std::pair<std::string, std::string> > Term::namespaces;

NamesTable<std::string> Term::names;

NamesTable<std::string> Term::auxnames;


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


Term::Term(const std::string& name, bool addQuotes)
{
	if (name[0] == '\"')
	{
		constantString = names.insert(name);
		type = STRING;
	}
	else
	{
		if (addQuotes)
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


Term::Term(const char* name, bool addQuotes)
{
	if (name[0] == '\"')
	{
		constantString = names.insert(name);
		type = STRING;
	}
	else
	{
		if (addQuotes)
		{
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


Term::Term(int num)
	: type(INTEGER), constantInteger(num)
{
}


Term::TermType Term::getType() const
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


bool
Term::isAnon() const
{
	return type == NULLCONST;
}


const std::string&
Term::getString() const
{
	assert((type == STRING) || (type == SYMBOL));
	assert(constantString != names.end());

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
	// If at least one of them is variable, they unify
	//
	if (isVariable() || term2.isVariable())
		return 1;
	
	//
	// If at least one of them is anonymous, they unify
	//
	if (isAnon() || term2.isAnon())
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
  if (this != &term2)
    {
      this->constantString = term2.constantString;
      this->constantInteger = term2.constantInteger;
      this->variableString = term2.variableString;
      this->type = term2.type;
    }

  return *this;
}


int
Term::compare(const Term& term2) const
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
Term::operator!= (const Term& term2) const
{
	return compare(term2) != 0;
}

bool
Term::operator== (const Term& term2) const
{
	return compare(term2) == 0;
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
	return compare(term2) < 0;
}


bool
Term::operator<= (const Term& term2) const
{
	return compare(term2) <= 0;
}


bool
Term::operator> (const Term& term2) const
{
	return compare(term2) > 0;
}


bool
Term::operator>= (const Term& term2) const
{
	return compare(term2) >= 0;
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


DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
