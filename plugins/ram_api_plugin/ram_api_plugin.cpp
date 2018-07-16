/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */
#include <enumivo/ram_api_plugin/ram_api_plugin.hpp>
#include <enumivo/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace enumivo {

using namespace enumivo;

static appbase::abstract_plugin& _ram_api_plugin = app().register_plugin<ram_api_plugin>();

ram_api_plugin::ram_api_plugin(){}
ram_api_plugin::~ram_api_plugin(){}

void ram_api_plugin::set_program_options(options_description&, options_description&) {}
void ram_api_plugin::plugin_initialize(const variables_map&) {}

#define CALL(api_name, api_handle, api_namespace, call_name) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(200, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define CHAIN_RO_CALL(call_name) CALL(ram, ro_api, ram_apis::read_only, call_name)

void ram_api_plugin::plugin_startup() {
   ilog( "starting ram_api_plugin" );
   auto ro_api = app().get_plugin<ram_plugin>().get_read_only_api();

   app().get_plugin<http_plugin>().add_api({
      CHAIN_RO_CALL(get_actions)
   });
}

void ram_api_plugin::plugin_shutdown() {}

}
