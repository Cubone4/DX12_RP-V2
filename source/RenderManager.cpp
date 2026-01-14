#include <RenderManager.h>
#include <iostream>
#include <cassert>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <Camera.h>
#include <functional>
#include <ModelLoader.h>
#include <_Wrapper/DX12Wrapper.h>
#include <_Wrapper/BufferManager.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool RenderManager::Initialize(HWND hwnd)
{
    ModelLoader loader;
    vertices = loader.LoadObj("../../Resources/Models/BalconyModel.obj");
    std::cout << sizeof(vertices) << " bytes of vertices: " << vertices.size() << "\n";

    // Device
    DX12Wrapper w(device);
    
    // Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
    
    #pragma region Define Depth Stencil
    
    #pragma region define depthBuffer
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // CreateBuffer() function defined in BufferManager.h cannot be used here since
    // it does not have the flexibility to define an riidSource or WRITE state
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&depthBuffer)
    );
    // This could later be modified, though this would not have any benefits
    // because adding any more parameters to CreateBuffer() would simply be
    // recreating the CreateCommittedResource function.
    #pragma endregion

    #pragma region dsvHeap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    device->CreateDescriptorHeap(
        &dsvHeapDesc,
        IID_PPV_ARGS(&dsvHeap)
    );
    #pragma endregion

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(
        depthBuffer.Get(),
        &dsvDesc,
        dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );
    #pragma endregion

    #pragma region Swap Chain
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.BufferCount = FrameCount;
    scDesc.Width = width;
    scDesc.Height = height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> tempSwapChain;
    factory->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        &tempSwapChain
    );

    tempSwapChain.As(&swapChain);
    frameIndex = swapChain->GetCurrentBackBufferIndex();
    #pragma endregion

    #pragma region Render Target View (RTV) Heap + Back Buffers
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Back Buffers + RTVs
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        rtvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    // Create "FrameCount" number of buffers
    // By default this is 2, though I've read that most engines prefer 3 to keep the GPU busy
    // This Engine is simple, so either will work just fine, though you can tweak this value in
    //                              include/RenderManager.h
    for (UINT i = 0; i < FrameCount; i++) {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        device->CreateRenderTargetView(
            renderTargets[i].Get(),
            nullptr,
            rtvHandle
        );
        rtvHandle.Offset(1, rtvDescriptorSize);
    }
    #pragma endregion

    #pragma region Command Allocator + List
    device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator)
    );

    device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList)
    );

    // commandList->Close();
    #pragma endregion

    #pragma region Fence (Synchronization)
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    fenceValue = 1;
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    #pragma endregion

    #pragma region Define Root Signature
    CD3DX12_ROOT_PARAMETER rootParams[1];
    CD3DX12_DESCRIPTOR_RANGE cbvRange;
    cbvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 1 descriptor, b0
    rootParams[0].InitAsDescriptorTable(1, &cbvRange, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_ROOT_SIGNATURE_DESC rootDesc;
    rootDesc.Init(
        _countof(rootParams),
        rootParams,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );


    hr = D3D12SerializeRootSignature(
        &rootDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );

    if(FAILED(hr)) {std::cout << "Error SerializeRootSignature\n"; system("pause");}
    else {std::cout << "Successfully Serialized Root SIgnature\n";}

    hr = device->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)
    );

    if(FAILED(hr)) {std::cout << "Error Creating RootSignature\n"; system("Pause");}
    else {std::cout << "Correctly creted Root Signature\n";}
    #pragma endregion

    #pragma region Define Buffers
    // Vertex Buffer
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    w.b.CreateBuffer(heapProps, vertices, vertexBuffer);
    w.b.UploadToGPU(vertexBuffer, &vertexBufferView, vertices);

    // Constant Buffer
    CD3DX12_HEAP_PROPERTIES constHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    w.b.CreateBuffer(constHeapProps, cam.GetDXBuffer(), constBuffer);
    cam.UpdateCamBuffer(constBuffer);
    cam.hwnd = hwnd;


    D3D12_DESCRIPTOR_HEAP_DESC constHeapDesc = {};
    constHeapDesc.NumDescriptors = 1;
    constHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    constHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = device->CreateDescriptorHeap(&constHeapDesc, IID_PPV_ARGS(&cbvHeap));

    if(FAILED(hr)) {std::cout <<"couldn't create descriptor heap for Constant Buffer\n"; system("pause");}
    else {std::cout << "Created Descriptor Heap for Constant Buffer\n";}

    D3D12_CONSTANT_BUFFER_VIEW_DESC constBufferView = {};
    constBufferView.BufferLocation = constBuffer->GetGPUVirtualAddress();
    constBufferView.SizeInBytes = 256;
    // Hard coded 256 byte size because 3 XMMATRIX (world, view, proj) has to round to 256 bytes for GPU standards
    // This is explicit in include/Camera.h "DXCamBuffer" struct definition alignas.

    D3D12_CPU_DESCRIPTOR_HANDLE HDesc = cbvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateConstantBufferView(&constBufferView, HDesc);

    AddUpdateCallback([this]() { cam.MouseLook(); });
    std::cout << "Created a constant buffer\n";
    #pragma endregion

    #pragma region Define render pipeline state object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = { // Define vertexShader.hlsl input
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    }; pso.InputLayout = {inputLayout, _countof(inputLayout)};

    // Compile vertexShader.hlsl
    hr = D3DCompileFromFile(
        L"../../Resources/shaders/vertexShader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSMain",
        "vs_5_0",
        0, 0,
        &vsBlob,
        &errorBlob
    ); 
    
    if(FAILED(hr)) {std::cout << "Error compiling vertexShader.hlsl\n"; system("Pause");}
    else { pso.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()}; std::cout << "Correctly compiled vs\n";}

    // Compile pixelShader.hlsl
    hr = D3DCompileFromFile(
        L"../../Resources/shaders/pixelShader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSMain",
        "ps_5_0",
        0, 0,
        &psBlob,
        &errorBlob
    ); 
    
    if(FAILED(hr)) {std::cout << "Error compiling pixelShader.hlsl\n"; system("Pause");}
    else { pso.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()}; std::cout << "Correctly compiled ps\n"; }

    pso.pRootSignature = rootSignature.Get();
    pso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.SampleDesc.Count = 1;

    // Define ds (DEPTH_STENCIL_DESC)
    D3D12_DEPTH_STENCIL_DESC ds = {};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    pso.DepthStencilState = ds;
    pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;


    hr = device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));
    if(FAILED(hr)) {std::cout << "Error CreateGraphicsPipelineState: 0x" << std::hex << hr << "\n"; system("Pause"); }
    else {std::cout << "Correctly created GraphicsPipelineState\n";}
    #pragma endregion

    #pragma region Viewport & Scissor
    // viewport
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width  = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    // scissor
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = width;
    scissor.bottom = height;
    #pragma endregion

    if(!FAILED(hr)) { std::cout << "Successfully finished Initialize\n"; }
    else return false;

    return true;
}

