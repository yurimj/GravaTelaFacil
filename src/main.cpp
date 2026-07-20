#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tlhelp32.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace fs = std::filesystem;

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

constexpr int ID_RECORD = 1001;
constexpr int ID_SIZE = 1002;
constexpr int ID_SOUND = 1003;
constexpr int ID_OPEN = 1004;
constexpr int ID_PAUSE = 1005;
constexpr int ID_BROWSE_OUTPUT = 1006;
constexpr int ID_TIMER_RECORD = 2001;

constexpr int ID_SIZE_FREE = 3001;
constexpr int ID_SIZE_9X16 = 3002;
constexpr int ID_SIZE_16X9 = 3003;
constexpr int ID_SOUND_SYSTEM = 4001;
constexpr int ID_SOUND_NO_MIC = 4002;
constexpr int ID_SOUND_NO_AUDIO = 4003;
constexpr int ID_MIC_BASE = 4100;
constexpr COLORREF OVERLAY_TRANSPARENT_COLOR = RGB(255, 0, 255);

enum class SizeMode { Free, Vertical916, Horizontal169 };
enum class DragMode { None, Move, N, S, E, W, NE, NW, SE, SW };

struct MicDevice {
    std::wstring name;
    std::wstring id;
};

struct AudioDevice {
    std::wstring name;
    std::wstring id;
};

struct FfmpegCapabilities {
    bool hasGdiGrab = false;
    bool hasWasapi = false;
    bool hasDshow = false;
    bool hasAac = false;
    bool hasH264 = false;
    std::wstring bestVideoEncoder = L"libx264";
};

struct AppState {
    HINSTANCE instance{};
    HWND mainWindow{};
    HWND overlayWindow{};
    HWND recordButton{};
    HWND sizeButton{};
    HWND soundButton{};
    HWND openButton{};
    HWND pauseButton{};
    HWND browseButton{};
    HFONT titleFont{};
    HFONT uiFont{};
    HICON iconSmall{};
    HICON iconLarge{};
    SizeMode sizeMode = SizeMode::Free;
    bool recordSystemAudio = true;
    bool noAudio = false;
    bool paused = false;
    ULONGLONG recordStartTick = 0;
    ULONGLONG pauseStartTick = 0;
    ULONGLONG pausedTotalMs = 0;
    int selectedMic = -1;
    std::vector<MicDevice> microphones;
    AudioDevice defaultRenderDevice{};
    fs::path outputDirectoryOverride;
    RECT captureRect{ 160, 120, 1120, 660 };
    bool recording = false;
    PROCESS_INFORMATION recorderProcess{};
    HANDLE recorderStdinWrite{};
    HANDLE recorderNullOutput{};
    std::wstring lastOutputPath;
    fs::path tempVideoPath;
    fs::path tempSystemAudioPath;
    fs::path tempMicAudioPath;
    std::thread systemAudioThread;
    std::thread micAudioThread;
    std::atomic_bool stopSystemAudio{ false };
    std::atomic_bool stopMicAudio{ false };
    std::atomic_bool pauseAudio{ false };
    bool systemAudioActive = false;
    bool micAudioActive = false;
    DragMode dragMode = DragMode::None;
    POINT dragStart{};
    RECT dragOriginal{};
};

AppState g_app;

std::wstring GetLastErrorMessage(DWORD error = GetLastError()) {
    if (error == 0) return L"";
    wchar_t* buffer = nullptr;
    DWORD size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    std::wstring message(buffer, size);
    LocalFree(buffer);
    return message;
}

void ShowError(HWND parent, const std::wstring& message) {
    MessageBoxW(parent, message.c_str(), L"GravaTelaFacil", MB_ICONERROR | MB_OK);
}

std::wstring Quote(const std::wstring& text) {
    std::wstring quoted = L"\"";
    for (wchar_t c : text) {
        if (c == L'"') quoted += L'\\';
        quoted += c;
    }
    quoted += L"\"";
    return quoted;
}

fs::path OutputDirectory() {
    if (!g_app.outputDirectoryOverride.empty()) {
        return g_app.outputDirectoryOverride;
    }
    PWSTR videos = nullptr;
    fs::path result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Videos, 0, nullptr, &videos))) {
        result = fs::path(videos) / L"GTFacil";
        CoTaskMemFree(videos);
    } else {
        wchar_t profile[MAX_PATH]{};
        GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH);
        result = fs::path(profile) / L"Videos" / L"GTFacil";
    }
    return result;
}

bool EnsureOutputDirectory(HWND parent) {
    try {
        fs::create_directories(OutputDirectory());
        return true;
    } catch (const std::exception&) {
        ShowError(parent, L"Nao foi possivel criar a pasta de gravacao. Verifique as permissoes do usuario.");
        return false;
    }
}

bool ChooseOutputDirectory(HWND hwnd) {
    IFileDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        return false;
    }
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Escolha a pasta para salvar as gravacoes");

    fs::path current = OutputDirectory();
    IShellItem* currentItem = nullptr;
    if (SUCCEEDED(SHCreateItemFromParsingName(current.c_str(), nullptr, IID_PPV_ARGS(&currentItem)))) {
        dialog->SetFolder(currentItem);
        currentItem->Release();
    }

    bool changed = false;
    if (SUCCEEDED(dialog->Show(hwnd))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                g_app.outputDirectoryOverride = fs::path(path);
                CoTaskMemFree(path);
                EnsureOutputDirectory(hwnd);
                changed = true;
            }
            item->Release();
        }
    }
    dialog->Release();
    return changed;
}

std::wstring TimestampFileName() {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t buffer[64]{};
    swprintf_s(buffer, L"GTFacil_%04u-%02u-%02u_%02u-%02u-%02u.mp4",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    return buffer;
}

HICON CreateAppIcon(int size) {
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP color = CreateCompatibleBitmap(screen, size, size);
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    HGDIOBJ old = SelectObject(dc, color);

    RECT rc{ 0, 0, size, size };
    HBRUSH bg = CreateSolidBrush(RGB(246, 248, 251));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    HPEN green = CreatePen(PS_SOLID, (std::max)(2, size / 8), RGB(77, 190, 42));
    HGDIOBJ oldPen = SelectObject(dc, green);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    int pad = (std::max)(2, size / 6);
    Rectangle(dc, pad, pad, size - pad, size - pad);

    HPEN red = CreatePen(PS_SOLID, (std::max)(1, size / 12), RGB(220, 38, 38));
    SelectObject(dc, red);
    MoveToEx(dc, size / 2, size / 4, nullptr);
    LineTo(dc, size / 2, size * 3 / 4);
    MoveToEx(dc, size / 4, size / 2, nullptr);
    LineTo(dc, size * 3 / 4, size / 2);

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    SelectObject(dc, old);
    DeleteObject(green);
    DeleteObject(red);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);

    ICONINFO info{};
    info.fIcon = TRUE;
    info.hbmColor = color;
    info.hbmMask = mask;
    HICON icon = CreateIconIndirect(&info);
    DeleteObject(color);
    DeleteObject(mask);
    return icon;
}

std::wstring FormatElapsed() {
    if (!g_app.recording || g_app.recordStartTick == 0) return L"00:00:00";
    ULONGLONG now = g_app.paused ? g_app.pauseStartTick : GetTickCount64();
    ULONGLONG elapsed = now > g_app.recordStartTick ? now - g_app.recordStartTick : 0;
    elapsed = elapsed > g_app.pausedTotalMs ? elapsed - g_app.pausedTotalMs : 0;
    unsigned long long total = elapsed / 1000;
    unsigned long long h = total / 3600;
    unsigned long long m = (total / 60) % 60;
    unsigned long long s = total % 60;
    wchar_t buffer[32]{};
    swprintf_s(buffer, L"%02llu:%02llu:%02llu", h, m, s);
    return buffer;
}

fs::path AppDirectory() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return fs::path(path).parent_path();
}

std::wstring FindFfmpeg() {
    fs::path bundled = AppDirectory() / L"tools" / L"ffmpeg.exe";
    if (fs::exists(bundled)) return bundled.wstring();

    wchar_t resolved[MAX_PATH]{};
    if (SearchPathW(nullptr, L"ffmpeg.exe", nullptr, MAX_PATH, resolved, nullptr) > 0) {
        return resolved;
    }
    return L"";
}

