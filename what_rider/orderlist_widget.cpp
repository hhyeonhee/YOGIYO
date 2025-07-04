#include "orderlist_widget.h"
#include "ui_orderlist_widget.h"
#include "mainwindow.h"
orderlist_widget::orderlist_widget(const ORDER &ord,
                                    const QList<ORDER_DETAIL> &details,
                                   QWidget *parent)
    : QWidget(parent)
    , m_order(ord)
    , ui(new Ui::orderlist_widget)
{
    ui->setupUi(this);

    // “Yes”/“No” 버튼 클릭 시 슬롯 연결
    connect(ui->pushButton_yes, &QPushButton::clicked,
            this, &orderlist_widget::on_pushButton_yes_clicked);

    // 1) 메뉴 & 옵션 목록
    QStringList menuLines;
    for (const auto &od : details) {
        QString line = QString("%1 x%2").arg(od.MENU_NAME).arg(od.MENU_CNT);
        if (!od.OPT_NAME_ALL.isEmpty())
            line += QString(" (%1)").arg(od.OPT_NAME_ALL);
        menuLines << line;
    }
    ui->label_selected_menus->setText(menuLines.join("\n"));

    // 2) 총 가격
    ui->label_totall_price->setText(QString::number(ord.TOTAL_PRICE));

    // 3) 주소
    ui->label_address->setText(ord.ADDRESS + " " + ord.ADDRESS_DETAIL);

    // 4) 가게 요청사항
    ui->label_request->setText(ord.ORDER_MSG.isEmpty() ? "-" : ord.ORDER_MSG);

    // 5) 라이더 요청사항
    ui->label_rider_request->setText(ord.RIDER_MSG.isEmpty() ? "-" : ord.RIDER_MSG);

    // (필요시 수락/거절 버튼 시그널 연결도 여기서)
}
void orderlist_widget::on_pushButton_yes_clicked()
{
    // 1) DB: STATUS_ORDER = 2
    QSqlQuery q;
    q.prepare(R"(
        UPDATE `ORDER_`
           SET STATUS_ORDER = 2
         WHERE ORDER_ID = :id
    )");
    q.bindValue(":id", m_order.ORDER_ID);
    if (!q.exec()) {
        qDebug() << "UPDATE 실패 (Yes):" << q.lastError().text();
        return;
    }

    // 2) 메인에게 주문 ID 전달
    emit orderAccepted(m_order.ORDER_ID);

    // 3) 이 위젯은 숨기지 않고, 화면 전환은 메인에서 처리
}


orderlist_widget::~orderlist_widget()
{
    delete ui;
}
