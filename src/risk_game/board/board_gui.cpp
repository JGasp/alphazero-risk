#ifdef GUI

#include "board_gui.h"

int BoardGui::openGui(Game* game)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(BoardGui::g_pd3dDevice, BoardGui::g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImVec2 window_pos = ImVec2(0.0f, 0.0f);
    ImVec2 display_size = ImVec2(io.DisplaySize.x, io.DisplaySize.y);

    bool risk_map_window_open = true;
    bool info_window_open = true;

    int my_image_width = 0;
    int my_image_height = 0;
    ID3D11ShaderResourceView* my_texture = NULL;
    bool ret = LoadTextureFromFile("risk_map_clear.jpg", &my_texture, &my_image_width, &my_image_height);
    
    
    float scale = 0.9f;
    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }        

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        
        ImGui::NewFrame();
        
        { 
            ImGui::SetNextWindowPos(window_pos);
            ImGui::SetNextWindowSize(display_size);
            
            ImGui::Begin("AlphaZero Risk Map", &risk_map_window_open, ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::Image((void*)my_texture, ImVec2(my_image_width * scale, my_image_height * scale));


            for (uint8_t i = 0; i < LAND_INDEX_SIZE; i++)
            {
                const LandIndex li = static_cast<LandIndex>(i);

                ImVec2 p = getLandIndexPos(li);
                ImGui::SetCursorPos(ImVec2(p.x * scale, p.y * scale + 20));
                ImGui::Text(Land::getName(li).c_str());

                int army = game->state->getRawLandArmyValue(i);
                std::string label = +"Army: " + std::to_string(army);
                ImGui::SetCursorPos(ImVec2(p.x * scale, p.y * scale + 20 + 10));
                ImGui::Text(label.c_str());
            }

            ImGui::End();
        }

        {
            ImGui::Begin("AlphaZero Risk Info", &info_window_open);
            ImGui::Text("size = %d x %d", my_image_width, my_image_height);
            ImGui::Text("scale = %f", scale);
            ImGui::Text("mouse = %fx %fy", io.MousePos.x, io.MousePos.y);
            
            if (ImGui::Button("Next turn"))
            {
                game->playTurn();
            }

            if (game->isInSetupPhase())
            {
                if (ImGui::Button("Play through setup"))
                {
                    game->playThroughSetup();
                }
            }

            ImGui::End();
        }
       
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*) &clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

ImVec2 BoardGui::getLandIndexPos(const LandIndex landIndex)
{
    switch (landIndex)
    {
    case LandIndex::ALASKA: return ImVec2(85.0f, 200.0f);
    case LandIndex::NORTHWEST_TERRIOTRY: return ImVec2(200.0f, 200.0f);
    case LandIndex::GREENLAND: return ImVec2(600.0f, 120.0f);
    case LandIndex::ALBERTA: return ImVec2(250.0f, 290.0f);
    case LandIndex::ONTARIO: return ImVec2(365.0f, 300.0f);
    case LandIndex::QUEBEC: return ImVec2(480.0f, 300.0f);
    case LandIndex::WESTERN_UNITED_STATES: return ImVec2(210.0f, 380.0f);
    case LandIndex::EASTERN_UNITED_STATES: return ImVec2(330.0f, 410.0f);
    case LandIndex::CENTRAL_AMERICA: return ImVec2(260.0f, 490.0f);

    case LandIndex::VENEZUELA: return ImVec2(340.0f, 620.0f);
    case LandIndex::PERU: return ImVec2(345.0f, 750.0f);
    case LandIndex::BRAZIL: return ImVec2(440.0f, 710.0f);
    case LandIndex::ARGENTINA: return ImVec2(370.0f, 870.0f);

    case LandIndex::NORTH_AFRICA: return ImVec2(650.0f, 650.0f);
    case LandIndex::EGYPT: return ImVec2(800.0f, 560.0f);
    case LandIndex::CONGO: return ImVec2(770.0f, 770.0f);
    case LandIndex::SOUTH_AFRICA: return ImVec2(760.0f, 910.0f);
    case LandIndex::MADAGASKAR: return ImVec2(900.0f, 900.0f);
    case LandIndex::EAST_AFRICA: return ImVec2(835.0f, 690.0f);

    case LandIndex::ICELAND: return ImVec2(655.0f, 235.0f);
    case LandIndex::GREAT_BRITAIN: return ImVec2(650.0f, 335.0f);
    case LandIndex::SCANDINAVIA: return ImVec2(750.0f, 240.0f);
    case LandIndex::UKRAINE: return ImVec2(870.0f, 300.0f);
    case LandIndex::NORTHERN_EUROPE: return ImVec2(750.0f, 350.0f);
    case LandIndex::WESTERN_EUROPE: return ImVec2(650.0f, 450.0f);
    case LandIndex::SOUTHERN_EUROPE: return ImVec2(760.0f, 420.0f);

    case LandIndex::URAL: return ImVec2(1040.0f, 270.0f);
    case LandIndex::AFGHANISTAN: return ImVec2(990.0f, 390.0f);
    case LandIndex::MIDDLE_EAST: return ImVec2(910.0f, 510.0f);
    case LandIndex::INDIA: return ImVec2(1060.0f, 550.0f);
    case LandIndex::SIBERIA: return ImVec2(1130.0f, 225.0f);
    case LandIndex::YAKUTSK: return ImVec2(1310.0f, 200.0f);
    case LandIndex::KAMCHATKA: return ImVec2(1450.0f, 210.0f);
    case LandIndex::IRKUTSK: return ImVec2(1260.0f, 300.0f);
    case LandIndex::JAPAN: return ImVec2(1400.0f, 430.0f);
    case LandIndex::MONGOLIA: return ImVec2(1260.0f, 380.0f);
    case LandIndex::CHINA: return ImVec2(1200.0f, 450.0f);
    case LandIndex::SIAM: return ImVec2(1200.0f, 580.0f);

    case LandIndex::INDONESIA: return ImVec2(1220.0f, 720.0f);
    case LandIndex::NEW_GUINEA: return ImVec2(1410.0f, 740.0f);
    case LandIndex::WESTERN_AUSTRALIA: return ImVec2(1220.0f, 890.0f);
    case LandIndex::EASTERN_AUSTRALIA: return ImVec2(1330.0f, 840.0f);
    }

    return ImVec2();
}



// Helper functions

bool BoardGui::LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

bool BoardGui::CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void BoardGui::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void BoardGui::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void BoardGui::CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}


// Win32 message handler
LRESULT WINAPI BoardGui::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

#endif