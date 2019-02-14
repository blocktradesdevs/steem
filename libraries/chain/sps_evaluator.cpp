#include <steem/protocol/sps_operations.hpp>

#include <steem/chain/database.hpp>

#include <steem/chain/steem_evaluator.hpp>
#include <steem/chain/sps_objects.hpp>


namespace steem { namespace chain {

using steem::chain::create_proposal_evaluator;

void create_proposal_evaluator::do_apply( const create_proposal_operation& o )
{
   try
   {
      elog("create_proposal_evaluator: ${op}", ("op", o));

      asset fee_sbd( STEEM_TREASURY_FEE, SBD_SYMBOL );

      FC_ASSERT( _db.get_balance( o.creator, SBD_SYMBOL ) >= fee_sbd,
         "Account does not have sufficient funds for specified fee of ${of}", ("of", fee_sbd) );

      const auto& owner_account = _db.get_account( o.creator );
      const auto& treasury_account = _db.get_account( STEEM_TREASURY_ACCOUNT );

      _db.create< proposal_object >( [&]( proposal_object& proposal )
      {
         proposal.creator = o.creator;
         proposal.receiver = o.receiver;

         proposal.start_date = o.start_date;
         proposal.end_date = o.end_date;

         proposal.daily_pay = o.daily_pay;

         proposal.subject = o.subject.c_str();

         proposal.url = o.url.c_str();
      });

      _db.adjust_balance( owner_account, -fee_sbd );
      _db.adjust_balance( treasury_account, fee_sbd );
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}

void update_proposal_votes_evaluator::do_apply( const update_proposal_votes_operation& o )
{
   try
   {
      if( o.proposal_ids.empty() )
         return;

      const auto& pidx = _db.get_index< proposal_index >().indices().get< by_id >();
      const auto& pvidx = _db.get_index< proposal_vote_index >().indices().get< by_voter_proposal >();

      for( const auto id : o.proposal_ids )
      {
         //checking if proposal id exists
         auto found_id = pidx.find( id );
         if( found_id == pidx.end() )
            continue;

         auto found = pvidx.find( boost::make_tuple( o.voter, id ) );

         if( o.approve )
         {
            if( found == pvidx.end() )
               _db.create< proposal_vote_object >( [&]( proposal_vote_object& proposal_vote )
               {
                  proposal_vote.voter = o.voter;
                  proposal_vote.proposal_id = id;
               } );
         }
         else
         {
            if( found != pvidx.end() )
               _db.remove( *found );
         }
      }
   }
   FC_CAPTURE_AND_RETHROW( (o) )
}


} } // steem::chain
