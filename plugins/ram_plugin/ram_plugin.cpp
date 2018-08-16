/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */
#include <enumivo/chain/trace.hpp>
#include <enumivo/ram_plugin.hpp>
#include <enumivo/chain_plugin/chain_plugin.hpp>

#include <boost/signals2/connection.hpp>
#include <cmath>
//#include <algorithm>


namespace enumivo {
    using namespace chain;
    using namespace fc;
    static appbase::abstract_plugin& _ram_plugin = app().register_plugin<ram_plugin>();
    using boost::signals2::scoped_connection;

    struct ram_action_object : public chainbase::object<ram_action_object_type, ram_action_object>  {
        OBJECT_CTOR( ram_action_object );

        id_type      id;

        account_name payer;
        account_name receiver;

        action_name name;
        
        asset token;
        asset fee;
        int64_t exp_ram;
        int64_t act_ram;

        uint32_t             block_num;
        block_timestamp_type block_time;
        transaction_id_type  trx_id;
    };

    struct account_ram_object : public chainbase::object<account_ram_object_type, account_ram_object>  {
        OBJECT_CTOR( account_ram_object );

        id_type      id;
        account_name account;
        int64_t      ram;
    };

    using ram_action_object_id_type  = ram_action_object::id_type;
    using account_ram_object_id_type = account_ram_object::id_type;

    using ram_action_index = chainbase::shared_multi_index_container<
        ram_action_object,
        indexed_by<
             ordered_unique<tag<by_id>, member<ram_action_object, ram_action_object::id_type, &ram_action_object::id>>
        >
    >;

    struct by_account;

    using account_ram_index = chainbase::shared_multi_index_container<
        account_ram_object,
        indexed_by<
             ordered_unique< tag<by_id>, member<account_ram_object, account_ram_object::id_type, &account_ram_object::id> >,
             ordered_unique< tag<by_account>, member<account_ram_object, account_name, &account_ram_object::account> >
        >
    >;
}

CHAINBASE_SET_INDEX_TYPE(enumivo::ram_action_object, enumivo::ram_action_index)
CHAINBASE_SET_INDEX_TYPE(enumivo::account_ram_object, enumivo::account_ram_index)


namespace enumivo {
    class ram_plugin_impl {
        public:
            chain_plugin*          chain_plug = nullptr;
            fc::optional<scoped_connection> applied_transaction_connection;
            fc::microseconds abi_serializer_max_time;

            int64_t calc_ram_delta( account_name account ) {
                auto& chain = chain_plug->chain();
                const auto& rm = chain.get_resource_limits_manager();
                auto& db = chain.db();

                int64_t old_ram_balance, new_ram_balance;
                int64_t net_quant, cpu_quant;
                rm.get_account_limits( account, new_ram_balance, net_quant, cpu_quant);
                if( nullptr == db.find<account_ram_object, by_account>(account) )
                {
                    old_ram_balance = 0;
                    db.create<account_ram_object>( [&]( auto& o ) {
                        o.account = account;
                        o.ram = new_ram_balance;
                    });
                } else {
                    auto& aro = db.get<account_ram_object, by_account>(account);
                    old_ram_balance = aro.ram;
                    db.modify<account_ram_object>( aro, [&]( auto& o ) {
                        o.ram = new_ram_balance;
                    });
                }
                return std::abs(new_ram_balance - old_ram_balance);
            }

