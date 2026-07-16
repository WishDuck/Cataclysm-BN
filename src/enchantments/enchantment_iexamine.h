#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "character.h"
#include "generic_readers.h"
#include "mapdata.h"
#include "json.h"

namespace enchantment_iexamine {

std::vector<std::string> enchantment_info(
    const enchant_info& info, Character& crafter, int fold_width);

int enchantment_selector_menu(std::vector<enchant_info> options, Character& user) {

};

class enchant_info_reader : public generic_typed_reader<enchant_info_reader>
{
    enchant_info get_next( JsonIn &jin ) const {
        enchant_info info();
        JsonObject obj = jin.get_object();
        mandatory( obj, false, "name", info.name );
        mandatory( obj, false, "enchant", info.to_enchant_with );
        mandatory( obj, false, "requirements", info.requirement );
        if( obj.has_string( "requirements" ) ) {
            requirements = { requirement_id( obj.get_string( "requirements" ) ) }
        } else if( obj.has_array( "requirements" ) ) {
            for( JsonValue val : obj.get_string_array( "requirements" ) ) {
                if( val.is_string() ) {
                    requirements.push_back( requirement_id( val.get_string() ) );
                } else {
                    JsonObject iobj = val.get_object();
                    requirements.push_back( requirement_id( val.get_string()))
                }
            }
        }
        if( obj.has_array( "skill_levels" ) ) {
            for( JsonValue val : obj.get_array( "skill_levels" ) ) {
                JsonObject iobj = val.get_object();
                info.required_skills[skill_id(iobj.get_string( "skill" ))] = iobj.get_int( "level" );
            }
        }
        
    }
};
namespace {
class requirement_reader : public generic_typed_reader<requirement_reader>
{
    public:
        std::pair<requirement_id, int> get_next( JsonIn &jin ) const {
            JsonObject jo = jin.get_object();
            return std::pair<skill_id, int>( skill_id( jo.get_string( "name" ) ), jo.get_int( "level" ) );
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            const skill_id id = skill_id( jin.get_string() );
            reader_detail::handler<C>().erase_if( container, [&id]( const std::pair<skill_id, int> &e ) {
                return e.first == id;
            } );
        }
};
}
