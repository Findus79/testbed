#include <string.h>

#include <3ds.h>
#include "device.h"


int main()
{
	using namespace D3DS;

	auto device = D3DSDevice::CreateDevice(
		GSP_BGR8_OES,
		GSP_BGR8_OES,
		false,
		false
		);


	// Main loop
	while (aptMainLoop())
	{
		device.WaitForVBlank();
		hidScanInput();

		// Your code goes here
		

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Example rendering code that displays a white pixel
		// Please note that the 3DS screens are sideways (thus 240x400 and 240x320)
		//u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		//memset(fb, 0xFF, 240*400*3);
		
		//fb = gfxGetFramebuffer( GFX_TOP, GFX_RIGHT, NULL, NULL );
		//memset(fb, 0x00, 240*400*3 );

		// Flush and swap framebuffers
		device.FlushBuffers();
		device.SwapBuffers( true );
	}

	//gfxExit();
	return 0;
}
