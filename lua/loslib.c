/*
** $Id: loslib.c,v 1.19.1.3 2008/01/18 16:38:18 roberto Exp $
** Standard Operating System library
** See Copyright Notice in lua.h
*/


#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define loslib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "lrotable.h"

#include "vmlog.h"
#include "vmpwr.h"
#include "vmfs.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include "vmwdt.h"
#include "vmsystem.h"

static int os_pushresult (lua_State *L, int i, const char *filename) {
  int en = errno;  /* calls to Lua API may change this value */
  if (i) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    lua_pushfstring(L, "%s: %s", filename, strerror(en));
    lua_pushinteger(L, en);
    return 3;
  }
}


static int os_execute (lua_State *L) {
  lua_pushinteger(L, system(luaL_optstring(L, 1, NULL)));
  return 1;
}


static int os_remove (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  return os_pushresult(L, remove(filename) == 0, filename);
}


static int os_rename (lua_State *L) {
  const char *fromname = luaL_checkstring(L, 1);
  const char *toname = luaL_checkstring(L, 2);
  return os_pushresult(L, rename(fromname, toname) == 0, fromname);
}


static int os_tmpname (lua_State *L) {
  char buff[LUA_TMPNAMBUFSIZE];
  int err;
  lua_tmpnam(buff, err);
  if (err)
    return luaL_error(L, "unable to generate a unique filename");
  lua_pushstring(L, buff);
  return 1;
}


static int os_getenv (lua_State *L) {
  lua_pushstring(L, getenv(luaL_checkstring(L, 1)));  /* if NULL push nil */
  return 1;
}


static int os_clock (lua_State *L) {
  lua_pushnumber(L, ((lua_Number)clock())/(lua_Number)CLOCKS_PER_SEC);
  return 1;
}


/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield (lua_State *L, const char *key, int value) {
  lua_pushinteger(L, value);
  lua_setfield(L, -2, key);
}

static void setboolfield (lua_State *L, const char *key, int value) {
  if (value < 0)  /* undefined? */
    return;  /* does not set field */
  lua_pushboolean(L, value);
  lua_setfield(L, -2, key);
}

static int getboolfield (lua_State *L, const char *key) {
  int res;
  lua_getfield(L, -1, key);
  res = lua_isnil(L, -1) ? -1 : lua_toboolean(L, -1);
  lua_pop(L, 1);
  return res;
}


static int getfield (lua_State *L, const char *key, int d) {
  int res;
  lua_getfield(L, -1, key);
  if (lua_isnumber(L, -1))
    res = (int)lua_tointeger(L, -1);
  else {
    if (d < 0)
      return luaL_error(L, "field " LUA_QS " missing in date table", key);
    res = d;
  }
  lua_pop(L, 1);
  return res;
}


static int os_date (lua_State *L) {
  const char *s = luaL_optstring(L, 1, "%c");
  time_t t = luaL_opt(L, (time_t)luaL_checknumber, 2, time(NULL));
  struct tm *stm;
  if (*s == '!') {  /* UTC? */
    stm = gmtime(&t);
    s++;  /* skip `!' */
  }
  else
    stm = localtime(&t);
  if (stm == NULL)  /* invalid date? */
    lua_pushnil(L);
  else if (strcmp(s, "*t") == 0) {
    lua_createtable(L, 0, 9);  /* 9 = number of fields */
    setfield(L, "sec", stm->tm_sec);
    setfield(L, "min", stm->tm_min);
    setfield(L, "hour", stm->tm_hour);
    setfield(L, "day", stm->tm_mday);
    setfield(L, "month", stm->tm_mon+1);
    setfield(L, "year", stm->tm_year+1900);
    setfield(L, "wday", stm->tm_wday+1);
    setfield(L, "yday", stm->tm_yday+1);
    setboolfield(L, "isdst", stm->tm_isdst);
  }
  else {
    char cc[3];
    luaL_Buffer b;
    cc[0] = '%'; cc[2] = '\0';
    luaL_buffinit(L, &b);
    for (; *s; s++) {
      if (*s != '%' || *(s + 1) == '\0')  /* no conversion specifier? */
        luaL_addchar(&b, *s);
      else {
        size_t reslen;
        char buff[200];  /* should be big enough for any conversion result */
        cc[1] = *(++s);
        reslen = strftime(buff, sizeof(buff), cc, stm);
        luaL_addlstring(&b, buff, reslen);
      }
    }
    luaL_pushresult(&b);
  }
  return 1;
}


