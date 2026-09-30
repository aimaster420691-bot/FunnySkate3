#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>

using SetFlagByNameFn = bool (*)(std::string_view, std::string_view);

static HMODULE g_runtime = nullptr;
static SetFlagByNameFn g_set = nullptr;
static std::atomic<bool> g_running{true};
static std::atomic<bool> g_menu{false};
static std::atomic<int> g_preset{0};
static HANDLE g_thread = nullptr;
static HWND g_window = nullptr;

static constexpr const char* kSetName =
    "?SetFlagByName@cvar@rex@@YA_NV?$basic_string_view@DU?$char_traits@D@std@@@std@@0@Z";

bool SetCvar(const char* name, const char* value) {
    if (!g_set) return false;
    return g_set(std::string_view(name), std::string_view(value));
}

void SetBool(const char* n, bool v) { SetCvar(n, v ? "true" : "false"); }
void SetDouble(const char* n, double v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.6g", v);
    SetCvar(n, b);
}
void SetInt(const char* n, int v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%d", v);
    SetCvar(n, b);
}

void Restore() {
    SetDouble("skate3_field_of_view", 60.0);
    SetBool("skate3_native_render_scene_freecam", false);
    SetBool("skate3_native_render_scene_freecam_capture_input", true);
    SetDouble("skate3_native_render_scene_freecam_speed", 8.0);
    SetDouble("skate3_native_render_scene_freecam_look_speed", 90.0);
    SetBool("skate3_native_render_scene_sun_override", false);
    SetDouble("skate3_native_render_scene_sun_azimuth", 220.0);
    SetDouble("skate3_native_render_scene_sun_elevation", 35.0);
    SetBool("skate3_native_render_scene_bloom", true);
    SetBool("skate3_native_render_scene_shafts", true);
    SetBool("skate3_native_render_scene_haze", true);
    SetBool("skate3_native_render_scene_ssr", false);
}

void Moonlight() {
    SetBool("skate3_native_render_scene_sun_override", true);
    SetDouble("skate3_native_render_scene_sun_azimuth", 30.0);
    SetDouble("skate3_native_render_scene_sun_elevation", 8.0);
    SetBool("skate3_native_render_scene_shafts", true);
    SetDouble("skate3_native_render_scene_shafts_intensity", 1.2);
    SetBool("skate3_native_render_scene_haze", true);
    SetDouble("skate3_native_render_scene_haze_intensity", 0.25);
    SetBool("skate3_native_render_scene_bloom", true);
    SetDouble("skate3_native_render_scene_bloom_intensity", 0.08);
}

void Chaos() {
    SetBool("skate3_native_render_scene_ssr", true);
    SetDouble("skate3_native_render_scene_ssr_intensity", 2.0);
    SetBool("skate3_native_render_scene_bloom", true);
    SetDouble("skate3_native_render_scene_bloom_intensity", 0.25);
    SetDouble("skate3_native_render_scene_bloom_threshold", 0.15);
    SetBool("skate3_native_render_scene_haze", true);
    SetDouble("skate3_native_render_scene_haze_intensity", 0.5);
    SetDouble("skate3_native_render_scene_haze_density", 0.02);
    SetBool("skate3_native_render_scene_ssao", true);
    SetDouble("skate3_native_render_scene_ssao_intensity", 3.5);
}

void SuperFov() {
    SetDouble("skate3_field_of_view", 115.0);
}

void Freecam() {
    bool current = false;
    // We don't call Query here because the trainer only needs a toggle. Flip state locally.
    static bool enabled = false;
    enabled = !enabled;
    SetBool("skate3_native_render_scene_freecam", enabled);
    SetBool("skate3_native_render_scene_freecam_capture_input", enabled);
}

