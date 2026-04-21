#include <iostream>
#include <javascript/function.hpp>
#include <json.hpp>
#include <webview.hpp>

struct CustomType
{
    int a;
    std::string b;
};

namespace nlohmann
{
    template <> struct adl_serializer<CustomType>
    {
        static void to_json(json &j, const CustomType &obj)
        {
            j = {{"a", obj.a}, {"b", obj.b}};
        }
        static void from_json(const json &j, CustomType &obj)
        {
            j.at("a").get_to(obj.a);
            j.at("b").get_to(obj.b);
        }
    };
} // namespace nlohmann

int main()
{
#if defined(_WIN32)
    Webview::Window webview("webview", 800, 900); //* We have to provide an identifier on windows
#else
    Webview::Window webview(800, 900);
#endif

    webview.setTitle("Example");
    webview.enableDevTools(true);
    webview.enableContextMenu(true);

    webview.expose(Webview::Function("test", [](int someInt) { return someInt + 10; }));

    webview.expose(Webview::AsyncFunction("asyncTest", [](Webview::Promise promise, int someInt) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        promise.resolve(someInt + 10);
    }));

    webview.expose(Webview::Function("returnCustomType", [](int a, std::string b) {
        return CustomType{a, std::move(b)};
    }));

    webview.expose(Webview::Function("takeCustomType", [](const CustomType &a) { return a.a; }));

    webview.expose(Webview::AsyncFunction("pow", [&webview](Webview::Promise promise, int a, int b) {
        //! You should never call a javascript function in a non async context!
        auto pow = webview.callFunction<long>(Webview::JavaScriptFunction("Math.pow", a, b));
        promise.resolve(pow.get());
    }));

    webview.setCloseCallback([] {
        //* If we return `true` the window will not close!
        return true;
    });

    webview.setUrl("https://ddg.gg");
    webview.show();
    webview.run();

    return 0;
}