std::string ReadTextFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

FfmpegCapabilities DetectFfmpegCapabilities(const std::wstring& ffmpeg) {
    FfmpegCapabilities caps{};
    wchar_t tempPath[MAX_PATH]{};
    wchar_t devicesFile[MAX_PATH]{};
    wchar_t encodersFile[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, tempPath) ||
        !GetTempFileNameW(tempPath, L"gtd", 0, devicesFile) ||
        !GetTempFileNameW(tempPath, L"gte", 0, encodersFile)) {
        return caps;
    }

    auto runToFile = [&](const std::wstring& args, const wchar_t* outputPath) {
        HANDLE output = CreateFileW(outputPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (output == INVALID_HANDLE_VALUE) {
            return;
        }

        SetHandleInformation(output, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
        std::wstring command = Quote(ffmpeg) + L" " + args;
        std::vector<wchar_t> mutableCommand(command.begin(), command.end());
        mutableCommand.push_back(L'\0');

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = output;
        si.hStdError = output;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        PROCESS_INFORMATION pi{};

        if (CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
            nullptr, nullptr, &si, &pi)) {
            if (WaitForSingleObject(pi.hProcess, 3000) == WAIT_TIMEOUT) {
                TerminateProcess(pi.hProcess, 1);
                WaitForSingleObject(pi.hProcess, 1000);
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        CloseHandle(output);
    };

    runToFile(L"-hide_banner -devices", devicesFile);
    runToFile(L"-hide_banner -encoders", encodersFile);

    std::string devices = ReadTextFile(devicesFile);
    std::string encoders = ReadTextFile(encodersFile);
    DeleteFileW(devicesFile);
    DeleteFileW(encodersFile);

    caps.hasGdiGrab = devices.find("gdigrab") != std::string::npos;
    caps.hasWasapi = devices.find("wasapi") != std::string::npos;
    caps.hasDshow = devices.find("dshow") != std::string::npos;
    caps.hasAac = encoders.find("aac") != std::string::npos;
    caps.hasH264 = encoders.find("libx264") != std::string::npos ||
        encoders.find("h264_nvenc") != std::string::npos ||
        encoders.find("h264_qsv") != std::string::npos ||
        encoders.find("h264_amf") != std::string::npos;

    if (encoders.find("libx264") != std::string::npos) caps.bestVideoEncoder = L"libx264";
    else if (encoders.find("h264_nvenc") != std::string::npos) caps.bestVideoEncoder = L"h264_nvenc";
    else if (encoders.find("h264_qsv") != std::string::npos) caps.bestVideoEncoder = L"h264_qsv";
    else if (encoders.find("h264_amf") != std::string::npos) caps.bestVideoEncoder = L"h264_amf";
    return caps;
}

RECT VirtualScreenRect() {
    RECT rect{};
    rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return rect;
}

void ClampCaptureRect() {
    RECT screen = VirtualScreenRect();
    int minW = 160;
    int minH = 120;
    if (g_app.captureRect.right - g_app.captureRect.left < minW) g_app.captureRect.right = g_app.captureRect.left + minW;
    if (g_app.captureRect.bottom - g_app.captureRect.top < minH) g_app.captureRect.bottom = g_app.captureRect.top + minH;
    if (g_app.captureRect.left < screen.left) OffsetRect(&g_app.captureRect, screen.left - g_app.captureRect.left, 0);
    if (g_app.captureRect.top < screen.top) OffsetRect(&g_app.captureRect, 0, screen.top - g_app.captureRect.top);
    if (g_app.captureRect.right > screen.right) OffsetRect(&g_app.captureRect, screen.right - g_app.captureRect.right, 0);
    if (g_app.captureRect.bottom > screen.bottom) OffsetRect(&g_app.captureRect, 0, screen.bottom - g_app.captureRect.bottom);
}

void RefreshOverlayPosition() {
    if (!g_app.overlayWindow) return;
    ClampCaptureRect();
    SetWindowPos(g_app.overlayWindow, HWND_TOPMOST, g_app.captureRect.left, g_app.captureRect.top,
        g_app.captureRect.right - g_app.captureRect.left, g_app.captureRect.bottom - g_app.captureRect.top,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
    if (g_app.mainWindow) {
        SetWindowPos(g_app.mainWindow, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
    InvalidateRect(g_app.overlayWindow, nullptr, TRUE);
}

void ExcludeWindowFromCapture(HWND hwnd) {
    if (!hwnd) return;
    // Temporarily disabled for demo screenshots. Re-enable before release recording validation.
#if 0
    if (!SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE)) {
        SetWindowDisplayAffinity(hwnd, WDA_MONITOR);
    }
#endif
}

void ApplyAspect(SizeMode mode) {
    g_app.sizeMode = mode;
    RECT screen = VirtualScreenRect();
    int screenW = screen.right - screen.left;
    int screenH = screen.bottom - screen.top;
    int height = static_cast<int>(screenH * 0.72);
    int width = static_cast<int>(height * (mode == SizeMode::Vertical916 ? 9.0 / 16.0 : 16.0 / 9.0));
    if (width > static_cast<int>(screenW * 0.82)) {
        width = static_cast<int>(screenW * 0.82);
        height = static_cast<int>(width * (mode == SizeMode::Vertical916 ? 16.0 / 9.0 : 9.0 / 16.0));
    }
    int cx = screen.left + screenW / 2;
    int cy = screen.top + screenH / 2;
    g_app.captureRect = { cx - width / 2, cy - height / 2, cx + width / 2, cy + height / 2 };
    RefreshOverlayPosition();
}

double SelectedAspectRatio() {
    if (g_app.sizeMode == SizeMode::Vertical916) return 9.0 / 16.0;
    if (g_app.sizeMode == SizeMode::Horizontal169) return 16.0 / 9.0;
    return 0.0;
}

void KeepAspectForResize(RECT& r, DragMode mode) {
    double aspect = SelectedAspectRatio();
    if (aspect <= 0.0 || mode == DragMode::Move || mode == DragMode::None) return;

    int minW = 160;
    int minH = 120;
    int width = (std::max)(minW, static_cast<int>(r.right - r.left));
    int height = (std::max)(minH, static_cast<int>(r.bottom - r.top));

    bool horizontal = mode == DragMode::E || mode == DragMode::W || mode == DragMode::NE || mode == DragMode::NW || mode == DragMode::SE || mode == DragMode::SW;
    if (horizontal) {
        height = (std::max)(minH, static_cast<int>(width / aspect));
    } else {
        width = (std::max)(minW, static_cast<int>(height * aspect));
    }

    switch (mode) {
    case DragMode::N:
        r.top = r.bottom - height;
        r.right = r.left + width;
        break;
    case DragMode::S:
        r.bottom = r.top + height;
        r.right = r.left + width;
        break;
    case DragMode::E:
        r.right = r.left + width;
        r.bottom = r.top + height;
        break;
    case DragMode::W:
        r.left = r.right - width;
        r.bottom = r.top + height;
        break;
    case DragMode::NE:
        r.right = r.left + width;
        r.top = r.bottom - height;
        break;
    case DragMode::NW:
        r.left = r.right - width;
        r.top = r.bottom - height;
        break;
    case DragMode::SE:
        r.right = r.left + width;
        r.bottom = r.top + height;
        break;
    case DragMode::SW:
        r.left = r.right - width;
        r.bottom = r.top + height;
        break;
    default:
        break;
    }
}

std::wstring GetDeviceName(IMMDevice* device) {
    IPropertyStore* props = nullptr;
    PROPVARIANT name;
    PropVariantInit(&name);
    std::wstring result;
    if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props)) &&
        SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &name)) &&
        name.vt == VT_LPWSTR && name.pwszVal) {
        result = name.pwszVal;
    }
    PropVariantClear(&name);
    if (props) props->Release();
    return result;
}