void RenderManager::Render() {
    // Reset commandAllocator, commandList, and DepthStencilView each frame
    commandAllocator->Reset();
    commandList->Reset(commandAllocator.Get(), pipelineState.Get());
    commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), 
                    D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    
    // Execute all CallBack functions
    for(auto& ucb : updateCallback){ ucb(); }

    // Present to RenderTarget (Swap Chain)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTargets[frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ); commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        frameIndex,
        rtvDescriptorSize
    );

    // Clear background color
    FLOAT clearColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);


    // Old, with no depth stencil: commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    // Apply Depth Stencil:
    commandList->OMSetRenderTargets(1, &rtvHandle, 
        FALSE, 
        &dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pipelineState.Get());

    // Set Viewport & Scissor
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);


    ID3D12DescriptorHeap* heaps[] = { cbvHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    commandList->SetGraphicsRootDescriptorTable(0, cbvHeap->GetGPUDescriptorHandleForHeapStart());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // Potentially change vertices.size to some sort of loop through meshes
    // This could add potential for having multiple objects, though it might be problematic
    // for optimization in huge scenes where there might be unnecessary draw calls
    commandList->DrawInstanced(vertices.size(), 1, 0, 0);

    // RenderTarget to Present
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTargets[frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    commandList->ResourceBarrier(1, &barrier);

    

    // Close commandList and execute
    commandList->Close();
    ID3D12CommandList* lists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, lists);

    swapChain->Present(1, 0);

    // Signal fence (synchronization)
    const UINT64 currentFence = fenceValue;
    commandQueue->Signal(fence.Get(), currentFence);
    fenceValue++;

    if (fence->GetCompletedValue() < currentFence) {
        fence->SetEventOnCompletion(currentFence, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    frameIndex = swapChain->GetCurrentBackBufferIndex();
}
