/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <enumivo/chain_plugin/chain_plugin.hpp>

namespace fc { class variant; }

namespace enumivo {
    using namespace appbase;
    using std::shared_ptr;
    using chain::transaction_id_type;
    using chain::action_name;
    using fc::optional;

    typedef shared_ptr<class ram_plugin_impl> ram_ptr;
    typedef shared_ptr<const class ram_plugin_impl> ram_const_ptr;

namespace ram_apis {

class read_only {
   ram_const_ptr ram;

   public:
      read_only(ram_const_ptr&& ram)
         : ram(ram) {}

      struct get_actions_params {
         optional<int32_t> pos;
         optional<int32_t> offset;
      };

      struct ram_action_result {
          uint64_t action_seq = 0;

          account_name payer;
          account_name receiver;

          action_name name;

          asset token;
          asset fee;
          int64_t exp_ram;
          int64_t act_ram;

          uint32_t                     block_num;
          chain::block_timestamp_type  block_time;
          transaction_id_type  trx_id;
      };

      struct get_actions_result {
          vector<ram_action_result>     actions;
          uint32_t                      last_irreversible_block;
          optional<bool>                time_limit_exceeded_error;
      };

      struct evaluate_params {
          asset from;
      };

      struct evaluate_result {
          asset to;
          asset fee;

          asset rammarket_base;
          asset rammarket_quote;

          uint32_t  last_irreversible_block;
      };

      get_actions_result get_actions( const get_actions_params& )const;
      evaluate_result evaluate( const evaluate_params& )const;

};
} // namespace ram_apis

class ram_plugin : public appbase::plugin<ram_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   ram_plugin();
   virtual ~ram_plugin();
 
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   ram_apis::read_only  get_read_only_api()const { return ram_apis::read_only(ram_const_ptr(my)); }

private:
   ram_ptr my;
};

} // namespace enumivo


FC_REFLECT( enumivo::ram_apis::read_only::get_actions_params, (pos)(offset) )
FC_REFLECT( enumivo::ram_apis::read_only::get_actions_result, (actions)(last_irreversible_block)(time_limit_exceeded_error) )
FC_REFLECT( enumivo::ram_apis::read_only::ram_action_result, (action_seq)(payer)(receiver)(name)(token)(fee)(exp_ram)(act_ram)(block_num)(block_time)(trx_id) )