            void on_buyrambytes( const action_trace& t ) {
                auto& chain = chain_plug->chain();
                auto& db = chain.db();
                fc::variant vt = chain.to_variant_with_abi(t, abi_serializer_max_time);

                // get RAM balance before this action applied, and update it with the new value
                int64_t net_quant, cpu_quant, ram_delta;
                account_name receiver = account_name(vt["act"]["data"]["receiver"].get_string());
                ram_delta = calc_ram_delta(receiver);

                // calculate the cost
                asset token_sub_fee, fee, token;
                for( const auto& iline : t.inline_traces ) {
                    fc::variant vi = chain.to_variant_with_abi(iline, abi_serializer_max_time);
                    if( vi["act"]["data"]["to"].get_string() == "enu.ram" )
                    {
                        token_sub_fee = asset::from_string(vi["act"]["data"]["quantity"].get_string());
                    }
                    if( vi["act"]["data"]["to"].get_string() == "enu.ramfee" )
                    {
                        fee = asset::from_string(vi["act"]["data"]["quantity"].get_string());
                    }
                }
                token = token_sub_fee + fee;

                // create ram action object
                const auto& o = db.create<ram_action_object>( [&]( auto& rao ) {
                    rao.payer       = account_name(vt["act"]["data"]["payer"].get_string());
                    rao.receiver    = receiver;
                    rao.name        = t.act.name;

                    rao.token       = token;
                    rao.fee         = fee;
                    rao.exp_ram     = vt["act"]["data"]["bytes"].as_int64();
                    rao.act_ram     = ram_delta;

                    rao.block_num   = chain.pending_block_state()->block_num;
                    rao.block_time  = chain.pending_block_time();
                    rao.trx_id      = t.trx_id;
                });
                ilog("ram action found, ${action}", ("action", o));
            }

            void on_buyram( const action_trace& t ) {
                auto& chain = chain_plug->chain();
                auto& db = chain.db();
                fc::variant vt = chain.to_variant_with_abi(t, abi_serializer_max_time);

                // get RAM balance before this action applied, and update it with the new value
                int64_t net_quant, cpu_quant, ram_delta;
                account_name receiver = account_name(vt["act"]["data"]["receiver"].get_string());
                ram_delta = calc_ram_delta(receiver);

                // calculate the cost
                asset token_sub_fee, fee, token;
                for( const auto& iline : t.inline_traces ) {
                    fc::variant vi = chain.to_variant_with_abi(iline, abi_serializer_max_time);
                    if( vi["act"]["data"]["to"].get_string() == "enu.ram" )
                    {
                        token_sub_fee = asset::from_string(vi["act"]["data"]["quantity"].get_string());
                    }
                    if( vi["act"]["data"]["to"].get_string() == "enu.ramfee" )
                    {
                        fee = asset::from_string(vi["act"]["data"]["quantity"].get_string());
                    }
                }
                token = token_sub_fee + fee;

                // create ram action object
                const auto& o = db.create<ram_action_object>( [&]( auto& rao ) {
                    rao.payer       = account_name(vt["act"]["data"]["payer"].get_string());
                    rao.receiver    = receiver;
                    rao.name        = t.act.name;

                    rao.token       = token;
                    rao.fee         = fee;
                    rao.exp_ram     = ram_delta;
                    rao.act_ram     = ram_delta;

                    rao.block_num   = chain.pending_block_state()->block_num;
                    rao.block_time  = chain.pending_block_time();
                    rao.trx_id      = t.trx_id;
                });
                ilog("ram action found, ${action}", ("action", o));
            }

            void on_sellram( const action_trace& t ) {
                auto& chain = chain_plug->chain();
                auto& db = chain.db();
                fc::variant vt = chain.to_variant_with_abi(t, abi_serializer_max_time);

                int64_t net_quant, cpu_quant, ram_delta;
                account_name account = account_name(vt["act"]["data"]["account"].get_string());
                ram_delta = calc_ram_delta(account);

                // calculate the incoming
                asset token;
                for( const auto& iline : t.inline_traces ) {
                    fc::variant vi = chain.to_variant_with_abi(iline, abi_serializer_max_time);
                    if( vi["act"]["data"]["from"].get_string() == "enu.ram" )
                    {
                        token = asset::from_string(vi["act"]["data"]["quantity"].get_string());
                    }
                }

                // create ram action object
                const auto& o = db.create<ram_action_object>( [&]( auto& rao ) {
                    rao.payer       = account;
                    rao.receiver    = account;
                    rao.name        = t.act.name;

                    rao.token       = token;
                    rao.fee         = asset::from_string("0.0000 ENU");
                    rao.exp_ram     = ram_delta;
                    rao.act_ram     = ram_delta;

                    rao.block_num   = chain.pending_block_state()->block_num;
                    rao.block_time  = chain.pending_block_time();
                    rao.trx_id      = t.trx_id;
                });
                ilog("ram action found, ${action}", ("action", o));
            }

