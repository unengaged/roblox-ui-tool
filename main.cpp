#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wrl.h>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include "webcontent.h"
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comdlg32.lib")

using namespace Microsoft::WRL;
namespace fs = std::filesystem;
using W = std::wstring;

static ComPtr<ICoreWebView2Controller> g_ctrl;
static ComPtr<ICoreWebView2> g_view;
static ComPtr<ICoreWebView2Environment> g_env;
static HWND g_hwnd;
static int g_show = SW_SHOW;
static std::map<W, fs::path> g_src;  // target id -> file the user picked

static const std::vector<W> kEmote = { L"SegmentedCircle.png", L"SegmentedCircle@2x.png", L"SegmentedCircle@3x.png" };
static const std::vector<W> kPlayer = { L"NewAvatarBackground.png", L"NewAvatarBackground@2x.png", L"NewAvatarBackground@3x.png" };
static const std::vector<W> kCursor = { L"ArrowCursor.png", L"ArrowFarCursor.png", L"IBeamCursor.png" };
static const std::vector<W> kCursorShift = { L"MouseLockedCursor.png"};
static const wchar_t* kEmoteRel = L"content\\textures\\ui\\Emotes\\Large";
static const wchar_t* kPlayerRel = L"content\\textures\\ui\\PlayerList";
static const wchar_t* kCursorRel = L"content\\textures\\Cursors\\KeyboardMouse";
static const wchar_t* kCursorShiftRel = L"content\\textures";

static fs::path LAD() {
    static fs::path p = [] { PWSTR s = nullptr; fs::path r;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &s))) r = s;
    CoTaskMemFree(s); return r; }();
    return p;
}
static fs::path Assets() { wchar_t p[MAX_PATH]; GetModuleFileNameW(nullptr, p, MAX_PATH); return fs::path(p).parent_path() / L"assets"; }

// don't show the user profile part of paths shown in the UI
static W Pretty(const fs::path& p) {
    W s = p.wstring(), l = LAD().wstring();
    return (!l.empty() && s.size() >= l.size() && _wcsnicmp(s.c_str(), l.c_str(), l.size()) == 0) ? L"%LOCALAPPDATA%" + s.substr(l.size()) : s;
}
static W Err(DWORD e) {
    wchar_t* b = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, e, 0, (LPWSTR)&b, 0, nullptr);
    W s = b ? b : L"unknown error"; LocalFree(b);
    while (!s.empty() && iswspace(s.back())) s.pop_back();
    return (e == ERROR_SHARING_VIOLATION || e == ERROR_ACCESS_DENIED) ? s + L" (file in use? close Roblox)" : s;
}

// Vanilla: newest version-* folder containing RobloxPlayerBeta.exe. Bloxstrap: Modifications folder.
static bool Root(bool blox, fs::path& out, W& err) {
    std::error_code ec;
    fs::path base = LAD() / (blox ? L"Bloxstrap" : L"Roblox\\Versions");
    if (!fs::is_directory(base, ec)) { err = Pretty(base) + L" not found"; return false; }
    if (blox) { out = base / L"Modifications"; return true; }
    fs::file_time_type best{}; bool found = false;
    for (auto& d : fs::directory_iterator(base, ec)) {
        auto exe = d.path() / L"RobloxPlayerBeta.exe";
        if (d.path().filename().wstring().rfind(L"version-", 0) || !fs::exists(exe, ec)) continue;
        auto t = fs::last_write_time(exe, ec);
        if (!found || t > best) { best = t; out = d.path(); found = true; }
    }
    if (!found) err = L"no valid version-* folder in " + Pretty(base);
    return found;
}

