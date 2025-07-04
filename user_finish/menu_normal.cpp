#include "menu_normal.h"
#include "ui_menu_normal.h"

#include <QLocale>
#include <QMouseEvent>

menu_normal::menu_normal(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::menu_normal)
{
    ui->setupUi(this);
    ui->label_img->setFixedSize(100, 100);  // 이미지 고정 크기
    ui->label_img->setScaledContents(true); // 비율 유지하며 꽉 채우기

    this->setMinimumWidth(300);  // 위젯 너비 제한
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

menu_normal::menu_normal(QWidget *parent, QString img,
                         QString name, QString price, QString info)
    : QWidget(parent)
    , ui(new Ui::menu_normal)
    , menu_name(name)         // 멤버 변수 초기화
    , menu_info(info)
{
    ui->setupUi(this);

    QLocale locale(QLocale::Korean);
    int price_int = price.toInt();
    QString formattedPrice = locale.toString(price_int);

    ui->label_img->setStyleSheet(QString("border-image: url(%1);").arg(img));
    ui->label_name->setText(name);
    ui->label_price->setText(QString("%1원").arg(formattedPrice));
    ui->label_info->setText(info);
}

menu_normal::~menu_normal()
{
    delete ui;
}