            void on_applied_transaction( const transaction_trace_ptr& trace ) {
                for( const auto& atrace : trace->action_traces ) {
                    if( atrace.act.name == N(buyram) )
                    {
                        on_buyram(atrace);
                    }
                    else if ( atrace.act.name == N(buyrambytes) )
                    {
                        on_buyrambytes(atrace);
                    }
                    else if ( atrace.act.name == N(sellram) )
                    {
                        on_sellram(atrace);
                    }
                }
            }
    };

    ram_plugin::ram_plugin():my(new ram_plugin_impl()){}
    ram_plugin::~ram_plugin(){}

    void ram_plugin::set_program_options(options_description&, options_description& cfg) {
       cfg.add_options()
             ("option-name", bpo::value<string>()->default_value("default value"),
              "Option Description")
             ;
    }

    void ram_plugin::plugin_initialize(const variables_map& options) {
        if(options.count("option-name")) {
          // Handle the option
        }

        my->chain_plug = app().find_plugin<chain_plugin>();
        auto& chain = my->chain_plug->chain();
        my->abi_serializer_max_time = my->chain_plug->get_abi_serializer_max_time();

        chain.db().add_index<ram_action_index>();
        chain.db().add_index<account_ram_index>();

        my->applied_transaction_connection.emplace(chain.applied_transaction.connect( [&]( const transaction_trace_ptr& p ){
            my->on_applied_transaction(p);
        }));

    }

    void ram_plugin::plugin_startup() {
    }

    void ram_plugin::plugin_shutdown() {
      my->applied_transaction_connection.reset();
    }


    namespace ram_apis { 

       asset exchange_state::convert_to_exchange( connector&c, asset in ) {
           real_type R(supply.get_amount());
           real_type C(c.balance.get_amount()+in.get_amount());
           real_type F(c.weight/1000.0);
           real_type T(in.get_amount());
           real_type ONE(1.0);

           real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
           int64_t issued = int64_t(E);

           supply += asset( issued, supply.get_symbol() );
           c.balance += in;

           return asset( issued, supply.get_symbol() );
       }

       asset exchange_state::convert_from_exchange( connector&c, asset in ) {
           real_type R((supply - in).get_amount());
           real_type C(c.balance.get_amount());
           real_type F(1000.0/c.weight);
           real_type E(in.get_amount());
           real_type ONE(1.0);

           real_type T = C * (std::pow( ONE + E/R, F) - ONE);
           int64_t out = int64_t(T);

           supply -= in;
           c.balance -= asset( out, c.balance.get_symbol() );

           return asset( out, c.balance.get_symbol() );
       }

       asset exchange_state::convert( asset from, symbol to ) {
           auto sell_symbol  = from.get_symbol();
           auto ex_symbol    = supply.get_symbol();
           auto base_symbol  = base.balance.get_symbol();
           auto quote_symbol = quote.balance.get_symbol();

           if( sell_symbol != ex_symbol ) {
              if( sell_symbol == base_symbol ) {
                 from = convert_to_exchange( base, from );
              } else if( sell_symbol == quote_symbol ) {
                 from = convert_to_exchange( quote, from );
              } else {
                 FC_ASSERT(false, "invalid sell");
              }
           } else {
              if( to == base_symbol ) {
                 from = convert_from_exchange( base, from );
              } else if( to == quote_symbol ) {
                 from = convert_from_exchange( quote, from );
              } else {
                 FC_ASSERT(false, "invalid conversion");
              }
           }

           if( to != from.get_symbol() )
              return convert( from, to );

           return from;
       }