void DrawTextLine(HDC dc, int x, int y, const char* s) {
    SetTextColor(dc, RGB(240,240,240));
    SetBkMode(dc, TRANSPARENT);
    TextOutA(dc, x, y, s, (int)std::strlen(s));
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(h, &ps);
        RECT r{};
        GetClientRect(h, &r);
        HBRUSH bg = CreateSolidBrush(RGB(18,18,22));
        FillRect(dc, &r, bg);
        DeleteObject(bg);

        DrawTextLine(dc, 18, 16, "SKATE 3 FUN TRAINER - v2.0.2");
        DrawTextLine(dc, 18, 48, "F3  Moonlight");
        DrawTextLine(dc, 18, 72, "F4  Chaos visuals");
        DrawTextLine(dc, 18, 96, "F5  Super FOV");
        DrawTextLine(dc, 18, 120, "F6  Drone / freecam");
        DrawTextLine(dc, 18, 144, "F7  Restore");
        DrawTextLine(dc, 18, 178, "F2  Hide menu");
        DrawTextLine(dc, 18, 214, "Gameplay physics cheats are not patched by this build.");
        DrawTextLine(dc, 18, 232, "Use the source-build trainer for player-state hooks.");

        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_CLOSE) {
        ShowWindow(h, SW_HIDE);
        g_menu = false;
        return 0;
    }
    return DefWindowProcA(h,m,w,l);
}

void ShowTrainerWindow() {
    if (!g_window) return;
    ShowWindow(g_window, SW_SHOWNOACTIVATE);
    SetWindowPos(g_window, HWND_TOPMOST, 40, 40, 430, 275,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    g_menu = true;
}

void HideTrainerWindow() {
    if (g_window) ShowWindow(g_window, SW_HIDE);
    g_menu = false;
}

DWORD WINAPI TrainerThread(void*) {
    g_runtime = GetModuleHandleW(L"rexruntime.dll");
    if (!g_runtime) {
        for (int i=0; i<100 && !g_runtime; ++i) {
            Sleep(100);
            g_runtime = GetModuleHandleW(L"rexruntime.dll");
        }
    }
    if (g_runtime) {
        g_set = reinterpret_cast<SetFlagByNameFn>(GetProcAddress(g_runtime, kSetName));
    }

    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "Skate3FunTrainerWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);

    g_window = CreateWindowExA(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        wc.lpszClassName,
        "Skate 3 Fun Trainer",
        WS_POPUP | WS_BORDER,
        40, 40, 430, 275,
        nullptr, nullptr, wc.hInstance, nullptr);

    while (g_running) {
        MSG msg{};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        static bool p2=false,p3=false,p4=false,p5=false,p6=false,p7=false;
        bool f2 = (GetAsyncKeyState(VK_F2)&0x8000)!=0;
        bool f3 = (GetAsyncKeyState(VK_F3)&0x8000)!=0;
        bool f4 = (GetAsyncKeyState(VK_F4)&0x8000)!=0;
        bool f5 = (GetAsyncKeyState(VK_F5)&0x8000)!=0;
        bool f6 = (GetAsyncKeyState(VK_F6)&0x8000)!=0;
        bool f7 = (GetAsyncKeyState(VK_F7)&0x8000)!=0;

        if (f2 && !p2) g_menu ? HideTrainerWindow() : ShowTrainerWindow();
        if (f3 && !p3) Moonlight();
        if (f4 && !p4) Chaos();
        if (f5 && !p5) SuperFov();
        if (f6 && !p6) Freecam();
        if (f7 && !p7) Restore();

        p2=f2;p3=f3;p4=f4;p5=f5;p6=f6;p7=f7;
        Sleep(25);
    }

    HideTrainerWindow();
    return 0;
}

extern "C" __declspec(dllexport) void Skate3TrainerStop() {
    g_running = false;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        g_thread = CreateThread(nullptr, 0, TrainerThread, nullptr, 0, nullptr);
    } else if (reason == DLL_PROCESS_DETACH) {
        g_running = false;
    }
    return TRUE;
}