static int os_time (lua_State *L) {
  time_t t;
  if (lua_isnoneornil(L, 1))  /* called without args? */
    t = time(NULL);  /* get current time */
  else {
    struct tm ts;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);  /* make sure table is at the top */
    ts.tm_sec = getfield(L, "sec", 0);
    ts.tm_min = getfield(L, "min", 0);
    ts.tm_hour = getfield(L, "hour", 12);
    ts.tm_mday = getfield(L, "day", -1);
    ts.tm_mon = getfield(L, "month", -1) - 1;
    ts.tm_year = getfield(L, "year", -1) - 1900;
    ts.tm_isdst = getboolfield(L, "isdst");
    t = mktime(&ts);
  }
  if (t == (time_t)(-1))
    lua_pushnil(L);
  else
    lua_pushnumber(L, (lua_Number)t);
  return 1;
}

#if !defined LUA_NUMBER_INTEGRAL
static int os_difftime (lua_State *L) {
  lua_pushnumber(L, difftime((time_t)(luaL_checknumber(L, 1)),
                             (time_t)(luaL_optnumber(L, 2, 0))));
  return 1;
}
#endif

/* }====================================================== */


static int os_setlocale (lua_State *L) {
  static const int cat[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
                      LC_NUMERIC, LC_TIME};
  static const char *const catnames[] = {"all", "collate", "ctype", "monetary",
     "numeric", "time", NULL};
  const char *l = luaL_optstring(L, 1, NULL);
  int op = luaL_checkoption(L, 2, "all", catnames);
  lua_pushstring(L, setlocale(cat[op], l));
  return 1;
}

//==================================
static int os_mkdir (lua_State *L) {
  const char *dirname = luaL_checkstring(L, 1);
  VMWCHAR ucs_dirname[VM_FS_MAX_PATH_LENGTH+1];

  vm_chset_ascii_to_ucs2(ucs_dirname, VM_FS_MAX_PATH_LENGTH, dirname);

  lua_pushinteger(L, vm_fs_create_directory(ucs_dirname));
  return 1;
}

//==================================
static int os_rmdir (lua_State *L) {
  const char *dirname = luaL_checkstring(L, 1);
  VMWCHAR ucs_dirname[VM_FS_MAX_PATH_LENGTH+1];

  vm_chset_ascii_to_ucs2(ucs_dirname, VM_FS_MAX_PATH_LENGTH, dirname);

  lua_pushinteger(L, vm_fs_remove_directory(ucs_dirname));
  return 1;
}

//===================================
static int os_copy (lua_State *L) {
  VMWCHAR ucs_fromname[VM_FS_MAX_PATH_LENGTH+1];
  VMWCHAR ucs_toname[VM_FS_MAX_PATH_LENGTH+1];

  const char *fromname = luaL_checkstring(L, 1);
  const char *toname = luaL_checkstring(L, 2);

  vm_chset_ascii_to_ucs2(ucs_fromname, VM_FS_MAX_PATH_LENGTH, fromname);
  vm_chset_ascii_to_ucs2(ucs_toname, VM_FS_MAX_PATH_LENGTH, toname);

  lua_pushinteger(L, vm_fs_copy(ucs_toname, ucs_fromname, NULL));

  return 1;
}

