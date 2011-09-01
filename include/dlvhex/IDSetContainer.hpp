/**
 * @file   IDSetContainer.h
 * @author Tri Kurniawan Wijaya
 * @date   Tue 19 Apr 2011 02:50:10 PM CEST 
 * 
 * @brief  Data structure for an ID set = A (in the paper Relevance ...)
 */

#if !defined(_DLVHEX_IDSETCONTAINER_H)
#define _DLVHEX_IDSETCONTAINER_H

#include "dlvhex/ID.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

#include <iostream>
#include <string>
#include <fstream>


DLVHEX_NAMESPACE_BEGIN

class IDSetContainer
{

  // to store/index ID
  typedef boost::multi_index_container<
    ID, 
    boost::multi_index::indexed_by<
      boost::multi_index::random_access<boost::multi_index::tag<impl::AddressTag> >,
      boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, boost::multi_index::identity<ID> >
    > 
  > IDSet; 
  typedef IDSet::index<impl::AddressTag>::type IDSAddressIndex;
  typedef IDSet::index<impl::ElementTag>::type IDSElementIndex;
  // vector of IDTable, the index of the i/S should match with the index in tableInst

  std::vector<IDSet> AVector;

  struct AElement{
    int idx;
    int idxToVector;
    AElement(int idx, int idxToVector):idx(idx), idxToVector(idxToVector) {}
  };

  // to store the whole A
  typedef boost::multi_index_container<
    AElement, 
    boost::multi_index::indexed_by<
      boost::multi_index::random_access<boost::multi_index::tag<impl::AddressTag> >,
      boost::multi_index::hashed_unique<boost::multi_index::tag<impl::ElementTag>, boost::multi_index::member<AElement, int, &AElement::idx> >
    > 
  > AContainer; 
  typedef AContainer::index<impl::AddressTag>::type AContainerAddressIndex;
  typedef AContainer::index<impl::ElementTag>::type AContainerElementIndex;

  AContainer A; 
  void set(int idx, ID IDElement, int clearing);

  public:
  inline IDSetContainer();
  inline void clear();
  inline bool isEmpty(int idx);
  inline int size();
  inline void add(int idx, ID IDElement);
  inline void assignFin(int idx);
  inline bool containFin(int idx);
  inline bool containID(int idx, ID id);
  inline void print(std::ostringstream& oss, const RegistryPtr& reg1 ) const;
};


IDSetContainer::IDSetContainer()
{
  AVector.clear();  
  A.clear();
}


void IDSetContainer::clear()
{
  AVector.clear();  
  A.clear();
}

bool IDSetContainer::isEmpty(int idx)
{
  AContainerElementIndex& Aindex = A.get<impl::ElementTag>();
  AContainerElementIndex::iterator itAi = Aindex.find( idx ); 
  if ( itAi == Aindex.end() ) return true;
  if ( AVector.at((*itAi).idxToVector).size() == 0 ) return true;
      else return false;
}


int IDSetContainer::size()
{
  return A.size();
}


// clearing==1: clear IDSet before inserting IDElement
void IDSetContainer::set(int idx, ID IDElement, int clearing)
{
  AContainerElementIndex& Aindex = A.get<impl::ElementTag>();
  AContainerElementIndex::iterator itAi = Aindex.find( idx ); 
  if ( itAi == Aindex.end() ) 
    { // not found, insert a new element
	AVector.resize(AVector.size()+1);
	AVector.back().get<impl::ElementTag>().insert(IDElement);
	AElement aElement(idx, AVector.size()-1);
	A.get<impl::ElementTag>().insert( aElement );
    }
  else 
    { // if found
	if ( clearing == 1 ) AVector.at((*itAi).idxToVector).clear();
	AVector.at((*itAi).idxToVector).get<impl::ElementTag>().insert( IDElement );
    }
}


void IDSetContainer::add(int idx, ID IDElement)
{
  set(idx, IDElement, 0);
}


void IDSetContainer::assignFin(int idx)
{ 
  set(idx, ID_FAIL, 1);
}


bool IDSetContainer::containID(int idx, ID id)
{
  AContainerElementIndex::const_iterator itAi = A.get<impl::ElementTag>().find( idx ); 
  if ( itAi == A.get<impl::ElementTag>().end() ) return false;
  IDSElementIndex::const_iterator itEi = AVector.at((*itAi).idxToVector).get<impl::ElementTag>().find( id ); 
  if ( itEi == AVector.at((*itAi).idxToVector).get<impl::ElementTag>().end() ) return false; 
    else return true;  
}

// we treat Fin as ID_FAIL
bool IDSetContainer::containFin(int idx)
{
  containID(idx, ID_FAIL);
}


void IDSetContainer::print(std::ostringstream& oss , const RegistryPtr& reg1 ) const
{
  RawPrinter printer(oss, reg1);

  AContainerAddressIndex::const_iterator itAi = A.begin(); 
  bool first;
  while ( itAi != A.end() )
    {
      oss << "A[" << (*itAi).idx << "]: "; 	
      IDSAddressIndex::const_iterator itIDSet = AVector.at((*itAi).idxToVector).begin();
      first = true;
      while ( itIDSet != AVector.at((*itAi).idxToVector).end() )	
	{
	  // print here
	  if (first == false) oss << ", ";
	  if ( *itIDSet == ID_FAIL )
		oss << "fin";
	  else 
	    {
	       printer.print(*itIDSet);
	    }		

	  itIDSet++;
	  first = false;
	}
      oss << std::endl;	
      itAi++;
    }
}


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_IDSETCONTAINER_H */

