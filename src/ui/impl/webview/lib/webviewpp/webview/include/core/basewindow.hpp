#pragma once
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "resource.hpp"
#include <javascript/function.hpp>

#if __has_include(<embedded/include.hpp>)
#define WEBVIEW_EMBEDDED
#include <embedded/include.hpp>
#endif

namespace Webview
{
    class Promise;
    class BaseWindow
    {
        friend class Promise;

      protected:
        std::size_t width;
        std::size_t height;

        std::string url;
        std::string title;
        std::string identifier; //* Only required on windows

        bool hidden = true;
        bool isContextMenuAllowed = true;

        std::function<bool()> closeCallback;
        std::function<void(const std::string &)> navigateCallback;
        std::function<void(std::size_t, std::size_t)> resizeCallback;

        static const std::string setupRpc;
        static const std::string resolveCall;
        static const std::string resolveNativeCall;
        static const std::string resolveNativeFunction;
        static const std::string callbackFunctionDefinition;

        std::mutex functionsMutex;
        std::map<std::string, std::shared_ptr<Function>> functions;

        std::mutex nativeCallRequestsMutex;
        std::map<std::uint32_t, JavaScriptFunction> nativeCallRequests;

      protected:
        virtual bool onClose();
        virtual void onNavigate(std::string);
        virtual void onResize(std::size_t, std::size_t);

#if defined(WEBVIEW_EMBEDDED)
        Resource getResource(const std::string &);
#endif

        virtual std::string formatCode(const std::string &);
        virtual void handleRawCallRequest(const std::string &);
        JavaScriptFunction &callFunctionInternal(JavaScriptFunction &&);

      public:
        BaseWindow(const BaseWindow &) = delete;
        virtual BaseWindow &operator=(const BaseWindow &) = delete;
        BaseWindow(std::string identifier, std::size_t width, std::size_t height);

        /// \effects Hides the window
        virtual void hide();
        /// \effects Shows the window
        virtual void show();
        /// \returns Whether or the window is hidden
        virtual bool isHidden();

        /// \effects Sets the window size
        virtual void setSize(std::size_t, std::size_t);
        /// \returns The width and height in form of an `std::pair`
        virtual std::pair<std::size_t, std::size_t> getSize();

        /// \returns The title of the window
        virtual std::string getTitle();
        /// \effects Sets the window title
        virtual void setTitle(std::string);

        /// \effects Runs the mainloop
        /// \remarks Is blocking
        virtual void run() = 0;
        /// \effects Closes the webview
        virtual void exit() = 0;

        /// \returns The current url
        virtual std::string getUrl();
        /// \effects Navigates to the given url
        virtual void setUrl(std::string);

        /// \effects Enables the context menu
        virtual void enableContextMenu(bool);
        /// \effects Enables the developer tools
        virtual void enableDevTools(bool) = 0;

        /// \effects Exposes the given function
        /// \remarks If the given Function is an `AsyncFunction` it will be run in a new thread
        void expose(const Function &);
        /// \effects Calls the given javascript function
        /// \returns The result of the javascript function call as `T`
        /// \preconditions `T` must be serializable by nlohmann::json
        /// \remarks You should never call `.get()` on the returned future in a **non async** context as it will
        /// freeze the webview
        template <typename T = void> std::future<T> callFunction(JavaScriptFunction &&function)
        {
            auto &future = callFunctionInternal(std::forward<JavaScriptFunction>(function));
            auto result = future.getResult();

            return std::async(std::launch::async, [result] {
                if constexpr (!std::is_same_v<T, void>)
                {
                    return result.get().get<T>();
                }
            });
        }

        /// \effects Runs the given javascript code
        virtual void runCode(const std::string &) = 0;
        /// \effects Makes the given javascript code run on document load
        virtual void injectCode(const std::string &) = 0;

        /// \effects Sets the close-callback to the given callback
        /// \remarks If the callback returns `true` the webview will not close
        virtual void setCloseCallback(std::function<bool()>);
        /// \effects Sets the navigate-callback to the given callback
        virtual void setNavigateCallback(std::function<void(const std::string &)>);
        /// \effects Sets the resize-callback to the given callback
        virtual void setResizeCallback(std::function<void(std::size_t, std::size_t)>);
    };
} // namespace Webview