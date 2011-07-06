#include "dlvhex/HexGrammar.h"
#include "dlvhex/SpiritFilePositionNode.h"

DLVHEX_NAMESPACE_BEGIN

#include "dlvhex/HexGrammar.tcc"

// explicit instantiation

// obtained from linker error message
// or obtained via "nm -CA HexGrammar.o"
// libdlvhexbase.so: undefined reference to `dlvhex::HexGrammarBase::definition<boost::spirit::scanner<boost::spirit::position_iterator<char const*, boost::spirit::file_position_base<std::string>, boost::spirit::nil_t>, boost::spirit::scanner_policies<boost::spirit::skip_parser_iteration_policy<boost::spirit::space_parser, boost::spirit::iteration_policy>, boost::spirit::pt_match_policy<boost::spirit::position_iterator<char const*, boost::spirit::file_position_base<std::string>, boost::spirit::nil_t>, dlvhex::FilePositionNodeFactory<dlvhex::FilePositionNodeData>, boost::spirit::nil_t>, boost::spirit::action_policy> > >::definition(dlvhex::HexGrammarBase const&)

typedef
boost::spirit::scanner<
  boost::spirit::position_iterator<
    char const*, boost::spirit::file_position_base<std::string>, boost::spirit::nil_t>,
  boost::spirit::scanner_policies<
    boost::spirit::skip_parser_iteration_policy<boost::spirit::space_parser, boost::spirit::iteration_policy>,
    boost::spirit::pt_match_policy<
      boost::spirit::position_iterator<
        char const*, boost::spirit::file_position_base<std::string>, boost::spirit::nil_t>,
      dlvhex::FilePositionNodeFactory<dlvhex::FilePositionNodeData>,
      boost::spirit::nil_t>,
    boost::spirit::action_policy> >
    my_scanner_t;

template struct HexGrammarBase::definition<my_scanner_t>;

DLVHEX_NAMESPACE_END