std::vector<AudioDevice> EnumerateAudioDevices(EDataFlow flow) {
    std::vector<AudioDevice> devices;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDeviceCollection* collection = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) return devices;
    if (SUCCEEDED(enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection))) {
        UINT count = 0;
        collection->GetCount(&count);
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* device = nullptr;
            LPWSTR id = nullptr;
            if (SUCCEEDED(collection->Item(i, &device))) {
                std::wstring name = GetDeviceName(device);
                if (SUCCEEDED(device->GetId(&id))) {
                    devices.push_back({ name.empty() ? L"Dispositivo de audio" : name, id ? id : L"" });
                }
            }
            if (id) CoTaskMemFree(id);
            if (device) device->Release();
        }
        collection->Release();
    }
    enumerator->Release();
    return devices;
}

AudioDevice GetDefaultRenderDevice() {
    AudioDevice result{};
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    LPWSTR id = nullptr;
    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator))) &&
        SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
        result.name = GetDeviceName(device);
        if (SUCCEEDED(device->GetId(&id))) {
            result.id = id ? id : L"";
        }
    }
    if (id) CoTaskMemFree(id);
    if (device) device->Release();
    if (enumerator) enumerator->Release();
    return result;
}

std::vector<MicDevice> EnumerateMicrophones() {
    std::vector<MicDevice> microphones;
    for (const AudioDevice& device : EnumerateAudioDevices(eCapture)) {
        microphones.push_back({ device.name, device.id });
    }
    return microphones;
}

void RefreshMicrophones() {
    g_app.microphones = EnumerateMicrophones();
    g_app.defaultRenderDevice = GetDefaultRenderDevice();
    if (g_app.microphones.empty()) {
        g_app.selectedMic = -1;
    } else if (g_app.selectedMic < 0 || g_app.selectedMic >= static_cast<int>(g_app.microphones.size())) {
        g_app.selectedMic = 0;
    }
}

void PaintMain(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);
    HBRUSH bg = CreateSolidBrush(RGB(246, 248, 251));
    FillRect(dc, &client, bg);
    DeleteObject(bg);

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, g_app.titleFont);
    SetTextColor(dc, RGB(28, 35, 45));
    RECT title{ 28, 22, client.right - 28, 58 };
    DrawTextW(dc, L"GravaTelaFacil", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, g_app.uiFont);
    SetTextColor(dc, RGB(90, 101, 116));
    std::wstring status = g_app.recording ? (g_app.paused ? L"Pausado. Clique em Retomar para continuar no mesmo arquivo." : L"Gravando agora. A moldura vermelha indica a area ativa.") :
        L"Pronto para gravar. Ajuste tamanho e som antes de iniciar.";
    RECT subtitle{ 30, 60, client.right - 30, 92 };
    DrawTextW(dc, status.c_str(), -1, &subtitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(dc, g_app.recording ? RGB(220, 38, 38) : RGB(90, 101, 116));
    std::wstring elapsed = L"Tempo: " + FormatElapsed();
    RECT timeRect{ 30, 150, 210, 178 };
    DrawTextW(dc, elapsed.c_str(), -1, &timeRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(dc, RGB(90, 101, 116));
    std::wstring folder = L"Pasta: " + OutputDirectory().wstring();
    RECT folderRect{ 30, 184, client.right - 74, 212 };
    DrawTextW(dc, folder.c_str(), -1, &folderRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    EndPaint(hwnd, &ps);
}

void StyleButton(HWND button, bool primary = false) {
    SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.uiFont), TRUE);
    if (primary) {
        SetWindowTextW(button, g_app.recording ? L"Parar" : L"Gravar");
    }
}

void LayoutMain(HWND hwnd) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int y = 104;
    int x = 28;
    int gap = 12;
    int h = 38;
    SetWindowPos(g_app.recordButton, nullptr, x, y, 100, h, SWP_NOZORDER);
    x += 100 + gap;
    SetWindowPos(g_app.pauseButton, nullptr, x, y, 100, h, SWP_NOZORDER);
    x += 100 + gap;
    SetWindowPos(g_app.sizeButton, nullptr, x, y, 100, h, SWP_NOZORDER);
    x += 100 + gap;
    SetWindowPos(g_app.soundButton, nullptr, x, y, 100, h, SWP_NOZORDER);
    x += 100 + gap;
    SetWindowPos(g_app.openButton, nullptr, x, y, 100, h, SWP_NOZORDER);
    SetWindowPos(g_app.browseButton, nullptr, rc.right - 58, 184, 30, 28, SWP_NOZORDER);
}

bool OpenOutputFolder(HWND hwnd) {
    if (!EnsureOutputDirectory(hwnd)) return false;
    HINSTANCE result = ShellExecuteW(hwnd, L"open", OutputDirectory().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

int RunSelfTestRuntime() {
    fs::path output = OutputDirectory();
    std::wstring outputText = output.wstring();
    if (outputText.find(L"Videos\\GTFacil") == std::wstring::npos &&
        outputText.find(L"Videos/GTFacil") == std::wstring::npos) {
        return 50;
    }
    if (!EnsureOutputDirectory(nullptr)) return 51;
    if (!fs::exists(output) || !fs::is_directory(output)) return 52;

    RECT screen = VirtualScreenRect();
    if (screen.right <= screen.left || screen.bottom <= screen.top) return 53;
    if (GetSystemMetrics(SM_CXVIRTUALSCREEN) <= 0 || GetSystemMetrics(SM_CYVIRTUALSCREEN) <= 0) return 54;

    DPI_AWARENESS_CONTEXT dpi = GetThreadDpiAwarenessContext();
    if (!AreDpiAwarenessContextsEqual(dpi, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) &&
        !AreDpiAwarenessContextsEqual(dpi, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE)) {
        return 55;
    }

    std::wstring ffmpeg = FindFfmpeg();
    if (ffmpeg.empty()) return 56;
    if (!PathFileExistsW(ffmpeg.c_str())) return 57;
    FfmpegCapabilities caps = DetectFfmpegCapabilities(ffmpeg);
    if (!caps.hasGdiGrab || !caps.hasH264 || !caps.hasAac || caps.bestVideoEncoder.empty()) return 58;

    HMENU sizeMenu = CreatePopupMenu();
    AppendMenuW(sizeMenu, MF_STRING, ID_SIZE_FREE, L"Selecao livre");
    AppendMenuW(sizeMenu, MF_STRING, ID_SIZE_9X16, L"9x16 (smartphone)");
    AppendMenuW(sizeMenu, MF_STRING, ID_SIZE_16X9, L"16x9");
    if (GetMenuItemCount(sizeMenu) != 3) {
        DestroyMenu(sizeMenu);
        return 59;
    }
    DestroyMenu(sizeMenu);

    return 0;
}

int RunSelfTestOpenFolder() {
    if (!EnsureOutputDirectory(nullptr)) return 60;
    if (!OpenOutputFolder(nullptr)) return 61;
    return 0;
}

void ShowSizeMenu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | (g_app.sizeMode == SizeMode::Free ? MF_CHECKED : 0), ID_SIZE_FREE, L"Selecao livre");
    AppendMenuW(menu, MF_STRING | (g_app.sizeMode == SizeMode::Vertical916 ? MF_CHECKED : 0), ID_SIZE_9X16, L"9x16 (smartphone)");
    AppendMenuW(menu, MF_STRING | (g_app.sizeMode == SizeMode::Horizontal169 ? MF_CHECKED : 0), ID_SIZE_16X9, L"16x9");
    RECT rc{};
    GetWindowRect(g_app.sizeButton, &rc);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, rc.left, rc.bottom + 4, 0, hwnd, nullptr);
    DestroyMenu(menu);
}

void ShowSoundMenu(HWND hwnd) {
    RefreshMicrophones();
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | (g_app.recordSystemAudio && !g_app.noAudio ? MF_CHECKED : 0), ID_SOUND_SYSTEM, L"Sistema de Gravacao de Audio");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    for (int i = 0; i < static_cast<int>(g_app.microphones.size()); ++i) {
        UINT flags = MF_STRING;
        if (!g_app.noAudio && g_app.selectedMic == i) flags |= MF_CHECKED;
        AppendMenuW(menu, flags, ID_MIC_BASE + i, g_app.microphones[i].name.c_str());
    }
    if (g_app.microphones.empty()) {
        AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, L"Nenhum microfone encontrado");
    }
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (!g_app.noAudio && g_app.selectedMic < 0 ? MF_CHECKED : 0), ID_SOUND_NO_MIC, L"Nao gravar Microfone");
    AppendMenuW(menu, MF_STRING | (g_app.noAudio ? MF_CHECKED : 0), ID_SOUND_NO_AUDIO, L"Nao gravar nenhum som");
    RECT rc{};
    GetWindowRect(g_app.soundButton, &rc);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, rc.left, rc.bottom + 4, 0, hwnd, nullptr);
    DestroyMenu(menu);
}

