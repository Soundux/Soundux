## traypp
A cross-platform C++17 library that allows you to create simple tray menus.

## Compatibility
| Platform | Implementation |
| -------- | -------------- |
| Windows  | WinAPI         |
| Linux    | AppIndicator   |

## Dependencies
- Linux
  - libappindicator-gtk3

## Basic Usage
```cpp
#include <tray.hpp>
using Tray::Tray;
using Tray::Button;

int main()
{
  Tray tray("My Tray", "icon.ico");
  tray.addEntry(Button("Exit", [&]{
    tray.exit();
  }));

  tray.run();

  return 0;
}
```
> On Windows it is not necessary to pass an icon path as icon, you can also use an icon-resource or an existing HICON.

## Menu components
### Button
```cpp
Button(std::string text, std::function<void()> callback);
```
**Parameters:**
- `callback` - The function that is called when the button is pressed
----
### ImageButton
```cpp
ImageButton(std::string text, Image image, std::function<void()> callback);
```
**Parameters:**
- `image` - The image tho show
  - Windows
    > Image should either be a path to a bitmap or an HBITMAP
  - Linux
    > Image should either be a path to a png or a GtkImage
- `callback` - The function that is called when the button is pressed
----
### Toggle
```cpp
Toggle(std::string text, bool state, std::function<void(bool)> callback);
```
**Parameters:**
- `state` - The default state of the Toggle
- `callback` - The function that is called when the toggle is pressed
----
### Synced Toggle
```cpp
SyncedToggle(std::string text, bool &state, std::function<void(bool &)> callback);
```
**Parameters:**
- `state` - Reference to a boolean that holds the toggle state
  > The provided boolean will influence the toggled state and **will be modified** if the toggle-state changes
- `callback` - The function that is called when the toggle is pressed
----
### Submenu
```cpp
Submenu(std::string text);

template <typename... T> 
Submenu(std::string text, const T &...entries);
```
**Parameters:**
- `entries` - The entries that should be added upon construction
  > Can be empty - you can add children later with `addEntry`/`addEntries`
----
### Label
```cpp
Label(std::string text);
```
----
### Separator
```cpp
Separator();
```
----

## Installation

- Add the library to your project
  ```cmake
  add_subdirectory(/path/to/traypp EXCLUDE_FROM_ALL)
  link_libraries(tray)
  ```
- Use the library
  - See `example` for examples