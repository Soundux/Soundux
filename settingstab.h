#ifndef SETTINGSTAB_H
#define SETTINGSTAB_H

#include <QWidget>
#include <json.hpp>

using json = nlohmann::json;

class SettingsTab : public QWidget
{
public:
  SettingsTab(std::string _name, json _data);
  std::string name;
  json data = nullptr;
  virtual json tabSettings() = 0;
  virtual void reset() = 0;
};

#endif // SETTINGSTAB_H
