#define CINTERFACE
#include <d3d11_1.h>
#include <assert.h>
#include <stdio.h>
#include "fm5_performance_context.h"
static void *expected; static ULONG refs=1; static unsigned calls;
static HRESULT nq(void*self,const GUID*g,void**out){(void)self;(void)g;*out=NULL;return E_NOINTERFACE;}
static ULONG nr(void*self){assert(self==expected&&refs);return --refs;}
static HRESULT WINAPI desc(void*r,void*d){(void)r;ZeroMemory(d,48);((UINT*)d)[5]=2;return S_OK;}
static HRESULT t_4(void *self, const GUID * const guid, UINT *pDataSize, void *pData){assert(self==expected);assert(guid==(const GUID * const)(ULONG_PTR)0x100400); assert(pDataSize==(UINT *)(ULONG_PTR)0x100410); assert(pData==(void *)(ULONG_PTR)0x100420);++calls;return (HRESULT)37;}
static HRESULT t_5(void *self, const GUID * const guid, UINT DataSize, const void *pData){assert(self==expected);assert(guid==(const GUID * const)(ULONG_PTR)0x100500); assert(DataSize==(UINT)27); assert(pData==(const void *)(ULONG_PTR)0x100520);++calls;return (HRESULT)37;}
static HRESULT t_6(void *self, const GUID * const guid, const IUnknown *pData){assert(self==expected);assert(guid==(const GUID * const)(ULONG_PTR)0x100600); assert(pData==(const IUnknown *)(ULONG_PTR)0x100610);++calls;return (HRESULT)37;}
static void t_8(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)41); assert(NumBuffers==(UINT)42); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x100820);++calls;}
static void t_9(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)46); assert(NumViews==(UINT)47); assert(ppShaderResourceViews==(ID3D11ShaderResourceView *const *)(ULONG_PTR)0x100920);++calls;}
static void t_10(void *self, ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances){assert(self==expected);assert(pPixelShader==(ID3D11PixelShader *)(ULONG_PTR)0x100a00); assert(ppClassInstances==(ID3D11ClassInstance *const *)(ULONG_PTR)0x100a10); assert(NumClassInstances==(UINT)53);++calls;}
static void t_11(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers){assert(self==expected);assert(StartSlot==(UINT)56); assert(NumSamplers==(UINT)57); assert(ppSamplers==(ID3D11SamplerState *const *)(ULONG_PTR)0x100b20);++calls;}
static void t_12(void *self, ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances){assert(self==expected);assert(pVertexShader==(ID3D11VertexShader *)(ULONG_PTR)0x100c00); assert(ppClassInstances==(ID3D11ClassInstance *const *)(ULONG_PTR)0x100c10); assert(NumClassInstances==(UINT)63);++calls;}
static void t_14(void *self, UINT VertexCount, UINT StartVertexLocation){assert(self==expected);assert(VertexCount==(UINT)71); assert(StartVertexLocation==(UINT)72);++calls;}
static HRESULT t_15(void *self, ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource){assert(self==expected);assert(pResource==(ID3D11Resource *)(ULONG_PTR)0x100f00); assert(Subresource==(UINT)77); assert(MapType==(D3D11_MAP)78); assert(MapFlags==(UINT)79); assert(pMappedResource==(D3D11_MAPPED_SUBRESOURCE *)(ULONG_PTR)0x100f40);++calls;return (HRESULT)37;}
static void t_16(void *self, ID3D11Resource *pResource, UINT Subresource){assert(self==expected);assert(pResource==(ID3D11Resource *)(ULONG_PTR)0x101000); assert(Subresource==(UINT)82);++calls;}
static void t_17(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)86); assert(NumBuffers==(UINT)87); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x101120);++calls;}
static void t_18(void *self, ID3D11InputLayout *pInputLayout){assert(self==expected);assert(pInputLayout==(ID3D11InputLayout *)(ULONG_PTR)0x101200);++calls;}
static void t_19(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets){assert(self==expected);assert(StartSlot==(UINT)96); assert(NumBuffers==(UINT)97); assert(ppVertexBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x101320); assert(pStrides==(const UINT *)(ULONG_PTR)0x101330); assert(pOffsets==(const UINT *)(ULONG_PTR)0x101340);++calls;}
static void t_23(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)116); assert(NumBuffers==(UINT)117); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x101720);++calls;}
static void t_24(void *self, ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances){assert(self==expected);assert(pShader==(ID3D11GeometryShader *)(ULONG_PTR)0x101800); assert(ppClassInstances==(ID3D11ClassInstance *const *)(ULONG_PTR)0x101810); assert(NumClassInstances==(UINT)123);++calls;}
static void t_26(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)4); assert(NumViews==(UINT)5); assert(ppShaderResourceViews==(ID3D11ShaderResourceView *const *)(ULONG_PTR)0x101a20);++calls;}
static void t_27(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers){assert(self==expected);assert(StartSlot==(UINT)9); assert(NumSamplers==(UINT)10); assert(ppSamplers==(ID3D11SamplerState *const *)(ULONG_PTR)0x101b20);++calls;}
static void t_28(void *self, ID3D11Asynchronous *pAsync){assert(self==expected);assert(pAsync==(ID3D11Asynchronous *)(ULONG_PTR)0x101c00);++calls;}
static void t_29(void *self, ID3D11Asynchronous *pAsync){assert(self==expected);assert(pAsync==(ID3D11Asynchronous *)(ULONG_PTR)0x101d00);++calls;}
static HRESULT t_30(void *self, ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags){assert(self==expected);assert(pAsync==(ID3D11Asynchronous *)(ULONG_PTR)0x101e00); assert(pData==(void *)(ULONG_PTR)0x101e10); assert(DataSize==(UINT)26); assert(GetDataFlags==(UINT)27);++calls;return (HRESULT)37;}
static void t_31(void *self, ID3D11Predicate *pPredicate, BOOL PredicateValue){assert(self==expected);assert(pPredicate==(ID3D11Predicate *)(ULONG_PTR)0x101f00); assert(PredicateValue==(BOOL)30);++calls;}
static void t_32(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)34); assert(NumViews==(UINT)35); assert(ppShaderResourceViews==(ID3D11ShaderResourceView *const *)(ULONG_PTR)0x102020);++calls;}
static void t_33(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers){assert(self==expected);assert(StartSlot==(UINT)39); assert(NumSamplers==(UINT)40); assert(ppSamplers==(ID3D11SamplerState *const *)(ULONG_PTR)0x102120);++calls;}
static void t_34(void *self, UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView){assert(self==expected);assert(NumViews==(UINT)44); assert(ppRenderTargetViews==(ID3D11RenderTargetView *const *)(ULONG_PTR)0x102210); assert(pDepthStencilView==(ID3D11DepthStencilView *)(ULONG_PTR)0x102220);++calls;}
static void t_35(void *self, UINT NumRTVs, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts){assert(self==expected);assert(NumRTVs==(UINT)49); assert(ppRenderTargetViews==(ID3D11RenderTargetView *const *)(ULONG_PTR)0x102310); assert(pDepthStencilView==(ID3D11DepthStencilView *)(ULONG_PTR)0x102320); assert(UAVStartSlot==(UINT)52); assert(NumUAVs==(UINT)53); assert(ppUnorderedAccessViews==(ID3D11UnorderedAccessView *const *)(ULONG_PTR)0x102350); assert(pUAVInitialCounts==(const UINT *)(ULONG_PTR)0x102360);++calls;}
static void t_36(void *self, ID3D11BlendState *pBlendState, const FLOAT BlendFactor[ 4 ], UINT SampleMask){assert(self==expected);assert(pBlendState==(ID3D11BlendState *)(ULONG_PTR)0x102400); assert(BlendFactor==(const FLOAT *)(ULONG_PTR)0x102410); assert(SampleMask==(UINT)56);++calls;}
static void t_37(void *self, ID3D11DepthStencilState *pDepthStencilState, UINT StencilRef){assert(self==expected);assert(pDepthStencilState==(ID3D11DepthStencilState *)(ULONG_PTR)0x102500); assert(StencilRef==(UINT)60);++calls;}
static void t_38(void *self, UINT NumBuffers, ID3D11Buffer *const *ppSOTargets, const UINT *pOffsets){assert(self==expected);assert(NumBuffers==(UINT)64); assert(ppSOTargets==(ID3D11Buffer *const *)(ULONG_PTR)0x102610); assert(pOffsets==(const UINT *)(ULONG_PTR)0x102620);++calls;}
static void t_39(void *self){assert(self==expected);++calls;}
static void t_40(void *self, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs){assert(self==expected);assert(pBufferForArgs==(ID3D11Buffer *)(ULONG_PTR)0x102800); assert(AlignedByteOffsetForArgs==(UINT)75);++calls;}
static void t_41(void *self, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs){assert(self==expected);assert(pBufferForArgs==(ID3D11Buffer *)(ULONG_PTR)0x102900); assert(AlignedByteOffsetForArgs==(UINT)80);++calls;}
static void t_42(void *self, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ){assert(self==expected);assert(ThreadGroupCountX==(UINT)84); assert(ThreadGroupCountY==(UINT)85); assert(ThreadGroupCountZ==(UINT)86);++calls;}
static void t_43(void *self, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs){assert(self==expected);assert(pBufferForArgs==(ID3D11Buffer *)(ULONG_PTR)0x102b00); assert(AlignedByteOffsetForArgs==(UINT)90);++calls;}
static void t_44(void *self, ID3D11RasterizerState *pRasterizerState){assert(self==expected);assert(pRasterizerState==(ID3D11RasterizerState *)(ULONG_PTR)0x102c00);++calls;}
static void t_45(void *self, UINT NumViewports, const D3D11_VIEWPORT *pViewports){assert(self==expected);assert(NumViewports==(UINT)99); assert(pViewports==(const D3D11_VIEWPORT *)(ULONG_PTR)0x102d10);++calls;}
static void t_46(void *self, UINT NumRects, const D3D11_RECT *pRects){assert(self==expected);assert(NumRects==(UINT)104); assert(pRects==(const D3D11_RECT *)(ULONG_PTR)0x102e10);++calls;}
static void t_47(void *self, ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox){assert(self==expected);assert(pDstResource==(ID3D11Resource *)(ULONG_PTR)0x102f00); assert(DstSubresource==(UINT)110); assert(DstX==(UINT)111); assert(DstY==(UINT)112); assert(DstZ==(UINT)113); assert(pSrcResource==(ID3D11Resource *)(ULONG_PTR)0x102f50); assert(SrcSubresource==(UINT)115); assert(pSrcBox==(const D3D11_BOX *)(ULONG_PTR)0x102f70);++calls;}
static void t_48(void *self, ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource){assert(self==expected);assert(pDstResource==(ID3D11Resource *)(ULONG_PTR)0x103000); assert(pSrcResource==(ID3D11Resource *)(ULONG_PTR)0x103010);++calls;}
static void t_49(void *self, ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch){assert(self==expected);assert(pDstResource==(ID3D11Resource *)(ULONG_PTR)0x103100); assert(DstSubresource==(UINT)120); assert(pDstBox==(const D3D11_BOX *)(ULONG_PTR)0x103120); assert(pSrcData==(const void *)(ULONG_PTR)0x103130); assert(SrcRowPitch==(UINT)123); assert(SrcDepthPitch==(UINT)124);++calls;}
static void t_50(void *self, ID3D11Buffer *pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView *pSrcView){assert(self==expected);assert(pDstBuffer==(ID3D11Buffer *)(ULONG_PTR)0x103200); assert(DstAlignedByteOffset==(UINT)125); assert(pSrcView==(ID3D11UnorderedAccessView *)(ULONG_PTR)0x103220);++calls;}
static void t_51(void *self, ID3D11RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[ 4 ]){assert(self==expected);assert(pRenderTargetView==(ID3D11RenderTargetView *)(ULONG_PTR)0x103300); assert(ColorRGBA==(const FLOAT *)(ULONG_PTR)0x103310);++calls;}
static void t_52(void *self, ID3D11UnorderedAccessView *pUnorderedAccessView, const UINT Values[ 4 ]){assert(self==expected);assert(pUnorderedAccessView==(ID3D11UnorderedAccessView *)(ULONG_PTR)0x103400); assert(Values==(const UINT *)(ULONG_PTR)0x103410);++calls;}
static void t_53(void *self, ID3D11UnorderedAccessView *pUnorderedAccessView, const FLOAT Values[ 4 ]){assert(self==expected);assert(pUnorderedAccessView==(ID3D11UnorderedAccessView *)(ULONG_PTR)0x103500); assert(Values==(const FLOAT *)(ULONG_PTR)0x103510);++calls;}
static void t_54(void *self, ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil){assert(self==expected);assert(pDepthStencilView==(ID3D11DepthStencilView *)(ULONG_PTR)0x103600); assert(ClearFlags==(UINT)18); assert(Depth==(FLOAT)3.375f); assert(Stencil==(UINT8)20);++calls;}
static void t_55(void *self, ID3D11ShaderResourceView *pShaderResourceView){assert(self==expected);assert(pShaderResourceView==(ID3D11ShaderResourceView *)(ULONG_PTR)0x103700);++calls;}
static void t_56(void *self, ID3D11Resource *pResource, FLOAT MinLOD){assert(self==expected);assert(pResource==(ID3D11Resource *)(ULONG_PTR)0x103800); assert(MinLOD==(FLOAT)2.375f);++calls;}
static FLOAT t_57(void *self, ID3D11Resource *pResource){assert(self==expected);assert(pResource==(ID3D11Resource *)(ULONG_PTR)0x103900);++calls;return 27.125f;}
static void t_58(void *self, ID3D11Resource *pDstResource, UINT DstSubresource, ID3D11Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format){assert(self==expected);assert(pDstResource==(ID3D11Resource *)(ULONG_PTR)0x103a00); assert(DstSubresource==(UINT)38); assert(pSrcResource==(ID3D11Resource *)(ULONG_PTR)0x103a20); assert(SrcSubresource==(UINT)40); assert(Format==(DXGI_FORMAT)41);++calls;}
static void t_59(void *self, ID3D11CommandList *pCommandList, BOOL RestoreContextState){assert(self==expected);assert(pCommandList==(ID3D11CommandList *)(ULONG_PTR)0x103b00); assert(RestoreContextState==(BOOL)43);++calls;}
static void t_60(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)47); assert(NumViews==(UINT)48); assert(ppShaderResourceViews==(ID3D11ShaderResourceView *const *)(ULONG_PTR)0x103c20);++calls;}
static void t_61(void *self, ID3D11HullShader *pHullShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances){assert(self==expected);assert(pHullShader==(ID3D11HullShader *)(ULONG_PTR)0x103d00); assert(ppClassInstances==(ID3D11ClassInstance *const *)(ULONG_PTR)0x103d10); assert(NumClassInstances==(UINT)54);++calls;}
static void t_62(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers){assert(self==expected);assert(StartSlot==(UINT)57); assert(NumSamplers==(UINT)58); assert(ppSamplers==(ID3D11SamplerState *const *)(ULONG_PTR)0x103e20);++calls;}
static void t_63(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)62); assert(NumBuffers==(UINT)63); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x103f20);++calls;}
static void t_64(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)67); assert(NumViews==(UINT)68); assert(ppShaderResourceViews==(ID3D11ShaderResourceView *const *)(ULONG_PTR)0x104020);++calls;}
static void t_65(void *self, ID3D11DomainShader *pDomainShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances){assert(self==expected);assert(pDomainShader==(ID3D11DomainShader *)(ULONG_PTR)0x104100); assert(ppClassInstances==(ID3D11ClassInstance *const *)(ULONG_PTR)0x104110); assert(NumClassInstances==(UINT)74);++calls;}
static void t_66(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers){assert(self==expected);assert(StartSlot==(UINT)77); assert(NumSamplers==(UINT)78); assert(ppSamplers==(ID3D11SamplerState *const *)(ULONG_PTR)0x104220);++calls;}
static void t_67(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)82); assert(NumBuffers==(UINT)83); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x104320);++calls;}
static void t_68(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)87); assert(NumViews==(UINT)88); assert(ppShaderResourceViews==(ID3D11ShaderResourceView *const *)(ULONG_PTR)0x104420);++calls;}
static void t_69(void *self, UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts){assert(self==expected);assert(StartSlot==(UINT)92); assert(NumUAVs==(UINT)93); assert(ppUnorderedAccessViews==(ID3D11UnorderedAccessView *const *)(ULONG_PTR)0x104520); assert(pUAVInitialCounts==(const UINT *)(ULONG_PTR)0x104530);++calls;}
static void t_70(void *self, ID3D11ComputeShader *pComputeShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances){assert(self==expected);assert(pComputeShader==(ID3D11ComputeShader *)(ULONG_PTR)0x104600); assert(ppClassInstances==(ID3D11ClassInstance *const *)(ULONG_PTR)0x104610); assert(NumClassInstances==(UINT)99);++calls;}
static void t_71(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers){assert(self==expected);assert(StartSlot==(UINT)102); assert(NumSamplers==(UINT)103); assert(ppSamplers==(ID3D11SamplerState *const *)(ULONG_PTR)0x104720);++calls;}
static void t_72(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)107); assert(NumBuffers==(UINT)108); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x104820);++calls;}
static void t_73(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)112); assert(NumBuffers==(UINT)113); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x104920);++calls;}
static void t_74(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)117); assert(NumViews==(UINT)118); assert(ppShaderResourceViews==(ID3D11ShaderResourceView **)(ULONG_PTR)0x104a20);++calls;}
static void t_75(void *self, ID3D11PixelShader **ppPixelShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances){assert(self==expected);assert(ppPixelShader==(ID3D11PixelShader **)(ULONG_PTR)0x104b00); assert(ppClassInstances==(ID3D11ClassInstance **)(ULONG_PTR)0x104b10); assert(pNumClassInstances==(UINT *)(ULONG_PTR)0x104b20);++calls;}
static void t_77(void *self, ID3D11VertexShader **ppVertexShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances){assert(self==expected);assert(ppVertexShader==(ID3D11VertexShader **)(ULONG_PTR)0x104d00); assert(ppClassInstances==(ID3D11ClassInstance **)(ULONG_PTR)0x104d10); assert(pNumClassInstances==(UINT *)(ULONG_PTR)0x104d20);++calls;}
static void t_78(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)10); assert(NumBuffers==(UINT)11); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x104e20);++calls;}
static void t_79(void *self, ID3D11InputLayout **ppInputLayout){assert(self==expected);assert(ppInputLayout==(ID3D11InputLayout **)(ULONG_PTR)0x104f00);++calls;}
static void t_80(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppVertexBuffers, UINT *pStrides, UINT *pOffsets){assert(self==expected);assert(StartSlot==(UINT)20); assert(NumBuffers==(UINT)21); assert(ppVertexBuffers==(ID3D11Buffer **)(ULONG_PTR)0x105020); assert(pStrides==(UINT *)(ULONG_PTR)0x105030); assert(pOffsets==(UINT *)(ULONG_PTR)0x105040);++calls;}
static void t_81(void *self, ID3D11Buffer **pIndexBuffer, DXGI_FORMAT *Format, UINT *Offset){assert(self==expected);assert(pIndexBuffer==(ID3D11Buffer **)(ULONG_PTR)0x105100); assert(Format==(DXGI_FORMAT *)(ULONG_PTR)0x105110); assert(Offset==(UINT *)(ULONG_PTR)0x105120);++calls;}
static void t_82(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)30); assert(NumBuffers==(UINT)31); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x105220);++calls;}
static void t_83(void *self, ID3D11GeometryShader **ppGeometryShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances){assert(self==expected);assert(ppGeometryShader==(ID3D11GeometryShader **)(ULONG_PTR)0x105300); assert(ppClassInstances==(ID3D11ClassInstance **)(ULONG_PTR)0x105310); assert(pNumClassInstances==(UINT *)(ULONG_PTR)0x105320);++calls;}
static void t_84(void *self, D3D11_PRIMITIVE_TOPOLOGY *pTopology){assert(self==expected);assert(pTopology==(D3D11_PRIMITIVE_TOPOLOGY *)(ULONG_PTR)0x105400);++calls;}
static void t_85(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)45); assert(NumViews==(UINT)46); assert(ppShaderResourceViews==(ID3D11ShaderResourceView **)(ULONG_PTR)0x105520);++calls;}
static void t_86(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers){assert(self==expected);assert(StartSlot==(UINT)50); assert(NumSamplers==(UINT)51); assert(ppSamplers==(ID3D11SamplerState **)(ULONG_PTR)0x105620);++calls;}
static void t_87(void *self, ID3D11Predicate **ppPredicate, BOOL *pPredicateValue){assert(self==expected);assert(ppPredicate==(ID3D11Predicate **)(ULONG_PTR)0x105700); assert(pPredicateValue==(BOOL *)(ULONG_PTR)0x105710);++calls;}
static void t_88(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)60); assert(NumViews==(UINT)61); assert(ppShaderResourceViews==(ID3D11ShaderResourceView **)(ULONG_PTR)0x105820);++calls;}
static void t_89(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers){assert(self==expected);assert(StartSlot==(UINT)65); assert(NumSamplers==(UINT)66); assert(ppSamplers==(ID3D11SamplerState **)(ULONG_PTR)0x105920);++calls;}
static void t_90(void *self, UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView){assert(self==expected);assert(NumViews==(UINT)70); assert(ppRenderTargetViews==(ID3D11RenderTargetView **)(ULONG_PTR)0x105a10); assert(ppDepthStencilView==(ID3D11DepthStencilView **)(ULONG_PTR)0x105a20);++calls;}
static void t_91(void *self, UINT NumRTVs, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews){assert(self==expected);assert(NumRTVs==(UINT)75); assert(ppRenderTargetViews==(ID3D11RenderTargetView **)(ULONG_PTR)0x105b10); assert(ppDepthStencilView==(ID3D11DepthStencilView **)(ULONG_PTR)0x105b20); assert(UAVStartSlot==(UINT)78); assert(NumUAVs==(UINT)79); assert(ppUnorderedAccessViews==(ID3D11UnorderedAccessView **)(ULONG_PTR)0x105b50);++calls;}
static void t_92(void *self, ID3D11BlendState **ppBlendState, FLOAT BlendFactor[ 4 ], UINT *pSampleMask){assert(self==expected);assert(ppBlendState==(ID3D11BlendState **)(ULONG_PTR)0x105c00); assert(BlendFactor==(FLOAT *)(ULONG_PTR)0x105c10); assert(pSampleMask==(UINT *)(ULONG_PTR)0x105c20);++calls;}
static void t_93(void *self, ID3D11DepthStencilState **ppDepthStencilState, UINT *pStencilRef){assert(self==expected);assert(ppDepthStencilState==(ID3D11DepthStencilState **)(ULONG_PTR)0x105d00); assert(pStencilRef==(UINT *)(ULONG_PTR)0x105d10);++calls;}
static void t_94(void *self, UINT NumBuffers, ID3D11Buffer **ppSOTargets){assert(self==expected);assert(NumBuffers==(UINT)90); assert(ppSOTargets==(ID3D11Buffer **)(ULONG_PTR)0x105e10);++calls;}
static void t_95(void *self, ID3D11RasterizerState **ppRasterizerState){assert(self==expected);assert(ppRasterizerState==(ID3D11RasterizerState **)(ULONG_PTR)0x105f00);++calls;}
static void t_96(void *self, UINT *pNumViewports, D3D11_VIEWPORT *pViewports){assert(self==expected);assert(pNumViewports==(UINT *)(ULONG_PTR)0x106000); assert(pViewports==(D3D11_VIEWPORT *)(ULONG_PTR)0x106010);++calls;}
static void t_98(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)110); assert(NumViews==(UINT)111); assert(ppShaderResourceViews==(ID3D11ShaderResourceView **)(ULONG_PTR)0x106220);++calls;}
static void t_99(void *self, ID3D11HullShader **ppHullShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances){assert(self==expected);assert(ppHullShader==(ID3D11HullShader **)(ULONG_PTR)0x106300); assert(ppClassInstances==(ID3D11ClassInstance **)(ULONG_PTR)0x106310); assert(pNumClassInstances==(UINT *)(ULONG_PTR)0x106320);++calls;}
static void t_100(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers){assert(self==expected);assert(StartSlot==(UINT)120); assert(NumSamplers==(UINT)121); assert(ppSamplers==(ID3D11SamplerState **)(ULONG_PTR)0x106420);++calls;}
static void t_101(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)125); assert(NumBuffers==(UINT)126); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x106520);++calls;}
static void t_102(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)3); assert(NumViews==(UINT)4); assert(ppShaderResourceViews==(ID3D11ShaderResourceView **)(ULONG_PTR)0x106620);++calls;}
static void t_103(void *self, ID3D11DomainShader **ppDomainShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances){assert(self==expected);assert(ppDomainShader==(ID3D11DomainShader **)(ULONG_PTR)0x106700); assert(ppClassInstances==(ID3D11ClassInstance **)(ULONG_PTR)0x106710); assert(pNumClassInstances==(UINT *)(ULONG_PTR)0x106720);++calls;}
static void t_104(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers){assert(self==expected);assert(StartSlot==(UINT)13); assert(NumSamplers==(UINT)14); assert(ppSamplers==(ID3D11SamplerState **)(ULONG_PTR)0x106820);++calls;}
static void t_105(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)18); assert(NumBuffers==(UINT)19); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x106920);++calls;}
static void t_106(void *self, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews){assert(self==expected);assert(StartSlot==(UINT)23); assert(NumViews==(UINT)24); assert(ppShaderResourceViews==(ID3D11ShaderResourceView **)(ULONG_PTR)0x106a20);++calls;}
static void t_107(void *self, UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews){assert(self==expected);assert(StartSlot==(UINT)28); assert(NumUAVs==(UINT)29); assert(ppUnorderedAccessViews==(ID3D11UnorderedAccessView **)(ULONG_PTR)0x106b20);++calls;}
static void t_108(void *self, ID3D11ComputeShader **ppComputeShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances){assert(self==expected);assert(ppComputeShader==(ID3D11ComputeShader **)(ULONG_PTR)0x106c00); assert(ppClassInstances==(ID3D11ClassInstance **)(ULONG_PTR)0x106c10); assert(pNumClassInstances==(UINT *)(ULONG_PTR)0x106c20);++calls;}
static void t_109(void *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers){assert(self==expected);assert(StartSlot==(UINT)38); assert(NumSamplers==(UINT)39); assert(ppSamplers==(ID3D11SamplerState **)(ULONG_PTR)0x106d20);++calls;}
static void t_110(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers){assert(self==expected);assert(StartSlot==(UINT)43); assert(NumBuffers==(UINT)44); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x106e20);++calls;}
static void t_111(void *self){assert(self==expected);++calls;}
static void t_112(void *self){assert(self==expected);++calls;}
static D3D11_DEVICE_CONTEXT_TYPE t_113(void *self){assert(self==expected);++calls;return (D3D11_DEVICE_CONTEXT_TYPE)37;}
static UINT t_114(void *self){assert(self==expected);++calls;return (UINT)37;}
static HRESULT t_115(void *self, BOOL RestoreDeferredContextState, ID3D11CommandList **ppCommandList){assert(self==expected);assert(RestoreDeferredContextState==(BOOL)68); assert(ppCommandList==(ID3D11CommandList **)(ULONG_PTR)0x107310);++calls;return (HRESULT)37;}
static void t_116(void *self, ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox, UINT CopyFlags){assert(self==expected);assert(pDstResource==(ID3D11Resource *)(ULONG_PTR)0x107400); assert(DstSubresource==(UINT)74); assert(DstX==(UINT)75); assert(DstY==(UINT)76); assert(DstZ==(UINT)77); assert(pSrcResource==(ID3D11Resource *)(ULONG_PTR)0x107450); assert(SrcSubresource==(UINT)79); assert(pSrcBox==(const D3D11_BOX *)(ULONG_PTR)0x107470); assert(CopyFlags==(UINT)81);++calls;}
static void t_117(void *self, ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags){assert(self==expected);assert(pDstResource==(ID3D11Resource *)(ULONG_PTR)0x107500); assert(DstSubresource==(UINT)79); assert(pDstBox==(const D3D11_BOX *)(ULONG_PTR)0x107520); assert(pSrcData==(const void *)(ULONG_PTR)0x107530); assert(SrcRowPitch==(UINT)82); assert(SrcDepthPitch==(UINT)83); assert(CopyFlags==(UINT)84);++calls;}
static void t_118(void *self, ID3D11Resource *pResource){assert(self==expected);assert(pResource==(ID3D11Resource *)(ULONG_PTR)0x107600);++calls;}
static void t_119(void *self, ID3D11View *pResourceView){assert(self==expected);assert(pResourceView==(ID3D11View *)(ULONG_PTR)0x107700);++calls;}
static void t_120(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)93); assert(NumBuffers==(UINT)94); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x107820); assert(pFirstConstant==(const UINT *)(ULONG_PTR)0x107830); assert(pNumConstants==(const UINT *)(ULONG_PTR)0x107840);++calls;}
static void t_121(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)98); assert(NumBuffers==(UINT)99); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x107920); assert(pFirstConstant==(const UINT *)(ULONG_PTR)0x107930); assert(pNumConstants==(const UINT *)(ULONG_PTR)0x107940);++calls;}
static void t_122(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)103); assert(NumBuffers==(UINT)104); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x107a20); assert(pFirstConstant==(const UINT *)(ULONG_PTR)0x107a30); assert(pNumConstants==(const UINT *)(ULONG_PTR)0x107a40);++calls;}
static void t_123(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)108); assert(NumBuffers==(UINT)109); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x107b20); assert(pFirstConstant==(const UINT *)(ULONG_PTR)0x107b30); assert(pNumConstants==(const UINT *)(ULONG_PTR)0x107b40);++calls;}
static void t_124(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)113); assert(NumBuffers==(UINT)114); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x107c20); assert(pFirstConstant==(const UINT *)(ULONG_PTR)0x107c30); assert(pNumConstants==(const UINT *)(ULONG_PTR)0x107c40);++calls;}
static void t_125(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)118); assert(NumBuffers==(UINT)119); assert(ppConstantBuffers==(ID3D11Buffer *const *)(ULONG_PTR)0x107d20); assert(pFirstConstant==(const UINT *)(ULONG_PTR)0x107d30); assert(pNumConstants==(const UINT *)(ULONG_PTR)0x107d40);++calls;}
static void t_126(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)123); assert(NumBuffers==(UINT)124); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x107e20); assert(pFirstConstant==(UINT *)(ULONG_PTR)0x107e30); assert(pNumConstants==(UINT *)(ULONG_PTR)0x107e40);++calls;}
static void t_127(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)1); assert(NumBuffers==(UINT)2); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x107f20); assert(pFirstConstant==(UINT *)(ULONG_PTR)0x107f30); assert(pNumConstants==(UINT *)(ULONG_PTR)0x107f40);++calls;}
static void t_128(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)6); assert(NumBuffers==(UINT)7); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x108020); assert(pFirstConstant==(UINT *)(ULONG_PTR)0x108030); assert(pNumConstants==(UINT *)(ULONG_PTR)0x108040);++calls;}
static void t_129(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)11); assert(NumBuffers==(UINT)12); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x108120); assert(pFirstConstant==(UINT *)(ULONG_PTR)0x108130); assert(pNumConstants==(UINT *)(ULONG_PTR)0x108140);++calls;}
static void t_130(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)16); assert(NumBuffers==(UINT)17); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x108220); assert(pFirstConstant==(UINT *)(ULONG_PTR)0x108230); assert(pNumConstants==(UINT *)(ULONG_PTR)0x108240);++calls;}
static void t_131(void *self, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants){assert(self==expected);assert(StartSlot==(UINT)21); assert(NumBuffers==(UINT)22); assert(ppConstantBuffers==(ID3D11Buffer **)(ULONG_PTR)0x108320); assert(pFirstConstant==(UINT *)(ULONG_PTR)0x108330); assert(pNumConstants==(UINT *)(ULONG_PTR)0x108340);++calls;}
static void t_132(void *self, ID3DDeviceContextState *pState, ID3DDeviceContextState **ppPreviousState){assert(self==expected);assert(pState==(ID3DDeviceContextState *)(ULONG_PTR)0x108400); assert(ppPreviousState==(ID3DDeviceContextState **)(ULONG_PTR)0x108410);++calls;}
static void t_133(void *self, ID3D11View *pView, const FLOAT Color[ 4 ], const D3D11_RECT *pRect, UINT NumRects){assert(self==expected);assert(pView==(ID3D11View *)(ULONG_PTR)0x108500); assert(Color==(const FLOAT *)(ULONG_PTR)0x108510); assert(pRect==(const D3D11_RECT *)(ULONG_PTR)0x108520); assert(NumRects==(UINT)34);++calls;}
static void t_134(void *self, ID3D11View *pResourceView, const D3D11_RECT *pRects, UINT NumRects){assert(self==expected);assert(pResourceView==(ID3D11View *)(ULONG_PTR)0x108600); assert(pRects==(const D3D11_RECT *)(ULONG_PTR)0x108610); assert(NumRects==(UINT)38);++calls;}
int main(void){
void *table[270]={0}; void **object=table; void *proxy; expected=&object;table[0]=(void*)nq;table[2]=(void*)nr;
FM5ConfigureContextResolve(desc);
assert(FM5WrapPerformanceContext(expected,&proxy)==S_OK);
table[4]=(void*)t_4;
assert(((HRESULT(*)(void*, const GUID * const, UINT *, void *))(*(void***)proxy)[4])(proxy, (const GUID * const)(ULONG_PTR)0x100400, (UINT *)(ULONG_PTR)0x100410, (void *)(ULONG_PTR)0x100420)==(HRESULT)37);
table[5]=(void*)t_5;
assert(((HRESULT(*)(void*, const GUID * const, UINT, const void *))(*(void***)proxy)[5])(proxy, (const GUID * const)(ULONG_PTR)0x100500, (UINT)27, (const void *)(ULONG_PTR)0x100520)==(HRESULT)37);
table[6]=(void*)t_6;
assert(((HRESULT(*)(void*, const GUID * const, const IUnknown *))(*(void***)proxy)[6])(proxy, (const GUID * const)(ULONG_PTR)0x100600, (const IUnknown *)(ULONG_PTR)0x100610)==(HRESULT)37);
table[8]=(void*)t_8;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *))(*(void***)proxy)[8])(proxy, (UINT)41, (UINT)42, (ID3D11Buffer *const *)(ULONG_PTR)0x100820);
table[9]=(void*)t_9;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView *const *))(*(void***)proxy)[9])(proxy, (UINT)46, (UINT)47, (ID3D11ShaderResourceView *const *)(ULONG_PTR)0x100920);
table[10]=(void*)t_10;
((void(*)(void*, ID3D11PixelShader *, ID3D11ClassInstance *const *, UINT))(*(void***)proxy)[10])(proxy, (ID3D11PixelShader *)(ULONG_PTR)0x100a00, (ID3D11ClassInstance *const *)(ULONG_PTR)0x100a10, (UINT)53);
table[11]=(void*)t_11;
((void(*)(void*, UINT, UINT, ID3D11SamplerState *const *))(*(void***)proxy)[11])(proxy, (UINT)56, (UINT)57, (ID3D11SamplerState *const *)(ULONG_PTR)0x100b20);
table[12]=(void*)t_12;
((void(*)(void*, ID3D11VertexShader *, ID3D11ClassInstance *const *, UINT))(*(void***)proxy)[12])(proxy, (ID3D11VertexShader *)(ULONG_PTR)0x100c00, (ID3D11ClassInstance *const *)(ULONG_PTR)0x100c10, (UINT)63);
table[14]=(void*)t_14;
((void(*)(void*, UINT, UINT))(*(void***)proxy)[14])(proxy, (UINT)71, (UINT)72);
table[15]=(void*)t_15;
assert(((HRESULT(*)(void*, ID3D11Resource *, UINT, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE *))(*(void***)proxy)[15])(proxy, (ID3D11Resource *)(ULONG_PTR)0x100f00, (UINT)77, (D3D11_MAP)78, (UINT)79, (D3D11_MAPPED_SUBRESOURCE *)(ULONG_PTR)0x100f40)==(HRESULT)37);
table[16]=(void*)t_16;
((void(*)(void*, ID3D11Resource *, UINT))(*(void***)proxy)[16])(proxy, (ID3D11Resource *)(ULONG_PTR)0x101000, (UINT)82);
table[17]=(void*)t_17;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *))(*(void***)proxy)[17])(proxy, (UINT)86, (UINT)87, (ID3D11Buffer *const *)(ULONG_PTR)0x101120);
table[18]=(void*)t_18;
((void(*)(void*, ID3D11InputLayout *))(*(void***)proxy)[18])(proxy, (ID3D11InputLayout *)(ULONG_PTR)0x101200);
table[19]=(void*)t_19;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *))(*(void***)proxy)[19])(proxy, (UINT)96, (UINT)97, (ID3D11Buffer *const *)(ULONG_PTR)0x101320, (const UINT *)(ULONG_PTR)0x101330, (const UINT *)(ULONG_PTR)0x101340);
table[23]=(void*)t_23;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *))(*(void***)proxy)[23])(proxy, (UINT)116, (UINT)117, (ID3D11Buffer *const *)(ULONG_PTR)0x101720);
table[24]=(void*)t_24;
((void(*)(void*, ID3D11GeometryShader *, ID3D11ClassInstance *const *, UINT))(*(void***)proxy)[24])(proxy, (ID3D11GeometryShader *)(ULONG_PTR)0x101800, (ID3D11ClassInstance *const *)(ULONG_PTR)0x101810, (UINT)123);
table[25]=(void*)t_26;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView *const *))(*(void***)proxy)[26])(proxy, (UINT)4, (UINT)5, (ID3D11ShaderResourceView *const *)(ULONG_PTR)0x101a20);
table[26]=(void*)t_27;
((void(*)(void*, UINT, UINT, ID3D11SamplerState *const *))(*(void***)proxy)[27])(proxy, (UINT)9, (UINT)10, (ID3D11SamplerState *const *)(ULONG_PTR)0x101b20);
table[27]=(void*)t_28;
((void(*)(void*, ID3D11Asynchronous *))(*(void***)proxy)[28])(proxy, (ID3D11Asynchronous *)(ULONG_PTR)0x101c00);
table[28]=(void*)t_29;
((void(*)(void*, ID3D11Asynchronous *))(*(void***)proxy)[29])(proxy, (ID3D11Asynchronous *)(ULONG_PTR)0x101d00);
table[29]=(void*)t_30;
assert(((HRESULT(*)(void*, ID3D11Asynchronous *, void *, UINT, UINT))(*(void***)proxy)[30])(proxy, (ID3D11Asynchronous *)(ULONG_PTR)0x101e00, (void *)(ULONG_PTR)0x101e10, (UINT)26, (UINT)27)==(HRESULT)37);
table[30]=(void*)t_31;
((void(*)(void*, ID3D11Predicate *, BOOL))(*(void***)proxy)[31])(proxy, (ID3D11Predicate *)(ULONG_PTR)0x101f00, (BOOL)30);
table[31]=(void*)t_32;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView *const *))(*(void***)proxy)[32])(proxy, (UINT)34, (UINT)35, (ID3D11ShaderResourceView *const *)(ULONG_PTR)0x102020);
table[32]=(void*)t_33;
((void(*)(void*, UINT, UINT, ID3D11SamplerState *const *))(*(void***)proxy)[33])(proxy, (UINT)39, (UINT)40, (ID3D11SamplerState *const *)(ULONG_PTR)0x102120);
table[33]=(void*)t_34;
((void(*)(void*, UINT, ID3D11RenderTargetView *const *, ID3D11DepthStencilView *))(*(void***)proxy)[34])(proxy, (UINT)44, (ID3D11RenderTargetView *const *)(ULONG_PTR)0x102210, (ID3D11DepthStencilView *)(ULONG_PTR)0x102220);
table[34]=(void*)t_35;
((void(*)(void*, UINT, ID3D11RenderTargetView *const *, ID3D11DepthStencilView *, UINT, UINT, ID3D11UnorderedAccessView *const *, const UINT *))(*(void***)proxy)[35])(proxy, (UINT)49, (ID3D11RenderTargetView *const *)(ULONG_PTR)0x102310, (ID3D11DepthStencilView *)(ULONG_PTR)0x102320, (UINT)52, (UINT)53, (ID3D11UnorderedAccessView *const *)(ULONG_PTR)0x102350, (const UINT *)(ULONG_PTR)0x102360);
table[35]=(void*)t_36;
((void(*)(void*, ID3D11BlendState *, const FLOAT *, UINT))(*(void***)proxy)[36])(proxy, (ID3D11BlendState *)(ULONG_PTR)0x102400, (const FLOAT *)(ULONG_PTR)0x102410, (UINT)56);
table[36]=(void*)t_37;
((void(*)(void*, ID3D11DepthStencilState *, UINT))(*(void***)proxy)[37])(proxy, (ID3D11DepthStencilState *)(ULONG_PTR)0x102500, (UINT)60);
table[37]=(void*)t_38;
((void(*)(void*, UINT, ID3D11Buffer *const *, const UINT *))(*(void***)proxy)[38])(proxy, (UINT)64, (ID3D11Buffer *const *)(ULONG_PTR)0x102610, (const UINT *)(ULONG_PTR)0x102620);
table[38]=(void*)t_39;
((void(*)(void*))(*(void***)proxy)[39])(proxy);
table[39]=(void*)t_40;
((void(*)(void*, ID3D11Buffer *, UINT))(*(void***)proxy)[40])(proxy, (ID3D11Buffer *)(ULONG_PTR)0x102800, (UINT)75);
table[40]=(void*)t_41;
((void(*)(void*, ID3D11Buffer *, UINT))(*(void***)proxy)[41])(proxy, (ID3D11Buffer *)(ULONG_PTR)0x102900, (UINT)80);
table[41]=(void*)t_42;
((void(*)(void*, UINT, UINT, UINT))(*(void***)proxy)[42])(proxy, (UINT)84, (UINT)85, (UINT)86);
table[42]=(void*)t_43;
((void(*)(void*, ID3D11Buffer *, UINT))(*(void***)proxy)[43])(proxy, (ID3D11Buffer *)(ULONG_PTR)0x102b00, (UINT)90);
table[43]=(void*)t_44;
((void(*)(void*, ID3D11RasterizerState *))(*(void***)proxy)[44])(proxy, (ID3D11RasterizerState *)(ULONG_PTR)0x102c00);
table[44]=(void*)t_45;
((void(*)(void*, UINT, const D3D11_VIEWPORT *))(*(void***)proxy)[45])(proxy, (UINT)99, (const D3D11_VIEWPORT *)(ULONG_PTR)0x102d10);
table[45]=(void*)t_46;
((void(*)(void*, UINT, const D3D11_RECT *))(*(void***)proxy)[46])(proxy, (UINT)104, (const D3D11_RECT *)(ULONG_PTR)0x102e10);
table[46]=(void*)t_47;
((void(*)(void*, ID3D11Resource *, UINT, UINT, UINT, UINT, ID3D11Resource *, UINT, const D3D11_BOX *))(*(void***)proxy)[47])(proxy, (ID3D11Resource *)(ULONG_PTR)0x102f00, (UINT)110, (UINT)111, (UINT)112, (UINT)113, (ID3D11Resource *)(ULONG_PTR)0x102f50, (UINT)115, (const D3D11_BOX *)(ULONG_PTR)0x102f70);
table[47]=(void*)t_48;
((void(*)(void*, ID3D11Resource *, ID3D11Resource *))(*(void***)proxy)[48])(proxy, (ID3D11Resource *)(ULONG_PTR)0x103000, (ID3D11Resource *)(ULONG_PTR)0x103010);
table[48]=(void*)t_49;
((void(*)(void*, ID3D11Resource *, UINT, const D3D11_BOX *, const void *, UINT, UINT))(*(void***)proxy)[49])(proxy, (ID3D11Resource *)(ULONG_PTR)0x103100, (UINT)120, (const D3D11_BOX *)(ULONG_PTR)0x103120, (const void *)(ULONG_PTR)0x103130, (UINT)123, (UINT)124);
table[49]=(void*)t_50;
((void(*)(void*, ID3D11Buffer *, UINT, ID3D11UnorderedAccessView *))(*(void***)proxy)[50])(proxy, (ID3D11Buffer *)(ULONG_PTR)0x103200, (UINT)125, (ID3D11UnorderedAccessView *)(ULONG_PTR)0x103220);
table[50]=(void*)t_51;
((void(*)(void*, ID3D11RenderTargetView *, const FLOAT *))(*(void***)proxy)[51])(proxy, (ID3D11RenderTargetView *)(ULONG_PTR)0x103300, (const FLOAT *)(ULONG_PTR)0x103310);
table[51]=(void*)t_52;
((void(*)(void*, ID3D11UnorderedAccessView *, const UINT *))(*(void***)proxy)[52])(proxy, (ID3D11UnorderedAccessView *)(ULONG_PTR)0x103400, (const UINT *)(ULONG_PTR)0x103410);
table[52]=(void*)t_53;
((void(*)(void*, ID3D11UnorderedAccessView *, const FLOAT *))(*(void***)proxy)[53])(proxy, (ID3D11UnorderedAccessView *)(ULONG_PTR)0x103500, (const FLOAT *)(ULONG_PTR)0x103510);
table[53]=(void*)t_54;
((void(*)(void*, ID3D11DepthStencilView *, UINT, FLOAT, UINT8))(*(void***)proxy)[54])(proxy, (ID3D11DepthStencilView *)(ULONG_PTR)0x103600, (UINT)18, (FLOAT)3.375f, (UINT8)20);
table[54]=(void*)t_55;
((void(*)(void*, ID3D11ShaderResourceView *))(*(void***)proxy)[55])(proxy, (ID3D11ShaderResourceView *)(ULONG_PTR)0x103700);
table[55]=(void*)t_56;
((void(*)(void*, ID3D11Resource *, FLOAT))(*(void***)proxy)[56])(proxy, (ID3D11Resource *)(ULONG_PTR)0x103800, (FLOAT)2.375f);
table[56]=(void*)t_57;
assert(((FLOAT(*)(void*, ID3D11Resource *))(*(void***)proxy)[57])(proxy, (ID3D11Resource *)(ULONG_PTR)0x103900)==27.125f);
table[57]=(void*)t_58;
((void(*)(void*, ID3D11Resource *, UINT, ID3D11Resource *, UINT, DXGI_FORMAT))(*(void***)proxy)[58])(proxy, (ID3D11Resource *)(ULONG_PTR)0x103a00, (UINT)38, (ID3D11Resource *)(ULONG_PTR)0x103a20, (UINT)40, (DXGI_FORMAT)41);
table[58]=(void*)t_59;
((void(*)(void*, ID3D11CommandList *, BOOL))(*(void***)proxy)[59])(proxy, (ID3D11CommandList *)(ULONG_PTR)0x103b00, (BOOL)43);
table[59]=(void*)t_60;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView *const *))(*(void***)proxy)[60])(proxy, (UINT)47, (UINT)48, (ID3D11ShaderResourceView *const *)(ULONG_PTR)0x103c20);
table[60]=(void*)t_61;
((void(*)(void*, ID3D11HullShader *, ID3D11ClassInstance *const *, UINT))(*(void***)proxy)[61])(proxy, (ID3D11HullShader *)(ULONG_PTR)0x103d00, (ID3D11ClassInstance *const *)(ULONG_PTR)0x103d10, (UINT)54);
table[61]=(void*)t_62;
((void(*)(void*, UINT, UINT, ID3D11SamplerState *const *))(*(void***)proxy)[62])(proxy, (UINT)57, (UINT)58, (ID3D11SamplerState *const *)(ULONG_PTR)0x103e20);
table[62]=(void*)t_63;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *))(*(void***)proxy)[63])(proxy, (UINT)62, (UINT)63, (ID3D11Buffer *const *)(ULONG_PTR)0x103f20);
table[63]=(void*)t_64;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView *const *))(*(void***)proxy)[64])(proxy, (UINT)67, (UINT)68, (ID3D11ShaderResourceView *const *)(ULONG_PTR)0x104020);
table[64]=(void*)t_65;
((void(*)(void*, ID3D11DomainShader *, ID3D11ClassInstance *const *, UINT))(*(void***)proxy)[65])(proxy, (ID3D11DomainShader *)(ULONG_PTR)0x104100, (ID3D11ClassInstance *const *)(ULONG_PTR)0x104110, (UINT)74);
table[65]=(void*)t_66;
((void(*)(void*, UINT, UINT, ID3D11SamplerState *const *))(*(void***)proxy)[66])(proxy, (UINT)77, (UINT)78, (ID3D11SamplerState *const *)(ULONG_PTR)0x104220);
table[66]=(void*)t_67;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *))(*(void***)proxy)[67])(proxy, (UINT)82, (UINT)83, (ID3D11Buffer *const *)(ULONG_PTR)0x104320);
table[67]=(void*)t_68;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView *const *))(*(void***)proxy)[68])(proxy, (UINT)87, (UINT)88, (ID3D11ShaderResourceView *const *)(ULONG_PTR)0x104420);
table[68]=(void*)t_69;
((void(*)(void*, UINT, UINT, ID3D11UnorderedAccessView *const *, const UINT *))(*(void***)proxy)[69])(proxy, (UINT)92, (UINT)93, (ID3D11UnorderedAccessView *const *)(ULONG_PTR)0x104520, (const UINT *)(ULONG_PTR)0x104530);
table[69]=(void*)t_70;
((void(*)(void*, ID3D11ComputeShader *, ID3D11ClassInstance *const *, UINT))(*(void***)proxy)[70])(proxy, (ID3D11ComputeShader *)(ULONG_PTR)0x104600, (ID3D11ClassInstance *const *)(ULONG_PTR)0x104610, (UINT)99);
table[70]=(void*)t_71;
((void(*)(void*, UINT, UINT, ID3D11SamplerState *const *))(*(void***)proxy)[71])(proxy, (UINT)102, (UINT)103, (ID3D11SamplerState *const *)(ULONG_PTR)0x104720);
table[71]=(void*)t_72;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *))(*(void***)proxy)[72])(proxy, (UINT)107, (UINT)108, (ID3D11Buffer *const *)(ULONG_PTR)0x104820);
table[72]=(void*)t_73;
((void(*)(void*, UINT, UINT, ID3D11Buffer **))(*(void***)proxy)[73])(proxy, (UINT)112, (UINT)113, (ID3D11Buffer **)(ULONG_PTR)0x104920);
table[73]=(void*)t_74;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView **))(*(void***)proxy)[74])(proxy, (UINT)117, (UINT)118, (ID3D11ShaderResourceView **)(ULONG_PTR)0x104a20);
table[74]=(void*)t_75;
((void(*)(void*, ID3D11PixelShader **, ID3D11ClassInstance **, UINT *))(*(void***)proxy)[75])(proxy, (ID3D11PixelShader **)(ULONG_PTR)0x104b00, (ID3D11ClassInstance **)(ULONG_PTR)0x104b10, (UINT *)(ULONG_PTR)0x104b20);
table[76]=(void*)t_77;
((void(*)(void*, ID3D11VertexShader **, ID3D11ClassInstance **, UINT *))(*(void***)proxy)[77])(proxy, (ID3D11VertexShader **)(ULONG_PTR)0x104d00, (ID3D11ClassInstance **)(ULONG_PTR)0x104d10, (UINT *)(ULONG_PTR)0x104d20);
table[77]=(void*)t_78;
((void(*)(void*, UINT, UINT, ID3D11Buffer **))(*(void***)proxy)[78])(proxy, (UINT)10, (UINT)11, (ID3D11Buffer **)(ULONG_PTR)0x104e20);
table[78]=(void*)t_79;
((void(*)(void*, ID3D11InputLayout **))(*(void***)proxy)[79])(proxy, (ID3D11InputLayout **)(ULONG_PTR)0x104f00);
table[79]=(void*)t_80;
((void(*)(void*, UINT, UINT, ID3D11Buffer **, UINT *, UINT *))(*(void***)proxy)[80])(proxy, (UINT)20, (UINT)21, (ID3D11Buffer **)(ULONG_PTR)0x105020, (UINT *)(ULONG_PTR)0x105030, (UINT *)(ULONG_PTR)0x105040);
table[80]=(void*)t_81;
((void(*)(void*, ID3D11Buffer **, DXGI_FORMAT *, UINT *))(*(void***)proxy)[81])(proxy, (ID3D11Buffer **)(ULONG_PTR)0x105100, (DXGI_FORMAT *)(ULONG_PTR)0x105110, (UINT *)(ULONG_PTR)0x105120);
table[81]=(void*)t_82;
((void(*)(void*, UINT, UINT, ID3D11Buffer **))(*(void***)proxy)[82])(proxy, (UINT)30, (UINT)31, (ID3D11Buffer **)(ULONG_PTR)0x105220);
table[82]=(void*)t_83;
((void(*)(void*, ID3D11GeometryShader **, ID3D11ClassInstance **, UINT *))(*(void***)proxy)[83])(proxy, (ID3D11GeometryShader **)(ULONG_PTR)0x105300, (ID3D11ClassInstance **)(ULONG_PTR)0x105310, (UINT *)(ULONG_PTR)0x105320);
table[83]=(void*)t_84;
((void(*)(void*, D3D11_PRIMITIVE_TOPOLOGY *))(*(void***)proxy)[84])(proxy, (D3D11_PRIMITIVE_TOPOLOGY *)(ULONG_PTR)0x105400);
table[84]=(void*)t_85;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView **))(*(void***)proxy)[85])(proxy, (UINT)45, (UINT)46, (ID3D11ShaderResourceView **)(ULONG_PTR)0x105520);
table[85]=(void*)t_86;
((void(*)(void*, UINT, UINT, ID3D11SamplerState **))(*(void***)proxy)[86])(proxy, (UINT)50, (UINT)51, (ID3D11SamplerState **)(ULONG_PTR)0x105620);
table[86]=(void*)t_87;
((void(*)(void*, ID3D11Predicate **, BOOL *))(*(void***)proxy)[87])(proxy, (ID3D11Predicate **)(ULONG_PTR)0x105700, (BOOL *)(ULONG_PTR)0x105710);
table[87]=(void*)t_88;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView **))(*(void***)proxy)[88])(proxy, (UINT)60, (UINT)61, (ID3D11ShaderResourceView **)(ULONG_PTR)0x105820);
table[88]=(void*)t_89;
((void(*)(void*, UINT, UINT, ID3D11SamplerState **))(*(void***)proxy)[89])(proxy, (UINT)65, (UINT)66, (ID3D11SamplerState **)(ULONG_PTR)0x105920);
table[89]=(void*)t_90;
((void(*)(void*, UINT, ID3D11RenderTargetView **, ID3D11DepthStencilView **))(*(void***)proxy)[90])(proxy, (UINT)70, (ID3D11RenderTargetView **)(ULONG_PTR)0x105a10, (ID3D11DepthStencilView **)(ULONG_PTR)0x105a20);
table[90]=(void*)t_91;
((void(*)(void*, UINT, ID3D11RenderTargetView **, ID3D11DepthStencilView **, UINT, UINT, ID3D11UnorderedAccessView **))(*(void***)proxy)[91])(proxy, (UINT)75, (ID3D11RenderTargetView **)(ULONG_PTR)0x105b10, (ID3D11DepthStencilView **)(ULONG_PTR)0x105b20, (UINT)78, (UINT)79, (ID3D11UnorderedAccessView **)(ULONG_PTR)0x105b50);
table[91]=(void*)t_92;
((void(*)(void*, ID3D11BlendState **, FLOAT *, UINT *))(*(void***)proxy)[92])(proxy, (ID3D11BlendState **)(ULONG_PTR)0x105c00, (FLOAT *)(ULONG_PTR)0x105c10, (UINT *)(ULONG_PTR)0x105c20);
table[92]=(void*)t_93;
((void(*)(void*, ID3D11DepthStencilState **, UINT *))(*(void***)proxy)[93])(proxy, (ID3D11DepthStencilState **)(ULONG_PTR)0x105d00, (UINT *)(ULONG_PTR)0x105d10);
table[93]=(void*)t_94;
((void(*)(void*, UINT, ID3D11Buffer **))(*(void***)proxy)[94])(proxy, (UINT)90, (ID3D11Buffer **)(ULONG_PTR)0x105e10);
table[94]=(void*)t_95;
((void(*)(void*, ID3D11RasterizerState **))(*(void***)proxy)[95])(proxy, (ID3D11RasterizerState **)(ULONG_PTR)0x105f00);
table[95]=(void*)t_96;
((void(*)(void*, UINT *, D3D11_VIEWPORT *))(*(void***)proxy)[96])(proxy, (UINT *)(ULONG_PTR)0x106000, (D3D11_VIEWPORT *)(ULONG_PTR)0x106010);
table[97]=(void*)t_98;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView **))(*(void***)proxy)[98])(proxy, (UINT)110, (UINT)111, (ID3D11ShaderResourceView **)(ULONG_PTR)0x106220);
table[98]=(void*)t_99;
((void(*)(void*, ID3D11HullShader **, ID3D11ClassInstance **, UINT *))(*(void***)proxy)[99])(proxy, (ID3D11HullShader **)(ULONG_PTR)0x106300, (ID3D11ClassInstance **)(ULONG_PTR)0x106310, (UINT *)(ULONG_PTR)0x106320);
table[99]=(void*)t_100;
((void(*)(void*, UINT, UINT, ID3D11SamplerState **))(*(void***)proxy)[100])(proxy, (UINT)120, (UINT)121, (ID3D11SamplerState **)(ULONG_PTR)0x106420);
table[100]=(void*)t_101;
((void(*)(void*, UINT, UINT, ID3D11Buffer **))(*(void***)proxy)[101])(proxy, (UINT)125, (UINT)126, (ID3D11Buffer **)(ULONG_PTR)0x106520);
table[101]=(void*)t_102;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView **))(*(void***)proxy)[102])(proxy, (UINT)3, (UINT)4, (ID3D11ShaderResourceView **)(ULONG_PTR)0x106620);
table[102]=(void*)t_103;
((void(*)(void*, ID3D11DomainShader **, ID3D11ClassInstance **, UINT *))(*(void***)proxy)[103])(proxy, (ID3D11DomainShader **)(ULONG_PTR)0x106700, (ID3D11ClassInstance **)(ULONG_PTR)0x106710, (UINT *)(ULONG_PTR)0x106720);
table[103]=(void*)t_104;
((void(*)(void*, UINT, UINT, ID3D11SamplerState **))(*(void***)proxy)[104])(proxy, (UINT)13, (UINT)14, (ID3D11SamplerState **)(ULONG_PTR)0x106820);
table[104]=(void*)t_105;
((void(*)(void*, UINT, UINT, ID3D11Buffer **))(*(void***)proxy)[105])(proxy, (UINT)18, (UINT)19, (ID3D11Buffer **)(ULONG_PTR)0x106920);
table[105]=(void*)t_106;
((void(*)(void*, UINT, UINT, ID3D11ShaderResourceView **))(*(void***)proxy)[106])(proxy, (UINT)23, (UINT)24, (ID3D11ShaderResourceView **)(ULONG_PTR)0x106a20);
table[106]=(void*)t_107;
((void(*)(void*, UINT, UINT, ID3D11UnorderedAccessView **))(*(void***)proxy)[107])(proxy, (UINT)28, (UINT)29, (ID3D11UnorderedAccessView **)(ULONG_PTR)0x106b20);
table[107]=(void*)t_108;
((void(*)(void*, ID3D11ComputeShader **, ID3D11ClassInstance **, UINT *))(*(void***)proxy)[108])(proxy, (ID3D11ComputeShader **)(ULONG_PTR)0x106c00, (ID3D11ClassInstance **)(ULONG_PTR)0x106c10, (UINT *)(ULONG_PTR)0x106c20);
table[108]=(void*)t_109;
((void(*)(void*, UINT, UINT, ID3D11SamplerState **))(*(void***)proxy)[109])(proxy, (UINT)38, (UINT)39, (ID3D11SamplerState **)(ULONG_PTR)0x106d20);
table[109]=(void*)t_110;
((void(*)(void*, UINT, UINT, ID3D11Buffer **))(*(void***)proxy)[110])(proxy, (UINT)43, (UINT)44, (ID3D11Buffer **)(ULONG_PTR)0x106e20);
table[110]=(void*)t_111;
((void(*)(void*))(*(void***)proxy)[111])(proxy);
table[111]=(void*)t_112;
((void(*)(void*))(*(void***)proxy)[112])(proxy);
table[112]=(void*)t_113;
assert(((D3D11_DEVICE_CONTEXT_TYPE(*)(void*))(*(void***)proxy)[113])(proxy)==(D3D11_DEVICE_CONTEXT_TYPE)37);
table[113]=(void*)t_114;
assert(((UINT(*)(void*))(*(void***)proxy)[114])(proxy)==(UINT)37);
table[114]=(void*)t_115;
assert(((HRESULT(*)(void*, BOOL, ID3D11CommandList **))(*(void***)proxy)[115])(proxy, (BOOL)68, (ID3D11CommandList **)(ULONG_PTR)0x107310)==(HRESULT)37);
table[115]=(void*)t_116;
((void(*)(void*, ID3D11Resource *, UINT, UINT, UINT, UINT, ID3D11Resource *, UINT, const D3D11_BOX *, UINT))(*(void***)proxy)[116])(proxy, (ID3D11Resource *)(ULONG_PTR)0x107400, (UINT)74, (UINT)75, (UINT)76, (UINT)77, (ID3D11Resource *)(ULONG_PTR)0x107450, (UINT)79, (const D3D11_BOX *)(ULONG_PTR)0x107470, (UINT)81);
table[116]=(void*)t_117;
((void(*)(void*, ID3D11Resource *, UINT, const D3D11_BOX *, const void *, UINT, UINT, UINT))(*(void***)proxy)[117])(proxy, (ID3D11Resource *)(ULONG_PTR)0x107500, (UINT)79, (const D3D11_BOX *)(ULONG_PTR)0x107520, (const void *)(ULONG_PTR)0x107530, (UINT)82, (UINT)83, (UINT)84);
table[117]=(void*)t_118;
((void(*)(void*, ID3D11Resource *))(*(void***)proxy)[118])(proxy, (ID3D11Resource *)(ULONG_PTR)0x107600);
table[118]=(void*)t_119;
((void(*)(void*, ID3D11View *))(*(void***)proxy)[119])(proxy, (ID3D11View *)(ULONG_PTR)0x107700);
table[119]=(void*)t_120;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *))(*(void***)proxy)[120])(proxy, (UINT)93, (UINT)94, (ID3D11Buffer *const *)(ULONG_PTR)0x107820, (const UINT *)(ULONG_PTR)0x107830, (const UINT *)(ULONG_PTR)0x107840);
table[120]=(void*)t_121;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *))(*(void***)proxy)[121])(proxy, (UINT)98, (UINT)99, (ID3D11Buffer *const *)(ULONG_PTR)0x107920, (const UINT *)(ULONG_PTR)0x107930, (const UINT *)(ULONG_PTR)0x107940);
table[121]=(void*)t_122;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *))(*(void***)proxy)[122])(proxy, (UINT)103, (UINT)104, (ID3D11Buffer *const *)(ULONG_PTR)0x107a20, (const UINT *)(ULONG_PTR)0x107a30, (const UINT *)(ULONG_PTR)0x107a40);
table[122]=(void*)t_123;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *))(*(void***)proxy)[123])(proxy, (UINT)108, (UINT)109, (ID3D11Buffer *const *)(ULONG_PTR)0x107b20, (const UINT *)(ULONG_PTR)0x107b30, (const UINT *)(ULONG_PTR)0x107b40);
table[123]=(void*)t_124;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *))(*(void***)proxy)[124])(proxy, (UINT)113, (UINT)114, (ID3D11Buffer *const *)(ULONG_PTR)0x107c20, (const UINT *)(ULONG_PTR)0x107c30, (const UINT *)(ULONG_PTR)0x107c40);
table[124]=(void*)t_125;
((void(*)(void*, UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *))(*(void***)proxy)[125])(proxy, (UINT)118, (UINT)119, (ID3D11Buffer *const *)(ULONG_PTR)0x107d20, (const UINT *)(ULONG_PTR)0x107d30, (const UINT *)(ULONG_PTR)0x107d40);
table[125]=(void*)t_126;
((void(*)(void*, UINT, UINT, ID3D11Buffer **, UINT *, UINT *))(*(void***)proxy)[126])(proxy, (UINT)123, (UINT)124, (ID3D11Buffer **)(ULONG_PTR)0x107e20, (UINT *)(ULONG_PTR)0x107e30, (UINT *)(ULONG_PTR)0x107e40);
table[126]=(void*)t_127;
((void(*)(void*, UINT, UINT, ID3D11Buffer **, UINT *, UINT *))(*(void***)proxy)[127])(proxy, (UINT)1, (UINT)2, (ID3D11Buffer **)(ULONG_PTR)0x107f20, (UINT *)(ULONG_PTR)0x107f30, (UINT *)(ULONG_PTR)0x107f40);
table[127]=(void*)t_128;
((void(*)(void*, UINT, UINT, ID3D11Buffer **, UINT *, UINT *))(*(void***)proxy)[128])(proxy, (UINT)6, (UINT)7, (ID3D11Buffer **)(ULONG_PTR)0x108020, (UINT *)(ULONG_PTR)0x108030, (UINT *)(ULONG_PTR)0x108040);
table[128]=(void*)t_129;
((void(*)(void*, UINT, UINT, ID3D11Buffer **, UINT *, UINT *))(*(void***)proxy)[129])(proxy, (UINT)11, (UINT)12, (ID3D11Buffer **)(ULONG_PTR)0x108120, (UINT *)(ULONG_PTR)0x108130, (UINT *)(ULONG_PTR)0x108140);
table[129]=(void*)t_130;
((void(*)(void*, UINT, UINT, ID3D11Buffer **, UINT *, UINT *))(*(void***)proxy)[130])(proxy, (UINT)16, (UINT)17, (ID3D11Buffer **)(ULONG_PTR)0x108220, (UINT *)(ULONG_PTR)0x108230, (UINT *)(ULONG_PTR)0x108240);
table[130]=(void*)t_131;
((void(*)(void*, UINT, UINT, ID3D11Buffer **, UINT *, UINT *))(*(void***)proxy)[131])(proxy, (UINT)21, (UINT)22, (ID3D11Buffer **)(ULONG_PTR)0x108320, (UINT *)(ULONG_PTR)0x108330, (UINT *)(ULONG_PTR)0x108340);
table[131]=(void*)t_132;
((void(*)(void*, ID3DDeviceContextState *, ID3DDeviceContextState **))(*(void***)proxy)[132])(proxy, (ID3DDeviceContextState *)(ULONG_PTR)0x108400, (ID3DDeviceContextState **)(ULONG_PTR)0x108410);
table[132]=(void*)t_133;
((void(*)(void*, ID3D11View *, const FLOAT *, const D3D11_RECT *, UINT))(*(void***)proxy)[133])(proxy, (ID3D11View *)(ULONG_PTR)0x108500, (const FLOAT *)(ULONG_PTR)0x108510, (const D3D11_RECT *)(ULONG_PTR)0x108520, (UINT)34);
table[133]=(void*)t_134;
((void(*)(void*, ID3D11View *, const D3D11_RECT *, UINT))(*(void***)proxy)[134])(proxy, (ID3D11View *)(ULONG_PTR)0x108600, (const D3D11_RECT *)(ULONG_PTR)0x108610, (UINT)38);
assert(calls==123);
assert(((ULONG(*)(void*))(*(void***)proxy)[2])(proxy)==0&&refs==0);
puts("PASS: 123 typed standard methods, every scalar/pointer/stack argument and return value");
return 0;}
