// todo:
// next match moet winnaar kleur behouden.

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <vector>
#include "Player.h"
#include "Match.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void SavePlayersToCSV(const std::vector<Player>& players, const std::string& filename) {
    fs::path p(filename);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }

    std::ofstream file(filename);

    if (!file.is_open()) {
        printf("ERROR: Could not create file %s\n", filename.c_str());
        return;
    }

    for (const auto& player : players) {
        file << player.GetName() << "\n";
    }

    file.close();
    printf("Successfully saved %zu players to %s\n", players.size(), filename.c_str());
}

void LoadPlayersFromCSV(std::vector<Player>& players, const std::string& filename) {
    std::ifstream file(filename);
    std::string line;

    if (file.is_open()) {
        players.clear();
        while (std::getline(file, line)) {
            if (!line.empty()) {
                players.emplace_back(line, 0);
            }
        }
        file.close();
    }
}

bool NameExists(const std::vector<Player>& list, const std::string& name) {
    return std::find_if(list.begin(), list.end(), [&](const Player& p) {
        return std::string(p.GetName()) == name;
    }) != list.end();
}

Player* FindPlayer(std::vector<Player>& list, const std::string& name) {
    for (auto& p : list) {
        if (std::string(p.GetName()) == name) return &p;
    }
    return nullptr;
}


