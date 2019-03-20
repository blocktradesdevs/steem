#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys

LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)-15s - %(name)s - %(levelname)s - %(message)s"
MAIN_LOG_PATH = "./sps_proposal_payment.log"

MODULE_NAME = "SPS Tester via steempy - proposal payment test"
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


# 1. create few proposals.
# 2. vote on them to show differences in asset distribution (depending on collected votes)
# 3. wait for proposal payment phase
# 4. verify (using account history and by checking regular account balance) that given accounts have been correctly paid.


# create_account "initminer" "pychol" "" true
def create_accounts(node, creator, accounts):
    for account in accounts:
        node.commit.create_account(account['name'], 
            owner_key=account['owner_key'], 
            active_key=account['active_key'], 
            posting_key=account['posting_key'],
            memo_key=account['memo_key'],
            creator=creator)


# transfer_to_vesting initminer pychol "310.000 TESTS" true
def transfer_to_vesting(node, from_account, accounts):
    for acnt in accounts:
        node.commit.transfer_to_vesting("310.000 TESTS", to = acnt['name'], 
            account = from_account)


# transfer initminer pychol "399.000 TESTS" "initial transfer" true
# transfer initminer pychol "398.000 TBD" "initial transfer" true
def transfer_assets_to_accounts(node, from_account, accounts):
    for acnt in accounts:
        node.commit.transfer(acnt['name'], "399.000", "TESTS", memo = "initial transfer", account = from_account)
        node.commit.transfer(acnt['name'], "398.000", "TBD", memo = "initial transfer", account = from_account)


def create_posts(node, accounts):
    for acnt in accounts:
        node.commit.post("Steempy proposal title [{}]".format(acnt['name']), 
            "Steempy proposal body [{}]".format(acnt['name']), 
            acnt['name'], 
            permlink = "steempy-proposal-title-{}".format(acnt['name']), 
            tags = "proposals")


if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create test accounts with")
    parser.add_argument("wif", help = "Private key for accout for test accounts generation")
    parser.add_argument("--node-address", help = "IP address and port of steem node", default = "http://127.0.0.1:8090", dest = "node_url")
    parser.add_argument("--no-erase-proposal", action='store_false', dest = "no_erase_proposal", help = "Do not erase proposal created with this test")

    args = parser.parse_args()

    logger.info("Using node at: {}".format(args.node_url))

    node = Steem(nodes = [args.node_url], no_broadcast = False, keys = [args.wif])
    accounts = []
    
    create_accounts(node, args.creator, accounts)
    node.debug_generate_blocks(args.wif, 10, 0, 0, True)
    
    transfer_to_vesting(node, args.creator, accounts)
    node.debug_generate_blocks(args.wif, 10, 0, 0, True)
    
    transfer_assets_to_accounts(node, args.creator, accounts)
    node.debug_generate_blocks(args.wif, 10, 0, 0, True)

    create_posts(node, accounts)
    node.debug_generate_blocks(args.wif, 10, 0, 0, True)