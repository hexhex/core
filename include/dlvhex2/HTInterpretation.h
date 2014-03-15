/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2013 Andreas Humenberger
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
 * 02110-1301 USA
 */


/**
 * @file   HTInterpretation.h
 * @author Andreas Humenberger <e1026602@student.tuwien.ac.at>
 * 
 * @brief  HT interpretation
 * 
 */

#ifndef HT_INTERPRETATION_H
#define HT_INTERPRETATION_H

#include "dlvhex2/ModelGenerator.h"

#include <bm/bm.h>
#include <boost/shared_ptr.hpp>
#include <ostream>

DLVHEX_NAMESPACE_BEGIN

class HTInterpretation:
	public InterpretationBase,
	public ostream_printable<HTInterpretation>
{
public:
	typedef boost::shared_ptr<HTInterpretation> Ptr;
	typedef boost::shared_ptr<const HTInterpretation> ConstPtr;
	typedef bm::bvector<> Storage;
protected:
	// registry
	RegistryPtr registry_;
	// storage
	Storage here_;
	Storage there_;
public:
	RegistryPtr registry()
	{	return registry_;	}
	const Storage& here() const
	{	return here_;	}
	Storage& here()
	{	return here_;	}
	const Storage& there() const
	{	return there_;	}
	Storage& there()
	{	return there_;	}
	void add(const HTInterpretation& other)
	{
		here_ |= other.here_;
		there_ |= other.there_;
	}
	virtual std::ostream& print(std::ostream& o) const {}

	HTInterpretation() {}
	HTInterpretation(const Storage& there, const Storage& gap = Storage()):
		there_(there),
		here_(there - gap)
	{
	}
	HTInterpretation(RegistryPtr reg):
		registry_(reg)
	{
	}
};

typedef HTInterpretation::Ptr HTInterpretationPtr;
typedef HTInterpretation::ConstPtr HTInterpretationConstPtr;

DLVHEX_NAMESPACE_END

#endif // HT_INTERPRETATION_H
