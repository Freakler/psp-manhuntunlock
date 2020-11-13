#include <pspsdk.h>
#include <pspkernel.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <systemctrl.h>

PSP_MODULE_INFO("ManhuntUnlock", 0, 1, 0);

STMOD_HANDLER previous;


int OnModuleStart(SceModule2 *mod) {
	char *modname = mod->modname;
	u32 text_addr = mod->text_addr;
	
	if (strcmp(modname, "MAN2") == 0) {
			
		u32 i;
		for (i = 0; i < mod->text_size; i += 4) {
			u32 addr = text_addr + i;

			///nop out crashing function
			if( _lw(addr + 0x4) == 0x02402025 && _lw(addr - 0x200) == 0x00000000 && _lw(addr + 0x2E0) == 0x34040002  ) {  //0x1D0548 / 0x89D4548
				_sw(0x00000000, addr);
			}
			
		}

		sceKernelDcacheWritebackAll();
	}

	if (!previous)
		return 0;

	return previous(mod);
}

int module_start(SceSize args, void *argp) {
	previous = sctrlHENSetStartModuleHandler(OnModuleStart);
	return 0;
}
