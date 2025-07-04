#ifndef MENU_HEAD_H
#define MENU_HEAD_H

#include <QWidget>//0619서현 수정 헤더도,.

namespace Ui {
class menu_head;
}

class menu_head : public QWidget
{
    Q_OBJECT

public:
    explicit menu_head(QWidget *parent = nullptr, QString img ="",
                       QString name ="", QString price = "");
    ~menu_head();

signals:
    void head_clicked(QString menu_name);



protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit head_clicked(this->menu_name);  // 메뉴 이름 전달
    }
private:
    QString menu_name;
    Ui::menu_head *ui;
};

#endif // MENU_HEAD_H
