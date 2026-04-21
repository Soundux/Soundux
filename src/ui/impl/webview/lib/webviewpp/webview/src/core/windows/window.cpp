#if defined(_WIN32)
#include <core/windows/window.hpp>
#include <cstdlib>
#include <stdexcept>

#if defined(WEBVIEWPP_WINDOWS_8)
#include <shellscalingapi.h>
#endif

#if defined(WEBVIEW_EMBEDDED)
#include <Shlwapi.h>
#include <core/windows/mimes.hpp>
#endif

Webview::Window::Window(std::string identifier, std::size_t width, std::size_t height)
    : BaseWindow(std::move(identifier), width, height), instance(GetModuleHandle(nullptr))
{
    if (!instance)
    {
        throw std::runtime_error("GetModuleHandle returned nullptr");
    }

    WNDCLASSEX wndClass;
    wndClass.style = 0;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = instance;
    wndClass.lpfnWndProc = WndProc;
    wndClass.lpszMenuName = nullptr;
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpszClassName = this->identifier.c_str();
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);     // NOLINT
    wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);   // NOLINT
    wndClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION); // NOLINT
    wndClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassEx(&wndClass))
    {
        throw std::runtime_error("Failed to register class");
    }

    hwnd = CreateWindow(this->identifier.c_str(), nullptr, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width,
                        height, nullptr, nullptr, instance, nullptr);
    if (!hwnd)
    {
        throw std::runtime_error("Failed to create window");
    }

#if defined(WEBVIEWPP_WINDOWS_8)

    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE); // NOLINT
#else
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE); // NOLINT
#endif

    RECT rect;
    GetWindowRect(hwnd, &rect);
    auto x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    auto y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    SetWindowPos(hwnd, nullptr, x, y, width, height, 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    SetFocus(hwnd);

    char *buffer = nullptr;
    std::size_t bufferLength = 0;
    _dupenv_s(&buffer, &bufferLength, "LOCALAPPDATA");

    std::string appdata(buffer, bufferLength);
    appdata += "\\MicrosoftEdge";

    if (FAILED(createEnvironment(appdata)))
    {
        throw std::runtime_error("Failed to create environment");
    }
}

LRESULT CALLBACK Webview::Window::Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto *webview = reinterpret_cast<Window *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (webview)
    {
        switch (msg)
        {
        case WM_SIZE:
            webview->onResize(LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_CLOSE:
            if (webview)
            {
                if (webview->onClose())
                {
                    return 0;
                }

                DestroyWindow(webview->hwnd);
                webview->hwnd = nullptr;
            }
            break;
        case WM_QUIT:
            webview->onClose();
            DestroyWindow(hwnd);
            break;
        case WM_CALL: {
            auto *func = reinterpret_cast<std::function<void()> *>(wParam);
            (*func)();
            delete func;
        }
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    else
    {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

HRESULT Webview::Window::Window::createEnvironment(const std::string &appdata)
{
    return CreateCoreWebView2EnvironmentWithOptions(
        nullptr, widen(appdata).c_str(), nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](auto res, ICoreWebView2Environment *env) -> HRESULT {
                if (FAILED(res))
                {
                    return res;
                }

                return env->CreateCoreWebView2Controller(
                    hwnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                              [&](auto result, ICoreWebView2Controller *controller) {
                                  if (FAILED(result))
                                  {
                                      return result;
                                  }
                                  return onControllerCreated(controller);
                              })
                              .Get());
            })
            .Get());
}

HRESULT Webview::Window::Window::onControllerCreated(ICoreWebView2Controller *controller)
{
    if (!controller)
    {
        throw std::runtime_error("Failed to create controller");
    }

    webViewController = controller;
    webViewController->get_CoreWebView2(&webViewWindow);

    EventRegistrationToken navigationCompleted;
    webViewWindow->add_NavigationCompleted(
        Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>([this](auto *sender, auto *args) {
            return onNavigationCompleted(sender, args);
        }).Get(),
        &navigationCompleted);

#if defined(WEBVIEW_EMBEDDED)
    webViewWindow->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
    EventRegistrationToken webResourceRequested;
    webViewWindow->add_WebResourceRequested(
        Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>([=](auto *sender, auto *args) {
            return onWebResourceRequested(sender, args);
        }).Get(),
        &webResourceRequested);
#endif

    EventRegistrationToken messageReceived;
    webViewWindow->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>([=](auto *sender, auto *args) {
            return onMessageReceived(sender, args);
        }).Get(),
        &messageReceived);

    injectCode("window.external.invoke=arg=>window.chrome.webview.postMessage(arg);");
    injectCode(setupRpc);

    for (const auto &func : runOnControllerCreated)
    {
        func();
    }
    runOnControllerCreated.clear();

    return S_OK;
}

HRESULT Webview::Window::Window::onMessageReceived([[maybe_unused]] ICoreWebView2 *sender,
                                                   ICoreWebView2WebMessageReceivedEventArgs *args)
{
    LPWSTR raw = nullptr;
    args->TryGetWebMessageAsString(&raw);

    auto message = narrow(raw);
    BaseWindow::handleRawCallRequest(message);

    CoTaskMemFree(raw);
    return S_OK;
}

HRESULT Webview::Window::Window::onNavigationCompleted([[maybe_unused]] ICoreWebView2 *sender,
                                                       [[maybe_unused]] ICoreWebView2NavigationCompletedEventArgs *args)
{
    //* This fix is fucking retarded. I can't do anything about this until WebView2 fixes this
    //* https://github.com/MicrosoftEdge/WebView2Feedback/issues/1077

    static bool first = true;
    if (first && hidden)
    {
        hide();
        first = false;
    }

    wil::unique_cotaskmem_string uri;
    webViewWindow->get_Source(&uri);

    BaseWindow::onNavigate(narrow(uri.get()));
    return S_OK;
}

