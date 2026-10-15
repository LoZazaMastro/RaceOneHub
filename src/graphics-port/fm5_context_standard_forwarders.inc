static HRESULT forward_4(PerformanceContext *context, const GUID * const guid, UINT *pDataSize, void *pData) {
    typedef HRESULT (*Target)(void *, const GUID * const , UINT *, void *);
    return ((Target)(*(void ***)context->native)[4])(context->native, guid, pDataSize, pData);
}

static HRESULT forward_5(PerformanceContext *context, const GUID * const guid, UINT DataSize, const void *pData) {
    typedef HRESULT (*Target)(void *, const GUID * const , UINT , const void *);
    return ((Target)(*(void ***)context->native)[5])(context->native, guid, DataSize, pData);
}

static HRESULT forward_6(PerformanceContext *context, const GUID * const guid, const IUnknown *pData) {
    typedef HRESULT (*Target)(void *, const GUID * const , const IUnknown *);
    return ((Target)(*(void ***)context->native)[6])(context->native, guid, pData);
}

static void forward_8(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *);
    ((Target)(*(void ***)context->native)[8])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_9(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView *const *);
    ((Target)(*(void ***)context->native)[9])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_10(PerformanceContext *context, ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
    typedef void (*Target)(void *, ID3D11PixelShader *, ID3D11ClassInstance *const *, UINT );
    ((Target)(*(void ***)context->native)[10])(context->native, pPixelShader, ppClassInstances, NumClassInstances);
}

static void forward_11(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState *const *);
    ((Target)(*(void ***)context->native)[11])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_12(PerformanceContext *context, ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
    typedef void (*Target)(void *, ID3D11VertexShader *, ID3D11ClassInstance *const *, UINT );
    ((Target)(*(void ***)context->native)[12])(context->native, pVertexShader, ppClassInstances, NumClassInstances);
}

static void forward_14(PerformanceContext *context, UINT VertexCount, UINT StartVertexLocation) {
    typedef void (*Target)(void *, UINT , UINT );
    ((Target)(*(void ***)context->native)[14])(context->native, VertexCount, StartVertexLocation);
}

static HRESULT forward_15(PerformanceContext *context, ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource) {
    typedef HRESULT (*Target)(void *, ID3D11Resource *, UINT , D3D11_MAP , UINT , D3D11_MAPPED_SUBRESOURCE *);
    return ((Target)(*(void ***)context->native)[15])(context->native, pResource, Subresource, MapType, MapFlags, pMappedResource);
}

static void forward_16(PerformanceContext *context, ID3D11Resource *pResource, UINT Subresource) {
    typedef void (*Target)(void *, ID3D11Resource *, UINT );
    ((Target)(*(void ***)context->native)[16])(context->native, pResource, Subresource);
}

static void forward_17(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *);
    ((Target)(*(void ***)context->native)[17])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_18(PerformanceContext *context, ID3D11InputLayout *pInputLayout) {
    typedef void (*Target)(void *, ID3D11InputLayout *);
    ((Target)(*(void ***)context->native)[18])(context->native, pInputLayout);
}

