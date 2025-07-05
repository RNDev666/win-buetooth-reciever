#pragma once
#include <windows.h>
#include <d3d11.h>
#include <functional>
#include <memory>

namespace BluetoothAudio {

class ImGuiApp {
public:
    ImGuiApp();
    ~ImGuiApp();
    
    bool Initialize(HWND hwnd);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    void Render();
    
    bool IsInitialized() const { return m_initialized; }
    
    // Event handling
    bool HandleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Style and theming
    void SetDarkTheme();
    void SetLightTheme();
    void ApplyCustomStyle();
    
private:
    bool CreateDeviceD3D(HWND hwnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    
    // DirectX 11 objects
    ID3D11Device* m_pd3dDevice = nullptr;
    ID3D11DeviceContext* m_pd3dDeviceContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    ID3D11RenderTargetView* m_mainRenderTargetView = nullptr;
    
    bool m_initialized = false;
    HWND m_hwnd = nullptr;
};

} // namespace BluetoothAudio 
