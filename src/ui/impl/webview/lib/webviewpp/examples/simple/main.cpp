#include <iostream>
#include <javascript/function.hpp>
#include <webview.hpp>

int main()
{
#if defined(_WIN32)
    Webview::Window webview("webview", 800, 900); //* We have to provide an identifier on windows
#else
    Webview::Window webview(800, 900);
#endif
    webview.setTitle("Example");
    webview.enableDevTools(true);

    // Call me using the dev tools!
    webview.expose(Webview::Function("testCallback", [](const std::string &someString, int someInt) {
        std::cout << "Got " << someString << " and " << someInt;
        return someInt * 10;
    }));

    webview.setUrl("https://ddg.gg");
    webview.show();
    webview.run();

    return 0;
}