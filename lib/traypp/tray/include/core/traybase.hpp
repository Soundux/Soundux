#pragma once
#include "entry.hpp"
#include "icon.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace Tray
{
    class BaseTray
    {
      protected:
        Icon icon;
        std::string identifier;
        std::vector<std::shared_ptr<TrayEntry>> entries;

      public:
        BaseTray(std::string identifier, Icon icon);

        template <typename... T> void addEntries(const T &...entries)
        {
            (addEntry(entries), ...);
        }
        template <typename T, std::enable_if_t<std::is_base_of<TrayEntry, T>::value> * = nullptr>
        auto addEntry(const T &entry)
        {
            entries.emplace_back(std::make_shared<T>(entry));
            auto back = entries.back();
            back->setParent(this);
            update();

            return std::dynamic_pointer_cast<std::decay_t<T>>(back);
        }

        virtual void run() = 0;
        virtual void exit() = 0;
        virtual void update() = 0;
        std::vector<std::shared_ptr<TrayEntry>> getEntries();
    };
} // namespace Tray