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
 * @file Term.cpp
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Term class.
 * @todo add handling for namespaces and uris, and better aux name support
 *
 */


#include "dlvhex/Term.h"

#include <cassert>

// include iostream, otherwise we do not have the declarations for all
// the standard operator<<'s, and we will fail to compile our own
// operator<<s disgracefully.
#include <iostream>
#include <iterator>

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
      std::copy(tuple.begin(), --tuple.end(),
		std::ostream_iterator<Term>(out, ","));

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
  : type(NULLCONST),
    constantString(names.end()),
    constantInteger(-1),
    variableString()
{ }


Term::Term(const Term& term2)
  : type(term2.type),
    constantString(term2.constantString),
    constantInteger(term2.constantInteger),
    variableString(term2.variableString)
{ }


Term::Term(const std::string& name, bool addQuotes)
  : constantString(names.end()),
    constantInteger(-1),
    variableString()
{
  setup(name, addQuotes);
}


Term::Term(const char* name, bool addQuotes)
  : constantString(names.end()),
    constantInteger(-1),
    variableString()
{
  setup(std::string(name), addQuotes);
}


void
Term::setup(const std::string& name, bool addQuotes)
{
  if (!name.empty())
    {
      std::string::value_type c = name[0];

      if (isupper(c)) // variable
	{
	  variableString = name;
	  type = VARIABLE;
	}
      else if (c == '\"') // quoted string
	{
	  constantString = names.insert(name);
	  type = STRING;
	}
      else if (addQuotes) // generate quoted string
	{
	  constantString = names.insert("\"" + name + "\"");
	  type = STRING;
	}
      else // standard symbol
	{
	  constantString = names.insert(name);
	  type = SYMBOL;
	}
    }
  else // name.empty()
    {
      if (addQuotes) // generate quoted string
	{
	  constantString = names.insert("\"\"");
	  type = STRING;
	}
      else // standard symbol
	{
	  constantString = names.insert(name);
	  type = SYMBOL;
	}
    }
}


Term::Term(int num)
  : type(INTEGER),
    constantString(names.end()),
    constantInteger(num),
    variableString()
{ }


Term::TermType
Term::getType() const
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
  // or If at least one of them is anonymous, they unify
  // or if they have both the same type and are equal, they unify
  //
  return
    isVariable() || term2.isVariable() ||
    isAnon() || term2.isAnon() ||
    (type == term2.type && *this == term2);
}


Term&
Term::operator= (const Term& term2)
{
  if (this != &term2)
    {
      this->type = term2.type;
      this->constantString = term2.constantString;
      this->constantInteger = term2.constantInteger;
      this->variableString = term2.variableString;
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
			return variableString.compare(term2.getVariable());
		
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
