#!/usr/bin/python3

from uuid import uuid4
from time import sleep
import logging
import sys
import steem_utils.steem_runner
import steem_utils.steem_tools


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
        logger.info("Creating account: {}".format(account['name']))
        node.commit.create_account(account['name'], 
            owner_key=account['public_key'], 
            active_key=account['public_key'], 
            posting_key=account['public_key'],
            memo_key=account['public_key'],
            store_keys = False,
            creator=creator,
            asset='TESTS')
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


# transfer_to_vesting initminer pychol "310.000 TESTS" true
def transfer_to_vesting(node, from_account, accounts):
    for acnt in accounts:
        logger.info("Transfer to vesting from {} to {} amount {} {}".format(from_account, acnt['name'], "310.000", "TESTS"))
        node.commit.transfer_to_vesting("310.000", to = acnt['name'], 
            account = from_account, asset='TESTS')
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


# transfer initminer pychol "399.000 TESTS" "initial transfer" true
# transfer initminer pychol "398.000 TBD" "initial transfer" true
def transfer_assets_to_accounts(node, from_account, accounts):
    for acnt in accounts:
        logger.info("Transfer from {} to {} amount {} {}".format(from_account, acnt['name'], "399.000", "TESTS"))
        node.commit.transfer(acnt['name'], "399.000", "TESTS", memo = "initial transfer", account = from_account)
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)
    for acnt in accounts:
        logger.info("Transfer from {} to {} amount {} {}".format(from_account, acnt['name'], "398.000", "TBD"))
        node.commit.transfer(acnt['name'], "398.000", "TBD", memo = "initial transfer", account = from_account)
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


def create_posts(node, accounts):
    logger.info("Creating posts...")
    for acnt in accounts:
        node.commit.post("Steempy proposal title [{}]".format(acnt['name']), 
            "Steempy proposal body [{}]".format(acnt['name']), 
            acnt['name'], 
            permlink = "steempy-proposal-title-{}".format(acnt['name']), 
            tags = "proposals")
    steem_utils.steem_tools.wait_for_blocks_produced(5, node.url)


if __name__ == '__main__':
    logger.info("Performing SPS tests")
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("creator", help = "Account to create test accounts with")

    args = parser.parse_args()

    node = steem_utils.steem_runner.SteemNode("/home/dariusz-work/Builds/steem/programs/steemd/steemd", "/home/dariusz-work/steem-data", "./steem_utils/resources/config.ini.in")
    node_url = node.get_node_url()
    wif = node.get_from_config('private-key')[0]

    logger.info("Using node at: {}".format(node_url))
    logger.info("Using private-key: {}".format(wif))

    accounts = [
        {"name" : "tester001", "private_key" : "", "public_key" : ""}
    ]

    keys = [wif]
    for account in accounts:
        keys.append(account["private_key"])
    
    node.run_steem_node()
    try:
        if node.is_running():
            node_client = Steem(nodes = [node_url], no_broadcast = False, keys = keys)

            create_accounts(node_client, args.creator, accounts)

            transfer_to_vesting(node_client, args.creator, accounts)            

            transfer_assets_to_accounts(node_client, args.creator, accounts)

            create_posts(node_client, accounts)

            node.stop_steem_node()
            sys.exit(0)
        sys.exit(1)
    except Exception as ex:
        logger.error("Exception: {}".format(ex))
        node.stop_steem_node()
        sys.exit(1)
