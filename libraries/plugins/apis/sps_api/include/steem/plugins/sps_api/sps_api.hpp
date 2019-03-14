#pragma once
#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/chain/sps_objects.hpp>

#define SPS_API_SINGLE_QUERY_LIMIT 1000

namespace steem { namespace plugins { namespace sps {
  using plugins::json_rpc::void_type;
  using steem::chain::account_name_type;
  using steem::chain::proposal_object;
  using steem::chain::proposal_id_type;
  using steem::protocol::asset;
  using steem::chain::to_string;

  namespace detail
  {
    class sps_api_impl;
  }

  enum order_direction_type : int
  {
    direction_ascending, ///< sort with ascending order
    direction_descending ///< sort with descending order
  };

  enum order_by_type : int
  {
    by_creator, ///< order by proposal creator
    by_start_date, ///< order by proposal start date
    by_end_date, ///< order by proposal end date
    by_total_votes, ///< order by total votes
  };

  enum proposal_status : int
  {
    active = 1,
    inactive = 0,
    all = -1,
  };

   inline order_direction_type to_order_direction(std::string _order_type)
   {
      std::transform(_order_type.begin(), _order_type.end(), _order_type.begin(), [](unsigned char c) {return std::tolower(c); });
      if(_order_type == "desc")
         return order_direction_type::direction_descending;
      else
         return order_direction_type::direction_ascending;
   }

   inline order_by_type to_order_by(std::string _order_by)
   {
      std::transform(_order_by.begin(), _order_by.end(), _order_by.begin(), [](unsigned char c) {return std::tolower(c); });
      if(_order_by == "start_date")
         return order_by_type::by_start_date;
      else if(_order_by == "end_date")
         return order_by_type::by_end_date;
      else if(_order_by == "total_votes")
         return order_by_type::by_total_votes;
      else
         return order_by_type::by_creator; /// Consider exception throw when no constant was matched...
   }

   inline proposal_status to_proposal_status(std::string _status)
   {
   std::transform(_status.begin(), _status.end(), _status.begin(), [](unsigned char c) {return std::tolower(c); });

   if(_status == "active")
      return proposal_status::active;
   else if(_status == "inactive")
      return proposal_status::inactive;
   else
      return proposal_status::all;  /// Consider exception throw when no constant was matched...
   }

  typedef uint64_t api_id_type;

  struct api_proposal_object
  {
    api_proposal_object() = default;

    api_proposal_object(const proposal_object& po) : 
      id(po.id),
      creator(po.creator),
      receiver(po.receiver),
      start_date(po.start_date),
      end_date(po.end_date),
      daily_pay(po.daily_pay),
      subject(to_string(po.subject)),
      permlink(to_string(po.permlink)),
      total_votes(po.total_votes)
    {}

    //internal key
    api_id_type id = 0;

    // account that created the proposal
    account_name_type creator;

    // account_being_funded
    account_name_type receiver;

    // start_date (when the proposal will begin paying out if it gets enough vote weight)
    time_point_sec start_date;

    // end_date (when the proposal expires and can no longer pay out)
    time_point_sec end_date;

    //daily_pay (the amount of SBD that is being requested to be paid out daily)
    asset daily_pay;

    //subject (a very brief description or title for the proposal)
    string subject;

    //permlink (a link to a page describing the work proposal in depth, generally this will probably be to a Steem post).
    string permlink;

    //This will be calculate every maintenance period
    uint64_t total_votes = 0;

    const bool is_active(const time_point_sec &head_time) const
    {
      if (head_time >= start_date && head_time <= end_date)
      {
        return true;
      }
      return false;
    }
  };

  // Struct with arguments for find_proposals methd
  struct find_proposals_args 
  {
    // set of ids of the proposals to find
    flat_set<api_id_type> id_set;
  };

  // Return type for find_proposal method
  typedef std::vector<api_proposal_object> find_proposals_return;
  
  // Struct with argumentse for list_proposals method
  struct list_proposals_args 
  {
    // starting value for querying results
    fc::variant start;
    // name of the field by which results will be sored
    order_by_type order_by;
    // sorting order (ascending or descending) of the result vector
    order_direction_type order_direction;
     // query limit
    uint16_t limit = 0;
    // result will contain only data with status flag set to this value
    proposal_status status = proposal_status::all;
  };

  // Return type for list_proposals
  typedef std::vector<api_proposal_object> list_proposals_return;
  
  // Struct with arguments for list_voter_proposals methid
  struct list_voter_proposals_args 
  {
    // list only proposal voted by this voter
    account_name_type voter;
    // name of the field by which results will be sored
    order_by_type order_by;
    // sorting order (ascending or descending) of the result vector
    order_direction_type order_direction;
    // query limit
    uint16_t limit = 0;
    // result will contain only data with status flag set to this value
    proposal_status status = proposal_status::all;
  };

  // Return type for list_voter_proposals
  typedef std::map<std::string, std::vector<api_proposal_object> > list_voter_proposals_return;
  
  class sps_api
  {
    public:
      sps_api();
      ~sps_api();

      DECLARE_API(
        (find_proposals)
        (list_proposals)
        (list_voter_proposals)
        )
    private:
        std::unique_ptr<detail::sps_api_impl> my;
  };

  } } }

// Args and return types need to be reflected. We do not reflect typedefs of already reflected types
FC_REFLECT_ENUM(steem::plugins::sps::order_direction_type, 
  (direction_ascending)
  (direction_descending)
  );

FC_REFLECT_ENUM(steem::plugins::sps::order_by_type, 
  (by_creator)
  (by_start_date)
  (by_end_date)
  (by_total_votes)
  );

FC_REFLECT_ENUM(steem::plugins::sps::proposal_status,
  (active)
  (inactive)
  (all)
  );

FC_REFLECT(steem::plugins::sps::api_proposal_object,
  (id)
  (creator)
  (receiver)
  (start_date)
  (end_date)
  (daily_pay)
  (subject)
  (permlink)
  (total_votes)
  );

FC_REFLECT(steem::plugins::sps::find_proposals_args, 
  (id_set)
  );

FC_REFLECT(steem::plugins::sps::list_proposals_args, 
  (start)
  (order_by)
  (order_direction)
  (limit)
  (status)
  );

FC_REFLECT(steem::plugins::sps::list_voter_proposals_args, 
  (voter)
  (order_by)
  (order_direction)
  (limit)
  (status)
  );

