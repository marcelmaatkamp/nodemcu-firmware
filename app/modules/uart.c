// Module for interfacing with serial

//#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"

#include "c_types.h"
#include "c_string.h"

static lua_State *gL = NULL;
static int uart_receive_rf = LUA_NOREF;
static bool run_input = true;
bool ICACHE_FLASH_ATTR uart_on_data_cb(const char *buf, size_t len){
  if(!buf || len==0)
    return false;
  if(uart_receive_rf == LUA_NOREF)
    return false;
  if(!gL)
    return false;
  lua_rawgeti(gL, LUA_REGISTRYINDEX, uart_receive_rf);
  lua_pushlstring(gL, buf, len);
  lua_call(gL, 1, 0);
  return !run_input;
}

// Lua: uart.on("method", function, [run_input])
static int ICACHE_FLASH_ATTR uart_on( lua_State* L )
{
  size_t sl;
  int32_t run = 1;
  const char *method = luaL_checklstring( L, 1, &sl );
  if (method == NULL)
    return luaL_error( L, "wrong arg type" );

  // luaL_checkanyfunction(L, 2);
  if (lua_type(L, 2) == LUA_TFUNCTION || lua_type(L, 2) == LUA_TLIGHTFUNCTION){
    if ( lua_isnumber(L, 3) ){
      run = lua_tointeger(L, 3);
    }
    lua_pushvalue(L, 2);  // copy argument (func) to the top of stack
  } else {
    lua_pushnil(L);
  }
  if(sl == 4 && c_strcmp(method, "data") == 0){
    run_input = true;
    if(uart_receive_rf != LUA_NOREF){
      luaL_unref(L, LUA_REGISTRYINDEX, uart_receive_rf);
      uart_receive_rf = LUA_NOREF;
    }
    if(!lua_isnil(L, -1)){
      uart_receive_rf = luaL_ref(L, LUA_REGISTRYINDEX);
      gL = L;
      if(run==0)
        run_input = false;
    } else {
      lua_pop(L, 1);
    }
  }else{
    lua_pop(L, 1);
    return luaL_error( L, "method not supported" );
  }
  return 0; 
}

bool uart0_echo = true;
// Lua: actualbaud = setup( id, baud, databits, parity, stopbits, echo )
static int uart_setup( lua_State* L )
{
  unsigned id, databits, parity, stopbits, echo = 1;
  u32 baud, res;
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( uart, id );
  baud = luaL_checkinteger( L, 2 );
  databits = luaL_checkinteger( L, 3 );
  parity = luaL_checkinteger( L, 4 );
  stopbits = luaL_checkinteger( L, 5 );
  if(lua_isnumber(L,6)){
    echo = lua_tointeger(L,6);
    if(echo!=0)
      uart0_echo = true;
    else
      uart0_echo = false;
  }

  res = platform_uart_setup( id, baud, databits, parity, stopbits );
  lua_pushinteger( L, res );
  return 1;
}

// Lua: write( id, string1, [string2], ..., [stringn] )
static int uart_write( lua_State* L )
{
  int id;
  const char* buf;
  size_t len, i;
  int total = lua_gettop( L ), s;
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( uart, id );
  for( s = 2; s <= total; s ++ )
  {
    if( lua_type( L, s ) == LUA_TNUMBER )
    {
      len = lua_tointeger( L, s );
      if( len > 255 )
        return luaL_error( L, "invalid number" );
      platform_uart_send( id, ( u8 )len );
    }
    else
    {
      luaL_checktype( L, s, LUA_TSTRING );
      buf = lua_tolstring( L, s, &len );
      for( i = 0; i < len; i ++ )
        platform_uart_send( id, buf[ i ] );
    }
  }
  return 0;
}

// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE uart_map[] = 
{
  { LSTRKEY( "setup" ),  LFUNCVAL( uart_setup ) },
  { LSTRKEY( "write" ), LFUNCVAL( uart_write ) },
  { LSTRKEY( "on" ), LFUNCVAL( uart_on ) },
#if LUA_OPTIMIZE_MEMORY > 0

#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int ICACHE_FLASH_ATTR luaopen_uart( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_UART, uart_map );
  // Add constants

  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}