void WriteLe16(std::ofstream& out, uint16_t value) {
    out.put(static_cast<char>(value & 0xff));
    out.put(static_cast<char>((value >> 8) & 0xff));
}

void WriteLe32(std::ofstream& out, uint32_t value) {
    out.put(static_cast<char>(value & 0xff));
    out.put(static_cast<char>((value >> 8) & 0xff));
    out.put(static_cast<char>((value >> 16) & 0xff));
    out.put(static_cast<char>((value >> 24) & 0xff));
}

void WriteWavHeader(std::ofstream& out, const WAVEFORMATEX& format, uint32_t dataSize) {
    uint16_t cbSize = format.wFormatTag == WAVE_FORMAT_EXTENSIBLE ? sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) : format.cbSize;
    uint32_t fmtSize = 16 + (format.wFormatTag == WAVE_FORMAT_PCM ? 0 : sizeof(uint16_t) + cbSize);
    out.seekp(0, std::ios::beg);
    out.write("RIFF", 4);
    WriteLe32(out, 4 + 8 + fmtSize + 8 + dataSize);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    WriteLe32(out, fmtSize);
    WriteLe16(out, format.wFormatTag);
    WriteLe16(out, format.nChannels);
    WriteLe32(out, format.nSamplesPerSec);
    WriteLe32(out, format.nAvgBytesPerSec);
    WriteLe16(out, format.nBlockAlign);
    WriteLe16(out, format.wBitsPerSample);
    if (format.wFormatTag != WAVE_FORMAT_PCM) {
        WriteLe16(out, cbSize);
        if (cbSize > 0) {
            const char* extra = reinterpret_cast<const char*>(&format) + sizeof(WAVEFORMATEX);
            out.write(extra, cbSize);
        }
    }
    out.write("data", 4);
    WriteLe32(out, dataSize);
}

bool CaptureAudioToWav(const fs::path& outputPath, std::atomic_bool& stopFlag, std::atomic_bool& pauseFlag, bool loopback, const std::wstring& deviceId = L"") {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool needsCoUninit = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE) {
        needsCoUninit = false;
    } else if (FAILED(hr)) {
        return false;
    }

    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    WAVEFORMATEX* mixFormat = nullptr;
    bool ok = false;

    do {
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) break;
        if (!deviceId.empty()) {
            if (FAILED(enumerator->GetDevice(deviceId.c_str(), &device))) break;
        } else if (FAILED(enumerator->GetDefaultAudioEndpoint(loopback ? eRender : eCapture, eConsole, &device))) {
            break;
        }
        if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient)))) break;
        if (FAILED(audioClient->GetMixFormat(&mixFormat))) break;

        REFERENCE_TIME bufferDuration = 10000000;
        DWORD streamFlags = loopback ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;
        if (FAILED(audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags, bufferDuration, 0, mixFormat, nullptr))) break;
        if (FAILED(audioClient->GetService(IID_PPV_ARGS(&captureClient)))) break;

        std::ofstream wav(outputPath, std::ios::binary | std::ios::trunc);
        if (!wav) break;
        WriteWavHeader(wav, *mixFormat, 0);

        if (FAILED(audioClient->Start())) break;
        uint32_t dataSize = 0;
        while (!stopFlag.load()) {
            UINT32 packetFrames = 0;
            if (FAILED(captureClient->GetNextPacketSize(&packetFrames))) break;
            if (packetFrames == 0) {
                Sleep(10);
                continue;
            }

            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) break;
            UINT32 bytes = frames * mixFormat->nBlockAlign;
            if (pauseFlag.load()) {
                captureClient->ReleaseBuffer(frames);
                continue;
            } else if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                std::vector<char> silence(bytes, 0);
                wav.write(silence.data(), silence.size());
            } else {
                wav.write(reinterpret_cast<const char*>(data), bytes);
            }
            dataSize += bytes;
            captureClient->ReleaseBuffer(frames);
        }
        audioClient->Stop();
        WriteWavHeader(wav, *mixFormat, dataSize);
        ok = dataSize > 0;
    } while (false);

    if (mixFormat) CoTaskMemFree(mixFormat);
    if (captureClient) captureClient->Release();
    if (audioClient) audioClient->Release();
    if (device) device->Release();
    if (enumerator) enumerator->Release();
    if (needsCoUninit) CoUninitialize();
    return ok;
}

