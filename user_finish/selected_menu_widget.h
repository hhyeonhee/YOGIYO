#ifndef SELECTED_MENU_WIDGET_H
#define SELECTED_MENU_WIDGET_H

#include <QWidget>
#include "struct_and_enum.h"

namespace Ui {
class selected_menu_widget;
}

class selected_menu_widget : public QWidget
{
    Q_OBJECT

public:
    explicit selected_menu_widget(QWidget *parent = nullptr); // 기본 생성자
    explicit selected_menu_widget(QWidget *parent, const menu& sel_menu); // 실제 사용
    explicit selected_menu_widget(QWidget* parent, const ORDER_DETAIL& order, int item_total);  // 메뉴별 총액 전달

    ~selected_menu_widget();

private slots:
    // void option_Check();      // 옵션 체크박스 변경
    void sel_plus_clicked();        // 수량 +
    void sel_minus_clicked();       // 수량 -
    void update_price_label();      //금액 표시 함수

signals:
    void orderUpdated(const ORDER_DETAIL& updatedOrder);
    void orderDeleted(const QString& menu_name, const QString& opt_name_all);   //메뉴 지우면 삭제보낸다 추가함 0620

private:
    Ui::selected_menu_widget *ui;
    ORDER_DETAIL order_;
    int item_total_;
};


#endif // SELECTED_MENU_WIDGET_H
