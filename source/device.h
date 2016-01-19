#pragma once


#include    "commandbuffer.h"
#include	<3ds.h>


namespace D3DS {

    enum D3DSScreen
    {
        D3DS_SCREEN_TOP     = 0,
        D3DS_SCREEN_BOTTOM  = 1
    };

    enum D3DSFramebuffer
    {
        D3DS_FRAMEBUFFER_TOP_LEFT   = 0,
        D3DS_FRAMEBUFFER_TOP_RIGHT  = 1,
        D3DS_FRAMEBUFFER_BOTTOM     = 2
    };

    class D3DSDevice
	{
	public:

		~D3DSDevice();

		static auto CreateDevice(
			GSPGPU_FramebufferFormats FBFormatTop,
			GSPGPU_FramebufferFormats FBFormatBottom,
			bool					  bVRamBuffers,
			bool                      bEnable3D
		) -> D3DSDevice;

        static auto GetBytesPerPixel(
            GSPGPU_FramebufferFormats FBFormat
        ) -> u32;

        auto GetFramebufferPtr(
            D3DSFramebuffer Framebuffer,
            u16* pWidth = nullptr,
            u16* pHeight = nullptr
        ) -> u8*;

        void FlushBuffers   ();
        void SwapBuffers    ( bool bImmediate );
        void WaitForVBlank  ();

        void ExecuteCommandBuffer( CommandBuffer& cmdBuffer );

	private:

		D3DSDevice() {}
		
        void SetFramebufferInfo( D3DSScreen Screen, u32 frameid );

        void WriteFramebufferInfo( D3DSScreen screen );
		
		bool m_b3DEnabled = false;

		GSPGPU_FramebufferFormats	m_FBFormatTop;
		GSPGPU_FramebufferFormats	m_FBFormatBottom;


        u8*     m_pSharedMemory = nullptr;
        Handle  m_hGSPEvent;
        Handle  m_hGSPSharedMemory;
        u8      m_ThreadId;
		
        u8*     m_FBTop[ 4 ]    = { nullptr, nullptr, nullptr, nullptr };
        u8*     m_FBBottom[ 2 ] = { nullptr, nullptr };

        GSPGPU_FramebufferInfo  m_FBInfoTop;
        GSPGPU_FramebufferInfo  m_FBInfoBottom;

        u8      m_FrameID = 0;


        void (*dealloc)(void*) = nullptr;

	}; // class D3DSDevice

} // namespace D3DS
