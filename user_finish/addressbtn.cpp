#include "addressbtn.h"
#include "ui_addressbtn.h"

addressBtn::addressBtn(QWidget *parent, QString address, QString address_detail)
    : QWidget(parent)
    , ui(new Ui::addressBtn)
    ,member_address_detail(address_detail)
{
    ui->setupUi(this);
    ui->Btn_address->setText(address);
    this->setProperty("address_detail", address_detail);  // MainWindow에서 비교할 수 있게

    //주소버튼 클릭시 주소 텍스트 반환하는 람다
    connect(ui->Btn_address, &QPushButton::clicked, this, [=](){
        emit Address_clicked(member_address_detail);
    });
    //삭제버튼 클릭시 삭제 텍스트 반환하는 람다
    connect(ui->Btn_delete, &QPushButton::clicked, this, [=]() {
        emit Address_delete_clicked(member_address_detail, this);  //자기 자신을 함께 넘김
    });
}

addressBtn::~addressBtn()
{
    delete ui;
}
