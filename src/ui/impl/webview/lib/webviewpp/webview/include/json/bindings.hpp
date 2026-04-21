#pragma once
#include <javascript/call.hpp>
#include <json.hpp>

namespace nlohmann
{
    template <> struct adl_serializer<Webview::FunctionCallRequest>
    {
        static void to_json(json &j, const Webview::FunctionCallRequest &obj)
        {
            j = {{"function", obj.function}, {"params", obj.params}, {"seq", obj.seq}};
        }
        static void from_json(const json &j, Webview::FunctionCallRequest &obj)
        {
            j.at("seq").get_to(obj.seq);
            j.at("params").get_to(obj.params);
            j.at("function").get_to(obj.function);
        }
    };

    template <> struct adl_serializer<Webview::NativeCallResponse>
    {
        static void to_json(json &j, const Webview::NativeCallResponse &obj)
        {
            j = {{"result", obj.result}, {"seq", obj.seq}};
        }
        static void from_json(const json &j, Webview::NativeCallResponse &obj)
        {
            j.at("seq").get_to(obj.seq);
            j.at("result").get_to(obj.result);
        }
    };
} // namespace nlohmann