#pragma once


namespace D3DS {


    class CommandBuffer
    {
    
    public:
        
        CommandBuffer();
        ~CommandBuffer();

        void    SetViewport         ();

        void    RenderPassBegin     ();
        void    RenderPassEnd       ();

        void    Clear               ();

        void    SetPipelineState    ();

        void    BindVertexBuffer    ();
        void    BindIndexBuffer     ();

        void    DrawArrays          ();
        void    DrawElements        ();
        
    };

} // namespace D3DS