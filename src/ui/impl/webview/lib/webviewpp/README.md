# Archived
This project has been superseded by [saucer](https://github.com/saucer/saucer).

---
<del>
  
## webviewpp
A cross-platform C++17 library that allows you to create a simple webview.

## Compatibility
| Platform | Used Browser                                                                    | GUI    |
| -------- | ------------------------------------------------------------------------------- | ------ |
| Windows  | [Webview2](https://docs.microsoft.com/microsoft-edge/webview2/) (Edge Chromium) | WinAPI |
| Linux    | WebKit2GTK                                                                      | GTK    |

### Supported Windows Versions
| Version | Remarks |
| ------- | ------- |
| 11      | Will not require Webview2 Runtime or Canary-Edge build |
| 10      | Explicit installation of the [`Webview2 Runtime`](https://developer.microsoft.com/microsoft-edge/webview2/#download-section) may be required |
| 8       | Requires `WINDOWS_8` to be set to `ON` from your CMakeLists |

## Usage

- Add the library to your project
  - ```cmake
    add_subdirectory(/path/to/webviewpp EXCLUDE_FROM_ALL)
    link_libraries(webview)
    ```
- Use the library
  - See [documentation](#documentation)
  - See `examples` for examples

## Dependencies
- Windows
  - (Runtime) [Webview2](https://developer.microsoft.com/microsoft-edge/webview2/#download-section) or Edge Chromium Canary Build
- Linux
  - (Runtime & Build) webkit2gtk

## Example
```cpp
#include <webview.hpp>

int main()
{
  Webview::Window webview("webview", 800, 900);
  webview.expose(Webview::Function("addTen", [](int num) {
      return num + 10;
  }));

  webview.show();
  webview.run();
  return 0;
}
```

For more examples see [`examples`](https://github.com/Soundux/webviewpp/tree/master/examples) 

## Embedding
webviewpp supports embedding of all required files.  
To embed your files you have to use the [embed-helper](https://github.com/Soundux/webviewpp/tree/master/embed-helper).

Usage:
  - Compile the embed-helper
    - ```bash
      mkdir build && cd build && cmake .. && cmake --build . --config Release
      ```
  - Run the embed-helper
    - ```bash
      ./embed_helper <path to folder containing all the required files>
      ```
  - Add the parent folder of the `embedded` folder to your include directories
  - Change `setUrl` calls to
    - `embedded:///<filepath>` on Linux
    - `file:///embedded/<filepath>` on Windows

> For an example see [examples/embedded](https://github.com/Soundux/webviewpp/tree/master/examples/embedded)

## Documentation
### Window::hide

``` cpp
void hide();
```

> Hides the window

-----

### Window::show

``` cpp
void show();
```

> Shows the window

-----

### Window::isHidden

``` cpp
bool isHidden();
```

**Returns:**
>  Whether or the window is hidden

-----

### Window::setSize

``` cpp
void setSize(std::size_t, std::size_t);
```

> Sets the window size

-----

### Window::getSize

``` cpp
std::pair<std::size_t, std::size_t> getSize();
```

**Returns:**
>  The width and height in form of an `std::pair`

-----

### Window::getTitle

``` cpp
std::string getTitle();
```

**Returns:**
>  The title of the window

-----

### Window::setTitle

``` cpp
void setTitle(std::string);
```

> Sets the window title

-----

### Window::run

``` cpp
void run();
```

> Runs the mainloop

**Remarks:**
>  Is blocking

-----

### Window::exit

``` cpp
void exit();
```

> Closes the webview

-----

### Window::getUrl

``` cpp
std::string getUrl();
```

**Returns:**
>  The current url

-----

### Window::setUrl

``` cpp
void setUrl(std::string);
```

> Navigates to the given url

-----

### Window::enableContextMenu

``` cpp
void enableContextMenu(bool);
```

> Enables the context menu

-----

### Window::enableDevTools

``` cpp
void enableDevTools(bool);
```

> Enables the developer tools

-----

### Window::expose

``` cpp
void expose(Webview::Function const&);
```

> Exposes the given function

**Remarks:**
>  If the given Function is an `AsyncFunction` it will be run in a new thread

-----

### Window::callFunction

``` cpp
template <typename T>
std::future<T> callFunction(Webview::JavaScriptFunction&& function);
```

> Calls the given javascript function

**Returns:**
>  The result of the javascript function call as `T`

**Preconditions**
>  `T` must be serializable by nlohmann::json

**Remarks:**
>  You should never call `.get()` on the returned future in a **non async** context as it will freeze the webview

-----

### Window::runCode

``` cpp
void runCode(std::string const&);
```

> Runs the given javascript code

-----

### Window::injectCode

``` cpp
void injectCode(std::string const&);
```

> Makes the given javascript code run on document load

-----

### Window::setCloseCallback

``` cpp
void setCloseCallback(std::function<bool ()>);
```

> Sets the close-callback to the given callback

**Remarks:**
>  If the callback returns `true` the webview will not close

-----

### Window::setNavigateCallback

``` cpp
void setNavigateCallback(std::function<void (const std::string &)>);
```

> Sets the navigate-callback to the given callback

-----

### Window::setResizeCallback

``` cpp
void setResizeCallback(std::function<void (std::size_t, std::size_t)>);
```

> Sets the resize-callback to the given callback

-----

> This work was originally based on the work of [MichaelKim](https://github.com/MichaelKim/webview)
</del>
