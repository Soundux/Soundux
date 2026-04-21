#include <webview.hpp>

int main()
{
    Webview::Window webview("webview", 800, 900);
    webview.setTitle("Embedded Example");
    webview.enableDevTools(true);

#if defined(_WIN32)
    webview.setUrl("file:///embedded/index.html");
#else
    webview.setUrl("embedded:///index.html");
#endif

    webview.show();
    webview.run();
    return 0;
}