void SetProcessThreadsSuspended(DWORD processId, bool suspend) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry)) {
        do {
            if (entry.th32OwnerProcessID == processId) {
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
                if (thread) {
                    if (suspend) SuspendThread(thread);
                    else ResumeThread(thread);
                    CloseHandle(thread);
                }
            }
        } while (Thread32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
}

void TogglePauseRecording(HWND hwnd) {
    if (!g_app.recording || !g_app.recorderProcess.dwProcessId) return;
    if (!g_app.paused) {
        g_app.paused = true;
        g_app.pauseStartTick = GetTickCount64();
        g_app.pauseAudio.store(true);
        SetProcessThreadsSuspended(g_app.recorderProcess.dwProcessId, true);
        SetWindowTextW(g_app.pauseButton, L"Retomar");
    } else {
        SetProcessThreadsSuspended(g_app.recorderProcess.dwProcessId, false);
        g_app.pauseAudio.store(false);
        if (g_app.pauseStartTick) {
            g_app.pausedTotalMs += GetTickCount64() - g_app.pauseStartTick;
        }
        g_app.pauseStartTick = 0;
        g_app.paused = false;
        SetWindowTextW(g_app.pauseButton, L"Pausar");
    }
    InvalidateRect(hwnd, nullptr, TRUE);
    RefreshOverlayPosition();
}

bool RunHiddenAndWait(const std::wstring& command, DWORD timeoutMs = INFINITE) {
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    DWORD wait = WaitForSingleObject(pi.hProcess, timeoutMs);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    if (wait == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        code = 1;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return wait != WAIT_TIMEOUT && code == 0;
}

bool MuxFinalMp4(HWND hwnd) {
    if (g_app.lastOutputPath.empty() || g_app.tempVideoPath.empty()) return false;
    bool hasSystemAudio = g_app.systemAudioActive && fs::exists(g_app.tempSystemAudioPath) && fs::file_size(g_app.tempSystemAudioPath) > 44;
    bool hasMicAudio = g_app.micAudioActive && fs::exists(g_app.tempMicAudioPath) && fs::file_size(g_app.tempMicAudioPath) > 44;
    if (!hasSystemAudio && !hasMicAudio) {
        try {
            fs::rename(g_app.tempVideoPath, g_app.lastOutputPath);
            return true;
        } catch (...) {
            try {
                fs::copy_file(g_app.tempVideoPath, g_app.lastOutputPath, fs::copy_options::overwrite_existing);
                fs::remove(g_app.tempVideoPath);
                return true;
            } catch (...) {
                ShowError(hwnd, L"Nao foi possivel finalizar o arquivo de video.");
                return false;
            }
        }
    }

    std::wstring ffmpeg = FindFfmpeg();
    std::wstring command = Quote(ffmpeg) + L" -y -hide_banner -loglevel error -i " + Quote(g_app.tempVideoPath.wstring());
    if (hasSystemAudio) command += L" -i " + Quote(g_app.tempSystemAudioPath.wstring());
    if (hasMicAudio) command += L" -i " + Quote(g_app.tempMicAudioPath.wstring());

    if (hasSystemAudio && hasMicAudio) {
        command += L" -filter_complex \"[1:a]aresample=async=1:first_pts=0[a1];[2:a]aresample=async=1:first_pts=0[a2];[a1][a2]amix=inputs=2:duration=shortest[aout]\" -map 0:v -map \"[aout]\"";
    } else {
        command += L" -filter_complex \"[1:a]aresample=async=1:first_pts=0[aout]\" -map 0:v -map \"[aout]\"";
    }
    command += L" -c:v copy -c:a aac -b:a 160k -shortest -movflags +faststart " + Quote(g_app.lastOutputPath);

    bool ok = RunHiddenAndWait(command, 120000);
    if (!ok) {
        ShowError(hwnd, L"Nao foi possivel combinar o audio do PC com o video final.");
        return false;
    }
    try {
        fs::remove(g_app.tempVideoPath);
        fs::remove(g_app.tempSystemAudioPath);
        fs::remove(g_app.tempMicAudioPath);
    } catch (...) {}
    return true;
}

int RunSelfTestRecord(bool includeSystemAudio, bool includeMicrophone, const wchar_t* fileStem) {
    if (!EnsureOutputDirectory(nullptr)) return 2;
    std::wstring ffmpeg = FindFfmpeg();
    if (ffmpeg.empty()) return 3;
    FfmpegCapabilities caps = DetectFfmpegCapabilities(ffmpeg);
    if (!caps.hasGdiGrab || !caps.hasH264 || !caps.hasAac) return 4;

    RECT screen = VirtualScreenRect();
    g_app.captureRect = { screen.left, screen.top, screen.left + 640, screen.top + 360 };
    ClampCaptureRect();
    g_app.noAudio = !(includeSystemAudio || includeMicrophone);
    g_app.recordSystemAudio = includeSystemAudio;
    RefreshMicrophones();
    if (includeMicrophone && g_app.microphones.empty()) return 8;
    g_app.selectedMic = includeMicrophone ? 0 : -1;
    g_app.micAudioActive = includeMicrophone;
    g_app.systemAudioActive = includeSystemAudio;
    g_app.lastOutputPath = (OutputDirectory() / (std::wstring(fileStem) + L".mp4")).wstring();
    g_app.tempVideoPath = OutputDirectory() / (std::wstring(fileStem) + L".video.mp4");
    g_app.tempSystemAudioPath = OutputDirectory() / (std::wstring(fileStem) + L".system.wav");
    g_app.tempMicAudioPath = OutputDirectory() / (std::wstring(fileStem) + L".mic.wav");

    try {
        fs::remove(g_app.lastOutputPath);
        fs::remove(g_app.tempVideoPath);
        fs::remove(g_app.tempSystemAudioPath);
        fs::remove(g_app.tempMicAudioPath);
    } catch (...) {}

    if (includeSystemAudio) {
        g_app.stopSystemAudio.store(false);
        g_app.systemAudioThread = std::thread([path = g_app.tempSystemAudioPath]() {
            CaptureAudioToWav(path, g_app.stopSystemAudio, g_app.pauseAudio, true);
        });
    }
    if (includeMicrophone) {
        g_app.stopMicAudio.store(false);
        std::wstring micId = g_app.microphones[g_app.selectedMic].id;
        g_app.micAudioThread = std::thread([path = g_app.tempMicAudioPath, micId]() {
            CaptureAudioToWav(path, g_app.stopMicAudio, g_app.pauseAudio, false, micId);
        });
    }

    RECT r = g_app.captureRect;
    int width = (std::max)(2, static_cast<int>(r.right - r.left));
    int height = (std::max)(2, static_cast<int>(r.bottom - r.top));
    width -= width % 2;
    height -= height % 2;

    wchar_t videoInput[256]{};
    swprintf_s(videoInput, L" -f gdigrab -framerate 10 -offset_x %d -offset_y %d -video_size %dx%d -i desktop",
        r.left, r.top, width, height);
    std::wstring command = Quote(ffmpeg) + L" -y -hide_banner -loglevel error";
    command += videoInput;
    command += L" -t 3 -map 0:v -c:v " + caps.bestVideoEncoder;
    if (caps.bestVideoEncoder == L"libx264") command += L" -preset ultrafast";
    command += L" -pix_fmt yuv420p -an " + Quote(g_app.tempVideoPath.wstring());

    bool videoOk = RunHiddenAndWait(command, 30000);
    if (includeSystemAudio) {
        g_app.stopSystemAudio.store(true);
        if (g_app.systemAudioThread.joinable()) g_app.systemAudioThread.join();
    }
    if (includeMicrophone) {
        g_app.stopMicAudio.store(true);
        if (g_app.micAudioThread.joinable()) g_app.micAudioThread.join();
    }
    if (!videoOk || !fs::exists(g_app.tempVideoPath)) return 5;

    if (!fs::exists(g_app.tempSystemAudioPath) || fs::file_size(g_app.tempSystemAudioPath) <= 44) {
        g_app.systemAudioActive = false;
    }
    if (!fs::exists(g_app.tempMicAudioPath) || fs::file_size(g_app.tempMicAudioPath) <= 44) {
        g_app.micAudioActive = false;
    }
    if (includeMicrophone && !g_app.micAudioActive) return 9;
    if (!MuxFinalMp4(nullptr)) return 6;
    return fs::exists(g_app.lastOutputPath) && fs::file_size(g_app.lastOutputPath) > 0 ? 0 : 7;
}

bool NearlyAspect(const RECT& rect, double expected) {
    double width = static_cast<double>(rect.right - rect.left);
    double height = static_cast<double>(rect.bottom - rect.top);
    if (height <= 0.0) return false;
    double actual = width / height;
    return std::abs(actual - expected) < 0.03;
}

COLORREF OverlayBorderColor() {
    return g_app.recording ? RGB(220, 38, 38) : RGB(77, 190, 42);
}

int RunSelfTestLogic() {
    RECT screen = VirtualScreenRect();
    if (screen.right <= screen.left || screen.bottom <= screen.top) return 10;

    g_app.captureRect = { screen.left - 500, screen.top - 500, screen.left - 10, screen.top - 10 };
    ClampCaptureRect();
    if (g_app.captureRect.left < screen.left || g_app.captureRect.top < screen.top) return 11;
    if (g_app.captureRect.right > screen.right || g_app.captureRect.bottom > screen.bottom) return 12;
    if (g_app.captureRect.right - g_app.captureRect.left < 160) return 13;
    if (g_app.captureRect.bottom - g_app.captureRect.top < 120) return 14;

    ApplyAspect(SizeMode::Vertical916);
    if (!NearlyAspect(g_app.captureRect, 9.0 / 16.0)) return 15;
    ApplyAspect(SizeMode::Horizontal169);
    if (!NearlyAspect(g_app.captureRect, 16.0 / 9.0)) return 16;

    RefreshMicrophones();
    OutputDirectory();

    std::wstring ffmpeg = FindFfmpeg();
    if (ffmpeg.empty()) return 17;
    FfmpegCapabilities caps = DetectFfmpegCapabilities(ffmpeg);
    if (!caps.hasGdiGrab || !caps.hasH264 || !caps.hasAac) return 18;
    return 0;
}

std::wstring BuildFfmpegCommand(const fs::path& output, const FfmpegCapabilities& caps) {
    RECT r = g_app.captureRect;
    int width = (std::max)(2, static_cast<int>(r.right - r.left));
    int height = (std::max)(2, static_cast<int>(r.bottom - r.top));
    width -= width % 2;
    height -= height % 2;

    std::wstring ffmpeg = FindFfmpeg();
    std::wstring videoEncoder = caps.bestVideoEncoder;
    std::wstring cmd = Quote(ffmpeg) + L" -y -hide_banner -loglevel error";

    wchar_t videoInput[256]{};
    swprintf_s(videoInput, L" -f gdigrab -framerate 30 -offset_x %d -offset_y %d -video_size %dx%d -i desktop",
        r.left, r.top, width, height);
    cmd += videoInput;

    cmd += L" -map 0:v";

    cmd += L" -c:v " + videoEncoder;
    if (videoEncoder == L"libx264") {
        cmd += L" -preset veryfast";
    }
    cmd += L" -pix_fmt yuv420p -movflags +faststart";
    cmd += L" -an";
    cmd += L" " + Quote(output.wstring());
    return cmd;
}

bool StartRecording(HWND hwnd) {
    if (!EnsureOutputDirectory(hwnd)) return false;
    std::wstring ffmpeg = FindFfmpeg();
    if (ffmpeg.empty()) {
        ShowError(hwnd, L"FFmpeg nao foi encontrado. Coloque ffmpeg.exe em tools\\ffmpeg.exe ao lado do aplicativo ou instale-o no PATH. O instalador final devera empacotar essa dependencia.");
        return false;
    }

    FfmpegCapabilities caps = DetectFfmpegCapabilities(ffmpeg);
    if (!caps.hasGdiGrab || !caps.hasH264) {
        ShowError(hwnd, L"O FFmpeg encontrado nao possui suporte necessario para capturar tela em H.264.");
        return false;
    }
    RefreshMicrophones();
    fs::path output = OutputDirectory() / TimestampFileName();
    g_app.lastOutputPath = output.wstring();
    g_app.tempVideoPath = output;
    g_app.tempVideoPath += L".video.mp4";
    g_app.tempSystemAudioPath = output;
    g_app.tempSystemAudioPath += L".system.wav";
    g_app.tempMicAudioPath = output;
    g_app.tempMicAudioPath += L".mic.wav";
    g_app.systemAudioActive = !g_app.noAudio && g_app.recordSystemAudio;
    g_app.micAudioActive = !g_app.noAudio && g_app.selectedMic >= 0 && g_app.selectedMic < static_cast<int>(g_app.microphones.size());

    if (g_app.systemAudioActive) {
        g_app.stopSystemAudio.store(false);
        g_app.pauseAudio.store(false);
        g_app.systemAudioThread = std::thread([path = g_app.tempSystemAudioPath]() {
            CaptureAudioToWav(path, g_app.stopSystemAudio, g_app.pauseAudio, true);
        });
    }
    if (g_app.micAudioActive) {
        g_app.stopMicAudio.store(false);
        std::wstring micId = g_app.microphones[g_app.selectedMic].id;
        g_app.micAudioThread = std::thread([path = g_app.tempMicAudioPath, micId]() {
            CaptureAudioToWav(path, g_app.stopMicAudio, g_app.pauseAudio, false, micId);
        });
    }

    std::wstring command = BuildFfmpegCommand(g_app.tempVideoPath, caps);
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdinRead = nullptr;
    HANDLE stdinWrite = nullptr;
    if (!CreatePipe(&stdinRead, &stdinWrite, &sa, 0)) {
        ShowError(hwnd, L"Nao foi possivel preparar o controle de gravacao.\n\n" + GetLastErrorMessage());
        return false;
    }
    SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0);

    HANDLE nullOutput = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = stdinRead;
    si.hStdOutput = nullOutput != INVALID_HANDLE_VALUE ? nullOutput : GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = nullOutput != INVALID_HANDLE_VALUE ? nullOutput : GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(stdinRead);
        CloseHandle(stdinWrite);
        if (nullOutput != INVALID_HANDLE_VALUE) CloseHandle(nullOutput);
        if (g_app.systemAudioThread.joinable()) {
            g_app.stopSystemAudio.store(true);
            g_app.systemAudioThread.join();
        }
        if (g_app.micAudioThread.joinable()) {
            g_app.stopMicAudio.store(true);
            g_app.micAudioThread.join();
        }
        ShowError(hwnd, L"Nao foi possivel iniciar a gravacao.\n\n" + GetLastErrorMessage());
        return false;
    }
    CloseHandle(stdinRead);

    g_app.recorderProcess = pi;
    g_app.recorderStdinWrite = stdinWrite;
    g_app.recorderNullOutput = nullOutput != INVALID_HANDLE_VALUE ? nullOutput : nullptr;
    g_app.recording = true;
    g_app.paused = false;
    g_app.pauseAudio.store(false);
    g_app.recordStartTick = GetTickCount64();
    g_app.pauseStartTick = 0;
    g_app.pausedTotalMs = 0;
    SetWindowTextW(g_app.recordButton, L"Parar");
    SetWindowTextW(g_app.pauseButton, L"Pausar");
    EnableWindow(g_app.pauseButton, TRUE);
    EnableWindow(g_app.browseButton, FALSE);
    SetTimer(hwnd, ID_TIMER_RECORD, 1000, nullptr);
    RefreshOverlayPosition();
    InvalidateRect(hwnd, nullptr, TRUE);
    return true;
}

