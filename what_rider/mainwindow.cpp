#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVBoxLayout>
#include <QDate>
#include <QTime>
#include <QPushButton>
#include <QHeaderView>

#include <QDialog>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>

#include "orderlist_widget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket_(new QTcpSocket(this))
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    this->db_open();
    // --- connect 버튼 클릭 to 페이지 전환 슬롯 ---
    // page_login (index 0)
    // connect(ui->pushButton_login,     &QPushButton::clicked, this, &MainWindow::go_to_page_main_menu);
    connect(ui->pushButton_find_id,   &QPushButton::clicked, this, &MainWindow::go_to_page_find_id);
    connect(ui->pushButton_find_pw,   &QPushButton::clicked, this, &MainWindow::go_to_page_find_pw);
    connect(ui->pushButton_signin,    &QPushButton::clicked, this, &MainWindow::go_to_page_sign_in);

    // page_find_id (index 1)
    connect(ui->pushButton_back1,     &QPushButton::clicked, this, &MainWindow::go_to_page_login);
    // ※ pushButton_findid 는 아이디 찾기 로직용

    // page_find_pw (index 2)
    connect(ui->pushButton_back2,     &QPushButton::clicked, this, &MainWindow::go_to_page_find_id);
    // ※ pushButton_find 은 비밀번호 찾기 로직용

    // page_sign_in (index 3)
    connect(ui->pushButton_back3,     &QPushButton::clicked, this, &MainWindow::go_to_page_login);
    // ※ pushButton_add_file, pushButton_check_id, pushButton_real_signin 은 각각 로직용

    // page_main_menu (index 4)
    connect(ui->pushButton_check_orderlist, &QPushButton::clicked, this, &MainWindow::go_to_page_order);
    // connect(ui->pushButton_logout_2,        &QPushButton::clicked, this, &MainWindow::go_to_page_login);
    // ※ pushButton_fix_save 는 저장 로직용

    // page_order (index 5)
    connect(ui->pushButton_back,        &QPushButton::clicked, this, &MainWindow::go_to_page_main_menu);
    connect(ui->pushButton_logout,      &QPushButton::clicked, this, &MainWindow::go_to_page_login);

    // 서버 연결
    on_login_button_clicked();
    connect(socket_, &QTcpSocket::connected, this, &MainWindow::on_connected);
    // connect(socket_, &QTcpSocket::readyRead, this, &MainWindow::on_ready_read);
    connect(socket_, &QTcpSocket::readyRead,
            this,    &MainWindow::onSocketReadyRead);

    connect(socket_, &QAbstractSocket::errorOccurred, this, &MainWindow::on_error);

    // 버튼 연결
    connect(ui->pushButton_login, &QPushButton::clicked, this, &MainWindow::sendLoginRequest);

    connect(ui->pushButton_check_orderlist, &QPushButton::clicked,
            this, &MainWindow::on_check_orderlist_clicked);
    connect(ui->pushButton_finish, &QPushButton::clicked,
            this, &MainWindow::onFinishButtonClicked);
    connect(ui->pushButton_finish, &QPushButton::clicked,
            this, &MainWindow::showDeliveryCompletePopup);

}
// 1) 주문 수락 시: ID 저장 & 페이지 전환
void MainWindow::onOrderAccepted(const QString &orderId)
{
    currentOrderId = orderId;
    qDebug() << "수락된 주문 ID:" << currentOrderId;
    ui->stackedWidget->setCurrentIndex(6);  // 배달 중 페이지
}

void MainWindow::onFinishButtonClicked()
{
    if (currentOrderId.isEmpty()) return;

    // 1) DB 업데이트: STATUS_ORDER = 3
    QSqlQuery q;
    q.prepare(R"(
        UPDATE `ORDER_`
           SET STATUS_ORDER = 3
         WHERE ORDER_ID = :id
    )");
    q.bindValue(":id", currentOrderId);
    if (!q.exec()) {
        qDebug() << "배달 완료 업데이트 실패:" << q.lastError().text();
    } else {
        qDebug() << "배달 완료 처리된 주문:" << currentOrderId;
        currentOrderId.clear();
    }

    // 2) 팝업 보여주기
    showDeliveryCompletePopup();

    // 3) 5페이지(page_order)로 돌아가기
    ui->stackedWidget->setCurrentIndex(5);

    // 4) 주문 목록을 새로 요청해서 스크롤 영역을 갱신
    on_check_orderlist_clicked();
}



