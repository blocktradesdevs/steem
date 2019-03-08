#!/usr/bin/python3

import time

#from ... import utils
from utils.test_utils import *
from utils.cmd_args   import args
from utils.cli_wallet import CliWallet
from utils.logger     import log

if __name__ == "__main__":
    try:
        error = False
        wallet = CliWallet( args.path,
                            args.server_rpc_endpoint,
                            args.cert_auth,
                            #args.rpc_endpoint,
                            args.rpc_tls_endpoint,
                            args.rpc_tls_cert,
                            args.rpc_http_endpoint,
                            args.deamon, 
                            args.rpc_allowip,
                            args.wallet_file,
                            args.chain_id  )
        wallet.set_and_run_wallet()

        wallet.create_account("initminer", "crttest", "", "true")
        json = last_message_as_json(wallet.list_proposals("initminer", "creator", "asc", 50, 1))
        proposals_before = len(json["result"])

        log.info("proposals_before {0}".format(proposals_before))

        wallet.create_proposal("initminer", "crttest", "2029-06-02T00:00:00", "2029-08-01T00:00:00", "1.000 TBD", "this is subject", "http://url.html", "true")
        json = last_message_as_json(wallet.list_proposals("initminer", "creator", "asc", 50, 1))
        proposals_after = len(json["result"])

        log.info("proposals_after {0}".format(proposals_after))

        if not proposals_before + 1 == proposals_after:
            raise ArgsCheckException("proposals_before +1 should be equal to proposals_after.")

    except Exception as _ex:
        log.exception(str(_ex))
        error = True
    finally:
        if error:
            log.error("TEST `{0}` failed".format(__file__))
            exit(1)
        else:
            log.info("TEST `{0}` passed".format(__file__))
            exit(0)

