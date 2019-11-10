#ifndef CLICKABLESLIDERSTYLE_H
#define CLICKABLESLIDERSTYLE_H

#include <QObject>
#include <QProxyStyle>

// Thanks to https://stackoverflow.com/a/26281608
class ClickableSliderStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(QStyle::StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
            return (Qt::LeftButton | Qt::MidButton | Qt::RightButton);
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

};

#endif // CLICKABLESLIDERSTYLE_H