void MainWindow::showDeliveryCompletePopup() {

    ui->stackedWidget->setCurrentIndex(5);
    // 1. 다이얼로그 생성 (프레임 없는 팝업 스타일)
    QDialog* popup = new QDialog(this, Qt::FramelessWindowHint | Qt::Dialog);
    popup->setAttribute(Qt::WA_TranslucentBackground);
    popup->setModal(false);
    popup->resize(300, 100);

    // 2. 배경 라벨 스타일
    QWidget* container = new QWidget(popup);
    container->setStyleSheet("background-color: white; border-radius: 15px; border: 2px solid #FF2D6F;");
    container->setGeometry(0, 0, 300, 100);

    QLabel* label = new QLabel("배달이 완료되었습니다!", container);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 16px; color: #FF2D6F; font-weight: bold;");
    label->setGeometry(0, 0, 300, 100);

    // 3. Fade in/out 효과
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(popup);
    popup->setGraphicsEffect(effect);

    QPropertyAnimation* anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(1000);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    // 4. 일정 시간 후 fade-out 및 자동 종료
    QTimer::singleShot(2000, [popup]() {
        QGraphicsOpacityEffect* effect = static_cast<QGraphicsOpacityEffect*>(popup->graphicsEffect());
        QPropertyAnimation* fadeOut = new QPropertyAnimation(effect, "opacity");
        fadeOut->setDuration(1000);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
        QObject::connect(fadeOut, &QPropertyAnimation::finished, popup, &QDialog::deleteLater);
    });

    // 5. 화면 중앙에 위치시키기
    popup->move(this->geometry().center() - QPoint(150, 50));
    popup->show();
}

void MainWindow::db_open(){

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");

    db.setHostName("10.10.20.99");
    db.setDatabaseName("WHAT");
    db.setUserName("JIN");
    db.setPassword("1234");
    db.setPort(3306);

    if (!db.open()) {
        qDebug() << "❌ DB 연결 실패:" << db.lastError().text();
        // return false;
    }
    qDebug() << "DB 연결 성공!";
}

void MainWindow::go_to_page_login()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::go_to_page_find_id()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::go_to_page_find_pw()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::go_to_page_sign_in()
{
    ui->stackedWidget->setCurrentIndex(3);
}

void MainWindow::go_to_page_main_menu()
{
    ui->stackedWidget->setCurrentIndex(4);
}

void MainWindow::go_to_page_order()
{
    ui->stackedWidget->setCurrentIndex(5);
    // populateOrderList();
}


void MainWindow::on_check_orderlist_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);

    QJsonObject req;
    req["action"]    = "110_0";
    req["place_uid"] = owner_info.PLACE_UID;
    QByteArray ba = QJsonDocument(req).toJson(QJsonDocument::Compact);
    qDebug() << "▶ 주문 조회 요청 보냄:" << ba;
    socket_->write(ba);
}

void MainWindow::on_login_button_clicked()
{
    if (socket_->state() == QAbstractSocket::UnconnectedState) {
        socket_->connectToHost("10.10.20.99", SERVER_PORT);
    }
}

void MainWindow::on_connected()
{
    socket_->write(QString("owner").toUtf8());
    qDebug() << "owner 타입 전송 완료";
}
void MainWindow::sendLoginRequest()
{
    QJsonObject loginJson;
    loginJson["action"] = "100_0";
    // loginJson["phonenumber"] = ui->lineEdit_id->text();
    // loginJson["type"] = ui->lineEdit_pw->text();
    loginJson["phonenumber"] = "01012345678";
    loginJson["type"] = "owner";
    socket_->write(QJsonDocument(loginJson).toJson(QJsonDocument::Compact));
}



void MainWindow::handleOrderResponse(const QJsonObject &resp)
{
    qDebug() << "사장 주문 목록 수신:" << resp;

    // 1) 기존 스크롤 위젯 있으면 제거
    if (QWidget *old = ui->scrollArea_order->takeWidget()) {
        old->deleteLater();
    }

    // 2) 새로운 컨테이너와 레이아웃 생성
    QWidget *container = new QWidget;
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QVBoxLayout *vbox = new QVBoxLayout(container);
    vbox->setAlignment(Qt::AlignTop);

    // 3) JSON 파싱하여 orderlist_widget 생성
    QJsonArray orders = resp["orders"].toArray();
    for (const QJsonValue &val : orders) {
        QJsonObject o = val.toObject();

        // ORDER 구조체에 담기
        ORDER ord;
        ord.ORDER_ID           = o["order_id"].toString();
        ord.UID                = o["uid"].toInt();
        ord.BRAND_UID          = o["brand_uid"].toInt();
        ord.PLACE_UID          = o["place_uid"].toInt();
        ord.TOTAL_PRICE        = o["total_price"].toInt();
        ord.ORDER_TIME         = o["order_time"].toString();
        ord.ADDRESS            = o["address"].toString();
        ord.ADDRESS_DETAIL     = o["address_detail"].toString();
        ord.ORDER_MSG          = o["order_msg"].toString();
        ord.RIDER_MSG          = o["rider_msg"].toString();
        ord.STATUS_TOGO        = o["status_togo"].toInt();
        ord.STATUS_DISPOSIBLE  = o["status_disposible"].toBool();
        ord.STATUS_ORDER       = o["status_order"].toInt();

        // STATUS_ORDER가 1인 주문만 처리
        if (ord.STATUS_ORDER != 1)
            continue;

        // ORDER_DETAIL 리스트에 담기
        QList<ORDER_DETAIL> detailsList;
        for (const QJsonValue &dv : o["details"].toArray()) {
            QJsonObject d = dv.toObject();
            ORDER_DETAIL od;
            od.ORDER_ID      = ord.ORDER_ID;
            od.MENU_NAME     = d["menu_name"].toString();
            od.MENU_PRICE    = d["menu_price"].toInt();
            od.MENU_CNT      = d["menu_cnt"].toInt();
            od.OPT_NAME_ALL  = d["opt_name_all"].toString();
            od.OPT_PRICE_ALL = d["opt_price_all"].toInt();
            detailsList.append(od);
        }

        // 4) orderlist_widget 생성 및 시그널 연결
        auto *w = new orderlist_widget(ord, detailsList, this);
        connect(w, &orderlist_widget::orderAccepted,
                this, &MainWindow::onOrderAccepted);

        // 5) 레이아웃에 추가
        vbox->addWidget(w);
    }

    // 6) 스크롤 영역에 새 컨테이너 배치
    container->setLayout(vbox);
    ui->scrollArea_order->setWidget(container);
    ui->scrollArea_order->setWidgetResizable(true);
}



