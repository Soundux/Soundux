#include <core/basewindow.hpp>
#include <javascript/promise.hpp>
#include <regex>

Webview::Promise::Promise(Webview::BaseWindow &parent, std::uint32_t id) : id(id), parent(parent) {}

void Webview::Promise::discard() const
{
    resolve(nullptr);
}

void Webview::Promise::resolve(const nlohmann::json &result) const
{
    auto code = std::regex_replace(BaseWindow::resolveCall, std::regex(R"(\{0\})"), std::to_string(id));
    code = std::regex_replace(code, std::regex(R"(\{1\})"), result.dump());
    parent.runCode(code);
}