       read_only::get_actions_result read_only::get_actions( const read_only::get_actions_params& params )const {
         auto& chain = ram->chain_plug->chain();
         const auto& db = chain.db();
         const auto& idx = db.get_index<ram_action_index, by_id>();

         int32_t pos = params.pos ? *params.pos : -1;
         int32_t offset = params.offset ? *params.offset : -20;
         int32_t start, end;
         int32_t size = idx.size();

         if ( pos <= -1 ) {
            pos = size;
         }
         if ( offset > 0 ) {
            start = pos;
            end = start + offset;
            if( end >= size ) end = size;
         } else {
            start = pos + offset;
            if( start < 0 ) start = 0;
            end = pos;
         }
         idump((start)(end));
         FC_ASSERT( end >= start );

         auto start_time = fc::time_point::now();
         auto end_time = start_time;
         get_actions_result result;
         result.last_irreversible_block = chain.last_irreversible_block_num();
         while( start != end ) {
            if ( nullptr == db.find<ram_action_object, by_id>( start ) )
                break;
            const auto& o = db.get<ram_action_object, by_id>( start );
            result.actions.emplace_back( ram_action_result {
                                uint64_t( o.id._id + 1 ), // sequence number starts from 1
                                o.payer, o.receiver, o.name,
                                o.token, o.fee, o.exp_ram, o.act_ram,
                                o.block_num, o.block_time, o.trx_id});

            end_time = fc::time_point::now();
            if( end_time - start_time > fc::microseconds(100000) ) {
               result.time_limit_exceeded_error = true;
               break;
            }
            ++start;
         }
         
         return result;
       }

       exchange_state read_only::get_exchange_state()const {
           auto& chain = ram->chain_plug->chain();
           const auto& db = chain.db();

	       const auto& acnt = chain.get_account(N(enumivo));
           const auto& abi = acnt.get_abi();
           abi_serializer abis;
           abis.set_abi(abi, ram->abi_serializer_max_time);

           const auto* const table_id = db.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(N(enumivo), N(enumivo), N(rammarket)));
           ENU_ASSERT(table_id, chain::contract_table_query_exception, "Missing table rammarket");

           const auto& kv_index = db.get_index<key_value_index, by_scope_primary>();
           const auto it = kv_index.find(boost::make_tuple(table_id->id));
           ENU_ASSERT(it != kv_index.end(), chain::contract_table_query_exception, "Missing row in table rammarket");
           vector<char> data;
           enumivo::chain_apis::read_only::copy_inline_row(*it, data);

           const auto& v = abis.binary_to_variant(abis.get_table_type(N(rammarket)), data, ram->abi_serializer_max_time);
           asset supply, base_balance, quote_balance;
           fc::from_variant(v["supply"], supply);
           fc::from_variant(v["base"]["balance"], base_balance);
           fc::from_variant(v["quote"]["balance"], quote_balance);
           exchange_state es = {
               .supply = supply,
               .base.balance = base_balance,
               .quote.balance = quote_balance
           };
           return es;
       }

       read_only::evaluate_result read_only::evaluate( const read_only::evaluate_params& params )const {
           FC_ASSERT( params.from.get_symbol() == symbol(SY(4,ENU)) || params.from.get_symbol() == symbol(SY(0,RAM)), "illegal symbol");
           auto to_symbol = params.from.get_symbol() == symbol(SY(4,ENU)) ? symbol(SY(0,RAM)) : symbol(SY(4,ENU));

           auto& chain = ram->chain_plug->chain();
           auto es = get_exchange_state();

           asset to, fee;
           if ( params.from.get_symbol() == symbol(SY(4,ENU)) ) { // from ENU to RAM
               fee = asset( ( params.from.get_amount() + 199 ) / 200, params.from.get_symbol());
               auto quant_after_fee = params.from - fee;
               to = es.convert( quant_after_fee, to_symbol );
           } else { // from RAM to ENU
               auto tokens_out = es.convert( params.from, to_symbol );
               fee = asset( ( tokens_out.get_amount() + 199 ) / 200, tokens_out.get_symbol() );
               to = tokens_out - fee;
           }

           evaluate_result result = {
               .to = to,
               .fee = fee,
               .last_irreversible_block = chain.last_irreversible_block_num()
           };
           return result;
       }
    } /// ram_apis

}

FC_REFLECT( enumivo::ram_action_object, (payer)(receiver)(name)(token)(fee)(exp_ram)(act_ram)(block_num)(block_time)(trx_id) )
