#include <steem/plugins/sps_api/sps_api_plugin.hpp>
#include <steem/plugins/sps_api/sps_api.hpp>
#include <steem/chain/sps_objects.hpp>
#include <appbase/application.hpp>
#include <steem/utilities/iterate_results.hpp>

namespace steem { namespace plugins { namespace sps {

namespace detail {

using namespace steem::chain;

class sps_api_impl
{
  public:
    sps_api_impl();
    ~sps_api_impl();

    DECLARE_API_IMPL(
        (find_proposals)
        (list_proposals)
        (list_voter_proposals)
        )
    
    chain::database& _db;
};

sps_api_impl::sps_api_impl() : _db(appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db()) {}

sps_api_impl::~sps_api_impl() {}

template<typename RESULT_TYPE, typename FIELD_TYPE>
void sort_results_helper(RESULT_TYPE& result, order_direction_type order_direction, FIELD_TYPE api_proposal_object::*field)
{
  switch (order_direction)
  {
    case direction_ascending:
    {
      std::sort(result.begin(), result.end(), [&](const api_proposal_object& a, const api_proposal_object& b)
      {
        return a.*field < b.*field;
      });
    }
    break;
    case direction_descending:
    {
      std::sort(result.begin(), result.end(), [&](const api_proposal_object& a, const api_proposal_object& b)
      {
        return a.*field > b.*field;
      });
    }
    break;
    default:
      FC_ASSERT(false, "Unknown or unsupported sort order");
  }
}

template<typename RESULT_TYPE>
void sort_results(RESULT_TYPE& result, order_by_type order_by, order_direction_type order_direction)
{
  switch (order_by)
  {
    case by_creator:
    {
      sort_results_helper<RESULT_TYPE, account_name_type>(result, order_direction, &api_proposal_object::creator);
      return;
    }

    case by_start_date:
    {
      sort_results_helper<RESULT_TYPE, time_point_sec>(result, order_direction, &api_proposal_object::start_date);
      return;
    }

    case by_end_date:
    {
      sort_results_helper<RESULT_TYPE, time_point_sec>(result, order_direction, &api_proposal_object::end_date);
      return;
    }

    case by_total_votes:
    {
      sort_results_helper<RESULT_TYPE, uint64_t>(result, order_direction, &api_proposal_object::total_votes);
      return;
    }
    default:
      FC_ASSERT(false, "Unknown or unsupported field name");
  }
}

DEFINE_API_IMPL(sps_api_impl, find_proposals) {
  ilog("find_proposal called");

  find_proposals_struct fps; 
  fps.id_set = args[0].get_array()[0].as<flat_set<uint64_t> >();
  // cannot query for more than SPS_API_SINGLE_QUERY_LIMIT ids
  FC_ASSERT(fps.id_set.size() <= SPS_API_SINGLE_QUERY_LIMIT);

  find_proposals_return result;
  
  std::for_each(fps.id_set.begin(), fps.id_set.end(), [&](auto& id) {
    auto po = _db.find<steem::chain::proposal_object, steem::chain::by_id>(id);
    if (po != nullptr)
    {
      result.emplace_back(api_proposal_object(*po));
    }
  });

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_proposals) {
  ilog("list_proposals called");

  const auto arg = args[0].get_array();

  list_proposals_struct lps;
  lps.start            = arg[0] ;
  lps.order_by         = arg[1].as< order_by_type >() ;
  lps.order_direction  = arg[2].as< order_direction_type >() ;
  lps.limit            = arg[3].as< uint16_t >() ;
  lps.active           = arg[4].as< int8_t >() ;

  FC_ASSERT(lps.limit <= SPS_API_SINGLE_QUERY_LIMIT);

  list_proposals_return result;
  result.reserve(lps.limit);

  switch(lps.order_by)
  {
    case by_creator:
    {
      steem::utilities::iterate_results<proposal_index, steem::chain::by_creator>(
        lps.start.as<account_name_type>(),
        result,
        lps.limit,
        _db,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    case by_start_date:
    {
      steem::utilities::iterate_results<proposal_index, steem::chain::by_start_date>(
        lps.start.as<time_point_sec>(),
        result,
        lps.limit,
        _db,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    case by_end_date:
    {
      steem::utilities::iterate_results<proposal_index, steem::chain::by_end_date>(
        lps.start.as<time_point_sec>(),
        result,
        lps.limit,
        _db,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    case by_total_votes:
    {
      steem::utilities::iterate_results<proposal_index, steem::chain::by_total_votes>(
        lps.start.as<uint64_t>(),
        result,
        lps.limit,
        _db,
        [&](auto& proposal) { return api_proposal_object(proposal); } 
      );
    }
    break;
    default:
      FC_ASSERT( false, "Unknown or unsupported sort order" );
  }

  if (!result.empty())
  {
    // sorting operations
    sort_results<list_proposals_return>(result, lps.order_by, lps.order_direction);
  }
  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_voter_proposals) {
  ilog("list_voter_proposals called");
  
  const auto arg = args[0].get_array();

  list_voter_proposals_struct lvps;
  lvps.voter            = arg[0].as< account_name_type >() ;
  lvps.order_by         = arg[1].as< order_by_type >() ;
  lvps.order_direction  = arg[2].as< order_direction_type >() ;
  lvps.limit            = arg[3].as< uint16_t >() ;
  lvps.active           = arg[4].as< int8_t >() ;

  FC_ASSERT(lvps.limit <= SPS_API_SINGLE_QUERY_LIMIT);

  list_voter_proposals_return result;
  result.reserve(lvps.limit);

  steem::utilities::iterate_results<proposal_vote_index, by_voter_proposal>(
    account_name_type(lvps.voter),
    result,
    lvps.limit,
    _db,
    [&](auto& vote_object) 
    { 
      auto po = _db.find<steem::chain::proposal_object, steem::chain::by_id>(vote_object.proposal_id);
      FC_ASSERT(po != nullptr, "Proposal with given id does not exists");
      return api_proposal_object(*po);
    }
  );

  if (!result.empty())
  {
    // sorting operations
    sort_results<list_voter_proposals_return>(result, lvps.order_by, lvps.order_direction);
  }
  return result;
}

} // detail

sps_api::sps_api(): my( new detail::sps_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_SPS_API_PLUGIN_NAME );
}

sps_api::~sps_api() {}

DEFINE_READ_APIS(sps_api,
  (find_proposals)
  (list_proposals)
  (list_voter_proposals)
)

} } } //steem::plugins::sps_api