void MainWindow::handleLoginResponse(const QJsonObject &resp)
{
    // 성공(100_1) / 실패(100_2) 분기
    if (resp["action"].toString() == "100_1") {
        qDebug() << "로그인 성공";
        owner_info.PHONENUMBER             = resp["PHONENUMBER"].toString();
        owner_info.type                    = resp["type"].toString();
        owner_info.BRAND_UID               = resp["BRAND_UID"].toString();
        owner_info.PLACE_UID               = resp["PLACE_UID"].toString();
        owner_info.ADDRESS                 = resp["ADDRESS"].toString();
        owner_info.ADDRESS_DETAIL          = resp["ADDRESS_DETAIL"].toString();
        owner_info.OPEN_DATE               = QDate::fromString(resp["OPEN_DATE"].toString(), "yyyy-MM-dd");
        owner_info.DELIVERY_TIM            = resp["DELIVERY_TIM"].toString();
        owner_info.OPEN_TIME               = QTime::fromString(resp["OPEN_TIME"].toString(), "HH:mm");
        owner_info.CLOSE_TIME              = QTime::fromString(resp["CLOSE_TIME"].toString(), "HH:mm");
        owner_info.ORIGIN_INFO             = resp["ORIGIN_INFO"].toString();
        owner_info.MIN_ORDER_PRICE         = resp["MIN_ORDER_PRICE"].toString().toInt();
        owner_info.DELIVERY_FEE            = resp["DELIVERY_FEE"].toString().toInt();
        owner_info.FREE_DELIVERY_STANDARD  = resp["FREE_DELIVERY_STANDARD"].toString().toInt();
        owner_info.IMAGE_PASS              = resp["IMAGE_PASS"].toString();
        owner_info.IMAGE_MAIN              = resp["IMAGE_MAIN"].toString();
        // 스택 위젯 페이지 전환
        ui->stackedWidget->setCurrentIndex(4);

    }
    else {
        QMessageBox::warning(this, "로그인 실패", resp["message"].toString());
    }
}

void MainWindow::onSocketReadyRead()
{
    // 1) 소켓에서 읽어들인 원본 바이트
    QByteArray chunk = socket_->readAll();
    qDebug() << "🔸 RAW READ:" << chunk;

    // 2) 버퍼 누적 직후
    recv_buffer += QString::fromUtf8(chunk);
    qDebug() << "🔸 BUFFER AFTER APPEND:" << recv_buffer;

    const QString delimiter = "<<END>>";
    int pos;
    while ((pos = recv_buffer.indexOf(delimiter)) != -1) {
        // 3) 구분자까지 추출한 순수 JSON 텍스트
        QString jsonText = recv_buffer.left(pos).trimmed();
        qDebug() << "🔸 EXTRACTED JSON TEXT:" << jsonText;

        // 버퍼에서 해당 구간 제거
        recv_buffer.remove(0, pos + delimiter.length());

        // 4) 실제 파싱 직전
        qDebug() << "🔸 ABOUT TO PARSE JSON";
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "JSON 파싱 실패:" << err.errorString();
            continue;
        }
        QJsonObject resp = doc.object();

        // 5) 파싱된 오브젝트 확인
        qDebug() << "🔸 PARSED QJsonObject:" << resp;

        // 기존 로직
        if (resp.value("action").toString() == "100_1") {
            handleLoginResponse(resp);
        } else if (resp.contains("orders")) {
            handleOrderResponse(resp);
        }
    }
}


void MainWindow::on_error(QAbstractSocket::SocketError err)
{
    if (err == QAbstractSocket::RemoteHostClosedError)
        return;
    QMessageBox::critical(this, "소켓 에러", socket_->errorString());
}

MainWindow::~MainWindow()
{
    delete ui;
}
