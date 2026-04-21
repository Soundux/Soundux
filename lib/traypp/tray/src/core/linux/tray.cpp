#if defined(__linux__)
#include <core/linux/tray.hpp>
#include <libayatana-appindicator/app-indicator.h>
#include <stdexcept>

#include <components/button.hpp>
#include <components/imagebutton.hpp>
#include <components/label.hpp>
#include <components/separator.hpp>
#include <components/submenu.hpp>
#include <components/syncedtoggle.hpp>
#include <components/toggle.hpp>

Tray::Tray::Tray(std::string identifier, Icon icon) : BaseTray(std::move(identifier), std::move(icon))
{
    if (gtk_init_check(nullptr, nullptr) != TRUE)
    {
        throw std::runtime_error("Gtk init check failed");
        return;
    }

    appIndicator = app_indicator_new(this->identifier.c_str(), this->icon, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status(appIndicator, APP_INDICATOR_STATUS_ACTIVE);
}

void Tray::Tray::exit()
{
    g_idle_add(
        [](gpointer data) -> gboolean {
            auto *tray = reinterpret_cast<Tray *>(data);
            g_object_unref(tray->appIndicator);
            tray->appIndicator = nullptr;
            return FALSE;
        },
        this);
}

void Tray::Tray::update()
{
    if (appIndicator)
    {
        app_indicator_set_menu(appIndicator, reinterpret_cast<GtkMenu *>(construct(entries, this)));
    }
}

GtkMenuShell *Tray::Tray::construct(const std::vector<std::shared_ptr<TrayEntry>> &entries, Tray *parent)
{
    auto *menu = reinterpret_cast<GtkMenuShell *>(gtk_menu_new());

    for (const auto &entry : entries)
    {
        auto *item = entry.get();
        GtkWidget *gtkItem = nullptr;

        if (auto *toggle = dynamic_cast<Toggle *>(item); toggle)
        {
            gtkItem = gtk_check_menu_item_new_with_label(toggle->getText().c_str());
            gtk_check_menu_item_set_active(reinterpret_cast<GtkCheckMenuItem *>(gtkItem), toggle->isToggled());
        }
        else if (auto *syncedToggle = dynamic_cast<SyncedToggle *>(item); syncedToggle)
        {
            gtkItem = gtk_check_menu_item_new_with_label(syncedToggle->getText().c_str());
            gtk_check_menu_item_set_active(reinterpret_cast<GtkCheckMenuItem *>(gtkItem), syncedToggle->isToggled());
        }
        else if (auto *submenu = dynamic_cast<Submenu *>(item); submenu)
        {
            gtkItem = gtk_menu_item_new_with_label(submenu->getText().c_str());
            gtk_menu_item_set_submenu(reinterpret_cast<GtkMenuItem *>(gtkItem),
                                      reinterpret_cast<GtkWidget *>(construct(submenu->getEntries(), parent)));
        }
        else if (auto *iconButton = dynamic_cast<ImageButton *>(item); iconButton)
        {
            gtkItem = gtk_menu_item_new();

            GtkWidget *image = iconButton->getImage();
            auto *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
            auto *label = gtk_label_new(iconButton->getText().c_str());

            bool handled = false;
            if (parent)
            {
                for (std::size_t i = 0; parent->imageWidgets.size() > i; i++)
                {
                    const auto &[container, widget] = parent->imageWidgets.at(i);

                    if (widget == image)
                    {
                        g_object_ref(widget); // NOLINT

                        gtk_container_remove(container, widget);
                        gtk_container_add(reinterpret_cast<GtkContainer *>(box),
                                          widget); // TODO(performance): This takes ages - find a way to improve it.

                        parent->imageWidgets.erase(parent->imageWidgets.begin() + i);
                        handled = true;
                        break;
                    }
                }
            }

            if (!handled)
            {
                gtk_container_add(reinterpret_cast<GtkContainer *>(box), image);
            }

            gtk_label_set_xalign(reinterpret_cast<GtkLabel *>(label), 0.0);
            gtk_box_pack_end(reinterpret_cast<GtkBox *>(box), label, TRUE, TRUE, 0);

            gtk_container_add(reinterpret_cast<GtkContainer *>(gtkItem), box);

            if (parent)
            {
                parent->imageWidgets.emplace_back(reinterpret_cast<GtkContainer *>(box),
                                                  reinterpret_cast<GtkWidget *>(image));
            }
        }
        else if (dynamic_cast<Button *>(item))
        {
            gtkItem = gtk_menu_item_new_with_label(item->getText().c_str());
        }
        else if (dynamic_cast<Label *>(item))
        {
            gtkItem = gtk_menu_item_new_with_label(item->getText().c_str());
            gtk_widget_set_sensitive(gtkItem, FALSE);
        }
        else if (dynamic_cast<Separator *>(item))
        {
            gtkItem = gtk_separator_menu_item_new();
        }

        if (!dynamic_cast<Label *>(item))
        {
            gtk_widget_set_sensitive(gtkItem, !item->isDisabled());
        }

        g_signal_connect(gtkItem, "activate", reinterpret_cast<GCallback>(callback), item);

        gtk_widget_show(gtkItem);
        gtk_menu_shell_append(menu, gtkItem);
    }

    return menu;
}

void Tray::Tray::callback([[maybe_unused]] GtkWidget *widget, gpointer data)
{
    auto *item = reinterpret_cast<TrayEntry *>(data);

    if (auto *button = dynamic_cast<Button *>(item); button)
    {
        button->clicked();
    }
    else if (auto *toggle = dynamic_cast<Toggle *>(item); toggle)
    {
        toggle->onToggled();
    }
    else if (auto *syncedToggle = dynamic_cast<SyncedToggle *>(item); syncedToggle)
    {
        syncedToggle->onToggled();
    }
}

void Tray::Tray::run()
{
    while (appIndicator)
    {
        gtk_main_iteration_do(true);
    }
}

#endif