// copy to temp file in target dir, then atomically swap over the original
static bool Replace(const fs::path& src, const fs::path& dir, const W& name, W& err) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) { err = L"cannot create " + Pretty(dir); return false; }
    fs::path tmp = dir / (name + L".luetmp");
    if (!CopyFileW(src.c_str(), tmp.c_str(), FALSE)) { err = name + L": " + Err(GetLastError()); return false; }
    if (!MoveFileExW(tmp.c_str(), (dir / name).c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DWORD e = GetLastError(); DeleteFileW(tmp.c_str()); err = name + L": " + Err(e); return false;
    }
    return true;
}

// true only if the file really starts with the PNG signature (not just a .png name)
static bool IsPng(const fs::path& p) {
    unsigned char h[8] = {}, sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    std::ifstream in(p, std::ios::binary); in.read((char*)h, 8);
    return in.gcount() == 8 && !memcmp(h, sig, 8);
}

// config.json (next to the exe) remembers the picked files between runs (should've used nlohmann json lib but i already had this from another project so eh why not)
static const char* kIds[] = { "emote", "player", "cursor" };
static fs::path CfgPath() { wchar_t p[MAX_PATH]; GetModuleFileNameW(nullptr, p, MAX_PATH); return fs::path(p).parent_path() / L"config.json"; }
static std::string U8(const W& w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n ? n - 1 : 0, 0); WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr); return s;
}
static W U16(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    W w(n ? n - 1 : 0, 0); MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n); return w;
}
static void SaveCfg() {
    std::ofstream o(CfgPath(), std::ios::binary); o << "{\n";
    for (int i = 0; i < 3; i++) {
        std::string v; auto it = g_src.find(U16(kIds[i]));
        if (it != g_src.end()) for (char c : U8(it->second.wstring())) { if (c == '\\' || c == '"') v += '\\'; v += c; }
        o << "  \"" << kIds[i] << "\": \"" << v << "\"" << (i < 2 ? "," : "") << "\n";
    }
    o << "}\n";
}
static void LoadCfg() {
    std::ifstream in(CfgPath(), std::ios::binary); std::string t((std::istreambuf_iterator<char>(in)), {});
    for (const char* id : kIds) {
        size_t k = t.find(std::string("\"") + id + "\""); if (k == std::string::npos) continue;
        k = t.find('"', t.find(':', k)); if (k == std::string::npos) continue;
        std::string v; for (size_t i = k + 1; i < t.size() && t[i] != '"'; i++) { if (t[i] == '\\' && i + 1 < t.size()) i++; v += t[i]; }
        fs::path p = U16(v); std::error_code ec;
        if (!v.empty() && fs::exists(p, ec) && (!strcmp(id, "cursor") || IsPng(p))) g_src[U16(id)] = p;
    }
}

static bool Cursor(const fs::path& zip, const fs::path& dir, const fs::path& shiftDir, W& err) {
    std::error_code ec;
    fs::path tmp = fs::temp_directory_path() / (L"lue_cursor_" + std::to_wstring(GetCurrentProcessId()));
    fs::create_directories(tmp, ec);

    W cmd = L"tar.exe -xf \"" + zip.wstring() + L"\" -C \"" + tmp.wstring() + L"\"";
    STARTUPINFOW si{ sizeof si }; PROCESS_INFORMATION pi{}; DWORD code = 1;

    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 30000);
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    bool ok = code == 0;
    if (!ok) err = L"failed to extract cursor zip";

    if (ok) {
        for (auto& names : { std::pair{ &kCursor, &dir }, std::pair{ &kCursorShift, &shiftDir } }) {
            for (auto& name : *names.first) {
                fs::path f;

                for (auto& e : fs::recursive_directory_iterator(tmp, ec))
                    if (e.is_regular_file() && _wcsicmp(e.path().filename().c_str(), name.c_str()) == 0) {
                        f = e.path();
                        break;
                    }

                if (f.empty())
                    continue; // not in zip, skip it

                if (!IsPng(f)) {
                    err = name + L" in zip is not a valid PNG";
                    ok = false;
                    break;
                }

                if (!Replace(f, *names.second, name, err)) {
                    ok = false;
                    break;
                }
            }
            if (!ok) break;
        }
    }

    fs::remove_all(tmp, ec);
    return ok;
}