static void forward_19(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *, const UINT *, const UINT *);
    ((Target)(*(void ***)context->native)[19])(context->native, StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

static void forward_23(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *);
    ((Target)(*(void ***)context->native)[23])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_24(PerformanceContext *context, ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
    typedef void (*Target)(void *, ID3D11GeometryShader *, ID3D11ClassInstance *const *, UINT );
    ((Target)(*(void ***)context->native)[24])(context->native, pShader, ppClassInstances, NumClassInstances);
}

static void forward_26(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView *const *);
    ((Target)(*(void ***)context->native)[25])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_27(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState *const *);
    ((Target)(*(void ***)context->native)[26])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_28(PerformanceContext *context, ID3D11Asynchronous *pAsync) {
    typedef void (*Target)(void *, ID3D11Asynchronous *);
    ((Target)(*(void ***)context->native)[27])(context->native, pAsync);
}

static void forward_29(PerformanceContext *context, ID3D11Asynchronous *pAsync) {
    typedef void (*Target)(void *, ID3D11Asynchronous *);
    ((Target)(*(void ***)context->native)[28])(context->native, pAsync);
}

static HRESULT forward_30(PerformanceContext *context, ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags) {
    typedef HRESULT (*Target)(void *, ID3D11Asynchronous *, void *, UINT , UINT );
    return ((Target)(*(void ***)context->native)[29])(context->native, pAsync, pData, DataSize, GetDataFlags);
}

static void forward_31(PerformanceContext *context, ID3D11Predicate *pPredicate, BOOL PredicateValue) {
    typedef void (*Target)(void *, ID3D11Predicate *, BOOL );
    ((Target)(*(void ***)context->native)[30])(context->native, pPredicate, PredicateValue);
}

static void forward_32(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView *const *);
    ((Target)(*(void ***)context->native)[31])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_33(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState *const *);
    ((Target)(*(void ***)context->native)[32])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_34(PerformanceContext *context, UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView) {
    typedef void (*Target)(void *, UINT , ID3D11RenderTargetView *const *, ID3D11DepthStencilView *);
    ((Target)(*(void ***)context->native)[33])(context->native, NumViews, ppRenderTargetViews, pDepthStencilView);
}

static void forward_35(PerformanceContext *context, UINT NumRTVs, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts) {
    typedef void (*Target)(void *, UINT , ID3D11RenderTargetView *const *, ID3D11DepthStencilView *, UINT , UINT , ID3D11UnorderedAccessView *const *, const UINT *);
    ((Target)(*(void ***)context->native)[34])(context->native, NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}

static void forward_36(PerformanceContext *context, ID3D11BlendState *pBlendState, const FLOAT BlendFactor[ 4 ], UINT SampleMask) {
    typedef void (*Target)(void *, ID3D11BlendState *, const FLOAT [ 4 ], UINT );
    ((Target)(*(void ***)context->native)[35])(context->native, pBlendState, BlendFactor, SampleMask);
}

static void forward_37(PerformanceContext *context, ID3D11DepthStencilState *pDepthStencilState, UINT StencilRef) {
    typedef void (*Target)(void *, ID3D11DepthStencilState *, UINT );
    ((Target)(*(void ***)context->native)[36])(context->native, pDepthStencilState, StencilRef);
}

static void forward_38(PerformanceContext *context, UINT NumBuffers, ID3D11Buffer *const *ppSOTargets, const UINT *pOffsets) {
    typedef void (*Target)(void *, UINT , ID3D11Buffer *const *, const UINT *);
    ((Target)(*(void ***)context->native)[37])(context->native, NumBuffers, ppSOTargets, pOffsets);
}

static void forward_39(PerformanceContext *context) {
    typedef void (*Target)(void *);
    ((Target)(*(void ***)context->native)[38])(context->native);
}

static void forward_40(PerformanceContext *context, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs) {
    typedef void (*Target)(void *, ID3D11Buffer *, UINT );
    ((Target)(*(void ***)context->native)[39])(context->native, pBufferForArgs, AlignedByteOffsetForArgs);
}

static void forward_41(PerformanceContext *context, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs) {
    typedef void (*Target)(void *, ID3D11Buffer *, UINT );
    ((Target)(*(void ***)context->native)[40])(context->native, pBufferForArgs, AlignedByteOffsetForArgs);
}

static void forward_42(PerformanceContext *context, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) {
    typedef void (*Target)(void *, UINT , UINT , UINT );
    ((Target)(*(void ***)context->native)[41])(context->native, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

static void forward_43(PerformanceContext *context, ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs) {
    typedef void (*Target)(void *, ID3D11Buffer *, UINT );
    ((Target)(*(void ***)context->native)[42])(context->native, pBufferForArgs, AlignedByteOffsetForArgs);
}

static void forward_44(PerformanceContext *context, ID3D11RasterizerState *pRasterizerState) {
    typedef void (*Target)(void *, ID3D11RasterizerState *);
    ((Target)(*(void ***)context->native)[43])(context->native, pRasterizerState);
}

static void forward_45(PerformanceContext *context, UINT NumViewports, const D3D11_VIEWPORT *pViewports) {
    typedef void (*Target)(void *, UINT , const D3D11_VIEWPORT *);
    ((Target)(*(void ***)context->native)[44])(context->native, NumViewports, pViewports);
}

static void forward_46(PerformanceContext *context, UINT NumRects, const D3D11_RECT *pRects) {
    typedef void (*Target)(void *, UINT , const D3D11_RECT *);
    ((Target)(*(void ***)context->native)[45])(context->native, NumRects, pRects);
}

static void forward_47(PerformanceContext *context, ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox) {
    typedef void (*Target)(void *, ID3D11Resource *, UINT , UINT , UINT , UINT , ID3D11Resource *, UINT , const D3D11_BOX *);
    ((Target)(*(void ***)context->native)[46])(context->native, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
}

static void forward_48(PerformanceContext *context, ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource) {
    typedef void (*Target)(void *, ID3D11Resource *, ID3D11Resource *);
    ((Target)(*(void ***)context->native)[47])(context->native, pDstResource, pSrcResource);
}

static void forward_49(PerformanceContext *context, ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) {
    typedef void (*Target)(void *, ID3D11Resource *, UINT , const D3D11_BOX *, const void *, UINT , UINT );
    ((Target)(*(void ***)context->native)[48])(context->native, pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}

static void forward_50(PerformanceContext *context, ID3D11Buffer *pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView *pSrcView) {
    typedef void (*Target)(void *, ID3D11Buffer *, UINT , ID3D11UnorderedAccessView *);
    ((Target)(*(void ***)context->native)[49])(context->native, pDstBuffer, DstAlignedByteOffset, pSrcView);
}

static void forward_51(PerformanceContext *context, ID3D11RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[ 4 ]) {
    typedef void (*Target)(void *, ID3D11RenderTargetView *, const FLOAT [ 4 ]);
    ((Target)(*(void ***)context->native)[50])(context->native, pRenderTargetView, ColorRGBA);
}

static void forward_52(PerformanceContext *context, ID3D11UnorderedAccessView *pUnorderedAccessView, const UINT Values[ 4 ]) {
    typedef void (*Target)(void *, ID3D11UnorderedAccessView *, const UINT [ 4 ]);
    ((Target)(*(void ***)context->native)[51])(context->native, pUnorderedAccessView, Values);
}

static void forward_53(PerformanceContext *context, ID3D11UnorderedAccessView *pUnorderedAccessView, const FLOAT Values[ 4 ]) {
    typedef void (*Target)(void *, ID3D11UnorderedAccessView *, const FLOAT [ 4 ]);
    ((Target)(*(void ***)context->native)[52])(context->native, pUnorderedAccessView, Values);
}

static void forward_54(PerformanceContext *context, ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) {
    typedef void (*Target)(void *, ID3D11DepthStencilView *, UINT , FLOAT , UINT8 );
    ((Target)(*(void ***)context->native)[53])(context->native, pDepthStencilView, ClearFlags, Depth, Stencil);
}

static void forward_55(PerformanceContext *context, ID3D11ShaderResourceView *pShaderResourceView) {
    typedef void (*Target)(void *, ID3D11ShaderResourceView *);
    ((Target)(*(void ***)context->native)[54])(context->native, pShaderResourceView);
}

static void forward_56(PerformanceContext *context, ID3D11Resource *pResource, FLOAT MinLOD) {
    typedef void (*Target)(void *, ID3D11Resource *, FLOAT );
    ((Target)(*(void ***)context->native)[55])(context->native, pResource, MinLOD);
}

static FLOAT forward_57(PerformanceContext *context, ID3D11Resource *pResource) {
    typedef FLOAT (*Target)(void *, ID3D11Resource *);
    return ((Target)(*(void ***)context->native)[56])(context->native, pResource);
}

static void forward_58(PerformanceContext *context, ID3D11Resource *pDstResource, UINT DstSubresource, ID3D11Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format) {
    typedef void (*Target)(void *, ID3D11Resource *, UINT , ID3D11Resource *, UINT , DXGI_FORMAT );
    ((Target)(*(void ***)context->native)[57])(context->native, pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
}

static void forward_59(PerformanceContext *context, ID3D11CommandList *pCommandList, BOOL RestoreContextState) {
    typedef void (*Target)(void *, ID3D11CommandList *, BOOL );
    ((Target)(*(void ***)context->native)[58])(context->native, pCommandList, RestoreContextState);
}

static void forward_60(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView *const *);
    ((Target)(*(void ***)context->native)[59])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_61(PerformanceContext *context, ID3D11HullShader *pHullShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
    typedef void (*Target)(void *, ID3D11HullShader *, ID3D11ClassInstance *const *, UINT );
    ((Target)(*(void ***)context->native)[60])(context->native, pHullShader, ppClassInstances, NumClassInstances);
}

static void forward_62(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState *const *);
    ((Target)(*(void ***)context->native)[61])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_63(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *);
    ((Target)(*(void ***)context->native)[62])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_64(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView *const *);
    ((Target)(*(void ***)context->native)[63])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_65(PerformanceContext *context, ID3D11DomainShader *pDomainShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
    typedef void (*Target)(void *, ID3D11DomainShader *, ID3D11ClassInstance *const *, UINT );
    ((Target)(*(void ***)context->native)[64])(context->native, pDomainShader, ppClassInstances, NumClassInstances);
}

static void forward_66(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState *const *);
    ((Target)(*(void ***)context->native)[65])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_67(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *);
    ((Target)(*(void ***)context->native)[66])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_68(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView *const *);
    ((Target)(*(void ***)context->native)[67])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_69(PerformanceContext *context, UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11UnorderedAccessView *const *, const UINT *);
    ((Target)(*(void ***)context->native)[68])(context->native, StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}

static void forward_70(PerformanceContext *context, ID3D11ComputeShader *pComputeShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) {
    typedef void (*Target)(void *, ID3D11ComputeShader *, ID3D11ClassInstance *const *, UINT );
    ((Target)(*(void ***)context->native)[69])(context->native, pComputeShader, ppClassInstances, NumClassInstances);
}

static void forward_71(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState *const *);
    ((Target)(*(void ***)context->native)[70])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_72(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *);
    ((Target)(*(void ***)context->native)[71])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_73(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **);
    ((Target)(*(void ***)context->native)[72])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_74(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView **);
    ((Target)(*(void ***)context->native)[73])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_75(PerformanceContext *context, ID3D11PixelShader **ppPixelShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) {
    typedef void (*Target)(void *, ID3D11PixelShader **, ID3D11ClassInstance **, UINT *);
    ((Target)(*(void ***)context->native)[74])(context->native, ppPixelShader, ppClassInstances, pNumClassInstances);
}

static void forward_76(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState **);
    ((Target)(*(void ***)context->native)[75])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_77(PerformanceContext *context, ID3D11VertexShader **ppVertexShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) {
    typedef void (*Target)(void *, ID3D11VertexShader **, ID3D11ClassInstance **, UINT *);
    ((Target)(*(void ***)context->native)[76])(context->native, ppVertexShader, ppClassInstances, pNumClassInstances);
}

static void forward_78(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **);
    ((Target)(*(void ***)context->native)[77])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_79(PerformanceContext *context, ID3D11InputLayout **ppInputLayout) {
    typedef void (*Target)(void *, ID3D11InputLayout **);
    ((Target)(*(void ***)context->native)[78])(context->native, ppInputLayout);
}

static void forward_80(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppVertexBuffers, UINT *pStrides, UINT *pOffsets) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **, UINT *, UINT *);
    ((Target)(*(void ***)context->native)[79])(context->native, StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

static void forward_81(PerformanceContext *context, ID3D11Buffer **pIndexBuffer, DXGI_FORMAT *Format, UINT *Offset) {
    typedef void (*Target)(void *, ID3D11Buffer **, DXGI_FORMAT *, UINT *);
    ((Target)(*(void ***)context->native)[80])(context->native, pIndexBuffer, Format, Offset);
}

static void forward_82(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **);
    ((Target)(*(void ***)context->native)[81])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_83(PerformanceContext *context, ID3D11GeometryShader **ppGeometryShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) {
    typedef void (*Target)(void *, ID3D11GeometryShader **, ID3D11ClassInstance **, UINT *);
    ((Target)(*(void ***)context->native)[82])(context->native, ppGeometryShader, ppClassInstances, pNumClassInstances);
}

static void forward_84(PerformanceContext *context, D3D11_PRIMITIVE_TOPOLOGY *pTopology) {
    typedef void (*Target)(void *, D3D11_PRIMITIVE_TOPOLOGY *);
    ((Target)(*(void ***)context->native)[83])(context->native, pTopology);
}

static void forward_85(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView **);
    ((Target)(*(void ***)context->native)[84])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_86(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState **);
    ((Target)(*(void ***)context->native)[85])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_87(PerformanceContext *context, ID3D11Predicate **ppPredicate, BOOL *pPredicateValue) {
    typedef void (*Target)(void *, ID3D11Predicate **, BOOL *);
    ((Target)(*(void ***)context->native)[86])(context->native, ppPredicate, pPredicateValue);
}

static void forward_88(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView **);
    ((Target)(*(void ***)context->native)[87])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_89(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState **);
    ((Target)(*(void ***)context->native)[88])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_90(PerformanceContext *context, UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView) {
    typedef void (*Target)(void *, UINT , ID3D11RenderTargetView **, ID3D11DepthStencilView **);
    ((Target)(*(void ***)context->native)[89])(context->native, NumViews, ppRenderTargetViews, ppDepthStencilView);
}

static void forward_91(PerformanceContext *context, UINT NumRTVs, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews) {
    typedef void (*Target)(void *, UINT , ID3D11RenderTargetView **, ID3D11DepthStencilView **, UINT , UINT , ID3D11UnorderedAccessView **);
    ((Target)(*(void ***)context->native)[90])(context->native, NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews);
}

static void forward_92(PerformanceContext *context, ID3D11BlendState **ppBlendState, FLOAT BlendFactor[ 4 ], UINT *pSampleMask) {
    typedef void (*Target)(void *, ID3D11BlendState **, FLOAT [ 4 ], UINT *);
    ((Target)(*(void ***)context->native)[91])(context->native, ppBlendState, BlendFactor, pSampleMask);
}

static void forward_93(PerformanceContext *context, ID3D11DepthStencilState **ppDepthStencilState, UINT *pStencilRef) {
    typedef void (*Target)(void *, ID3D11DepthStencilState **, UINT *);
    ((Target)(*(void ***)context->native)[92])(context->native, ppDepthStencilState, pStencilRef);
}

static void forward_94(PerformanceContext *context, UINT NumBuffers, ID3D11Buffer **ppSOTargets) {
    typedef void (*Target)(void *, UINT , ID3D11Buffer **);
    ((Target)(*(void ***)context->native)[93])(context->native, NumBuffers, ppSOTargets);
}

static void forward_95(PerformanceContext *context, ID3D11RasterizerState **ppRasterizerState) {
    typedef void (*Target)(void *, ID3D11RasterizerState **);
    ((Target)(*(void ***)context->native)[94])(context->native, ppRasterizerState);
}

static void forward_96(PerformanceContext *context, UINT *pNumViewports, D3D11_VIEWPORT *pViewports) {
    typedef void (*Target)(void *, UINT *, D3D11_VIEWPORT *);
    ((Target)(*(void ***)context->native)[95])(context->native, pNumViewports, pViewports);
}

static void forward_97(PerformanceContext *context, UINT *pNumRects, D3D11_RECT *pRects) {
    typedef void (*Target)(void *, UINT *, D3D11_RECT *);
    ((Target)(*(void ***)context->native)[96])(context->native, pNumRects, pRects);
}

static void forward_98(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView **);
    ((Target)(*(void ***)context->native)[97])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_99(PerformanceContext *context, ID3D11HullShader **ppHullShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) {
    typedef void (*Target)(void *, ID3D11HullShader **, ID3D11ClassInstance **, UINT *);
    ((Target)(*(void ***)context->native)[98])(context->native, ppHullShader, ppClassInstances, pNumClassInstances);
}

static void forward_100(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState **);
    ((Target)(*(void ***)context->native)[99])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_101(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **);
    ((Target)(*(void ***)context->native)[100])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_102(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView **);
    ((Target)(*(void ***)context->native)[101])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_103(PerformanceContext *context, ID3D11DomainShader **ppDomainShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) {
    typedef void (*Target)(void *, ID3D11DomainShader **, ID3D11ClassInstance **, UINT *);
    ((Target)(*(void ***)context->native)[102])(context->native, ppDomainShader, ppClassInstances, pNumClassInstances);
}

static void forward_104(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState **);
    ((Target)(*(void ***)context->native)[103])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_105(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **);
    ((Target)(*(void ***)context->native)[104])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_106(PerformanceContext *context, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11ShaderResourceView **);
    ((Target)(*(void ***)context->native)[105])(context->native, StartSlot, NumViews, ppShaderResourceViews);
}

static void forward_107(PerformanceContext *context, UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11UnorderedAccessView **);
    ((Target)(*(void ***)context->native)[106])(context->native, StartSlot, NumUAVs, ppUnorderedAccessViews);
}

static void forward_108(PerformanceContext *context, ID3D11ComputeShader **ppComputeShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) {
    typedef void (*Target)(void *, ID3D11ComputeShader **, ID3D11ClassInstance **, UINT *);
    ((Target)(*(void ***)context->native)[107])(context->native, ppComputeShader, ppClassInstances, pNumClassInstances);
}

static void forward_109(PerformanceContext *context, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11SamplerState **);
    ((Target)(*(void ***)context->native)[108])(context->native, StartSlot, NumSamplers, ppSamplers);
}

static void forward_110(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **);
    ((Target)(*(void ***)context->native)[109])(context->native, StartSlot, NumBuffers, ppConstantBuffers);
}

static void forward_111(PerformanceContext *context) {
    typedef void (*Target)(void *);
    ((Target)(*(void ***)context->native)[110])(context->native);
}

static void forward_112(PerformanceContext *context) {
    typedef void (*Target)(void *);
    ((Target)(*(void ***)context->native)[111])(context->native);
}

static D3D11_DEVICE_CONTEXT_TYPE forward_113(PerformanceContext *context) {
    typedef D3D11_DEVICE_CONTEXT_TYPE (*Target)(void *);
    return ((Target)(*(void ***)context->native)[112])(context->native);
}

static UINT forward_114(PerformanceContext *context) {
    typedef UINT (*Target)(void *);
    return ((Target)(*(void ***)context->native)[113])(context->native);
}

static HRESULT forward_115(PerformanceContext *context, BOOL RestoreDeferredContextState, ID3D11CommandList **ppCommandList) {
    typedef HRESULT (*Target)(void *, BOOL , ID3D11CommandList **);
    return ((Target)(*(void ***)context->native)[114])(context->native, RestoreDeferredContextState, ppCommandList);
}

static void forward_116(PerformanceContext *context, ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox, UINT CopyFlags) {
    typedef void (*Target)(void *, ID3D11Resource *, UINT , UINT , UINT , UINT , ID3D11Resource *, UINT , const D3D11_BOX *, UINT );
    ((Target)(*(void ***)context->native)[115])(context->native, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox, CopyFlags);
}

static void forward_117(PerformanceContext *context, ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags) {
    typedef void (*Target)(void *, ID3D11Resource *, UINT , const D3D11_BOX *, const void *, UINT , UINT , UINT );
    ((Target)(*(void ***)context->native)[116])(context->native, pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags);
}

static void forward_118(PerformanceContext *context, ID3D11Resource *pResource) {
    typedef void (*Target)(void *, ID3D11Resource *);
    ((Target)(*(void ***)context->native)[117])(context->native, pResource);
}

static void forward_119(PerformanceContext *context, ID3D11View *pResourceView) {
    typedef void (*Target)(void *, ID3D11View *);
    ((Target)(*(void ***)context->native)[118])(context->native, pResourceView);
}

static void forward_120(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *, const UINT *, const UINT *);
    ((Target)(*(void ***)context->native)[119])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_121(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *, const UINT *, const UINT *);
    ((Target)(*(void ***)context->native)[120])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_122(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *, const UINT *, const UINT *);
    ((Target)(*(void ***)context->native)[121])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_123(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *, const UINT *, const UINT *);
    ((Target)(*(void ***)context->native)[122])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_124(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *, const UINT *, const UINT *);
    ((Target)(*(void ***)context->native)[123])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_125(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer *const *, const UINT *, const UINT *);
    ((Target)(*(void ***)context->native)[124])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_126(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **, UINT *, UINT *);
    ((Target)(*(void ***)context->native)[125])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_127(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **, UINT *, UINT *);
    ((Target)(*(void ***)context->native)[126])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_128(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **, UINT *, UINT *);
    ((Target)(*(void ***)context->native)[127])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_129(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **, UINT *, UINT *);
    ((Target)(*(void ***)context->native)[128])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_130(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **, UINT *, UINT *);
    ((Target)(*(void ***)context->native)[129])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_131(PerformanceContext *context, UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) {
    typedef void (*Target)(void *, UINT , UINT , ID3D11Buffer **, UINT *, UINT *);
    ((Target)(*(void ***)context->native)[130])(context->native, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}

static void forward_132(PerformanceContext *context, ID3DDeviceContextState *pState, ID3DDeviceContextState **ppPreviousState) {
    typedef void (*Target)(void *, ID3DDeviceContextState *, ID3DDeviceContextState **);
    ((Target)(*(void ***)context->native)[131])(context->native, pState, ppPreviousState);
}

static void forward_133(PerformanceContext *context, ID3D11View *pView, const FLOAT Color[ 4 ], const D3D11_RECT *pRect, UINT NumRects) {
    typedef void (*Target)(void *, ID3D11View *, const FLOAT [ 4 ], const D3D11_RECT *, UINT );
    ((Target)(*(void ***)context->native)[132])(context->native, pView, Color, pRect, NumRects);
}

static void forward_134(PerformanceContext *context, ID3D11View *pResourceView, const D3D11_RECT *pRects, UINT NumRects) {
    typedef void (*Target)(void *, ID3D11View *, const D3D11_RECT *, UINT );
    ((Target)(*(void ***)context->native)[133])(context->native, pResourceView, pRects, NumRects);
}
