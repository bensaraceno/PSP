#include <pspkernel.h>
#include <pspsdk.h>
#include <pspusb.h>
#include <pspusbstor.h>
#include <pspmodulemgr.h>

void start_usb() {
	pspSdkLoadStartModule("flash0:/kd/semawm.prx", PSP_MEMORY_PARTITION_KERNEL);
	pspSdkLoadStartModule("flash0:/kd/usbstor.prx", PSP_MEMORY_PARTITION_KERNEL);
	pspSdkLoadStartModule("flash0:/kd/usbstormgr.prx", PSP_MEMORY_PARTITION_KERNEL);
	pspSdkLoadStartModule("flash0:/kd/usbstorms.prx", PSP_MEMORY_PARTITION_KERNEL);
	pspSdkLoadStartModule("flash0:/kd/usbstorboot.prx", PSP_MEMORY_PARTITION_KERNEL);
	sceUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0);
	sceUsbStart(PSP_USBSTOR_DRIVERNAME, 0, 0);
	sceUsbstorBootSetCapacity(0x800000);
	sceUsbActivate(0x1c8);
}

void stop_usb() {
	sceUsbDeactivate(0);
	sceUsbStop(PSP_USBSTOR_DRIVERNAME, 0, 0);
	sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);
}
