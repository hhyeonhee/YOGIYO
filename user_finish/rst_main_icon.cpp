#include "rst_main_icon.h"
#include "ui_rst_main_icon.h"

rst_main_icon::rst_main_icon(QWidget *parent,
                             const QString name,
                             float star,
                             int review,
                             const QString deliverytime,
                             const QString deliveryfee,
                             QString img)
    : QWidget(parent),
    ui(new Ui::rst_main_icon)
{
    ui->setupUi(this);

    // 이미지 경로 적용
    ui->rst_image_icon->setStyleSheet(
        QString("border-image: url(%1); border-radius: 20px;").arg(img)
        );

    // 텍스트 설정
    ui->rst_name->setText(name);
    ui->rst_star->setText(QString::number(star, 'f', 1));
    ui->rst_review->setText(QString("(%1)").arg(review));
    ui->rst_deliverytime->setText(deliverytime);
    ui->rst_deliveryfee->setText(QString("배달비 %1원").arg(deliveryfee));
}

void rst_main_icon::mousePressEvent(QMouseEvent* event) {
    emit icon_clicked();  // 위젯 클릭 시 시그널 발생
    QWidget::mousePressEvent(event); // 부모 기본 동작 유지
}

rst_main_icon::~rst_main_icon()
{
    delete ui;
}
