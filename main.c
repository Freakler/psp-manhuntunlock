#include <pspsdk.h>
#include <pspkernel.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <systemctrl.h>

PSP_MODULE_INFO("ManhuntUnlock", 0x1007, 1, 0);

#define EMULATOR_DEVCTL__IS_EMULATOR 0x00000003

static STMOD_HANDLER previous;
//const char *logfile = "ms0:/log.txt";

#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a);

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a);

#define HIJACK_FUNCTION(a, f, ptr) { \
  u32 _func_ = a; \
  static u32 patch_buffer[3]; \
  _sw(_lw(_func_), (u32)patch_buffer); \
  _sw(_lw(_func_ + 4), (u32)patch_buffer + 8);\
  MAKE_JUMP((u32)patch_buffer + 4, _func_ + 8); \
  _sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), _func_); \
  _sw(0, _func_ + 4); \
  ptr = (void *)patch_buffer; \
}

#define REDIRECT_FUNCTION(a, f) { \
  u32 _func_ = a; \
  _sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), _func_); \
  _sw(0, _func_ + 4); \
}

#define MAKE_DUMMY_FUNCTION(a, r) { \
  u32 _func_ = a; \
  if (r == 0) { \
    _sw(0x03E00008, _func_); \
    _sw(0x00001021, _func_ + 4); \
  } else { \
    _sw(0x03E00008, _func_); \
    _sw(0x24020000 | r, _func_ + 4); \
  } \
}


/*int logPrintf(const char *text, ...) {
  va_list list;
  char string[512];

  va_start(list, text);
  vsprintf(string, text, list);
  va_end(list);

  SceUID fd = sceIoOpen(logfile, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
  if (fd >= 0) {
    sceIoWrite(fd, string, strlen(string));
    sceIoWrite(fd, "\n", 1);
    sceIoClose(fd);
  }

  return 0;
}*/


void applyPatch(u32 text_addr, u32 text_size, int emulator) {
	
	//logPrintf("applyPatch..");
	
	/////////////////////////////////////////////////////////
	
	u32 i, addr;
	for(i = 0; i < text_size; i += 4) {
		addr = text_addr + i;
		
		///nop out crashing function
		if( _lw(addr + 0x4) == 0x02402025 && _lw(addr - 0x200) == 0x00000000 && _lw(addr + 0x2E0) == 0x34040002  ) {  //0x1D0548 / 0x89D4548
			//logPrintf("> found @ 0x%08X", addr);
			_sw(0x00000000, addr);
		}
	}
	
	/////////////////////////////////////////////////////////	
	
	sceKernelDcacheWritebackAll();
	sceKernelIcacheClearAll();
  
	//logPrintf("..done!");
}

int OnModuleStart(SceModule2 *mod) {
  //logPrintf("OnModuleStart('%s', 0x%08X)", mod->modname, mod->text_addr);
	
  if (strcmp(mod->modname, "MAN2") == 0) {
    applyPatch(mod->text_addr, mod->text_size, 0);
  }

  if (!previous)
    return 0;

  return previous(mod);
}

static void CheckModules() {
  SceUID modules[10];
  SceKernelModuleInfo info;
  int i, count = 0;

  if (sceKernelGetModuleIdList(modules, sizeof(modules), &count) >= 0) {
    for (i = 0; i < count; ++i) {
      //logPrintf("CheckModules('%s', 0x%08X)", info.name, info.text_addr);
      info.size = sizeof(SceKernelModuleInfo);
      if (sceKernelQueryModuleInfo(modules[i], &info) < 0) {
        continue;
      }
      if (strcmp(info.name, "MAN2") == 0) {
        applyPatch(info.text_addr, info.text_size, 1);
      }
    }
  }
}

int module_start(SceSize args, void *argp) {
	
	//sceIoRemove(logfile);
	//logPrintf("module_start()");
	
	if(sceIoDevctl("kemulator:", EMULATOR_DEVCTL__IS_EMULATOR, NULL, 0, NULL, 0) == 0) {
		CheckModules(); //scan the modules using normal syscalls
	} else {
		previous = sctrlHENSetStartModuleHandler(OnModuleStart);
	}
	
	return 0;
}