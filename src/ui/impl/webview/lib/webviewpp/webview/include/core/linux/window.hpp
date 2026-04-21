#pragma once
#if defined(__linux__)
#include <core/basewindow.hpp>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

namespace Webview
{
    class Window : public BaseWindow
    {
        GtkWidget *window;
        GtkWidget *webview;

        static void destroy(GtkWidget *, gpointer);
        static gboolean closed(GtkWidget *, GdkEvent *, gpointer);
        static gboolean resize(WebKitWebView *, GdkEvent *, gpointer);

#if defined(WEBVIEW_EMBEDDED)
        static void onUriRequested(WebKitURISchemeRequest *, gpointer);
#endif

        static void loadChanged(WebKitWebView *, WebKitLoadEvent, gpointer);
        static void messageReceived(WebKitUserContentManager *, WebKitJavascriptResult *, gpointer);
        static gboolean contextMenu(WebKitWebView *, GtkWidget *, WebKitHitTestResultContext *, gboolean, gpointer);

      private:
        void runOnIdle(std::function<void()>);

      public:
        Window(std::size_t width, std::size_t height);
        Window(const std::string &identifier, std::size_t width,
               std::size_t height); //* Identifier is not required on linux.

        void hide() override;
        void show() override;

        void run() override;
        void exit() override;

        std::string getUrl() override;
        void setUrl(std::string newUrl) override;
        void setTitle(std::string newTitle) override;
        void setSize(std::size_t newWidth, std::size_t newHeight) override;

        void enableDevTools(bool state) override;
        void runCode(const std::string &code) override;
        void injectCode(const std::string &code) override;
    };
} // namespace Webview
#endif