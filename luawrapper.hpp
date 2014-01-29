/*******************************************************************************
  url/git: http://fpaboim.github.com/luawrapper
  Distributed under the MIT License.

  Header only wrapper library for lua, singleton class manages lua states and
  C API calls through API stack manipulation and utility functions. Use is very
  similar to regualar stack manipulation, e.g. to call io.open lua function, as
  implemented here, which takes two arguments: filename and opening mode
  (r-read, w-write, etc.), one should first use getGlobal to get function name
  from the lua global namespace and then push the arguments to the function. To
  call the function use "callFunction(number_of_arguments, number_of_retvalues".
  Return values should then be popped from stack.
*******************************************************************************/
#ifndef LUAWRAPPER_HPP
#define LUAWRAPPER_HPP

// includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lua.hpp"

// macro to call the singleton LuaWrapper
#define luaWrap LuaWrapper::instance()

extern int luaopen_commands (lua_State* tolua_S);

class LuaWrapper {

public:
  // @brief accesses the singleton object.
  static LuaWrapper& instance();


protected:

  // @brief default constructor.
  LuaWrapper();

  // @brief destructor.
  virtual ~LuaWrapper();


public:
  // Returns lua state, CAREFUL NOT TO MANAGE WHAT SHOULD BE MANAGED BY THE
  // LUAWRAPPER LIBRARY!
  lua_State* getLuaState();

  // wraps lua API for current lua state
  //////////////////////////////////////////////////////////////////////////////
  void getGlobal( const char* name );
  void setGlobal( const char* name );
  int  doFile( const char* filename );
  int  callFunction( int nargs, int nresults );
  void registerFunc( char* funcname, void (*f)(void) );
  int  doesFuncExist(char* luafuncname);

  // stack manipulation functions
  //////////////////////////////////////////////////////////////////////////////
  void        pushNumber( double n );
  void        pushInt( int n );
  void        pushString( const char* s );
  void        pushNil();
  void        pushLUserdata( void* p );
  void        pop(){ lua_pop( m_luastate, -1 );}
  int         popInt();
  double      popNumber();
  const char* popString();
  void*       popUserdata(); // Warning: Careful to use type casting correctly!
  void        moveToTop(int index);
  int         isNil( int index ); // query if index value is nil
  void        createTable(); // Creates table and places on top os stack
  // While setting/getting table values table must be at the top of the stack.
  void        pushTableValue(char* key); // Pushes key contents to stack
  void        pushTableValue(int index); // Pushes index contents to stack
  void        setTable(); // to use setTable first push key and value to stack
  int         pop2Ref();
  void        pushRef(int refval);

  void        stackDump();   // Dumps CtoLua stack information for debugging

  FILE* LuaWrapperOpenFile ( char* fname, char* stats );
  void  LuaWrapperCloseFile( FILE* fp );

  enum luaType {
    TNIL       = LUA_TNIL,
    TBOOL      = LUA_TBOOLEAN,
    TLUSERDATA = LUA_TLIGHTUSERDATA,
    TNUMBER    = LUA_TNUMBER,
    TSTRING    = LUA_TSTRING,
    TTABLE     = LUA_TTABLE,
    TFUNCTION  = LUA_TFUNCTION,
    TUSERDATA  = LUA_TUSERDATA,
    TTHREAD    = LUA_TTHREAD
  };

  struct luaObj {
    void* value;
    luaType type;
  };

private:
  // pointer to the SINGLETON object of this class
  static LuaWrapper* m_LuaWrapper;
  // Lua Variables
  lua_State*       m_luastate;
  char*            m_status;
};

////////////////////////////////////////////////////////////////////////////////

// Constructor - (bad) Constructor does work to have a global initialization of
// the lua state by the singleton object
inline LuaWrapper::LuaWrapper()
: m_luastate(NULL), // initialize lua state as null
  m_status(NULL)    // initialize status as null
{
  m_luastate = luaL_newstate();   /* opens Lua */
  luaL_openlibs(m_luastate);      /* auxiliary Lua libs. */
  luaopen_commands(m_luastate);
}

// Destructor - finalizes lua state and kills singleton object
inline LuaWrapper::~LuaWrapper() {
  if(m_luastate)
    lua_close(m_luastate);
  delete m_LuaWrapper;
}

