#include	"device.h"


#include    <3ds/allocator/linear.h>
#include    <3ds/allocator/mappable.h>
#include    <3ds/allocator/vram.h>

#include    <3ds/svc.h>

#include    <3ds/gpu/gx.h>




namespace D3DS {

D3DSDevice
D3DSDevice::CreateDevice( 
	GSPGPU_FramebufferFormats FBFormatTop,
	GSPGPU_FramebufferFormats FBFormatBottom,
	bool					  bVRamBuffers,
	bool                      bEnable3D ) 
{

    D3DSDevice device;

    device.m_b3DEnabled     = bEnable3D;
    device.m_FBFormatTop    = FBFormatTop;
    device.m_FBFormatBottom = FBFormatBottom;

    // select allocator
    void* (*allocator)(size_t);

    if (bVRamBuffers)
    {
        allocator = vramAlloc;
        device.dealloc   = vramFree;
    }
    else
    {
        allocator = linearAlloc;
        device.dealloc   = linearFree;
    }

    gspInit();

    device.m_pSharedMemory = (u8*) mappableAlloc( 0x1000 );

    GSPGPU_AcquireRight( 0x0 );

    // create shared memory
    svcCreateEvent( &device.m_hGSPEvent, 0x0 );
    GSPGPU_RegisterInterruptRelayQueue( device.m_hGSPEvent, 0x1, &device.m_hGSPSharedMemory, &device.m_ThreadId );
    svcMapMemoryBlock( device.m_hGSPSharedMemory, 
        (u32)device.m_pSharedMemory, 
        (MemPerm)(MEMPERM_READ|MEMPERM_WRITE), 
        MEMPERM_DONTCARE );

    // allocate framebuffers...
    u32 bytes_top    = 400 * 240 * D3DSDevice::GetBytesPerPixel( FBFormatTop );
    u32 bytes_bottom = 320 * 240 * D3DSDevice::GetBytesPerPixel( FBFormatBottom );

    device.m_FBBottom[0] = (u8*) allocator( bytes_bottom );
    device.m_FBBottom[1] = (u8*) allocator( bytes_bottom );
    device.m_FBTop[ 0 ]  = (u8*) allocator( bytes_top );
    device.m_FBTop[ 1 ]  = (u8*) allocator( bytes_top );
    if (device.m_b3DEnabled)
    {
        device.m_FBTop[ 2 ]  = (u8*) allocator( bytes_top );
        device.m_FBTop[ 3 ]  = (u8*) allocator( bytes_top );
    }

    device.SetFramebufferInfo( D3DS_SCREEN_TOP, 0 );
    device.SetFramebufferInfo( D3DS_SCREEN_BOTTOM, 0 );

    gxCmdBuf = (u32*) (device.m_pSharedMemory + 0x800 + device.m_ThreadId*0x200 );

    gspInitEventHandler( device.m_hGSPEvent, (vu8*)device.m_pSharedMemory, device.m_ThreadId );
    gspWaitForVBlank();


    GSPGPU_SetLcdForceBlack( 0x0 );

	return device;
}

u32
D3DSDevice::GetBytesPerPixel( GSPGPU_FramebufferFormats FBFormat )
{
    switch (FBFormat)
    {
        case GSP_RGBA8_OES:
            return 4;
        case GSP_BGR8_OES:
            return 3;
        case GSP_RGB565_OES:
        case GSP_RGB5_A1_OES:
        case GSP_RGBA4_OES:
            return 2;
        default:
            return 3;
    }
}

void 
D3DSDevice::SetFramebufferInfo( D3DSScreen Screen, u32 frameid )
{
    if (D3DS_SCREEN_TOP==Screen)
    {
        m_FBInfoTop.active_framebuf          = frameid;
        m_FBInfoTop.framebuf0_vaddr          = (u32*) m_FBTop[ frameid ];
        
        if (m_b3DEnabled)
            m_FBInfoTop.framebuf1_vaddr = (u32*) m_FBTop[ 2+frameid ];
        else
            m_FBInfoTop.framebuf1_vaddr = (u32*) m_FBTop[ frameid ];

        m_FBInfoTop.framebuf_widthbytesize   = 240 * GetBytesPerPixel(m_FBFormatBottom);

        u8 enabled3d = m_b3DEnabled;

        m_FBInfoTop.format                   = ((1)<<8) | ((1^enabled3d)<<6) | ((enabled3d)<<5) | m_FBFormatTop;
        m_FBInfoTop.framebuf_dispselect      = frameid;
        m_FBInfoTop.unk                      = 0x00000000;
    }
    else
    {
        m_FBInfoBottom.active_framebuf          = frameid;
        m_FBInfoBottom.framebuf0_vaddr          = (u32*) m_FBBottom[ frameid ];
        m_FBInfoBottom.framebuf1_vaddr          = 0x00000000;
        m_FBInfoBottom.framebuf_widthbytesize   = 240 * GetBytesPerPixel(m_FBFormatBottom);
        m_FBInfoBottom.format                   = m_FBFormatBottom;
        m_FBInfoBottom.framebuf_dispselect      = frameid;
        m_FBInfoBottom.unk                      = 0x00000000;
    }
}

u8*
D3DSDevice::GetFramebufferPtr(
    D3DSFramebuffer Framebuffer,
    u16* pWidth,
    u16* pHeight)
{
    if (nullptr!=pWidth)
        *pWidth = 240;

    if (D3DS_FRAMEBUFFER_BOTTOM==Framebuffer)
    {
        if (nullptr!=pHeight)
            *pHeight = 400;

        return m_FBBottom[ m_FrameID^1 ];
    }
    else
    {
        if (D3DS_FRAMEBUFFER_TOP_LEFT==Framebuffer || !m_b3DEnabled)
            return m_FBTop[ m_FrameID^1 ];
        else
            return m_FBTop[ 2+(m_FrameID^1) ];
    }
}
        
void 
D3DSDevice::SwapBuffers( bool bImmediate )
{
    m_FrameID ^= 1;

    SetFramebufferInfo( D3DS_SCREEN_TOP, m_FrameID );
    SetFramebufferInfo( D3DS_SCREEN_BOTTOM, m_FrameID );

    if (bImmediate)
    {
        GSPGPU_SetBufferSwap( D3DS_SCREEN_TOP, &m_FBInfoTop );
        GSPGPU_SetBufferSwap( D3DS_SCREEN_BOTTOM, &m_FBInfoBottom );
    }
    else
    {
        //WriteFramebufferInfo( D3DS_SCREEN_TOP );
        //WriteFramebufferInfo( D3DS_SCREEN_BOTTOM );
    }
}

void
D3DSDevice::FlushBuffers()
{
    u32 bytes_top = 400 * 240 * GetBytesPerPixel( m_FBFormatTop );
    u32 bytes_btm = 320 * 240 * GetBytesPerPixel( m_FBFormatBottom );

    GSPGPU_FlushDataCache( GetFramebufferPtr(D3DS_FRAMEBUFFER_TOP_LEFT,nullptr,nullptr), bytes_top );
    if (m_b3DEnabled)
        GSPGPU_FlushDataCache( GetFramebufferPtr(D3DS_FRAMEBUFFER_TOP_RIGHT,nullptr,nullptr), bytes_top );

    GSPGPU_FlushDataCache( GetFramebufferPtr(D3DS_FRAMEBUFFER_BOTTOM,nullptr,nullptr), bytes_btm );
}

D3DSDevice::~D3DSDevice()
{
    if (nullptr==dealloc)
        return; // should not happen!

    gspExitEventHandler();

    dealloc( m_FBBottom[0] );
    dealloc( m_FBBottom[1] );
    dealloc( m_FBTop[0] );
    dealloc( m_FBTop[1] );
    if (m_b3DEnabled)
    {
        dealloc( m_FBTop[2] );
        dealloc( m_FBTop[3] );
    }

    svcUnmapMemoryBlock( m_hGSPSharedMemory, 
                         (u32)m_pSharedMemory );

    GSPGPU_UnregisterInterruptRelayQueue();

    svcCloseHandle( m_hGSPSharedMemory );
    if (nullptr!=m_pSharedMemory)
    {
        mappableFree( m_pSharedMemory );
        m_pSharedMemory = nullptr;
    }

    svcCloseHandle( m_hGSPEvent );

    GSPGPU_ReleaseRight();

    gspExit();

    dealloc = nullptr;
}

void
D3DSDevice::WaitForVBlank()
{
    gspWaitForVBlank();
}

} // namespace D3DS