// Main code
int main(int, char**)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will call AddFontDefault() to select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    // io.Fonts->AddFontFromFileTTF("assets/fonts/DroidSans.ttf");
    io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_queue_window = true;
    bool show_player_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::vector<Player> players;
    LoadPlayersFromCSV(players, "data/players.csv");
    std::vector<Player> player_queue;

    std::vector<Match> match_history;
    std::unique_ptr<Match> active_match = nullptr;
    static std::string last_winner_name = "";

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {

            ImGui::Begin("TABS Score window selecter");
            ImGui::Checkbox("Show Player Window", &show_player_window);
            ImGui::Checkbox("Show Queue Window", &show_queue_window);
            
            if (!last_winner_name.empty()){
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Previous Winner: %s", last_winner_name.c_str());
            }

            if (active_match) {
                ImGui::SeparatorText("ACTIVE MATCH");

                // Calculate window width to make buttons even
                float width = ImGui::GetContentRegionAvail().x;

                // --- RED PLAYER BLOCK ---
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));       // Dark Red
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Bright Red
                
                if (ImGui::Button(active_match->GetRed(), ImVec2(width * 0.45f, 60.0f))) {
                    active_match->RedWins();
                }
                ImGui::PopStyleColor(2);

                ImGui::SameLine();
                ImGui::SetCursorPosX(width * 0.47f);
                ImGui::Text("VS");
                ImGui::SameLine();

                // --- BLUE PLAYER BLOCK ---
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.6f, 1.0f));       // Dark Blue
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.8f, 1.0f)); // Bright Blue

                if (ImGui::Button(active_match->GetBlue(), ImVec2(width * 0.45f, 60.0f))) {
                    active_match->BlueWins();
                }
                ImGui::PopStyleColor(2);

                if (!active_match->IsActive()) {
                    last_winner_name = active_match->GetWinner();
                    player_queue.push_back(*FindPlayer(players, active_match->GetLoser()));
                    match_history.push_back(*active_match);
                    active_match.reset();
                }

                if (ImGui::Button("Cancel Match")) {
                    // Find Red Player in the master list
                    Player* pRed = FindPlayer(players, active_match->GetRed());

                    // Find Blue Player in the master list
                    Player* pBlue = FindPlayer(players, active_match->GetBlue());

                    // Insert them back at the START of the queue (index 0)
                    // We do Blue then Red so that Red ends up being the very first item
                    if (pBlue && (last_winner_name.empty() || active_match->GetRed() == last_winner_name)) {
                        player_queue.insert(player_queue.begin(), *pBlue);
                    }
                    if (pRed && (last_winner_name.empty() || active_match->GetBlue() == last_winner_name)) {
                        player_queue.insert(player_queue.begin(), *pRed);
                    }

                    // Clean up the match
                    active_match.reset();
                }
            } else if (!last_winner_name.empty()){
                if (!player_queue.empty()) {
                    if (ImGui::Button("Next Match")) {
                        active_match = std::make_unique<Match>(player_queue[0].GetName(), last_winner_name);
                        player_queue.erase(player_queue.begin());
                    }
                } else {
                    ImGui::TextDisabled("(Waiting for next challenger to join the queue...)");
                }
            } else if (player_queue.size() >= 2 && !active_match) {
                if (ImGui::Button("Start Match")) {
                    active_match = std::make_unique<Match>(player_queue[0].GetName(), player_queue[1].GetName());
                    player_queue.erase(player_queue.begin());
                    player_queue.erase(player_queue.begin());
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::Button("Start Match (Need 2 Players and no active match)");
                ImGui::EndDisabled();
            }

            ImGui::End();
        }

        if (show_player_window) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("Player Window", &show_player_window);
            if (ImGui::BeginTable("PlayerTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedFit)) {
                // 1. Setup Headers
                ImGui::TableSetupColumn("Player Name");
                ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending | ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableHeadersRow();;

            if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
                    if (sorts_specs->SpecsDirty) {
                        // This block runs ONLY when the user clicks a header
                        std::sort(players.begin(), players.end(), [&](const Player& a, const Player& b) {
                            for (int n = 0; n < sorts_specs->SpecsCount; n++)
                            {
                                const ImGuiTableColumnSortSpecs* spec = &sorts_specs->Specs[n];
                                
                                bool res = false;
                                if (spec->ColumnIndex == 0) // Sort by Name
                                    res = strcmp(a.GetName(), b.GetName()) < 0;
                                else if (spec->ColumnIndex == 1) // Sort by Score
                                    res = a.GetScore() < b.GetScore();

                                if (spec->SortDirection == ImGuiSortDirection_Descending)
                                    res = !res;
                                
                                return res;
                            }
                            return false;
                        });
                        sorts_specs->SpecsDirty = false; // Mark as "handled"
                    }
                }

                // 2. Fill Rows
                for (const auto& player : players)
                {
                    bool alreadyInQueue = NameExists(player_queue, player.GetName());

                    bool isLastWinner = (player.GetName() == last_winner_name);

                    bool alreadyInMatch = false;
                    if (active_match) {
                        if (std::string(player.GetName()) == active_match->GetRed() || 
                            std::string(player.GetName()) == active_match->GetBlue()) {
                            alreadyInMatch = true;
                        }
                    }

                    // 2. Determine if the "Add" button should be disabled
                    bool disableAdd = alreadyInQueue || alreadyInMatch || isLastWinner;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", player.GetName());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", player.GetScore());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushID(&player);
                    if (!disableAdd) {
                        if (ImGui::Button("Add to Queue")) {
                            player_queue.push_back(player);
                        }
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }

            if (ImGui::Button("New Player"))
                ImGui::OpenPopup("Create New Player");
            ImGui::SameLine();
            if (ImGui::Button("Save Players")){SavePlayersToCSV(players, "data/players.csv");}
            ImGui::SameLine();
            if (ImGui::Button("Load Players")){LoadPlayersFromCSV(players, "data/players.csv");}
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Create New Player", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char name_input[32] = "";
                bool nameExists =  NameExists(players, name_input);
                bool isInvalid = nameExists || (name_input[0] == '\0');

                if (nameExists) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "This name is already taken!");
                }

                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere();
                }

                ImGui::InputTextWithHint("##NameInput", "Enter player name...", name_input, IM_COUNTOF(name_input));


                if (isInvalid)
                    ImGui::BeginDisabled();
                
                if (ImGui::Button("Save", ImVec2(120, 0))) {
                    players.emplace_back(name_input, 0);
                    SavePlayersToCSV(players, "data/players.csv");
                    name_input[0] = '\0';
                    ImGui::CloseCurrentPopup();
                }

                if (isInvalid)
                    ImGui::EndDisabled();
                else
                    ImGui::SetItemDefaultFocus();
                
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    name_input[0] = '\0';
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::End();
        }

        if (show_queue_window) {
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("Queue Window", &show_queue_window);
            for (int n = 0; n < player_queue.size(); n++) {
                ImGui::PushID(n);
                ImGui::Button(player_queue[n].GetName());
                // Our buttons are both drag sources and drag targets here!
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    ImGui::SetDragDropPayload("DND_DEMO_CELL", &n, sizeof(int));
                    ImGui::Text("Move %s", player_queue[n].GetName());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(int));
                        int source_index = *(const int*)payload->Data;
                        int target_index = n;

                        if (source_index != target_index) {
                            Player moved_player = player_queue[source_index];
                            player_queue.erase(player_queue.begin() + source_index);
                            player_queue.insert(player_queue.begin() + target_index, moved_player);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::SameLine();
                if (ImGui::Button("Remove")) {
                    player_queue.erase(player_queue.begin() + n);
                    ImGui::PopID();
                    break;
                }

                ImGui::PopID();
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
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
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    // Disable DXGI's default Alt+Enter fullscreen behavior.
    // - You are free to leave this enabled, but it will not work properly with multiple viewports.
    // - This must be done for all windows associated to the device. Our DX11 backend does this automatically for secondary viewports that it creates.
    IDXGIFactory* pSwapChainFactory;
    if (SUCCEEDED(g_pSwapChain->GetParent(IID_PPV_ARGS(&pSwapChainFactory))))
    {
        pSwapChainFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        pSwapChainFactory->Release();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
