#pragma once
#if defined(_WIN32)
#include <Windows.h>
#include <core/basewindow.hpp>
#include <wil/com.h>
#include <wrl.h>

#include <WebView2.h>
#include <WebView2Experimental.h>

namespace Webview
{
    class Window : public BaseWindow
    {
        static constexpr auto WM_CALL = WM_USER + 2;
        HINSTANCE instance = nullptr;
        HWND hwnd = nullptr;

        std::vector<std::function<void()>> runOnControllerCreated;
        wil::com_ptr<ICoreWebView2Controller> webViewController;
        wil::com_ptr<ICoreWebView2> webViewWindow;

        static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#if defined(WEBVIEW_EMBEDDED)
        HRESULT onWebResourceRequested(ICoreWebView2 *, ICoreWebView2WebResourceRequestedEventArgs *);
#endif

        HRESULT onNavigationCompleted(ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *);
        HRESULT onMessageReceived(ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *);
        HRESULT onControllerCreated(ICoreWebView2Controller *);
        HRESULT createEnvironment(const std::string &);

      private:
        std::wstring widen(const std::string &);
        std::string narrow(const std::wstring &);
        void dispatchMessage(std::function<void()>);
        void onResize(std::size_t width, std::size_t height) override;

      public:
        Window(std::string identifier, std::size_t width, std::size_t height);
        void disableAcceleratorKeys(bool);

        void hide() override;
        void show() override;

        void run() override;
        void exit() override;

        std::string getUrl() override;
        void setUrl(std::string newUrl) override;
        void setTitle(std::string newTitle) override;
        void setSize(std::size_t newWidth, std::size_t newHeight) override;

        void enableDevTools(bool state) override;
        void enableContextMenu(bool state) override;
        void runCode(const std::string &code) override;
        void injectCode(const std::string &code) override;
    };
} // namespace Webview
#endif