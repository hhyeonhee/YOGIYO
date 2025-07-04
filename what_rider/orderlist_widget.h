#ifndef ORDERLIST_WIDGET_H
#define ORDERLIST_WIDGET_H

#include <QWidget>
#include <QList>
#include <QString>
#include <QDateTime>
#include "mainwindow.h"  // MainWindow::ORDER, MainWindow::ORDER_DETAIL 정의 포함

namespace Ui {
class orderlist_widget;
}

class orderlist_widget : public QWidget
{
    Q_OBJECT

public:
    // 생성자에서 주문 정보와 상세 리스트를 인자로 받아 위젯을 초기화
    explicit orderlist_widget(const ORDER &ord,
                              const QList<ORDER_DETAIL> &details,
                              QWidget *parent = nullptr);
    ~orderlist_widget();
signals:
    void orderAccepted(const QString &orderId);
private slots:
    void on_pushButton_yes_clicked();
private:
    ORDER m_order;
    Ui::orderlist_widget *ui;
};

#endif // ORDERLIST_WIDGET_H
