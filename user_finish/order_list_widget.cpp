#include "order_list_widget.h"
#include "ui_order_list_widget.h"
#include <QLabel>
#include <QGridLayout>
#include <QPixmap>
#include <QMessageBox>

order_list_widget::order_list_widget(QWidget* parent,
                                     const ORDER& order,
                                     const QList<ORDER_DETAIL>& details)
    : QWidget(parent)
    , ui(new Ui::order_list_widget)
    , order_(order)
    , details_(details)
{
    ui->setupUi(this);

    // 1) 주문 날짜 (YY.MM.DD)
    //    예: "2024-06-14 13:45:00" -> "24.06.14"
    QString rawTime = order_.ORDER_TIME;
    if (rawTime.length() >= 10) {
        QString yy = rawTime.mid(2, 2);
        QString mm = rawTime.mid(5, 2);
        QString dd = rawTime.mid(8, 2);
        ui->label_order_date->setText(QString("%1.%2.%3").arg(yy).arg(mm).arg(dd));
    } else {
        ui->label_order_date->setText(rawTime);
    }

    // 2) 가게 이름 (BRAND_UID)
    static const QStringList storeNames = {
        "0",        // 0: 사용 안 함
        "더벤티",     // 1
        "벌크커피",   // 2
        "롯데리아",   // 3
        "4",        // 4
        "5",        // 5
        "BHC"       // 6
    };
    int brand = order_.BRAND_UID;
    ui->label_store_name->setText(
        storeNames.value(brand, "")
        );

    // 3) 주문 상태 (STATUS_ORDER)
    static const QStringList statusTexts = {
        "배달주문",  // 0
        "조리 중",       // 1
        "배달 중",       // 2
        "배달 완료",     // 3
        "4",            // 4
        "5",            // 5
        "6",            // 6
        "7",            // 7
        "8",            // 8
        "주문 거절"      // 9
    };
    int status = order_.STATUS_ORDER;
    ui->label_order_status->setText(
        statusTexts.value(status, "")
        );

    // 4) 브랜드 이미지 분기 설정 (1,2,6 번만 이미지 표시)
    QString imgPath;
    if (brand == 1) {
        imgPath = ":/image/header_img/THEVENTI.jpg";
    } else if (brand == 2) {
        imgPath = ":/image/header_img/BULKCOFFEE.jpg";
    } else if (brand == 3) {
        imgPath = ":/image/header_img/burger1.jpg";
    } else if (brand == 6) {
        imgPath = ":/image/header_img/BHC.jpg";
    } else {
        imgPath.clear();
    }

    if (!imgPath.isEmpty()) {
        ui->label_img->setScaledContents(true);

        QPixmap pix(imgPath);
        ui->label_img->setPixmap(pix);
        // QPixmap pix(imgPath);
        // ui->label_img->setPixmap(
        //     pix.scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        //     );
    } else {
        ui->label_img->clear();
    }

    // 5) 선택 메뉴 요약
    int totalCnt = 0;
    for (const ORDER_DETAIL& d : details_) {
        totalCnt += d.MENU_CNT;
    }
    int firstCnt = details_.isEmpty() ? 0 : details_.first().MENU_CNT;
    int otherCnt = totalCnt - firstCnt;

    if (details_.size() > 1) {
        ui->label_selected_menu->setText(
            QString("%1 x%2 외 %3건")
                .arg(details_.first().MENU_NAME)
                .arg(firstCnt)
                .arg(otherCnt)
            );
    } else if (details_.size() == 1) {
        ui->label_selected_menu->setText(
            QString("%1 x%2건")
                .arg(details_.first().MENU_NAME)
                .arg(firstCnt)
            );
    } else {
        ui->label_selected_menu->setText("-");
    }

    // 6) 버튼 시그널 연결
    connect(ui->pushButton_order_particular,
            &QPushButton::clicked,
            this,
            &order_list_widget::order_click_signal);
    connect(ui->pushButton_reorder,
            &QPushButton::clicked,
            this,
            &order_list_widget::order_reorder_signal);
    connect(ui->pushButton_write_review,
            &QPushButton::clicked,
            this,
            &order_list_widget::order_review_signal);
}

order_list_widget::~order_list_widget()
{
    delete ui;
}