void StopRecording(HWND hwnd) {
    if (!g_app.recording) return;
    if (g_app.recorderProcess.hProcess) {
        if (g_app.paused) {
            SetProcessThreadsSuspended(g_app.recorderProcess.dwProcessId, false);
            g_app.pauseAudio.store(false);
            g_app.paused = false;
        }
        if (g_app.recorderStdinWrite) {
            DWORD written = 0;
            const char quit[] = "q\n";
            WriteFile(g_app.recorderStdinWrite, quit, sizeof(quit) - 1, &written, nullptr);
            CloseHandle(g_app.recorderStdinWrite);
            g_app.recorderStdinWrite = nullptr;
        }
        if (WaitForSingleObject(g_app.recorderProcess.hProcess, 5000) == WAIT_TIMEOUT) {
            TerminateProcess(g_app.recorderProcess.hProcess, 0);
        }
        CloseHandle(g_app.recorderProcess.hThread);
        CloseHandle(g_app.recorderProcess.hProcess);
        g_app.recorderProcess = {};
    }
    if (g_app.systemAudioThread.joinable()) {
        g_app.stopSystemAudio.store(true);
        g_app.systemAudioThread.join();
    }
    if (g_app.micAudioThread.joinable()) {
        g_app.stopMicAudio.store(true);
        g_app.micAudioThread.join();
    }
    if (g_app.recorderNullOutput) {
        CloseHandle(g_app.recorderNullOutput);
        g_app.recorderNullOutput = nullptr;
    }
    MuxFinalMp4(hwnd);
    g_app.recording = false;
    g_app.paused = false;
    g_app.pauseAudio.store(false);
    g_app.recordStartTick = 0;
    g_app.pauseStartTick = 0;
    g_app.pausedTotalMs = 0;
    SetWindowTextW(g_app.recordButton, L"Gravar");
    SetWindowTextW(g_app.pauseButton, L"Pausar");
    EnableWindow(g_app.pauseButton, FALSE);
    EnableWindow(g_app.browseButton, TRUE);
    KillTimer(hwnd, ID_TIMER_RECORD);
    RefreshOverlayPosition();
    InvalidateRect(hwnd, nullptr, TRUE);
}

DragMode HitTestOverlay(POINT pt, RECT rc) {
    int grip = 12;
    bool left = pt.x <= grip;
    bool right = pt.x >= rc.right - grip;
    bool top = pt.y <= grip;
    bool bottom = pt.y >= rc.bottom - grip;
    if (left && top) return DragMode::NW;
    if (right && top) return DragMode::NE;
    if (left && bottom) return DragMode::SW;
    if (right && bottom) return DragMode::SE;
    if (top) return DragMode::N;
    if (bottom) return DragMode::S;
    if (left) return DragMode::W;
    if (right) return DragMode::E;
    return DragMode::Move;
}

void DrawCenterMoveHandle(HDC dc, const RECT& rc) {
    int cx = rc.right / 2;
    int cy = rc.bottom / 2;
    HPEN dark = CreatePen(PS_SOLID, 2, RGB(70, 145, 36));
    HPEN light = CreatePen(PS_SOLID, 1, RGB(160, 224, 100));
    HBRUSH fill = CreateSolidBrush(RGB(226, 245, 217));
    HGDIOBJ oldPen = SelectObject(dc, dark);
    HGDIOBJ oldBrush = SelectObject(dc, fill);

    MoveToEx(dc, cx, cy - 17, nullptr); LineTo(dc, cx, cy + 17);
    MoveToEx(dc, cx - 17, cy, nullptr); LineTo(dc, cx + 17, cy);
    POINT up[] = { {cx, cy - 22}, {cx - 6, cy - 14}, {cx + 6, cy - 14} };
    POINT down[] = { {cx, cy + 22}, {cx - 6, cy + 14}, {cx + 6, cy + 14} };
    POINT left[] = { {cx - 22, cy}, {cx - 14, cy - 6}, {cx - 14, cy + 6} };
    POINT right[] = { {cx + 22, cy}, {cx + 14, cy - 6}, {cx + 14, cy + 6} };
    Polygon(dc, up, 3);
    Polygon(dc, down, 3);
    Polygon(dc, left, 3);
    Polygon(dc, right, 3);
    SelectObject(dc, light);
    Ellipse(dc, cx - 3, cy - 3, cx + 4, cy + 4);

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(fill);
    DeleteObject(light);
    DeleteObject(dark);
}

