#pragma once
#include <core/entry.hpp>
#include <core/traybase.hpp>
#include <memory>
#include <type_traits>
#include <vector>

namespace Tray
{
    class Submenu : public TrayEntry
    {
        std::vector<std::shared_ptr<TrayEntry>> entries;

      public:
        Submenu(std::string text);
        ~Submenu() override = default;

        template <typename... T> Submenu(std::string text, const T &...entries) : Submenu(text)
        {
            addEntries(entries...);
        }

        template <typename... T> void addEntries(const T &...entries)
        {
            (addEntry(entries), ...);
        }

        template <typename T, std::enable_if_t<std::is_base_of_v<TrayEntry, T>> * = nullptr>
        auto addEntry(const T &entry)
        {
            entries.emplace_back(std::make_shared<T>(entry));
            auto back = entries.back();
            back->setParent(parent);

            if (parent)
            {
                parent->update();
            }

            return std::dynamic_pointer_cast<T>(back);
        }

        void update();
        std::vector<std::shared_ptr<TrayEntry>> getEntries();
    };
} // namespace Tray