#include "selected_menu_widget.h"
#include "ui_selected_menu_widget.h"
#include "struct_and_enum.h"

selected_menu_widget::selected_menu_widget(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::selected_menu_widget)
{
    ui->setupUi(this);
    // 기본 생성자에서는 아무 메뉴 정보 없이 UI만 구성

    // 창끄기 버튼 연결
    connect(ui->pushButton_xx, &QPushButton::clicked, this, &QWidget::close);
}

selected_menu_widget::selected_menu_widget(QWidget* parent, const ORDER_DETAIL& order, int item_total)
    : QWidget(parent), ui(new Ui::selected_menu_widget), order_(order), item_total_(item_total)
{
    ui->setupUi(this);

    ui->sel_menu_name->setText(order.MENU_NAME);
    ui->sel_menu_op->setText(order.OPT_NAME_ALL);
    ui->sel_menu_cnt->setText(QString::number(order.MENU_CNT));

    connect(ui->sel_menu_f, &QPushButton::clicked, this, &selected_menu_widget::sel_plus_clicked);
    connect(ui->sel_menu_m, &QPushButton::clicked, this, &selected_menu_widget::sel_minus_clicked);

    update_price_label();  // 가격 라벨 표시 함수

    // 이미지 처리
    if (!order.MENU_IMG_PATH.isEmpty()) {
        QPixmap pix(order.MENU_IMG_PATH);
        if (!pix.isNull()) {
            ui->sel_img->setPixmap(pix);
            ui->sel_img->setScaledContents(true);
        }
    } else {
        ui->sel_img->setStyleSheet("border: 1px solid gray; background-color: #eee;");
    }

    connect(ui->pushButton_xx, &QPushButton::clicked, this, &QWidget::close);
    connect(ui->pushButton_xx, &QPushButton::clicked, this, [=]() {
        emit orderDeleted(order_.MENU_NAME, order_.OPT_NAME_ALL);  // 시그널 보냄
    });
}

void selected_menu_widget::update_price_label() {
    int updated_price = (order_.MENU_PRICE + order_.OPT_PRICE_ALL) * order_.MENU_CNT;
    QLocale locale(QLocale::Korean);
    ui->sel_menu_price->setText(QString("%1원").arg(locale.toString(updated_price)));
}


void selected_menu_widget::sel_plus_clicked() { // +버튼 클릭시 수정
    order_.MENU_CNT++;
    ui->sel_menu_cnt->setText(QString::number(order_.MENU_CNT));
    update_price_label();

    emit orderUpdated(order_);
}

void selected_menu_widget::sel_minus_clicked() {    // -버튼 클릭시 수정
    if (order_.MENU_CNT > 1) {
        order_.MENU_CNT--;
        ui->sel_menu_cnt->setText(QString::number(order_.MENU_CNT));
        update_price_label();

        emit orderUpdated(order_);
    }
}



selected_menu_widget::~selected_menu_widget()
{
    delete ui;
}
