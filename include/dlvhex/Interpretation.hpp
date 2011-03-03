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
 * @file   Interpretation.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Bitset interpretation using bitmagic library.
 */

#ifndef INTERPRETATION_HPP_INCLUDED__08112010
#define INTERPRETATION_HPP_INCLUDED__08112010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
#include "dlvhex/ModelGenerator.hpp"
#include "dlvhex/ID.hpp"
#include <bm/bm.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class Interpretation:
  public InterpretationBase,
  public ostream_printable<Interpretation>
{
  // types
public:
  typedef boost::shared_ptr<Interpretation> Ptr;
  typedef boost::shared_ptr<const Interpretation> ConstPtr;
  typedef bm::bvector<> Storage;
  typedef boost::function<bool (IDAddress)> FilterCallback;

  // storage
protected:
  RegistryPtr registry;
  Storage bits;

  // members
public:
  inline Interpretation(){};
  Interpretation(RegistryPtr registry);
  virtual ~Interpretation();
  // TODO: bitset stuff with bitmagic

  // go through 1-bits and set to zero if callback returns false
  virtual unsigned filter(FilterCallback callback);

  virtual std::ostream& print(std::ostream& o, const char* first, const char* sep, const char* last) const;
  virtual std::ostream& print(std::ostream& o) const;
  virtual std::ostream& printAsFacts(std::ostream& o) const;

  void add(const Interpretation& other);
  void bit_and(const Interpretation& other);
  #warning todo we may want to name this "add" "remove" and "has" (...Fact)
  inline void setFact(IDAddress id)
    { bits.set(id); }
  inline void clearFact(IDAddress id)
    { bits.clear_bit(id); }
  inline bool getFact(IDAddress id) const
    { return bits.get_bit(id); }

  const Storage& getStorage() const { return bits; }
  Storage& getStorage() { return bits; }

  RegistryPtr getRegistry() const { return registry; }

  void setRegistry(RegistryPtr registry1) { registry = registry1; }

  inline bool isClear() const
    {  return bits.none();  }

  inline void clear() 
    {  bits.clear();  }

  bool operator==(const Interpretation& other) const;
  bool operator<(const Interpretation& other) const;
  

};

typedef Interpretation::Ptr InterpretationPtr;
typedef Interpretation::ConstPtr InterpretationConstPtr;

DLVHEX_NAMESPACE_END

#endif // INTERPRETATION_HPP_INCLUDED__08112010

