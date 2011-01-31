/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file Printer.cpp
 * @author Peter Schueller
 * @date 
 *
 * @brief Printer classes for printing objects stored in registry given registry and ID.
 */

#include "dlvhex/Printer.hpp"
#include "dlvhex/Registry.hpp"

#include <cassert>

DLVHEX_NAMESPACE_BEGIN

void Printer::printmany(const std::vector<ID>& ids, const std::string& separator)
{
	std::vector<ID>::const_iterator it = ids.begin();
	if( it != ids.end() )
	{
		print(*it);
		it++;
		while( it != ids.end() )
		{
			out << separator;
			print(*it);
			it++;
		}
	}
}

void RawPrinter::print(ID id)
{
	switch(id.kind & ID::MAINKIND_MASK)
	{
	case ID::MAINKIND_LITERAL:
		if(id.isNaf())
			out << "not ";
		// continue with atom here!
	case ID::MAINKIND_ATOM:
		switch(id.kind & ID::SUBKIND_MASK)
		{
		case ID::SUBKIND_ATOM_ORDINARYG:
			out << registry->ogatoms.getByID(id).text;
			break;
		case ID::SUBKIND_ATOM_ORDINARYN:
			out << registry->onatoms.getByID(id).text;
			break;
		case ID::SUBKIND_ATOM_BUILTIN:
			{
				const BuiltinAtom& atom = registry->batoms.getByID(id);
				assert(atom.tuple.size() > 1);
				assert(atom.tuple[0].isBuiltinTerm());
				//TODO prettier printing of builtins (infix vs prefix) (this is only a quick hack fixing only !=)
        if( atom.tuple[0].address == ID::TERM_BUILTIN_NE )
        {
          print(atom.tuple[1]);
          out << " ";
          print(atom.tuple[0]);
          out << " ";
          print(atom.tuple[2]);
        }
        else
        {
          print(atom.tuple[0]);
          out << "(";
          //TODO make the following more efficient
          std::vector<ID> tail(atom.tuple.begin() + 1, atom.tuple.end());
          printmany(tail,",");
          out << ")";
        }
			}
			break;
		case ID::SUBKIND_ATOM_AGGREGATE:
			{
				const AggregateAtom& atom = registry->aatoms.getByID(id);
				out << "TODO(AggregateAtom)";
			}
			break;
		case ID::SUBKIND_ATOM_EXTERNAL:
			{
				const ExternalAtom& atom = registry->eatoms.getByID(id);
				out << "&";
				print(atom.predicate);
				out << "[";
				printmany(atom.inputs,",");
				out << "](";
				printmany(atom.tuple,",");
				out << ")";
			}
			break;
		default:
			assert(false);
		}
		break;
	case ID::MAINKIND_TERM:
		switch(id.kind & ID::SUBKIND_MASK)
		{
		case ID::SUBKIND_TERM_CONSTANT:
		case ID::SUBKIND_TERM_VARIABLE:
			out << registry->terms.getByID(id).symbol;
			break;
		case ID::SUBKIND_TERM_INTEGER:
			out << id.address;
			break;
		case ID::SUBKIND_TERM_BUILTIN:
			out << ID::stringFromBuiltinTerm(id.address);
			break;
		default:
			assert(false);
		}
		break;
	case ID::MAINKIND_RULE:
		switch(id.kind & ID::SUBKIND_MASK)
		{
		case ID::SUBKIND_RULE_REGULAR:
			{
				const Rule& r = registry->rules.getByID(id);
				printmany(r.head, " v ");
				if( !r.body.empty() )
				{
					out << " :- ";
					printmany(r.body, ", ");
				}
				out << ".";
			}
			break;
		case ID::SUBKIND_RULE_CONSTRAINT:
			{
				out << ":- ";
				const Rule& r = registry->rules.getByID(id);
				printmany(r.body, ", ");
				out << ".";
			}
			break;
		case ID::SUBKIND_RULE_WEAKCONSTRAINT:
			{
				out << ":~ ";
				const Rule& r = registry->rules.getByID(id);
				printmany(r.body, ", ");
				out << ". [";
				print(r.weight);
				out << ":";
				print(r.level);
				out << "]";
			}
			break;
		default:
			assert(false);
		}
		break;
	default:
		assert(false);
	}
}

DLVHEX_NAMESPACE_END

