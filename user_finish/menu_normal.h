#ifndef MENU_NORMAL_H
#define MENU_NORMAL_H

#include <QWidget>

namespace Ui {
class menu_normal;
}

class menu_normal : public QWidget
{
    Q_OBJECT

public:
    explicit menu_normal(QWidget *parent = nullptr);
    explicit menu_normal(QWidget *parent, QString img,
                         QString name, QString price, QString info);
    ~menu_normal();

signals:
    void normal_clicked(QString menu_name, QString info);  // 클릭 시그널


private:
    Ui::menu_normal *ui;
    void mousePressEvent(QMouseEvent* event) override {
        emit normal_clicked(this->menu_name, this->menu_info);
    }

    QString menu_name;  // 클릭 시 전달할 메뉴 이름
    QString menu_info;  // 클릭 시 전달할 메뉴 정보
};

#endif // MENU_NORMAL_H
