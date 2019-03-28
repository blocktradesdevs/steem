#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys

LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./sps_test.log"

MODULE_NAME = "SPS Tester via steempy -- Create vote delete"
logger = logging.getLogger(MODULE_NAME)
logger.setLevel(LOG_LEVEL)

ch = logging.StreamHandler(sys.stdout)
ch.setLevel(LOG_LEVEL)
ch.setFormatter(logging.Formatter(LOG_FORMAT))

fh = logging.FileHandler(MAIN_LOG_PATH)
fh.setLevel(LOG_LEVEL)
fh.setFormatter(logging.Formatter(LOG_FORMAT))

if not logger.hasHandlers():
  logger.addHandler(ch)
  logger.addHandler(fh)

try:
    from steem import Steem
except Exception as ex:
    logger.error("SteemPy library is not installed.")
    sys.exit(1)

def get_date_as_isostr(date):
    return date.replace(microsecond=0).isoformat()


def get_isostr_start_end_date(now, start_date_delta, end_date_delta):
    from datetime import timedelta

    start_date = now + timedelta(days = start_date_delta)
    end_date = start_date + timedelta(days = end_date_delta)

    start_date = start_date.replace(microsecond=0).isoformat()
    end_date = end_date.replace(microsecond=0).isoformat()

    return start_date, end_date

def test_create_multiple_proposals(node, creator_account, receiver_account, wif, subject):
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    
    import datetime
    now = datetime.datetime.now()

    from steem.account import Account
    try:
        creator = Account(creator_account)
    except Exception as ex:
        logger.error("Account: {} not found. {}".format(creator_account, ex))
        sys.exit(1)
    
    try:
        receiver = Account(receiver_account)
    except Exception as ex:
        logger.error("Account: {} not found. {}".format(receiver_account, ex))
        sys.exit(1)

    start_end_pairs = [
        [1,1],
        [2,2],
        [4,3],
        [5,4],
        [5,5],
        [5,6],
        [5,7],
        [5,8],
        [6,9]
    ]

    for start_end_pair in start_end_pairs:
        start_date, end_date = get_isostr_start_end_date(now, start_end_pair[0], start_end_pair[1])

        s.commit.create_proposal(
            creator["name"], 
            receiver["name"], 
            start_date, 
            end_date,
            "16.000 TBD",
            subject,
            "steempy-proposal-title"
            )
        sleep(3)
    sleep(6)


def create_post(node, creator_account, wif):
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    from steem.account import Account
    try:
        creator = Account(creator_account)
    except Exception as ex:
        logger.error("Account: {} not found. {}".format(creator_account, ex))
        sys.exit(1)
    s.commit.post("Steempy proposal title", "Steempy proposal body", creator["name"], permlink = "steempy-proposal-title", tags = "proposals")


def test_create_proposal(node, creator_account, receiver_account, wif, subject):
    logger.info("Testing: create_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    
    import datetime
    now = datetime.datetime.now()

    start_date, end_date = get_isostr_start_end_date(now, 10, 2)

    from steem.account import Account
    try:
        creator = Account(creator_account)
    except Exception as ex:
        logger.error("Account: {} not found. {}".format(creator_account, ex))
        sys.exit(1)
    
    try:
        receiver = Account(receiver_account)
    except Exception as ex:
        logger.error("Account: {} not found. {}".format(receiver_account, ex))
        sys.exit(1)

    ret = s.commit.create_proposal(
      creator["name"], 
      receiver["name"], 
      start_date, 
      end_date,
      "16.000 TBD",
      subject,
      "steempy-proposal-title"
    )

    assert ret["operations"][0][1]["creator"] == creator["name"]
    assert ret["operations"][0][1]["receiver"] == receiver["name"]
    assert ret["operations"][0][1]["start_date"] == start_date
    assert ret["operations"][0][1]["end_date"] == end_date
    assert ret["operations"][0][1]["daily_pay"] == "16.000 TBD"
    assert ret["operations"][0][1]["subject"] == subject
    assert ret["operations"][0][1]["permlink"] == "steempy-proposal-title"

def test_list_proposals(node, account, wif, subject):
    logger.info("Testing: list_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # list inactive proposals, our proposal shoud be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")
    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None
    
    # list active proposals, our proposal shouldnt be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "active")
    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is None

    # list all proposals, our proposal should be here
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "all")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None

def test_find_proposals(node, account, wif, subject):
    logger.info("Testing: find_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    ret = s.find_proposals([proposal_id])
    assert ret[0]["subject"] == found["subject"]

def test_vote_proposal(node, account, wif, subject):
    logger.info("Testing: vote_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    # now lets vote
    ret = s.commit.update_proposal_votes(account, [proposal_id], True)
    assert ret["operations"][0][1]["voter"] == account
    assert ret["operations"][0][1]["proposal_ids"][0] == proposal_id
    assert ret["operations"][0][1]["approve"] == True
    sleep(6)

def test_list_voter_proposals(node, account, wif, subject):
    logger.info("Testing: list_voter_proposals")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    voter_proposals = s.list_voter_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for voter, proposals in voter_proposals.items():
        for proposal in proposals:
            if proposal["subject"] == subject:
                found = proposal
    
    assert found is not None

def test_remove_proposal(node, account, wif, subject):
    logger.info("Testing: remove_proposal")
    s = Steem(nodes = [node], no_broadcast = False, keys = [wif])
    # first we will find our special proposal and get its id
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is not None
    proposal_id = int(found["id"])

    # remove proposal
    s.commit.remove_proposal(account, [proposal_id])

    # try to find our special proposal
    proposals = s.list_proposals(account, "by_creator", "direction_ascending", 1000, "inactive")

    found = None
    for proposal in proposals:
        if proposal["subject"] == subject:
            found = proposal
    
    assert found is None


if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create proposals with")
    parser.add_argument("receiver", help = "Account to receive funds")
    parser.add_argument("wif", help = "Private key for accout for proposal generation")
    parser.add_argument("--node-address", help = "IP address and port of steem node", default = "http://127.0.0.1:8090", dest = "node_url")

    args = parser.parse_args()

    logger.info("Using node at: {}".format(args.node_url))

    subject = str(uuid4())
    
    logger.info("Creating initial post...")
    create_post(args.node_url, args.creator, args.wif)

    logger.info("Populating proposal index with some proposals...")
    test_create_multiple_proposals(args.node_url, args.creator, args.receiver, args.wif, subject)

    logger.info("At each iteration we will create proposal, list proposals, find proposals, vote proposal, list voter proposal and then remove proposal.")
    for n in range(0, 10):
      logger.info("Iteration: {}".format(n))
      subject = str(uuid4())
      logger.info("Subject of testing proposal is set to: {}".format(subject))
      test_create_proposal(args.node_url, args.creator, args.receiver, args.wif, subject)
      test_list_proposals(args.node_url, args.creator, args.wif, subject)
      test_find_proposals(args.node_url, args.creator, args.wif, subject)
      test_vote_proposal(args.node_url, args.creator, args.wif, subject)
      test_list_voter_proposals(args.node_url, args.creator, args.wif, subject)
      test_remove_proposal(args.node_url, args.creator, args.wif, subject)