#if defined(WEBVIEW_EMBEDDED)
HRESULT Webview::Window::Window::onWebResourceRequested([[maybe_unused]] ICoreWebView2 *sender,
                                                        ICoreWebView2WebResourceRequestedEventArgs *args)
{
    ICoreWebView2WebResourceRequest *req = nullptr;
    args->get_Request(&req);

    LPWSTR rawURI{};
    req->get_Uri(&rawURI);

    auto uri = narrow(rawURI);

    if (uri.length() > 16 && uri.substr(0, 16) == "file:///embedded")
    {
        auto fileName = uri.substr(uri.find_last_of('/') + 1);
        auto mime = fileName.substr(fileName.find_last_of('.'));

        auto content = getResource(fileName);
        if (content.data)
        {
            wil::com_ptr<ICoreWebView2Environment> env;
            wil::com_ptr<ICoreWebView2_2> webview2;
            webViewWindow->QueryInterface(IID_PPV_ARGS(&webview2));
            webview2->get_Environment(&env);

            wil::com_ptr<IStream> stream = SHCreateMemStream(content.data, content.size);

            wil::com_ptr<ICoreWebView2WebResourceResponse> response;
            env->CreateWebResourceResponse(stream.get(), 200, L"OK",
                                           widen("Content-Type: " + mimeTypeList.at(mime)).c_str(), &response);

            args->put_Response(response.get());
        }
    }
    return S_OK;
}
#endif

void Webview::Window::Window::hide()
{
    BaseWindow::hide();
    ShowWindow(hwnd, SW_HIDE);
}

void Webview::Window::Window::show()
{
    BaseWindow::show();

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetFocus(hwnd);
}

void Webview::Window::Window::run()
{
    MSG msg;
    while (hwnd && GetMessage(&msg, nullptr, 0, 0) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Webview::Window::Window::exit()
{
    PostMessage(hwnd, WM_QUIT, 0, 0);
}

void Webview::Window::setUrl(std::string newUrl)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([newUrl = std::move(newUrl), this] { setUrl(newUrl); });
        return;
    }

    BaseWindow::setUrl(newUrl);
    webViewWindow->Navigate(widen(url).c_str());
}

void Webview::Window::setTitle(std::string newTitle)
{
    BaseWindow::setTitle(newTitle);
    SetWindowText(hwnd, title.c_str());
}

void Webview::Window::setSize(std::size_t newWidth, std::size_t newHeight)
{
    BaseWindow::setSize(newWidth, newHeight);
    SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

void Webview::Window::enableDevTools(bool state)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { enableDevTools(state); });
        return;
    }

    wil::com_ptr<ICoreWebView2Settings> settings;

    webViewWindow->get_Settings(&settings);
    settings->put_AreDevToolsEnabled(state);
}

void Webview::Window::enableContextMenu(bool state)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { enableContextMenu(state); });
        return;
    }

    wil::com_ptr<ICoreWebView2Settings> settings;

    webViewWindow->get_Settings(&settings);
    settings->put_AreDefaultContextMenusEnabled(state);
}

void Webview::Window::dispatchMessage(std::function<void()> func)
{
    auto *funcPtr = new std::function<void()>(std::move(func));
    PostMessage(hwnd, WM_CALL, reinterpret_cast<ULONG_PTR>(funcPtr), 0);
}

void Webview::Window::runCode(const std::string &code)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { runCode(code); });
        return;
    }
    dispatchMessage([this, code] { webViewWindow->ExecuteScript(widen(formatCode(code)).c_str(), nullptr); });
}

void Webview::Window::injectCode(const std::string &code)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { injectCode(code); });
        return;
    }
    webViewWindow->AddScriptToExecuteOnDocumentCreated(widen(formatCode(code)).c_str(), nullptr);
}

void Webview::Window::onResize([[maybe_unused]] std::size_t _width, [[maybe_unused]] std::size_t _height)
{
    auto [newWidth, newHeight] = getSize(); //* Yes. I do this because what we get through the WndProc is bullshit

    BaseWindow::onResize(newWidth, newHeight);

    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { onResize(newWidth, newHeight); });
        return;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    webViewController->put_Bounds(rc);
}

std::wstring Webview::Window::Window::widen(const std::string &str)
{
    auto wsz = MultiByteToWideChar(65001, 0, str.c_str(), -1, nullptr, 0);
    if (!wsz)
    {
        return std::wstring();
    }

    std::wstring out(wsz, 0);
    MultiByteToWideChar(65001, 0, str.c_str(), -1, &out[0], wsz);
    out.resize(wsz - 1);
    return out;
}
std::string Webview::Window::Window::narrow(const std::wstring &wstr)
{
    auto sz =
        WideCharToMultiByte(65001, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
    if (!sz)
    {
        return std::string();
    }

    std::string out(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &out[0], sz, nullptr, nullptr);
    return out;
}

std::string Webview::Window::Window::getUrl()
{
    wil::unique_cotaskmem_string uri;
    webViewWindow->get_Source(&uri);

    return narrow(uri.get());
}

void Webview::Window::Window::disableAcceleratorKeys(bool state)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { disableAcceleratorKeys(state); });
        return;
    }

    wil::com_ptr<ICoreWebView2Settings> settings;
    webViewWindow->get_Settings(&settings);

    auto experimentalSettings = settings.try_query<ICoreWebView2ExperimentalSettings2>();
    if (experimentalSettings)
    {
        experimentalSettings->put_AreBrowserAcceleratorKeysEnabled(!state);
    }
}

#endif