// returns "ok|msg" / "err|msg". No targets = just detect the install.
static W Run(bool blox, const std::vector<W>& targets) {
    fs::path root; W err, done;
    if (!Root(blox, root, err)) return L"err|" + err;
    for (auto& t : targets) {
        if (t != L"emote" && t != L"player" && t != L"cursor") continue;
        if (!g_src.count(t)) return L"err|choose a file for " + t + L" first";
        bool ok;
        if (t == L"cursor") ok = Cursor(g_src[t], root / kCursorRel, root / kCursorShiftRel, err);
        else {
            auto& names = t == L"emote" ? kEmote : kPlayer;
            ok = true;
            for (size_t i = 0; ok && i < names.size(); i++)
                ok = Replace(g_src[t], root / (t == L"emote" ? kEmoteRel : kPlayerRel), names[i], err);
        }
        if (!ok) return L"err|" + t + L" failed: " + err;
        done += (done.empty() ? L"" : L", ") + t;
    }
    return targets.empty() ? L"ok|found " + Pretty(root) : L"ok|replaced " + done + L" in " + Pretty(root);
}

static fs::path Pick(bool zip) {
    wchar_t f[MAX_PATH] = {}; OPENFILENAMEW o{ sizeof o };
    o.hwndOwner = g_hwnd; o.lpstrFile = f; o.nMaxFile = MAX_PATH;
    o.lpstrFilter = zip ? L"Zip files\0*.zip\0" : L"PNG images\0*.png\0";
    o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameW(&o) ? fs::path(f) : fs::path();
}

// in-mem web resources
static void Respond(ICoreWebView2WebResourceRequestedEventArgs* a, const void* d, size_t n, int st, const wchar_t* mime) {
    IStream* s = SHCreateMemStream((const BYTE*)d, (UINT)n);
    ComPtr<ICoreWebView2WebResourceResponse> r;
    g_env->CreateWebResourceResponse(s, st, st == 200 ? L"OK" : L"Not Found", (W(L"Content-Type: ") + mime).c_str(), &r);
    if (s) s->Release();
    a->put_Response(r.Get());
}
static void OnResource(ICoreWebView2WebResourceRequestedEventArgs* a) {
    ComPtr<ICoreWebView2WebResourceRequest> req; a->get_Request(&req);
    LPWSTR u; req->get_Uri(&u); W path = W(u).substr(17); CoTaskMemFree(u);  // strip "https://app.local"
    path = path.substr(0, path.find_first_of(L"?#"));
    if (path == L"/" || path == L"/index.html") return Respond(a, kIndexHtml, kIndexHtmlSize, 200, L"text/html; charset=utf-8");
    if (path.find(L"..") == W::npos) {
        fs::path f = Assets() / path.substr(1);
        std::ifstream in(f, std::ios::binary);
        if (in) {
            std::vector<char> b((std::istreambuf_iterator<char>(in)), {});
            const std::pair<const wchar_t*, const wchar_t*> m[] = { {L".png", L"image/png"}, {L".gif", L"image/gif"},
                {L".webp", L"image/webp"}, {L".jpg", L"image/jpeg"}, {L".ttf", L"font/ttf"} };
            const wchar_t* mime = L"application/octet-stream";
            for (auto& x : m) if (_wcsicmp(f.extension().c_str(), x.first) == 0) mime = x.second;
            return Respond(a, b.data(), b.size(), 200, mime);
        }
    }
    Respond(a, "", 0, 404, L"text/plain");
}

// injected bridge
static const wchar_t* kBridgeJs = LR"JS(
addEventListener('dragstart',e=>e.preventDefault());

