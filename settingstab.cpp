#include "settingstab.h"

SettingsTab::SettingsTab(std::string _name, json _data)
{
    this->name = _name;
    if (_data.contains(_name)) {
        this->data = _data.find(_name)->get<json>();
    }
}
