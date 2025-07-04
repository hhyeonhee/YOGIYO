#ifndef RST_MAIN_ICON_H
#define RST_MAIN_ICON_H

#include <QWidget>
#include <QMouseEvent>
#include <QString>

namespace Ui {
class rst_main_icon;
}

class rst_main_icon : public QWidget
{
    Q_OBJECT

public:
    explicit rst_main_icon(QWidget *parent = nullptr,
                           QString name = "",
                           float star = 0.0,
                           int review = 0,
                           QString deliverytime = "",
                           QString deliveryfee = "",
                           QString img = "");
    ~rst_main_icon();
    // QString getName() const { return name_; }

signals:
    void icon_clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    Ui::rst_main_icon *ui;
    QString name_;
};

#endif // RST_MAIN_ICON_H