addEventListener('DOMContentLoaded',()=>{
  const post=m=>chrome.webview.postMessage(m); let mode='vanilla', tt;
  const toast=document.createElement('div'); toast.popover='manual';
  toast.style.cssText='position:fixed;inset:auto auto 18px 50%;margin:0;transform:translateX(-50%);background:#fff;color:#000;border:3px solid #000;border-radius:12px;padding:8px 14px;font-size:14px;max-width:90vw;text-align:center;word-break:break-all';
  document.body.append(toast);
  chrome.webview.addEventListener('message',e=>{
    const p=e.data.split('|');
    if(p[0]==='file'){ document.querySelector('#upload-'+p[1]+' .hint').textContent=p[2]||'upload image'; return; }
    toast.textContent=p.slice(1).join('|'); if(!toast.matches(':popover-open')) toast.showPopover();  /* top layer: shows above open dialogs */
    clearTimeout(tt); tt=setTimeout(()=>toast.hidePopover(),5000);
  });
  document.querySelectorAll('.mode').forEach(b=>b.addEventListener('click',()=>{
    mode=b.id==='m-bloxstrap'?'bloxstrap':'vanilla'; post('mode|'+mode);
  }));
  document.querySelectorAll('dialog').forEach(d=>{
    const id=d.id.slice(4);
    d.querySelector('.drop').addEventListener('click',()=>post('pick|'+id));
    d.querySelector('.clear').addEventListener('click',()=>post('clear|'+id));
  });
  const ids={'emote':'emote','player list':'player','cursor':'cursor'};
  document.getElementById('replace').addEventListener('click',()=>{
    const t=[...document.querySelectorAll('#targets .row')]
      .filter(r=>r.querySelector('.box').getAttribute('aria-checked')==='true')
      .map(r=>ids[r.querySelector('.lbl').textContent.trim()]);
    post('apply|'+mode+'|'+t.join(','));
  });
  post('ready');
});
)JS";

static std::vector<W> Split(const W& s, wchar_t c) {
    std::vector<W> v; std::wstringstream ss(s); W x;
    while (std::getline(ss, x, c)) if (!x.empty()) v.push_back(x);
    return v;
}
static void OnMessage(const W& m) {
    auto p = Split(m, L'|');
    if (!p.empty() && p[0] == L"ready") {  // page loaded: show remembered files
        for (auto& kv : g_src) g_view->PostWebMessageAsString((L"file|" + kv.first + L"|" + kv.second.filename().wstring()).c_str());
        return;
    }
    if (p.size() < 2) return;
    W reply;
    if (p[0] == L"mode") reply = Run(p[1] == L"bloxstrap", {});
    else if (p[0] == L"apply") reply = p.size() > 2 ? Run(p[1] == L"bloxstrap", Split(p[2], L',')) : L"err|nothing selected";
    else if (p[0] == L"pick") {
        fs::path f = Pick(p[1] == L"cursor");
        if (f.empty()) return;
        if (p[1] != L"cursor" && !IsPng(f)) { g_view->PostWebMessageAsString(L"err|that file is not a valid PNG"); return; }
        g_src[p[1]] = f; SaveCfg(); reply = L"file|" + p[1] + L"|" + f.filename().wstring();
    }
    else if (p[0] == L"clear") { g_src.erase(p[1]); SaveCfg(); reply = L"file|" + p[1] + L"|"; }
    else return;
    g_view->PostWebMessageAsString(reply.c_str());
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_SIZE && g_ctrl) { RECT r; GetClientRect(h, &r); g_ctrl->put_Bounds(r); return 0; }
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

