#pragma once
#include <steem/plugins/sps_api/sps_api.hpp>
#include <steem/plugins/json_rpc/json_rpc_plugin.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>

namespace steem { namespace plugins { namespace sps {

  #define STEEM_SPS_API_PLUGIN_NAME "sps_api"
  
  class sps_api_plugin : public appbase::plugin<sps_api_plugin>
  {
    public:
      sps_api_plugin();
      virtual ~sps_api_plugin();

      APPBASE_PLUGIN_REQUIRES(
        (plugins::json_rpc::json_rpc_plugin)
        (steem::plugins::chain::chain_plugin)
      );

      static const std::string& name() {
        static std::string name = STEEM_SPS_API_PLUGIN_NAME; 
        return name; 
      }

      virtual void set_program_options(boost::program_options::options_description&, boost::program_options::options_description&) override {
      }

      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      std::shared_ptr<class sps_api> api;
  };

} } }