void PaintOverlay(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT rc{};
    GetClientRect(hwnd, &rc);

    HBRUSH transparent = CreateSolidBrush(OVERLAY_TRANSPARENT_COLOR);
    FillRect(dc, &rc, transparent);
    DeleteObject(transparent);

    HBRUSH hollow = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
    SelectObject(dc, hollow);
    HPEN pen = CreatePen(PS_SOLID, 4, OverlayBorderColor());
    SelectObject(dc, pen);
    Rectangle(dc, 2, 2, rc.right - 2, rc.bottom - 2);
    DeleteObject(pen);

    HBRUSH grip = CreateSolidBrush(RGB(226, 245, 217));
    HPEN gripPen = CreatePen(PS_SOLID, 1, RGB(35, 80, 30));
    SelectObject(dc, grip);
    SelectObject(dc, gripPen);
    int s = 11;
    POINT points[] = {
        { 8, 8 }, { rc.right / 2, 8 }, { rc.right - 8, 8 },
        { 8, rc.bottom / 2 }, { rc.right - 8, rc.bottom / 2 },
        { 8, rc.bottom - 8 }, { rc.right / 2, rc.bottom - 8 }, { rc.right - 8, rc.bottom - 8 }
    };
    for (POINT p : points) Rectangle(dc, p.x - s / 2, p.y - s / 2, p.x + s / 2, p.y + s / 2);
    DrawCenterMoveHandle(dc, rc);
    DeleteObject(gripPen);
    DeleteObject(grip);
    EndPaint(hwnd, &ps);
}

