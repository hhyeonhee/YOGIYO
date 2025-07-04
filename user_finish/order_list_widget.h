#ifndef ORDER_LIST_WIDGET_H
#define ORDER_LIST_WIDGET_H

#include <QWidget>
#include <QList>
#include "struct_and_enum.h"

namespace Ui { class order_list_widget; }

class order_list_widget : public QWidget {
    Q_OBJECT
public:
    explicit order_list_widget(QWidget* parent,
                               const ORDER& order,
                               const QList<ORDER_DETAIL>& details);
    ~order_list_widget();

signals:
    void order_click_signal();
    void order_reorder_signal();
    void order_review_signal();

private:
    Ui::order_list_widget* ui;
    ORDER order_;                   // 단일 주문 정보
    QList<ORDER_DETAIL> details_;  // 주문 상세 리스트
};

#endif // ORDER_LIST_WIDGET_H
