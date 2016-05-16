
#include <stdio.h>
#include <string.h>

#include "vmtype.h"
#include "vmlog.h"
#include "vmsystem.h"
#include "vmgsm_tel.h"
#include "vmgsm_sim.h"
#include "vmtimer.h"
#include "vmthread.h"

// for hail mary keypad reboot
#include "vmkeypad.h"
#include "vmpwr.h"
#include "vmdcl.h"
#include "vmdcl_kbd.h"


#include "shell.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

extern void retarget_setup();
extern int luaopen_audio(lua_State* L);
extern int luaopen_gsm(lua_State* L);
extern int luaopen_timer(lua_State* L);
extern int luaopen_gpio(lua_State* L);
extern int luaopen_screen(lua_State* L);
extern int luaopen_i2c(lua_State* L);
extern int luaopen_tcp(lua_State* L);
extern int luaopen_https(lua_State* L);
extern int luaopen_bluetooth(lua_State* L);
extern int luaopen_button(lua_State* L);
extern int luaopen_os(lua_State* L);
extern int luaopen_io(lua_State* L);

// vm_log_* -> lua_log_printf
#if 0
extern int retarget_target;
extern VM_DCL_HANDLE retarget_device_handle;
extern VM_DCL_HANDLE retarget_uart1_handle;
void lua_log_printf(int type, const char *file, int line, const char *msg, ...)
{
  if ((retarget_target != retarget_device_handle) && (retarget_target != retarget_uart1_handle)) return;

  va_list ap;
  char *pos, message[256];
  int sz;
  int nMessageLen = 0;

  memset(message, 0, 256);
  pos = message;

  sz = 0;
  va_start(ap, msg);
  nMessageLen = vsnprintf(pos, 256 - sz, msg, ap);
  va_end(ap);

  if( nMessageLen<=0 ) return;

  if (type == 1) printf("\n[FATAL]");
  else if (type == 2) printf("\n[ERROR]");
  else if (type == 3) printf("\n[WARNING]");
  else if (type == 4) printf("\n[INFO]");
  else if (type == 5) printf("\n[DEBUG]");
  if (type > 0) printf(" %s:%d ", file, line);

  printf("%s\n", message);
}
#endif


lua_State* L = NULL;

VM_TIMER_ID_PRECISE sys_timer_id = 0;

static void key_init(void) {
	VM_DCL_HANDLE kbd_handle;
	vm_dcl_kbd_control_pin_t kbdmap;

	kbd_handle = vm_dcl_open(VM_DCL_KBD,0);
	kbdmap.col_map = 0x09;
	kbdmap.row_map = 0x05;
	vm_dcl_control(kbd_handle,VM_DCL_KBD_COMMAND_CONFIG_PIN, (void *)(&kbdmap));

	vm_dcl_close(kbd_handle);
}

static VMINT handle_keypad_event(VM_KEYPAD_EVENT event, VMINT code) {
	if (code == 30) {
		if (event == VM_KEYPAD_EVENT_LONG_PRESS) {
			printf("\nreset button long press, maybe shutdown?\n");
			vm_pwr_shutdown(0);
		} else if (event == VM_KEYPAD_EVENT_DOWN) {
			printf("\nreset button pressed\n");
		} else if (event == VM_KEYPAD_EVENT_UP) {
			printf("\nreset button APP REBOOT\n");
			vm_pwr_reboot();
		}
	}
	return 0;
}


void sys_timer_callback(VM_TIMER_ID_PRECISE sys_timer_id, void* user_data)
{
    vm_log_info("tick");
}

static int msleep_c(lua_State* L)
{
    long ms = lua_tointeger(L, -1);
    vm_thread_sleep(ms);
    return 0;
}

void lua_setup()
{
    VM_THREAD_HANDLE handle;

    L = lua_open();
    lua_gc(L, LUA_GCSTOP, 0); /* stop collector during initialization */
    luaL_openlibs(L);         /* open libraries */

//    luaopen_audio(L);
//    luaopen_gsm(L);
//    luaopen_timer(L);
//    luaopen_gpio(L);
    luaopen_screen(L);
//    luaopen_i2c(L);
//    luaopen_tcp(L);
//    luaopen_https(L);
//    luaopen_bluetooth(L);
//    luaopen_button(L);
    luaopen_os(L);
//    luaopen_io(L);

    lua_register(L, "msleep", msleep_c);

    lua_gc(L, LUA_GCRESTART, 0);

    luaL_dofile(L, "init.lua");

    handle = vm_thread_create(shell_thread, L, 0);
//    vm_thread_change_priority(handle, 245);
}

void handle_sysevt(VMINT message, VMINT param)
{
    switch(message) {
    case VM_EVENT_CREATE:
        // sys_timer_id = vm_timer_create_precise(1000, sys_timer_callback, NULL);
        lua_setup();
        break;
    case SHELL_MESSAGE_ID:
        shell_docall(L);
        break;
    case VM_EVENT_QUIT:
        break;
    }
}

/* Entry point */
void vm_main(void)
{

    retarget_setup();
    fputs("hello, linkit assist\n", stdout);

    /* register reset key reboot handler */
    key_init();
    vm_keypad_register_event_callback(handle_keypad_event);

    
    /* register system events handler */
    vm_pmng_register_system_event_callback(handle_sysevt);
}