//===================================
static int os_listfiles(lua_State* L)
{
    VMCHAR filename[VM_FS_MAX_PATH_LENGTH] = {0};
    VMWCHAR wfilename[VM_FS_MAX_PATH_LENGTH] = {0};
    VMWCHAR path[VM_FS_MAX_PATH_LENGTH] = {0};
    VMWCHAR fullname[VM_FS_MAX_PATH_LENGTH] = {0};
    VM_FS_HANDLE filehandle = -1;
    VM_FS_HANDLE filehandleex = -1;
    vm_fs_info_t fileinfo;
    vm_fs_info_ex_t fileinfoex;
    VMINT ret = 0;
    struct tm filetime;

    const char *fspec = luaL_optstring(L, 1, "*");

    sprintf(filename, "%c:\\%s", vm_fs_get_internal_drive_letter(), fspec);
    vm_chset_ascii_to_ucs2(wfilename, sizeof(wfilename), filename);
    vm_wstr_get_path(wfilename, path);

    // get the max file name length
    int len = 0;
    filehandle = vm_fs_find_first(wfilename, &fileinfo);
    if (filehandle >= 0) {
        do {
        	vm_chset_ucs2_to_ascii(filename, VM_FS_MAX_PATH_LENGTH, fileinfo.filename);
        	int flen = strlen(filename);
        	if (flen > len) len = flen;
            ret = vm_fs_find_next(filehandle, &fileinfo);
        } while (0 == ret);
        vm_fs_find_close(filehandle);
    }
    int total = 0;
    int nfiles = 0;
    filehandle = vm_fs_find_first(wfilename, &fileinfo);
    if (filehandle >= 0) {
    	vm_chset_ucs2_to_ascii(filename, VM_FS_MAX_PATH_LENGTH, path);
    	printf("%s\n", filename);
        do {
        	vm_wstr_copy(fullname, path);
        	vm_wstr_concatenate(fullname,(VMCWSTR)(&fileinfo.filename));
            filehandleex = vm_fs_find_first_ex(fullname, &fileinfoex);
        	vm_chset_ucs2_to_ascii(filename, VM_FS_MAX_PATH_LENGTH, fileinfoex.full_filename);
        	if (fileinfoex.attributes & VM_FS_ATTRIBUTE_DIRECTORY) {
            	printf(" %*s %8s\n", len, filename, "<DIR>");
        	}
        	else {
				filetime.tm_hour = fileinfoex.modify_datetime.hour;
				filetime.tm_min = fileinfoex.modify_datetime.minute;
				filetime.tm_sec = fileinfoex.modify_datetime.second;
				filetime.tm_mday = fileinfoex.modify_datetime.day;
				filetime.tm_mon = fileinfoex.modify_datetime.month-1;
				filetime.tm_year = fileinfoex.modify_datetime.year-1900;
				char ftime[20] = {0};
				strftime(ftime, 18, "%D %T", &filetime);
				printf(" %*s %8u %s\n", len, filename, fileinfoex.file_size, ftime);
				total += fileinfoex.file_size;
				nfiles++;
        	}
            vm_fs_find_close_ex(filehandleex);

            /* find the next file */
            ret = vm_fs_find_next(filehandle, &fileinfo);
        } while (0 == ret);
        vm_fs_find_close(filehandle);
        printf("\nTotal %d byte(s) in %d file(s)\n", total, nfiles);
        printf("Drive free: %d,  App data free: %d\n", vm_fs_get_disk_free_space(vm_fs_get_internal_drive_letter()), vm_fs_app_data_get_free_space());
    }
    else {
        printf("No files found.\n");
    }

    return 0;
}