typedef HRESULT(STDAPICALLTYPE* CreateEnvFn)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);
static void Fail(const wchar_t* m) { MessageBoxW(nullptr, m, L"lue's ui modder", MB_ICONERROR); PostQuitMessage(1); }

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int show) { // dear god this entire function is visual clutter
    g_show = show;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // Start the (slow) WebView2 startup first; the window stays hidden until the page has loaded
    HMODULE dll = LoadLibraryW((Assets() / L"WebView2Loader.dll").c_str());
    auto create = dll ? (CreateEnvFn)GetProcAddress(dll, "CreateCoreWebView2EnvironmentWithOptions") : nullptr;
    if (!create) { Fail(L"assets\\WebView2Loader.dll not found."); return 1; }
    LoadCfg();
    wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
    HICON bigIcon = nullptr, smallIcon = nullptr;
    ExtractIconExW(exe, 0, &bigIcon, &smallIcon, 1);  // the exe's own embedded icon
    WNDCLASSW wc{}; wc.lpfnWndProc = WndProc; wc.hInstance = hi; wc.lpszClassName = L"LueUiModder";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW); wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hIcon = bigIcon;
    RegisterClassW(&wc);

    RECT wa; SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    const int kWinW = 520, kWinH = 700;

    g_hwnd = CreateWindowW(wc.lpszClassName, L"lue's ui modder", WS_OVERLAPPEDWINDOW,
        wa.left + (wa.right - wa.left - kWinW) / 2, wa.top + (wa.bottom - wa.top - kWinH) / 2,
        kWinW, kWinH, nullptr, nullptr, hi, nullptr);

    SendMessageW(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
    SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);

    auto opt = Make<CoreWebView2EnvironmentOptions>();
    opt->put_AdditionalBrowserArguments(L"--disable-features=msSmartScreenProtection --disable-background-networking --disable-component-update");
    W udf = (LAD() / L"LueUiModder" / L"WebView2").wstring();
    HRESULT hr = create(nullptr, udf.c_str(), opt.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([](HRESULT r, ICoreWebView2Environment* env) -> HRESULT {
            if (FAILED(r) || !env) { Fail(L"WebView2 Runtime is required."); return r; }
            g_env = env;
            return env->CreateCoreWebView2Controller(g_hwnd,
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([](HRESULT r, ICoreWebView2Controller* c) -> HRESULT {
                    if (FAILED(r) || !c) { Fail(L"Could not start WebView2."); return E_FAIL; }
                    g_ctrl = c; c->get_CoreWebView2(&g_view);
                    ComPtr<ICoreWebView2Controller4> c4;
                    if (SUCCEEDED(g_ctrl.As(&c4))) c4->put_AllowExternalDrop(FALSE);
                    ComPtr<ICoreWebView2Settings> s; g_view->get_Settings(&s);
                    s->put_AreDefaultContextMenusEnabled(FALSE); s->put_AreDevToolsEnabled(FALSE);
                    RECT rc; GetClientRect(g_hwnd, &rc); c->put_Bounds(rc);
                    EventRegistrationToken tk;
                    g_view->AddWebResourceRequestedFilter(L"https://app.local/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
                    g_view->add_WebResourceRequested(Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* a) -> HRESULT { OnResource(a); return S_OK; }).Get(), &tk);
                    g_view->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* a) -> HRESULT {
                            LPWSTR s; if (SUCCEEDED(a->TryGetWebMessageAsString(&s))) { OnMessage(s); CoTaskMemFree(s); }
                            return S_OK; }).Get(), &tk);
                    g_view->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
                        [](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                            ShowWindow(g_hwnd, g_show);  // show only once the UI is ready
                            RECT rc; GetClientRect(g_hwnd, &rc); g_ctrl->put_Bounds(rc); g_ctrl->put_IsVisible(TRUE);
                            g_ctrl->NotifyParentWindowPositionChanged(); return S_OK; }).Get(), &tk);
                    g_view->AddScriptToExecuteOnDocumentCreated(kBridgeJs, nullptr);
                    g_view->Navigate(L"https://app.local/index.html");
                    return S_OK;
                    }).Get());
            }).Get());
    if (FAILED(hr)) { Fail(L"WebView2 Runtime is required."); return 1; }
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}