// Singleton instance access method
////////////////////////////////////////////////////////////////////////////////
inline LuaWrapper& LuaWrapper::instance() {
  return m_LuaWrapper ? *m_LuaWrapper : *(m_LuaWrapper = new LuaWrapper());
}

// getLuaState: Return lua state, not to be used to handle lua state itself,
// which should be done internally by THIS library!
////////////////////////////////////////////////////////////////////////////////
inline lua_State* LuaWrapper::getLuaState() {
  return m_luastate;
}


////////////////////////////////////////////////////////////////////////////////
// Stack Manipulation Functions
////////////////////////////////////////////////////////////////////////////////

// pushNumber: pushes a number to lua stack
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::pushNumber(double n) {
  lua_pushnumber(m_luastate, n);
}

// pushInt: pushes an integer to lua stack
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::pushInt(int n) {
  lua_pushinteger(m_luastate, n);
}

// pushString: pushes a c format string to lua stack
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::pushString(const char* s) {
  lua_pushstring(m_luastate, s);
}

// pushNil: pushes nil (lua NULL type)
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::pushNil() {
  lua_pushnil(m_luastate);
}

// pushUserdata: Pushed a generic (light/no GC) userdata type to stack
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::pushLUserdata(void* p) {
  lua_pushlightuserdata(m_luastate, p);
}

// popInt: Pops an integer from the stack
////////////////////////////////////////////////////////////////////////////////
inline int LuaWrapper::popInt() {
  if (!lua_isnumber(m_luastate, -1))
  {
    luaL_error(m_luastate,
               "ERROR: C-Lua stack value type mismatch (should be a number)!");
  }
  int n = lua_tointeger(m_luastate, -1);
  lua_pop(m_luastate, 1);

  return n;
}

// popNumber: Pops a double from the stack
////////////////////////////////////////////////////////////////////////////////
inline double LuaWrapper::popNumber() {
  if (!lua_isnumber(m_luastate, -1)) {
    luaL_error(m_luastate,
               "ERROR: C-Lua stack value type mismatch (should be a number)!");
  }

  double num = lua_tonumber (m_luastate, -1);
  lua_pop(m_luastate, 1);

  return num;
}

// popString: Pops a string from the stack
////////////////////////////////////////////////////////////////////////////////
inline const char*
LuaWrapper::popString() {
  if (!lua_isstring(m_luastate, -1)) {
    luaL_error(m_luastate,
               "ERROR: C-Lua stack value type mismatch (should be a string)!");
  }

  const char* string = lua_tostring(m_luastate, -1);
  lua_pop(m_luastate, 1);

  return string;
}

// popUserdata: Pops userdata generic lua type from the stack, extra care must
// by taken when type casting
////////////////////////////////////////////////////////////////////////////////
inline void*
LuaWrapper::popUserdata() {
  if (!lua_isuserdata(m_luastate, -1)) {
    luaL_error(m_luastate,
               "ERROR: C-Lua stack value type mismatch (should be userdata)!");
  }

  void* vp = lua_touserdata(m_luastate, -1);
  lua_pop(m_luastate, 1);

  return vp;
}

// moveToTop: Moves stack index to top of stack (-1 index)
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::moveToTop(int index) {
  lua_settop(m_luastate, index);
}

// isNil: Queries stack at "index" if entry is nil
////////////////////////////////////////////////////////////////////////////////
inline int LuaWrapper::isNil( int index ) {
  int ret = lua_isnil( m_luastate, index );
  return ret;
}

// createTable: Creates a lua table and places it on top of stack
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::createTable() {
  lua_newtable(m_luastate);
}

// pushTableValue: Gets field from table at top of stack and pushes value to
// top of the stack
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::pushTableValue(int index) {
  if (!lua_istable(m_luastate, -1))
    luaL_error(m_luastate,
      "ERROR: Trying to get table value without table at top of stack!");
  lua_pushinteger(m_luastate, index);
  lua_gettable(m_luastate, -2);
}

// pushTableValue: Gets field from table at top of stack and pushes value to
// top of the stack
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::pushTableValue(char* key) {
  if (!lua_istable(m_luastate, -1))
    luaL_error(m_luastate,
      "ERROR: Trying to get table value without table at top of stack!");
  lua_pushstring(m_luastate, key);
  lua_gettable(m_luastate, -2);
}

// setTable: Same functioning as lua api. with table on top of stack first push
// key then value, and finally call setTable to set lua table values.
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::setTable() {
  if (!lua_istable(m_luastate, -3))
    luaL_error(m_luastate,
      "ERROR: Trying to set table without pushing key and value to stack!");
  lua_settable(m_luastate, -3);
}

