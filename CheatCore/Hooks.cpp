#include "Hooks.h"
#include "Menu.h"
#include "ESP.h"
#include "Aimbot.h"
#include "LuaExecutor.h"
#include "Signatures.h"
#include "../Common/Logger.hpp"
#include <MinHook.h>
#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include "../External/imgui/backends/imgui_impl_win32.h"
#include "../External/imgui/backends/imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace Hooks {

    using Present_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffers_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    using WndProc_t = LRESULT(__stdcall*)(HWND, UINT, WPARAM, LPARAM);

    static Present_t oPresent = nullptr;
    static ResizeBuffers_t oResizeBuffers = nullptr;
    static WndProc_t oWndProc = nullptr;

    static HWND g_hwnd = nullptr;
    static ID3D11Device* g_device = nullptr;
    static ID3D11DeviceContext* g_context = nullptr;
    static ID3D11RenderTargetView* g_rtv = nullptr;
    static bool g_imguiInit = false;

    static void CreateRTV(IDXGISwapChain* sc) {
        ID3D11Texture2D* backBuffer = nullptr;
        sc->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (backBuffer) {
            g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
            backBuffer->Release();
        }
    }

    LRESULT __stdcall HookWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
        if (g_imguiInit && ImGui_ImplWin32_WndProcHandler(hwnd, msg, w, l))
            return true;
        return CallWindowProcW(oWndProc, hwnd, msg, w, l);
    }

    HRESULT __stdcall HookResizeBuffers(IDXGISwapChain* sc, UINT bc, UINT w, UINT h,
                                        DXGI_FORMAT fmt, UINT flags) {
        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
        HRESULT hr = oResizeBuffers(sc, bc, w, h, fmt, flags);
        if (SUCCEEDED(hr) && g_device) CreateRTV(sc);
        return hr;
    }

    HRESULT __stdcall HookPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
        if (!g_imguiInit) {
            if (SUCCEEDED(sc->GetDevice(IID_PPV_ARGS(&g_device)))) {
                g_device->GetImmediateContext(&g_context);
                DXGI_SWAP_CHAIN_DESC desc{};
                sc->GetDesc(&desc);
                g_hwnd = desc.OutputWindow;

                CreateRTV(sc);

                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                ImGui::StyleColorsDark();

                ImGui_ImplWin32_Init(g_hwnd);
                ImGui_ImplDX11_Init(g_device, g_context);

                oWndProc = reinterpret_cast<WndProc_t>(
                    SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC,
                        reinterpret_cast<LONG_PTR>(HookWndProc)));

                g_imguiInit = true;
            }
        }

        if (g_imguiInit) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            Menu::Render();
            ESP::Render();
            Aimbot::Update();

            ImGui::Render();
            g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        return oPresent(sc, sync, flags);
    }

    static bool GetD3D11VTable(void** present, void** resize) {
        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferCount = 1;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = GetForegroundWindow();
        desc.SampleDesc.Count = 1;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* dev = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        IDXGISwapChain* sc = nullptr;
        D3D_FEATURE_LEVEL lvl;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &desc, &sc, &dev, &lvl, &ctx);

        if (FAILED(hr)) return false;

        void** vt = *reinterpret_cast<void***>(sc);
        *present = vt[8];
        *resize = vt[13];

        sc->Release();
        dev->Release();
        ctx->Release();
        return true;
    }

    bool Init() {
        if (MH_Initialize() != MH_OK) {
            Logger::Error("MinHook init failed");
            return false;
        }

        void* present = nullptr;
        void* resize = nullptr;
        if (!GetD3D11VTable(&present, &resize)) {
            Logger::Error("Failed to get D3D11 vtable");
            return false;
        }

        MH_CreateHook(present, &HookPresent, reinterpret_cast<void**>(&oPresent));
        MH_CreateHook(resize, &HookResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers));
        MH_EnableHook(MH_ALL_HOOKS);

        if (!Signatures::Init()) {
            Logger::Warn("Signatures not fully resolved — features may be disabled");
        }

        LuaExecutor::Init();

        Logger::Info("Hooks initialized");
        return true;
    }

    void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();

        if (g_imguiInit) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            if (oWndProc && g_hwnd)
                SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));
        }

        if (g_rtv) g_rtv->Release();
        if (g_context) g_context->Release();
        if (g_device) g_device->Release();
    }
}