LPCTSTR CursorForDragMode(DragMode mode) {
    switch (mode) {
    case DragMode::N:
    case DragMode::S:
        return IDC_SIZENS;
    case DragMode::E:
    case DragMode::W:
        return IDC_SIZEWE;
    case DragMode::NE:
    case DragMode::SW:
        return IDC_SIZENESW;
    case DragMode::NW:
    case DragMode::SE:
        return IDC_SIZENWSE;
    case DragMode::Move:
    default:
        return IDC_SIZEALL;
    }
}

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND: {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        HBRUSH transparent = CreateSolidBrush(OVERLAY_TRANSPARENT_COLOR);
        FillRect(reinterpret_cast<HDC>(wParam), &rc, transparent);
        DeleteObject(transparent);
        return TRUE;
    }
    case WM_SETCURSOR: {
        POINT pt{};
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        SetCursor(LoadCursor(nullptr, CursorForDragMode(HitTestOverlay(pt, rc))));
        return TRUE;
    }
    case WM_LBUTTONDOWN: {
        if (g_app.recording) return 0;
        RECT rc{};
        GetClientRect(hwnd, &rc);
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        g_app.dragMode = HitTestOverlay(pt, rc);
        g_app.dragStart = pt;
        ClientToScreen(hwnd, &g_app.dragStart);
        g_app.dragOriginal = g_app.captureRect;
        SetCapture(hwnd);
        return 0;
    }
    case WM_MOUSEMOVE:
        if (GetCapture() == hwnd && g_app.dragMode != DragMode::None && !g_app.recording) {
            POINT now{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ClientToScreen(hwnd, &now);
            int dx = now.x - g_app.dragStart.x;
            int dy = now.y - g_app.dragStart.y;
            RECT r = g_app.dragOriginal;
            switch (g_app.dragMode) {
            case DragMode::Move: OffsetRect(&r, dx, dy); break;
            case DragMode::N: r.top += dy; break;
            case DragMode::S: r.bottom += dy; break;
            case DragMode::E: r.right += dx; break;
            case DragMode::W: r.left += dx; break;
            case DragMode::NE: r.top += dy; r.right += dx; break;
            case DragMode::NW: r.top += dy; r.left += dx; break;
            case DragMode::SE: r.bottom += dy; r.right += dx; break;
            case DragMode::SW: r.bottom += dy; r.left += dx; break;
            default: break;
            }
            KeepAspectForResize(r, g_app.dragMode);
            g_app.captureRect = r;
            RefreshOverlayPosition();
        }
        return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        g_app.dragMode = DragMode::None;
        return 0;
    case WM_PAINT:
        PaintOverlay(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_app.mainWindow = hwnd;
        g_app.uiFont = CreateFontW(18, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_app.titleFont = CreateFontW(28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_app.iconSmall = CreateAppIcon(16);
        g_app.iconLarge = CreateAppIcon(32);
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_app.iconSmall));
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_app.iconLarge));
        g_app.recordButton = CreateWindowW(L"BUTTON", L"Gravar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_RECORD)), g_app.instance, nullptr);
        g_app.pauseButton = CreateWindowW(L"BUTTON", L"Pausar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_PAUSE)), g_app.instance, nullptr);
        g_app.sizeButton = CreateWindowW(L"BUTTON", L"Tamanho", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_SIZE)), g_app.instance, nullptr);
        g_app.soundButton = CreateWindowW(L"BUTTON", L"Som", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_SOUND)), g_app.instance, nullptr);
        g_app.openButton = CreateWindowW(L"BUTTON", L"Abrir", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_OPEN)), g_app.instance, nullptr);
        g_app.browseButton = CreateWindowW(L"BUTTON", L"...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BROWSE_OUTPUT)), g_app.instance, nullptr);
        StyleButton(g_app.recordButton, true);
        StyleButton(g_app.pauseButton);
        StyleButton(g_app.sizeButton);
        StyleButton(g_app.soundButton);
        StyleButton(g_app.openButton);
        StyleButton(g_app.browseButton);
        EnableWindow(g_app.pauseButton, FALSE);
        EnsureOutputDirectory(hwnd);
        RefreshMicrophones();
        LayoutMain(hwnd);
        RefreshOverlayPosition();
        return 0;
    }
    case WM_SIZE:
        LayoutMain(hwnd);
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_RECORD) {
            if (g_app.recording) StopRecording(hwnd);
            else StartRecording(hwnd);
        } else if (id == ID_SIZE) {
            ShowSizeMenu(hwnd);
        } else if (id == ID_SOUND) {
            ShowSoundMenu(hwnd);
        } else if (id == ID_OPEN) {
            OpenOutputFolder(hwnd);
        } else if (id == ID_PAUSE) {
            TogglePauseRecording(hwnd);
        } else if (id == ID_BROWSE_OUTPUT) {
            if (!g_app.recording && ChooseOutputDirectory(hwnd)) {
                InvalidateRect(hwnd, nullptr, TRUE);
            }
        } else if (id == ID_SIZE_FREE) {
            g_app.sizeMode = SizeMode::Free;
            RefreshOverlayPosition();
        } else if (id == ID_SIZE_9X16) {
            ApplyAspect(SizeMode::Vertical916);
        } else if (id == ID_SIZE_16X9) {
            ApplyAspect(SizeMode::Horizontal169);
        } else if (id == ID_SOUND_SYSTEM) {
            g_app.noAudio = false;
            g_app.recordSystemAudio = !g_app.recordSystemAudio;
        } else if (id == ID_SOUND_NO_MIC) {
            g_app.noAudio = false;
            g_app.selectedMic = -1;
        } else if (id == ID_SOUND_NO_AUDIO) {
            g_app.noAudio = true;
            g_app.recordSystemAudio = false;
            g_app.selectedMic = -1;
        } else if (id >= ID_MIC_BASE && id < ID_MIC_BASE + 500) {
            g_app.noAudio = false;
            g_app.selectedMic = id - ID_MIC_BASE;
        }
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    case WM_TIMER:
        if (wParam == ID_TIMER_RECORD && g_app.recording && g_app.recorderProcess.hProcess) {
            if (!g_app.paused) {
                DWORD code = STILL_ACTIVE;
                GetExitCodeProcess(g_app.recorderProcess.hProcess, &code);
                if (code != STILL_ACTIVE) {
                    StopRecording(hwnd);
                    ShowError(hwnd, L"A gravacao foi encerrada pelo backend de video. Verifique se o FFmpeg instalado suporta gdigrab, H.264 e aac.");
                }
            }
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    case WM_PAINT:
        PaintMain(hwnd);
        return 0;
    case WM_DESTROY:
        StopRecording(hwnd);
        if (g_app.overlayWindow) DestroyWindow(g_app.overlayWindow);
        if (g_app.uiFont) DeleteObject(g_app.uiFont);
        if (g_app.titleFont) DeleteObject(g_app.titleFont);
        if (g_app.iconSmall) DestroyIcon(g_app.iconSmall);
        if (g_app.iconLarge) DestroyIcon(g_app.iconLarge);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

std::wstring WindowText(HWND hwnd) {
    wchar_t buffer[256]{};
    GetWindowTextW(hwnd, buffer, static_cast<int>(std::size(buffer)));
    return buffer;
}

int RunSelfTestUi(HINSTANCE instance) {
    HWND hwnd = CreateWindowExW(0, L"GravaTelaFacilMain", L"GravaTelaFacil",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 285, nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 30;

    g_app.overlayWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        L"GravaTelaFacilOverlay", L"", WS_POPUP,
        g_app.captureRect.left, g_app.captureRect.top,
        g_app.captureRect.right - g_app.captureRect.left,
        g_app.captureRect.bottom - g_app.captureRect.top,
        nullptr, nullptr, instance, nullptr);
    if (!g_app.overlayWindow) {
        DestroyWindow(hwnd);
        return 31;
    }

    SetLayeredWindowAttributes(g_app.overlayWindow, OVERLAY_TRANSPARENT_COLOR, 255, LWA_COLORKEY);
    ExcludeWindowFromCapture(g_app.overlayWindow);
    RefreshOverlayPosition();

    if (!g_app.recordButton || WindowText(g_app.recordButton) != L"Gravar") return 32;
    if (!g_app.sizeButton || WindowText(g_app.sizeButton) != L"Tamanho") return 33;
    if (!g_app.soundButton || WindowText(g_app.soundButton) != L"Som") return 34;
    if (!g_app.openButton || WindowText(g_app.openButton) != L"Abrir") return 35;
    if (!g_app.pauseButton || WindowText(g_app.pauseButton) != L"Pausar") return 48;
    if (!g_app.browseButton || WindowText(g_app.browseButton) != L"...") return 49;
    if (!g_app.iconSmall || !g_app.iconLarge) return 50;
    if (!IsWindow(g_app.overlayWindow)) return 36;
    if (CursorForDragMode(DragMode::N) != IDC_SIZENS) return 43;
    if (CursorForDragMode(DragMode::E) != IDC_SIZEWE) return 44;
    if (CursorForDragMode(DragMode::NE) != IDC_SIZENESW) return 45;
    if (CursorForDragMode(DragMode::NW) != IDC_SIZENWSE) return 46;
    DWORD affinity = 0;
    if (GetWindowDisplayAffinity(g_app.overlayWindow, &affinity) && affinity != WDA_EXCLUDEFROMCAPTURE && affinity != WDA_MONITOR) return 47;

    g_app.recording = false;
    if (OverlayBorderColor() != RGB(77, 190, 42)) return 37;
    g_app.recording = true;
    if (OverlayBorderColor() != RGB(220, 38, 38)) return 38;
    g_app.recording = false;
    SetWindowTextW(g_app.recordButton, L"Parar");
    if (WindowText(g_app.recordButton) != L"Parar") return 39;
    SetWindowTextW(g_app.recordButton, L"Gravar");

    RECT beforeMove = g_app.captureRect;
    RECT overlayClient{};
    GetClientRect(g_app.overlayWindow, &overlayClient);
    LPARAM center = MAKELPARAM((overlayClient.right - overlayClient.left) / 2, (overlayClient.bottom - overlayClient.top) / 2);
    SendMessageW(g_app.overlayWindow, WM_LBUTTONDOWN, MK_LBUTTON, center);
    SendMessageW(g_app.overlayWindow, WM_MOUSEMOVE, MK_LBUTTON,
        MAKELPARAM((overlayClient.right - overlayClient.left) / 2 + 40, (overlayClient.bottom - overlayClient.top) / 2 + 30));
    SendMessageW(g_app.overlayWindow, WM_LBUTTONUP, 0, center);
    if (g_app.captureRect.left == beforeMove.left || g_app.captureRect.top == beforeMove.top) return 40;

    RECT beforeResize = g_app.captureRect;
    GetClientRect(g_app.overlayWindow, &overlayClient);
    LPARAM bottomRight = MAKELPARAM(overlayClient.right - 4, overlayClient.bottom - 4);
    SendMessageW(g_app.overlayWindow, WM_LBUTTONDOWN, MK_LBUTTON, bottomRight);
    SendMessageW(g_app.overlayWindow, WM_MOUSEMOVE, MK_LBUTTON,
        MAKELPARAM(overlayClient.right + 30, overlayClient.bottom + 25));
    SendMessageW(g_app.overlayWindow, WM_LBUTTONUP, 0, bottomRight);
    if ((g_app.captureRect.right - g_app.captureRect.left) <= (beforeResize.right - beforeResize.left)) return 41;
    if ((g_app.captureRect.bottom - g_app.captureRect.top) <= (beforeResize.bottom - beforeResize.top)) return 42;

    DestroyWindow(hwnd);
    return 0;
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int show) {
    g_app.instance = instance;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    std::wstring commandLine = GetCommandLineW();
    if (commandLine.find(L"--self-test-record-no-audio") != std::wstring::npos) {
        int code = RunSelfTestRecord(false, false, L"GTFacil_self_test_no_audio");
        CoUninitialize();
        return code;
    }
    if (commandLine.find(L"--self-test-record-mic") != std::wstring::npos) {
        int code = RunSelfTestRecord(false, true, L"GTFacil_self_test_mic");
        CoUninitialize();
        return code;
    }
    if (commandLine.find(L"--self-test-record-mix") != std::wstring::npos) {
        int code = RunSelfTestRecord(true, true, L"GTFacil_self_test_mix");
        CoUninitialize();
        return code;
    }
    if (commandLine.find(L"--self-test-record") != std::wstring::npos) {
        int code = RunSelfTestRecord(true, false, L"GTFacil_self_test");
        CoUninitialize();
        return code;
    }
    if (commandLine.find(L"--self-test-logic") != std::wstring::npos) {
        int code = RunSelfTestLogic();
        CoUninitialize();
        return code;
    }
    if (commandLine.find(L"--self-test-runtime") != std::wstring::npos) {
        int code = RunSelfTestRuntime();
        CoUninitialize();
        return code;
    }
    if (commandLine.find(L"--self-test-open-folder") != std::wstring::npos) {
        int code = RunSelfTestOpenFolder();
        CoUninitialize();
        return code;
    }

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW mainClass{};
    mainClass.hInstance = instance;
    mainClass.lpszClassName = L"GravaTelaFacilMain";
    mainClass.lpfnWndProc = MainProc;
    mainClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    mainClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&mainClass);

    WNDCLASSW overlayClass{};
    overlayClass.hInstance = instance;
    overlayClass.lpszClassName = L"GravaTelaFacilOverlay";
    overlayClass.lpfnWndProc = OverlayProc;
    overlayClass.hCursor = LoadCursor(nullptr, IDC_SIZEALL);
    overlayClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
    RegisterClassW(&overlayClass);

    if (commandLine.find(L"--self-test-ui") != std::wstring::npos) {
        int code = RunSelfTestUi(instance);
        CoUninitialize();
        return code;
    }

    HWND hwnd = CreateWindowExW(WS_EX_TOPMOST, mainClass.lpszClassName, L"GravaTelaFacil", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 285, nullptr, nullptr, instance, nullptr);
    g_app.overlayWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED, overlayClass.lpszClassName, L"",
        WS_POPUP, g_app.captureRect.left, g_app.captureRect.top, g_app.captureRect.right - g_app.captureRect.left,
        g_app.captureRect.bottom - g_app.captureRect.top, nullptr, nullptr, instance, nullptr);
    SetLayeredWindowAttributes(g_app.overlayWindow, OVERLAY_TRANSPARENT_COLOR, 255, LWA_COLORKEY);
    ExcludeWindowFromCapture(g_app.overlayWindow);

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    ShowWindow(g_app.overlayWindow, SW_SHOWNOACTIVATE);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