////////////////////////////////////////////////////////////////////////////////
// C-Lua API Functions
////////////////////////////////////////////////////////////////////////////////

// getGlobal: gets global namespace lua variable with stack. Used to access lua
// variables from C
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::getGlobal(const char* name) {
  lua_getglobal(m_luastate, name);
}

// setglobal: sets global namespace lua variable with stack. Used to set lua
// variables from C
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::setGlobal(const char* name) {
  lua_setglobal(m_luastate, name);
}

// dofile: executes lua file, returns 0 if there are no errors or 1 in case of
// errors.
////////////////////////////////////////////////////////////////////////////////
inline int LuaWrapper::doFile(const char* filename) {
  int ret = luaL_dofile(m_luastate, filename);
  if ( ret == 1 ) {
    printf("Error running LuaWrapper::dofile doing file: %s\nerror: %s\n",
           filename, lua_tostring(m_luastate, -1));
  }
  return ret;
}

// callFunction: with lua functions name and arguments on stack(!), executes
// the lua functions (in doubt see header description in the beginning)
////////////////////////////////////////////////////////////////////////////////
inline int LuaWrapper::callFunction( int nargs, int nresults ) {
  if (lua_pcall(m_luastate, nargs, nresults, 0) != 0) {
    printf( "Error running function %s: %s",
            lua_tostring(m_luastate, -(nargs+1)),
            lua_tostring(m_luastate, -1) );
    return 0;
  } else {
    return 1;
  }
}

// registerFunc: Registers C function in lua namespace with name: funcname
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::registerFunc( char* funcname, void (*f)(void)) {
  lua_register(m_luastate, funcname, (lua_CFunction)f);
}

// doesFuncExist: return true if function exists and false otherwise
////////////////////////////////////////////////////////////////////////////////
inline int LuaWrapper::doesFuncExist(char* luafuncname) {
  getGlobal(luafuncname);
  int funcexists = lua_isfunction(m_luastate, -1);
  pop();
  if (funcexists) {
    return true;
  } else {
    return false;
  }
}

// makeRef: creates a reference to lua stack variables, useful for passing lua
// information without manipulating the stack from other function entry points
////////////////////////////////////////////////////////////////////////////////
inline int LuaWrapper::pop2Ref() {
  return luaL_ref(m_luastate, LUA_REGISTRYINDEX);
}

// makeRef: pushes saved reference back to stack and returns 1, failing rets 0
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::pushRef(int refval) {
  lua_rawgeti(m_luastate, LUA_REGISTRYINDEX, refval);
  luaL_unref(m_luastate, LUA_REGISTRYINDEX, refval);
}

// stackDump: Dumps contents of stack for debugging purposes
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::stackDump() {
  int n = lua_gettop(m_luastate);
  printf("Number of Elements on Stack: %i\n", n);
  for( int i=1; i<=n; i++ )
  {
    printf("Stack[%i]: %s\n", i, lua_tostring(m_luastate, -(i)));
  }
}

// LuaWrapperOpenFile: Opens file using lua io.open standard library
////////////////////////////////////////////////////////////////////////////////
inline FILE* LuaWrapper::LuaWrapperOpenFile( char* fname, char* s ) {
  if ( !strcmp(s, "w") || !strcmp(s, "wb") || !strcmp(s, "w+") ) {
    m_status = _strdup ( "_OUTPUT" );
  } else if ( !strcmp(s, "a") || !strcmp(s, "ab") || !strcmp(s, "a+") ) {
    m_status = _strdup ( "_OUTPUT" );
  } else if ( !strcmp(s, "r") || !strcmp(s, "rb") || !strcmp(s, "r+") ) {
    m_status  = _strdup ( "_INPUT" );
  } else {
    return NULL;
  }

  getGlobal("io.open");
  pushString(fname);
  pushString(s);
  callFunction(2, 1);

  FILE* fp = (FILE*)popUserdata();

  return fp;
}

// LuaWrapperCloseFile: Closes file using the lua io.close standard library
////////////////////////////////////////////////////////////////////////////////
inline void LuaWrapper::LuaWrapperCloseFile( FILE* fp ) {
  // closes default output file created by io.open
  getGlobal("io.close");
  callFunction(0, 0);
}

#endif // LUAWRAPPER_HPP header guard