static int os_exit (lua_State *L) {
  exit(luaL_optint(L, 1, EXIT_SUCCESS));
}
static uint32_t os_getaddr(lua_State *L, int index) 
{
	if (lua_isuserdata(L,index)) {
		return (uint32_t)lua_touserdata(L,index);
	} else {
		// TODO handle errors like LONG_MAX, LONG_MIN and check errno.
		return (uint32_t)strtoll(lua_tostring(L,index),NULL,0);
	}
}
static int os_jmp (lua_State *L) {
	uint32_t addr = os_getaddr(L,1);
	((void(*)(void))addr)();
}

static int os_peek (lua_State *L) 
{
	uint32_t addr = os_getaddr(L,1);
	uint32_t val = *((volatile uint32_t *)addr);

//	vm_log_debug("peek(%#08x)=%#08x\n", addr, val);

	lua_pushnumber(L, val);
	return 1;
}

static int os_poke (lua_State *L) 
{
	uint32_t addr = os_getaddr(L,1);
	uint32_t val = luaL_checklong(L,2);
//	vm_log_debug("poke(%#08x,%#08x)\n", addr, val);
	*((volatile uint32_t *)addr) = val;
	return 0;
}

static int os_symbol(lua_State *L)
{
	// CRAIG TODO take as a parameter a function name
	// then somehow we search for it... maybe with objdump-ish on percommon.a
	// or also with the header files on flash disk.
	

	printf("luaopen_os, vm_pwr_reboot=%p, vm_pwr_shutdown=%p\n", vm_pwr_reboot, vm_pwr_shutdown);
	lua_pushnumber(L,(uint32_t)vm_pwr_reboot);
	lua_pushnumber(L,(uint32_t)vm_pwr_shutdown);

	return 2;
}
// CRAIG TODO make sure on crash (vector table) to properly reboot
// so rephone goes back to mass storage mode!

#define MIN_OPT_LEVEL 1
#include "lrodefs.h"
const LUA_REG_TYPE syslib[] = {
  {LSTRKEY("clock"),     LFUNCVAL(os_clock)},
  {LSTRKEY("date"),      LFUNCVAL(os_date)},
#if !defined LUA_NUMBER_INTEGRAL
  {LSTRKEY("difftime"),  LFUNCVAL(os_difftime)},
#endif
  {LSTRKEY("execute"),   LFUNCVAL(os_execute)},
  {LSTRKEY("exit"),      LFUNCVAL(os_exit)},
  {LSTRKEY("getenv"),    LFUNCVAL(os_getenv)},
  {LSTRKEY("remove"),    LFUNCVAL(os_remove)},
  {LSTRKEY("rename"),    LFUNCVAL(os_rename)},
  {LSTRKEY("setlocale"), LFUNCVAL(os_setlocale)},
  {LSTRKEY("time"),      LFUNCVAL(os_time)},
  {LSTRKEY("tmpname"),   LFUNCVAL(os_tmpname)},
  {LSTRKEY("jmp"), LFUNCVAL(os_jmp)},
  {LSTRKEY("poke"), LFUNCVAL(os_poke)},
  {LSTRKEY("peek"), LFUNCVAL(os_peek)},
  {LSTRKEY("symbol"), LFUNCVAL(os_symbol)},

  {LSTRKEY("remove"),   	LFUNCVAL(os_remove)},
  {LSTRKEY("rename"),   	LFUNCVAL(os_rename)},
  {LSTRKEY("copy"),     	LFUNCVAL(os_copy)},
  {LSTRKEY("mkdir"),    	LFUNCVAL(os_mkdir)},
  {LSTRKEY("rmdir"),    	LFUNCVAL(os_rmdir)},
  {LSTRKEY("setlocale"),	LFUNCVAL(os_setlocale)},
  {LSTRKEY("time"),     	LFUNCVAL(os_time)},
  {LSTRKEY("list"),     	LFUNCVAL(os_listfiles)},
  {LNILKEY, LNILVAL}
};

/* }====================================================== */



LUALIB_API int luaopen_os (lua_State *L) {

  LREGISTER(L, LUA_OSLIBNAME, syslib);
}
