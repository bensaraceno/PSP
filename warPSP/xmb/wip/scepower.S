	.set noreorder 

#include "pspstub.s" 

	STUB_START "scePower", 0x40010000, 0x20005 
	STUB_FUNC 0x0442D852, scePower_0442D852
	STUB_FUNC 0xDB62C9CF, scePowerCancelRequest
	STUB_FUNC 0x7FA406DD, scePowerIsRequest
	STUB_FUNC 0x04B7766E, scePowerRegisterCallback 
	STUB_END
