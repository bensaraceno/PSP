

/* Exit callback 
int exit_callback(int arg1, int arg2, void *common) {
	while (loaded) { sceKernelDelayThread(10000); }
    sceKernelExitGame();
	return 0;
}*/

/* Power Callback 
int power_callback(int unknown, int pwrflags, void *common) {
    // Check for power switch and suspending as one is manual and the other automatic.
    if (pwrflags & PSP_POWER_CB_POWER_SWITCH || pwrflags & PSP_POWER_CB_SUSPENDING) {
		sprintf(err_buffer, "First arg: 0x%08X, Flags: 0x%08X: Suspending\n", unknown, pwrflags); kill_threads = 1;
		while (loaded) { sceKernelDelayThread(10000); }
    } else if (pwrflags & PSP_POWER_CB_RESUMING) {
		sprintf(err_buffer, "First arg: 0x%08X, Flags: 0x%08X: Resuming from suspend mode\n", unknown, pwrflags);
    } else if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE) {
		sprintf(err_buffer, "First arg: 0x%08X, Flags: 0x%08X: Resume complete\n", unknown, pwrflags);
    } else if (pwrflags & PSP_POWER_CB_STANDBY) {
		sprintf(err_buffer, "First arg: 0x%08X, Flags: 0x%08X: Entering standby mode\n", unknown, pwrflags); kill_threads = 1;
		while (loaded) { sceKernelDelayThread(10000); }
    } else {
		sprintf(err_buffer, "First arg: 0x%08X, Flags: 0x%08X: Unhandled power event\n", unknown, pwrflags); kill_threads = 1;
		while (loaded) { sceKernelDelayThread(10000); }
    }
    sceDisplayWaitVblankStart();
	return 0;
}*/

/* Callback thread 
int CallbackThread(SceSize args, void *argp) {
    int cbid;
    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL); sceKernelRegisterExitCallback(cbid);
    cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL); scePowerRegisterCallback(0, cbid);
    sceKernelSleepThreadCB();
	return 0;
}*/

/* Sets up the callback thread and returns its thread id 
int SetupCallbacks(void) {
    int thid = 0;
    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0)
	sceKernelStartThread(thid, 0, 0);
    return thid;
}*/