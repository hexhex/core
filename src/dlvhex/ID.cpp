#include "dlvhex/ID.hpp"
#include <boost/functional/hash.hpp>
#include <iomanip>

DLVHEX_NAMESPACE_BEGIN

std::size_t hash_value(const ID& id)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, id.kind);
  boost::hash_combine(seed, id.address);
  return seed;
}

std::ostream& ID::print(std::ostream& o) const
{
  o << "ID(0x" <<
      std::setfill('0') << std::hex << std::setw(8) << kind << "," << std::setfill(' ') <<
      std::dec << std::setw(4) << address << ",";
  if( !!(kind & NAF_MASK) )
    o << " naf";
  const unsigned MAINKIND_MAX = 4;
  const char* mainkinds[MAINKIND_MAX] = {
    " atom",
    " term",
    " literal",
    " rule",
  };
  const unsigned mainkind = (kind & MAINKIND_MASK) >> MAINKIND_SHIFT;
  assert(mainkind < MAINKIND_MAX);
  o << mainkinds[mainkind];

  const unsigned SUBKIND_MAX = 7;
  const char* subkinds[MAINKIND_MAX][SUBKIND_MAX] = {
    { " ordinary_ground", " ordinary_nonground", " builtin",         " aggregate", "", "", " external" },
    { " constant",        " integer",            " variable",        "",           "", "", ""          },
    { " ordinary_ground", " ordinary_nonground", " builtin",         " aggregate", "", "", " external" },
    { " regular"          " constraint",         " weak_constraint", "",           "", "", ""          }
  };
  const unsigned subkind = (kind & SUBKIND_MASK) >> SUBKIND_SHIFT;
  assert(subkind < SUBKIND_MAX);
  assert(subkinds[mainkind][subkind][0] != 0);
  o << subkinds[mainkind][subkind];
  return o << ")";
}

DLVHEX_NAMESPACE_END
