
#include "vmpwr.h"

#include "lua.h"
#include "lauxlib.h"

int battery_get_level(lua_State *L)
{
	int level = vm_pwr_get_battery_level();

	lua_pushnumber(L, level);
	
	return 1;
}

#undef MIN_OPT_LEVEL
#define MIN_OPT_LEVEL 0 // ???? TODO
#include "lrodefs.h"

const LUA_REG_TYPE battery_map[] =
{
	{LSTRKEY("level"), LFUNCVAL(battery_get_level)},
	{LNILKEY, LNILVAL}
};

LUALIB_API int luaopen_battery(lua_State *L)
{
	luaL_register(L, "battery", battery_map);
	return 1;
}
