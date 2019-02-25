#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>
#include <steem/protocol/sps_operations.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/chain/util/reward.hpp>

#include <steem/plugins/rc/rc_objects.hpp>
#include <steem/plugins/rc/resource_count.hpp>

#include <steem/chain/sps_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( proposal_tests, proposal_database_fixture )

BOOST_AUTO_TEST_CASE( proposal_object_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create_proposal_operation" );

      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto fee = asset( STEEM_TREASURY_FEE, SBD_SYMBOL );

      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay = asset( 100, SBD_SYMBOL );

      auto subject = "hello";
      auto url = "http:://something.html";

      FUND( creator, ASSET( "80.000 TBD" ) );

      signed_transaction tx;

      const account_object& before_alice_account = db->get_account( creator );
      const account_object& before_bob_account = db->get_account( receiver );

      auto before_alice_sbd_balance = before_alice_account.sbd_balance;
      auto before_bob_sbd_balance = before_bob_account.sbd_balance;

      create_proposal_operation op;

      op.creator = creator;
      op.receiver = receiver;

      op.start_date = start_date;
      op.end_date = end_date;

      op.daily_pay = daily_pay;

      op.subject = subject;
      op.url = url;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      const account_object& after_alice_account = db->get_account( creator );
      const account_object& after_bob_account = db->get_account( receiver );

      auto after_alice_sbd_balance = after_alice_account.sbd_balance;
      auto after_bob_sbd_balance = after_bob_account.sbd_balance;

      BOOST_REQUIRE( before_alice_sbd_balance == after_alice_sbd_balance + fee );
      BOOST_REQUIRE( before_bob_sbd_balance == after_bob_sbd_balance - fee );

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( creator );
      BOOST_REQUIRE( found != proposal_idx.end() );

      BOOST_REQUIRE( found->creator == creator );
      BOOST_REQUIRE( found->receiver == receiver );
      BOOST_REQUIRE( found->start_date == start_date );
      BOOST_REQUIRE( found->end_date == end_date );
      BOOST_REQUIRE( found->daily_pay == daily_pay );
      BOOST_REQUIRE( found->subject == subject );
      BOOST_REQUIRE( found->url == url );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposal_vote_object_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: proposal_vote_object_operation" );

      ACTORS( (alice)(bob)(carol)(dan) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay = asset( 100, SBD_SYMBOL );

      FUND( creator, ASSET( "80.000 TBD" ) );

      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay, alice_private_key );

      signed_transaction tx;
      update_proposal_votes_operation op;
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_voter_proposal >();

      auto voter_01 = "carol";
      auto voter_01_key = carol_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting for proposal( `id_proposal_00` )---" );
         op.voter = voter_01;
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         auto found = proposal_vote_idx.find( std::make_tuple( voter_01, id_proposal_00 ) );
         BOOST_REQUIRE( found->voter == voter_01 );
         BOOST_REQUIRE( static_cast< int64_t >( found->proposal_id ) == id_proposal_00 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting proposal( `id_proposal_00` )---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         auto found = proposal_vote_idx.find( std::make_tuple( voter_01, id_proposal_00 ) );
         BOOST_REQUIRE( found == proposal_vote_idx.end() );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( proposal_vote_object_01_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: proposal_vote_object_operation" );

      ACTORS( (alice)(bob)(carol)(dan) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto creator = "alice";
      auto receiver = "bob";

      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date = start_date + fc::days( 2 );

      auto daily_pay_00 = asset( 100, SBD_SYMBOL );
      auto daily_pay_01 = asset( 101, SBD_SYMBOL );
      auto daily_pay_02 = asset( 102, SBD_SYMBOL );

      FUND( creator, ASSET( "80.000 TBD" ) );

      int64_t id_proposal_00 = create_proposal( creator, receiver, start_date, end_date, daily_pay_00, alice_private_key );
      int64_t id_proposal_01 = create_proposal( creator, receiver, start_date, end_date, daily_pay_01, alice_private_key );

      signed_transaction tx;
      update_proposal_votes_operation op;
      const auto& proposal_vote_idx = db->get_index< proposal_vote_index >().indices().get< by_voter_proposal >();

      std::string voter_01 = "carol";
      auto voter_01_key = carol_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_01` for proposals( `id_proposal_00`, `id_proposal_01` )---" );
         op.voter = voter_01;
         op.proposal_ids.insert( id_proposal_00 );
         op.proposal_ids.insert( id_proposal_01 );
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 2 );
      }

      int64_t id_proposal_02 = create_proposal( creator, receiver, start_date, end_date, daily_pay_02, alice_private_key );
      std::string voter_02 = "dan";
      auto voter_02_key = dan_private_key;

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_02` for proposals( `id_proposal_00`, `id_proposal_01`, `id_proposal_02` )---" );
         op.voter = voter_02;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_02 );
         op.proposal_ids.insert( id_proposal_00 );
         op.proposal_ids.insert( id_proposal_01 );
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_02_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_02 );
         while( found != proposal_vote_idx.end() && found->voter == voter_02 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 3 );
      }

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_02` for proposals( `id_proposal_00` )---" );
         op.voter = voter_02;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_02_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_02 );
         while( found != proposal_vote_idx.end() && found->voter == voter_02 )
         {
            ++cnt;
            ++found;
         }
        BOOST_REQUIRE( cnt == 3 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_02` )---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_02 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 2 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_00` )---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 1 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_02` proposals( `id_proposal_00`, `id_proposal_01`, `id_proposal_02` )---" );
         op.voter = voter_02;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_02 );
         op.proposal_ids.insert( id_proposal_01 );
         op.proposal_ids.insert( id_proposal_00 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_02_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_02 );
         while( found != proposal_vote_idx.end() && found->voter == voter_02 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 0 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` proposals( `id_proposal_01` )---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.proposal_ids.insert( id_proposal_01 );
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 0 );
      }

      {
         BOOST_TEST_MESSAGE( "---Voting by `voter_01` for nothing---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.approve = true;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 0 );
      }

      {
         BOOST_TEST_MESSAGE( "---Unvoting by `voter_01` nothing---" );
         op.voter = voter_01;
         op.proposal_ids.clear();
         op.approve = false;

         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, voter_01_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         int32_t cnt = 0;
         auto found = proposal_vote_idx.find( voter_01 );
         while( found != proposal_vote_idx.end() && found->voter == voter_01 )
         {
            ++cnt;
            ++found;
         }
         BOOST_REQUIRE( cnt == 0 );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}


struct create_proposal_data {
      std::string creator    ;
      std::string receiver   ;
      fc::time_point_sec start_date ;
      fc::time_point_sec end_date   ;
      steem::protocol::asset daily_pay ;
      std::string subject ;   
      std::string url     ;   

      create_proposal_data(fc::time_point_sec _start) {
         creator    = "alice";
         receiver   = "bob";
         start_date = _start     + fc::days( 1 );
         end_date   = start_date + fc::days( 2 );
         daily_pay  = asset( 100, SBD_SYMBOL );
         subject    = "hello";
         url        = "http:://something.html";
      }
};

BOOST_AUTO_TEST_CASE( create_proposal_000 )
{
   try {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - all args are ok" );
      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      generate_block();

      auto creator    = "alice";
      auto receiver   = "bob";
      auto start_date = db->head_block_time() + fc::days( 1 );
      auto end_date   = start_date + fc::days( 2 );
      auto daily_pay  = asset( 100, SBD_SYMBOL );
      auto subject    = "hello";
      auto url        = "http:://something.html";

      FUND( creator, ASSET( "80.000 TBD" ) );

      {  //hande made create proposal 
         create_proposal_operation op;
         op.creator    = creator;
         op.receiver   = receiver;
         op.start_date = start_date;
         op.end_date   = end_date;
         op.daily_pay  = daily_pay;
         op.subject    = subject;
         op.url        = url;

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, alice_private_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
         auto found = proposal_idx.find( creator );
         BOOST_REQUIRE( found != proposal_idx.end() );
         BOOST_REQUIRE( found->creator    == creator );
         BOOST_REQUIRE( found->receiver   == receiver );
         BOOST_REQUIRE( found->start_date == start_date );
         BOOST_REQUIRE( found->end_date   == end_date );
         BOOST_REQUIRE( found->daily_pay  == daily_pay );
         BOOST_REQUIRE( found->subject    == subject );
         BOOST_REQUIRE( found->url        == url );
      }

      {
         int64_t proposal = create_proposal( creator, receiver, start_date, end_date, daily_pay, alice_private_key );
         BOOST_REQUIRE( proposal > 0 );
      }
      validate_database();
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_001 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid creator" );
      {
         create_proposal_data cpd(db->head_block_time());
         ACTORS( (alice)(bob) )
         generate_block();
         FUND( cpd.creator, ASSET( "80.000 TBD" ) );
         int64_t proposal = create_proposal( "", cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
         BOOST_REQUIRE( proposal == -1 );
      }
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_002 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid receiver" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      int64_t proposal = create_proposal( cpd.creator, "", cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE( proposal == -1 );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_003 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid start date" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      cpd.start_date = cpd.end_date + fc::days(2);
      int64_t proposal = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE( proposal == -1 );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_004 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid end date" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      cpd.end_date = cpd.start_date - fc::days(2);
      int64_t proposal = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE( proposal == -1 );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_005 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid daily pay (negative asset)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      cpd.daily_pay = asset( -10, SBD_SYMBOL );
      int64_t proposal = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE( proposal == -1 );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_006 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid daily pay (zero asset)" );
      create_proposal_data cpd(db->head_block_time());
      ACTORS( (alice)(bob) )
      generate_block();
      FUND( cpd.creator, ASSET( "80.000 TBD" ) );
      cpd.daily_pay = asset( 0, SBD_SYMBOL );
      int64_t proposal = create_proposal( cpd.creator, cpd.receiver, cpd.start_date, cpd.end_date, cpd.daily_pay, alice_private_key );
      BOOST_REQUIRE( proposal == -1 );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_proposal_007 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create proposal: opration arguments validation - invalid subject" );
      ACTORS( (alice)(bob) )
      create_proposal_operation cpo;
      cpo.creator    = "alice";
      cpo.receiver   = "bob";
      cpo.start_date = db->head_block_time() + fc::days( 1 );
      cpo.end_date   = cpo.start_date + fc::days( 2 );
      cpo.daily_pay  = asset( 100, SBD_SYMBOL );
      cpo.subject    = "hello";
      cpo.url        = "http:://something.html";
      signed_transaction tx;
      tx.operations.push_back( cpo );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      const auto& proposal_idx = db->get_index< proposal_index >().indices().get< by_creator >();
      auto found = proposal_idx.find( cpo.creator );
      BOOST_REQUIRE( found == proposal_idx.end() );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_000 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - all ok" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_001 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid voter" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_002 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid id array (empty array)" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_003 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid id array (array with negative digits)" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_004 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid id array (digit instead of array)" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_005 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid id array (array with greater number of digits than allowed)" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( update_proposal_votes_006 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: update proposal votes: opration arguments validation - invalid approve" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_000 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - all ok" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_001 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid deleter" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_002 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid array(empty array)" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_003 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid array(array with greater number of digits than allowed)" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( remove_proposal_004 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: remove proposal: opration arguments validation - invalid array(invalid args in array)" );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}
BOOST_AUTO_TEST_SUITE_END()
