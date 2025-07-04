#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QVBoxLayout>
#include <QDebug>

//드래그이벤트
#include <QScroller>
#include <QPropertyAnimation>
#include <QPauseAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QParallelAnimationGroup>

#include <QResizeEvent>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket_(new QTcpSocket(this))
{
    ui->setupUi(this);
    set_Btn_menu_finish_select();  // 초기 UI 상태 강제 반영
    ui->groupBox_8->hide();

    statusBar()->hide(); //메인창에서 크기를 조정 설정 없애기 왜냐!!!!1공백이 생기기 때문
    ui->label->setText("<span style='color:red;'>‧</span>");
    please_adress();
    setupButtonPaging();        // 10번 버튼 클릭 시 전체 보이게
    hideButtons(11, 20); //////////////////////////////////////////////////////////////////

    ui->pushButton_nickname->setText(user_info.name);
    ui->pushButton_email->setText(user_info.Email_id);
    ui->label_pnum->setText(user_info.phonenum);
    ui->label_where->setText(user_adr.adr_name);
    ui->label_dfee->setText(storeData.delivery_fee);

    for (int i = 1; i <= 6; ++i) {
        // 버튼과 프레임 객체 가져오기
        QString btnName   = QString("pushButton_heart%1").arg(i);
        QString frameName = QString("frame_heart%1").arg(i);

        QPushButton* btn  = findChild<QPushButton*>(btnName);
        QFrame*    frame  = findChild<QFrame*>(frameName);

        if (btn && frame) {
            // 람다 캡처로 frame 포인터를 고정시켜 각 버튼마다 해당 프레임을 hide()
            connect(btn, &QPushButton::clicked, this, [frame]() {
                frame->hide();
            });
        }
    }

    connect(ui->pushButton_my_close_2, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
    });



    QTimer *timer2 = new QTimer(this);
    connect(timer2, &QTimer::timeout, this, [this]() {
        auto sw = ui->stackedWidget_2;
        int next = (sw->currentIndex() + 1) % sw->count();
        slideTransition(sw, next);
    });
    timer2->start(2000);

    // stackedWidget_3 자동 슬라이드
    QTimer *timer3 = new QTimer(this);
    connect(timer3, &QTimer::timeout, this, [this]() {
        auto sw = ui->stackedWidget_3;
        int next = (sw->currentIndex() + 1) % sw->count();
        slideTransition(sw, next);
    });
    timer3->start(3000);

    stack = ui->stackedWidget;
    eff1  = new QGraphicsOpacityEffect(stack->widget(0));
    stack->widget(0)->setGraphicsEffect(eff1);
    eff1->setOpacity(1.0);
    eff2  = new QGraphicsOpacityEffect(stack->widget(1));
    stack->widget(1)->setGraphicsEffect(eff2);
    eff2->setOpacity(0.0);
    // 초기 로그인 페이지
    ui->stackedWidget->setCurrentIndex(0);
    QTimer::singleShot(3000, this, [=](){
        goToPage(1);
    });


    // ← 추가 끝

    //마우스드래그이벤트
    QScroller::grabGesture(ui->scrollArea->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_8->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_23->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_16->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_15->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_18->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_9->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_24->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_25->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_5->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_19->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_22->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_39->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_2->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_4->viewport(),
                           QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->scrollArea_7->viewport(),
                           QScroller::LeftMouseButtonGesture);



    ui->Btn_main_cart->setFixedSize(40, 40);
    ui->Btn_main_cart->setStyleSheet(
        "QPushButton {"
        "  border-radius: 20px;"
        "  border: 2px solid black;"
        "  background-image: url(:/image/rrrrealbaguni.drawio.png);"
        "  background-repeat: no-repeat;"
        "  background-position: center;"
        "  background-color: transparent;"
        "}"
        );

    // 서버 접속 및 신호 연결
    server_connect();
    connect(socket_, &QTcpSocket::connected, this, &MainWindow::on_connected);
    connect(socket_, &QTcpSocket::readyRead, this, &MainWindow::on_ready_read);
    connect(socket_, &QTcpSocket::errorOccurred, this, &MainWindow::on_error);

    //DB연결
    db = new DBManager("10.10.20.99", "WHAT", "JIN", "1234", 3306);
    if (!db->isConnected()) {
        QMessageBox::critical(this, "DB 연결 실패", "DB 연결이 안 됩니다.");
    }
    connect(ui->Btn_menu_finish_select, &QPushButton::clicked, this, &MainWindow::Btn_menu_finish_select_clicked);

    // 로그인 & 주소 처리
    connect(ui->pushButton_login, &QPushButton::clicked, this, &MainWindow::login_handling);
    connect(ui->Btn_address,     &QPushButton::clicked, this, &MainWindow::address_handling);
    connect(ui->Btn_address2,    &QPushButton::clicked, this, &MainWindow::address_handling);
    connect(ui->Btn_address, &QPushButton::clicked, this, &MainWindow::goaddress);
    connect(ui->Btn_address2, &QPushButton::clicked, this, &MainWindow::goaddress);
    connect(ui->Btnadr_basic,     &QPushButton::clicked, this, &MainWindow::go_store_and_menu);

    // 메인 네비게이션
    connect(ui->Btn_main_gohome,  &QPushButton::clicked, this, &MainWindow::gohome);
    connect(ui->Btn_main_order,   &QPushButton::clicked, this, &MainWindow::goorder);
    connect(ui->Btn_main_heart,   &QPushButton::clicked, this, &MainWindow::goheart);
    connect(ui->Btn_main_my,      &QPushButton::clicked, this, &MainWindow::gomy);
    connect(ui->Btn_main_cart,    &QPushButton::clicked, this, &MainWindow::gocart);
    connect(ui->pushButton_78,    &QPushButton::clicked, this, &MainWindow::gohome);
    // connect(ui->pushButton_77,    &QPushButton::clicked, this, &MainWindow::gohome);
    connect(ui->pushButton_heart_close, &QPushButton::clicked, this, &MainWindow::gohome);
    connect(ui->pushButton_my_close,    &QPushButton::clicked, this, &MainWindow::gohome);
    connect(ui->pushButton_order_ing,   &QPushButton::clicked, this, &MainWindow::gohome);
    connect(ui->pushButton_heart,   &QPushButton::clicked, this, &MainWindow::goheart);

    //=============[0620]=======================
    connect(ui->pushButton_pay, &QPushButton::clicked, this, &MainWindow::send_order_to_db);  // DB 저장
    connect(ui->pushButton_pay, &QPushButton::clicked, this, &MainWindow::go_page_pay);       // UI 전환


    connect(ui->Btn_goback, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
    });

    // 1번: 전체
    connect(ui->pushButton__1, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        set_rst_widget();  // 전체 매장
    });

    // 2번: 카페/디저트 (1~5)
    connect(ui->pushButton__2, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(1, 5);
    });
    connect(ui->pushButton_54, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(1, 5);
    });

    // 3번: 치킨 (6~10)
    connect(ui->pushButton__3, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(6, 10);
    });
    connect(ui->pushButton_56, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(6, 10);
    });

    // 4번: 한식 (11~15)
    connect(ui->pushButton__4, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(11, 15);
    });
    connect(ui->pushButton_58, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(11, 15);
    });

    // 5번: 중식 (16~20)
    connect(ui->pushButton__5, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(16, 20);
    });
    connect(ui->pushButton_76, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(16, 20);
    });

    // 6번: 분식 (21~25)
    connect(ui->pushButton__6, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(21, 25);
    });
    connect(ui->pushButton_79, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(21, 25);
    });

    // 7번: 피자/양식 (26~30)
    connect(ui->pushButton__7, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(26, 30);
    });
    connect(ui->pushButton_81, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(26, 30);
    });

    // 8번: 버거 (31~35)
    connect(ui->pushButton__8, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(31, 35);
    });
    connect(ui->pushButton_82, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(31, 35);
    });

    // 9번: 일식/돈까스 (36~40)
    connect(ui->pushButton__9, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(36, 40);
    });
    connect(ui->pushButton_83, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(36, 40);
    });

    // 13번: 고기/구이 (41~45)
    connect(ui->pushButton__13, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(41, 45);
    });
    connect(ui->pushButton_84, &QPushButton::clicked, this, [=](){
        ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
        showStoreIcons(41, 45);
    });

    connect(ui->    pushButton_sale,&QPushButton::clicked, this,[=](){
        ui->stackedWidget->setCurrentIndex(16);
    });

    //가게창에서 메인메뉴로
    connect(ui->pushButton_my_close_2, &QPushButton::clicked, this, &MainWindow::gohome);

    // 주문 페이지 버튼
    connect(ui->button_order1,      &QPushButton::clicked, this, &MainWindow::go_page_order2);
    connect(ui->pushButton_return_2,&QPushButton::clicked, this, &MainWindow::gocart);
    connect(ui->pushButton_pay,     &QPushButton::clicked, this, &MainWindow::go_page_pay);
    connect(ui->pushButton_return,  &QPushButton::clicked, this, &MainWindow::gohome);
    connect(ui->pushButton_return_3,&QPushButton::clicked, this, &MainWindow::gohome);

    // 주문 상세 버튼
    connect(ui->pushButton_reorder,  &QPushButton::clicked, this, &MainWindow::order_reorder);


    // 리뷰 버튼
    connect(ui->pushButton_review,         &QPushButton::clicked, this, &MainWindow::my_review);
    connect(ui->pushButton_review_close,   &QPushButton::clicked, this, &MainWindow::goorder);
    connect(ui->pushButton_review_close_2, &QPushButton::clicked, this, &MainWindow::gomy);

    connect(ui->pushButton_order_s_anstore,  &QPushButton::clicked, this, &MainWindow::goorder);
    connect(ui->pushButton_goooder_1,  &QPushButton::clicked, this, &MainWindow::goorder);
    connect(ui->pushButton_goooder_2,  &QPushButton::clicked, this, &MainWindow::goorder);
    connect(ui->pushButton_check, &QPushButton::clicked, this, &MainWindow::gohome);

    //초기숨김해야되는 위젯
    ui->frame_od->hide();
    ui->groupBox_bar->hide();
    // ui->Btn_main_cart->hide(); // 다시살려야됨 테스트해야되서 주석처리해놓음

    // 별점 초기 설정
    review_star();

    //현희추가
    connect(ui->    pushButton_53,&QPushButton::clicked, this,[=](){
        send_rstinfo_request();
    });
    connect(ui->pushButton_cancel, &QPushButton::clicked, this, &MainWindow::showCancelBottomSheet);

    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, [=](int index){
        set_Btn_menu_finish_select();
    });  //0620

}

void MainWindow::showStoreIcons(int minBrandUid, int maxBrandUid)
{
    // 1) 기존 위젯 삭제
    QLayoutItem* item;
    while ((item = ui->gridLayout_rstwidget->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    // 2) 필터링해서 추가
    int row = 0;
    for (const auto& store : lst_rst_info) {
        int uid = store.brand_uid.toInt();  // brand_uid가 QString이라면 toInt()

        // 전체 보기(1번 버튼용)일 때 maxBrandUid가 e.g. INT_MAX라면 항상 true
        if (uid < minBrandUid || uid > maxBrandUid)
            continue;

        // 해당 범위에 속하는 매장만 아이콘 생성
        rst_main_icon* icon = new rst_main_icon(
            this,
            store.name,
            3.9,        //일단 가짜값넣음
            101,        //일단 가짜값넣음2
            store.delivery_time,
            store.delivery_fee,
            store.image_main
            );
        ui->gridLayout_rstwidget->addWidget(icon, row++, 0);
    }

    qDebug() << "화면에 표시된 매장 수:" << row;
}

void MainWindow::slideTransition(QStackedWidget* sw, int toIndex)
{
    int fromIndex = sw->currentIndex();
    if (fromIndex == toIndex) return;

    QWidget *fromPage = sw->widget(fromIndex);
    QWidget *toPage   = sw->widget(toIndex);
    int w = sw->width(), h = sw->height();

    // 1) 초기 위치/크기 설정
    fromPage->setGeometry(0, 0, w, h);
    // toPage는 화면 아래(높이 만큼) 위치시켜 두고
    toPage->setGeometry(0, h, w, h);
    toPage->show();

    // 2) fromPage: 수평 슬라이드 아웃
    auto *animOut = new QPropertyAnimation(fromPage, "pos", this);
    animOut->setDuration(300);
    animOut->setStartValue(QPoint(0, 0));
    animOut->setEndValue(QPoint(-w, 0));
    animOut->setEasingCurve(QEasingCurve::OutCubic);

    // 3) toPage: 수직 슬라이드 인
    auto *animIn = new QPropertyAnimation(toPage, "pos", this);
    animIn->setDuration(300);
    animIn->setStartValue(QPoint(0, h));
    animIn->setEndValue(QPoint(0, 0));
    animIn->setEasingCurve(QEasingCurve::OutCubic);

    // 4) 병렬 그룹으로 묶어서 실행
    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(animOut);
    group->addAnimation(animIn);
    connect(group, &QParallelAnimationGroup::finished, [sw, toPage, fromPage]() {
        // 끝나면 실제 페이지 전환 및 이전 페이지 숨기기
        sw->setCurrentWidget(toPage);
        fromPage->hide();
    });
    group->start(QAbstractAnimation::DeleteWhenStopped);
}


void MainWindow::fadeTransition(int fromIndex, int toIndex)
{
    QWidget *fromPage = stack->widget(fromIndex);
    QWidget *toPage   = stack->widget(toIndex);

    auto *fromEff = qobject_cast<QGraphicsOpacityEffect*>(fromPage->graphicsEffect());
    auto *toEff   = qobject_cast<QGraphicsOpacityEffect*>(toPage->graphicsEffect());

    // 1) 페이드아웃
    auto *fadeOut = new QPropertyAnimation(fromEff, "opacity");
    fadeOut->setDuration(500);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::OutCubic);

    // 2) 잠깐 대기
    auto *pause = new QPauseAnimation(100);

    // 3) 페이드인
    auto *fadeIn = new QPropertyAnimation(toEff, "opacity");
    fadeIn->setDuration(500);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InCubic);

    // 순차 실행 그룹
    auto *seq = new QSequentialAnimationGroup(this);
    seq->addAnimation(fadeOut);
    seq->addAnimation(pause);

    // pause 단계가 시작될 때 페이지를 바꿔준다
    connect(seq, &QSequentialAnimationGroup::currentAnimationChanged, this, [=](QAbstractAnimation *anim){
        if (anim == pause) {
            stack->setCurrentIndex(toIndex);
        }
    });

    seq->addAnimation(fadeIn);
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::goToPage(int index)
{
    int curr = ui->stackedWidget->currentIndex();

    if (index == 1 && curr != 1) {
        // 0 → 1 전환 시에만 fade 효과
        fadeTransition(curr, 1);
    }
    else {
        // 그 외 (0 → 0, 1 → 0, 2 → N 등) 그냥 즉시 전환
        ui->stackedWidget->setCurrentIndex(index);
    }
}


// 서버 연결
void MainWindow::server_connect() {
    if (socket_->state() == QAbstractSocket::UnconnectedState) {
        socket_->connectToHost("10.10.20.99", SERVER_PORT);
    }
}

void MainWindow::on_connected() {
    QByteArray data = QString("user").toUtf8();
    socket_->write(data);
}

void MainWindow::on_ready_read() {
    recv_buffer += socket_->readAll();

    int endIdx;
    while ((endIdx = recv_buffer.indexOf("<<END>>")) != -1) {
        QByteArray completeMsg = recv_buffer.left(endIdx);
        recv_buffer.remove(0, endIdx + strlen("<<END>>"));

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(completeMsg, &err);

        if (err.error != QJsonParseError::NoError) {
            qDebug() << "[JSON 파싱 오류]" << err.errorString();
            continue;
        }

        QJsonObject json = doc.object();
        if (responseHandler_) {
            responseHandler_(json);
            responseHandler_ = nullptr;
        }
    }
}

void MainWindow::on_error(QAbstractSocket::SocketError err) {
    if (err != QAbstractSocket::RemoteHostClosedError)
        QMessageBox::critical(this, "소켓 에러", socket_->errorString());
}

// 로그인 처리
void MainWindow::login_handling() {
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[login_handling] 소켓 연결 안 됨";
        server_connect();
        connect(socket_, &QTcpSocket::connected, this, [=]() {
            send_login_request();
        });
        return;
    }

    send_login_request();
}

void MainWindow::send_login_request() {
    QJsonObject loginJson;
    loginJson["action"]   = "1_0";
    // loginJson["email_id"] = ui->lineEdit_id->text();
    // loginJson["pw"]       = ui->lineEdit_pw->text();
    loginJson["email_id"] = "seonseo@naver.com";
    loginJson["pw"]       = "1216";


    responseHandler_ = [this](const QJsonObject& o) {
        // action 비교
        if (o["action"].toString() == "1_1") {
            ui->stackedWidget->setCurrentIndex(GO_HOME);
            showAdPopup();
            ui->groupBox_bar->show();

            // uid가 JSON 문자열일 때
            user_info.uid   = o["uid"].toString().toInt();

            // 만약 서버가 숫자 타입으로 보내고 있다면
            user_info.Email_id = o["email_id"].toString();
            user_info.name     = o["name"].toString();
            user_info.birth    = o["birth"].toString();
            user_info.phonenum = o["phonenumber"].toString();

            qDebug() << "로그인된 uid =" << user_info.uid;

        }
    };
    socket_->write(QJsonDocument(loginJson).toJson(QJsonDocument::Compact));
}

// 주소 처리
void MainWindow::address_handling() {
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        server_connect();
        connect(socket_, &QTcpSocket::connected, this, &MainWindow::send_address_request);
    } else {
        send_address_request();
    }
}

void MainWindow::send_address_request() {
    QJsonObject adrJson;
    adrJson["action"] = "7_0";
    adrJson["uid"]    = QString::number(user_info.uid);

    responseHandler_ = [this](const QJsonObject& o) {
        if (o["action"] == "7_1") {
            lst_adr.clear(); // 리스트 비우기
            //주소 구조체 리스트 처리
            QJsonArray adrArray = o["address_list"].toArray();
            for (const QJsonValue& val : adrArray) {
                QJsonObject obj = val.toObject();
                address addr;
                addr.adr_name    = obj["address_name"].toString();
                addr.adr         = obj["address"].toString();
                addr.adr_detail  = obj["address_detail"].toString();
                addr.adr_type    = obj["address_type"].toString();
                addr.adr_default = obj["address_basic"].toString().toInt();
                lst_adr.append(addr);
            }
            set_addrBtn_widget();  //버튼 쓰기

            // address_base 있을 때만 시군구 갱신
            if (o.contains("address_base")) {
                cityToDistricts.clear();
                QJsonArray areaArray = o["address_base"].toArray();
                for (const QJsonValue& val : areaArray) {
                    QJsonObject obj = val.toObject();
                    QString city = obj["city"].toString();
                    QString district = obj["district"].toString();
                    cityToDistricts[city].append(district);
                }
                set_city_cbb();
            }
        } else QMessageBox::warning(this, "주소 오류", o["message"].toString());
    };
    socket_->write(QJsonDocument(adrJson).toJson(QJsonDocument::Compact));
}

void MainWindow::set_city_cbb() {
    ui->cbb_adr1->clear();
    ui->cbb_adr1->addItems(cityToDistricts.keys());

    static bool combo_connected = false;
    if (!combo_connected) {
        connect(ui->cbb_adr1, &QComboBox::currentTextChanged, this, [=](const QString& city) {
            ui->cbb_adr2->clear();
            ui->cbb_adr2->addItems(cityToDistricts[city]);
        });
        combo_connected = true;
    }

    static bool add_connected = false;
    if (!add_connected) {
        connect(ui->Btn_adr_save, &QPushButton::clicked, this, &MainWindow::send_address_add);
        add_connected = true;
    }

    // 수동 초기값 설정
    if (!cityToDistricts.isEmpty()) {
        QString firstCity = cityToDistricts.keys().first();
        ui->cbb_adr1->setCurrentText(firstCity);
        ui->cbb_adr2->clear();
        ui->cbb_adr2->addItems(cityToDistricts[firstCity]);
    }
}

void MainWindow::set_addrBtn_widget() {

    if (lst_adr.isEmpty()) {
        qDebug() << "주소 목록이 비어 있습니다.";
        return;
    }

    // 기존 위젯 삭제
    QLayoutItem* item;
    while ((item = ui->gridLayout_adress_scroll_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    lst_adr_btn.clear();

    int row = 0;
    for (int i = 0; i < lst_adr.size(); i++) {
        if (lst_adr[i].adr_default != 1) {
            QString adrText = lst_adr[i].adr + " " + lst_adr[i].adr_detail;
            addressBtn* adrWidget = new addressBtn(this, adrText, lst_adr[i].adr_detail);
            lst_adr_btn.append(adrWidget);
            ui->gridLayout_adress_scroll_->addWidget(adrWidget, row++, 0);
            connect(adrWidget, &addressBtn::Address_clicked, this, &MainWindow::Address_clicked);
            connect(adrWidget, &addressBtn::Address_delete_clicked,this, &MainWindow::Address_delete_clicked);
        }
        else {
            this->DefaultAdr = lst_adr[i].adr + " " + lst_adr[i].adr_detail;
            this->DefaultAdr_city = lst_adr[i].adr;
            this->DefaultAdr_detail = lst_adr[i].adr_detail;

            // 여기서 시군구 주소만 저장
            selected_city_address = lst_adr[i].adr;
            qDebug() << "[기본주소 기준 selected_city_address] =" << selected_city_address;
        }
    }

    ui->Btnadr_basic->setText(DefaultAdr);
    qDebug() << "버튼에 설정된 주소 이름:" << DefaultAdr;
}

void MainWindow::set_AddressBasic_btn(){
    qDebug() <<"버튼에 설정될 기본 주소지" << this->DefaultAdr;
    ui->Btn_address->setText(DefaultAdr);
    ui->Btn_address2->setText(DefaultAdr);
}
void MainWindow::Address_clicked(const QString& address_detail){
    qDebug() <<"내가~ 선택한 주소는: " << address_detail;

    for (const address& addr : lst_adr) {
        if (addr.adr_detail == address_detail) {
            selected_city_address = addr.adr;
            break;
        }
    }

    // 서버에 기본 주소 변경 요청
    QJsonObject adrJson_select;
    adrJson_select["action"] = "11_0";
    adrJson_select["uid"] = QString::number(user_info.uid);
    adrJson_select["address_detail"] = address_detail;

    responseHandler_ = [this,address_detail](const QJsonObject& json) {
        if (json["action"] == "11_1") {
            qDebug() << "기본 주소 변경 완료";

            for (address& addr : lst_adr) {
                if (addr.adr_detail == address_detail) {
                    addr.adr_default = 1;
                } else {
                    addr.adr_default = 0;
                }
            }

            set_addrBtn_widget();  // 즉시 UI 반영
            set_AddressBasic_btn(); //버튼 텍스트 갱신하기
            send_rstinfo_request(); //해당 주소의 가게 불러오기

        } else {
            QMessageBox::warning(this, "실패", json["message"].toString());
        }
    };

    socket_->write(QJsonDocument(adrJson_select).toJson(QJsonDocument::Compact));
}

void MainWindow::Address_delete_clicked(const QString& address_detail, QWidget* sender){
    qDebug() << "내가~ 삭제할 주소는: " << address_detail;

    // 서버에 삭제 요청
    QJsonObject adrJson_delete;
    adrJson_delete["action"] = "9_0";
    adrJson_delete["uid"] = QString::number(user_info.uid);
    adrJson_delete["del_address_detail"] = address_detail;
    socket_->write(QJsonDocument(adrJson_delete).toJson(QJsonDocument::Compact));
    qDebug() << "서버에 보낸 주소는~: " << address_detail;

    // UI에서 해당 위젯 삭제
    ui->gridLayout_adress_scroll_->removeWidget(sender);
    sender->deleteLater();  // 안전하게 삭제 예약

    // 리스트에서도 제거
    for (int i = 0; i < lst_adr_btn.size(); ++i) {
        if (lst_adr_btn[i] == sender) {
            lst_adr_btn.removeAt(i);
            break;
        }
    }
    for (int i = 0; i < lst_adr.size(); ++i) {
        if (lst_adr[i].adr_detail == address_detail) {
            lst_adr.removeAt(i);
            break;
        }
    }
}

void MainWindow::send_address_add() {
    QString city = ui->cbb_adr1->currentText();
    QString district = ui->cbb_adr2->currentText();
    QString new_address = city + " " + district;
    QString detail = ui->lineEdit_addressdetail->text().trimmed();
    QString name = ui->lineEdit_set_adrname->text();

    if (city.isEmpty() || district.isEmpty() || detail.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "모든 항목을 입력해 주세요.");
        return;
    }

    QJsonObject newAddrJson;
    newAddrJson["action"] = "8_0";
    newAddrJson["uid"] = QString::number(user_info.uid);
    newAddrJson["new_address"] = new_address;
    newAddrJson["new_address_detail"] = detail;
    newAddrJson["address_name"] = name;
    newAddrJson["new_address_type"] = "3";

    //서버에 요청은 보냄 ,임시로 UI에 반영
    responseHandler_ = [this, new_address, detail, name](const QJsonObject& json) {
        if (json["action"] == "8_1") {
            QMessageBox::information(this, "주소 추가", "주소가 추가되었습니다.");

            //직접 일단 추가하기
            address newAddr;
            newAddr.adr = new_address;
            newAddr.adr_detail = detail;
            newAddr.adr_name = name;
            newAddr.adr_type = "3";
            newAddr.adr_default = 0;

            lst_adr.append(newAddr);
            set_addrBtn_widget();  // 바로 UI 갱신

            // 입력창 초기화
            ui->lineEdit_addressdetail->clear();
            ui->lineEdit_set_adrname->clear();
        } else {
            QMessageBox::warning(this, "실패", json["message"].toString());
        }
    };

    socket_->write(QJsonDocument(newAddrJson).toJson(QJsonDocument::Compact));
}

//----[가게 리스트 출력]----
// 가게 정보 요청
void MainWindow::send_rstinfo_request(){
    QJsonObject rstinfo;
    rstinfo["action"] = "12_0";
    rstinfo["address"] = selected_city_address;
    qDebug() << "주소~" << selected_city_address;

    responseHandler_ = [this](const QJsonObject& o) {
        if (o["action"] == "12_1") {
            qDebug() << "[가게 정보 수신 완료]";
            lst_rst_info.clear();  // 가게 리스트 비우기

            QJsonArray rst_Array = o["address_list"].toArray();
            for (const QJsonValue& val : rst_Array) {
                QJsonObject obj = val.toObject();
                store_info rst_info;

                rst_info.brand_uid              = obj["BRAND_UID"].toString();
                rst_info.place_uid              = obj["PLACE_UID"].toString();
                rst_info.name                   = obj["NAME"].toString();
                rst_info.address                = obj["ADDRESS"].toString();
                rst_info.address_detail         = obj["ADDRESS_DETAIL"].toString();
                rst_info.open_date              = obj["OPEN_DATE"].toString();
                rst_info.phonenumber            = obj["PHONENUMBER"].toString();
                rst_info.delivery_time          = obj["DELIVERY_TIM"].toString();
                rst_info.open_time              = obj["OPEN_TIME"].toString();
                rst_info.close_time             = obj["CLOSE_TIME"].toString();
                rst_info.origin_info            = obj["ORIGIN_INFO"].toString();
                rst_info.min_order_price        = obj["MIN_ORDER_PRICE"].toString();
                rst_info.delivery_fee           = obj["DELIVERY_FEE"].toString();
                rst_info.free_delivery_standard = obj["FREE_DELIVERY_STANDARD"].toString();
                rst_info.image_main             = obj["IMAGE_MAIN"].toString();
                rst_info.image_pass             = obj["IMAGE_PASS"].toString();

                lst_rst_info.append(rst_info);  // 저장
            }

            qDebug() << "총 가게 수:" << lst_rst_info.size();
            set_rst_widget();

        } else {
            QMessageBox::warning(this, "가게 정보 오류", o["message"].toString());
        }
    };

    socket_->write(QJsonDocument(rstinfo).toJson(QJsonDocument::Compact));
}

//가게 아이콘 생성
void MainWindow::set_rst_widget(){
    QLayoutItem * item;
    int row = 0;
    while ((item = ui->gridLayout_rstwidget->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    for (const store_info& store : lst_rst_info) {
        rst_main_icon* icon = new rst_main_icon(this,
                                                store.name,
                                                4.0,  // 별점 예시
                                                128,  // 리뷰 개수 예시
                                                store.delivery_time,
                                                store.delivery_fee,
                                                store.image_main);
        ui->gridLayout_rstwidget->addWidget(icon,row++,0);
        connect(icon, &rst_main_icon::icon_clicked, this, [=]() {
            this->selected_store = store;
            qDebug() << "선택한 가게 이름" << selected_store.name;
            go_selected_store();
            send_menu_request();
        });
    }
}


//----[가게 정보, 메뉴 출력]----
void MainWindow::set_rst_page(){
    ui->label_rst_name->setText(selected_store.name);
    ui->label_main_img->setStyleSheet(QString("border-image: url(%1)").arg(selected_store.image_main));
    ui->label_deliverytime->setText(QString("평균 배달시간 %1").arg(selected_store.delivery_time));
    ui->label_deliveryfee->setText(QString("예상 배달비 %1원").arg(selected_store.delivery_fee));
    ui->label_delivery_minimum->setText(QString("최소 배달 금액 %1원").arg(selected_store.min_order_price));
}

//----[가게 메뉴 출력]----
void MainWindow::send_menu_request(){
    QJsonObject menuinfo;
    menuinfo["action"] = "13_0";
    menuinfo["brand_uid"] = selected_store.brand_uid;
    menuinfo["place_uid"] = selected_store.place_uid;
    responseHandler_ = [this](const QJsonObject& o) {
        if (o["action"] == "13_1") {
            qDebug() << "[메뉴 정보 수신 완료]";
            lst_menu.clear();  // 메뉴 리스트 비우기

            QJsonArray rst_Array = o["menu_list"].toArray();
            for (const QJsonValue& val : rst_Array) {
                QJsonObject obj = val.toObject();
                menu menus;
                menus.brand_uid      = obj["BRAND_UID"].toString();
                menus.name           = obj["MENU_NAME"].toString();
                menus.category       = obj["CATEGORY"].toString();
                menus.menu_info      = obj["MENU_INFO"].toString();
                menus.img            = obj["MENU_IMG"].toString();
                menus.price          = obj["MENU_PRICE"].toString();

                menus.option1        = obj["OPTION1_NAME"].toString();
                menus.option1_price  = obj["OPTION1_PRICE"].toString();

                menus.option2        = obj["OPTION2_NAME"].toString();
                menus.option2_price  = obj["OPTION2_PRICE"].toString();

                menus.option3        = obj["OPTION3_NAME"].toString();
                menus.option3_price  = obj["OPTION3_PRICE"].toString();

                menus.option4        = obj["OPTION4_NAME"].toString();
                menus.option4_price  = obj["OPTION4_PRICE"].toString();

                menus.option5        = obj["OPTION5_NAME"].toString();
                menus.option5_price  = obj["OPTION5_PRICE"].toString();

                menus.option6        = obj["OPTION6_NAME"].toString();
                menus.option6_price  = obj["OPTION6_PRICE"].toString();

                menus.option7        = obj["OPTION7_NAME"].toString();
                menus.option7_price  = obj["OPTION7_PRICE"].toString();

                menus.option8        = obj["OPTION8_NAME"].toString();
                menus.option8_price  = obj["OPTION8_PRICE"].toString();

                menus.option9        = obj["OPTION9_NAME"].toString();
                menus.option9_price  = obj["OPTION9_PRICE"].toString();

                menus.option10       = obj["OPTION10_NAME"].toString();
                menus.option10_price = obj["OPTION10_PRICE"].toString();

                lst_menu.append(menus);  // 저장
                qDebug() << "메뉴이름:" << menus.name << " | 카테고리:" << menus.category;
            }

            qDebug() << "총 메뉴 수:" << lst_menu.size();
            find_category_list();
            add_menu_head_widget();
            add_menu_normal_widget();
        } else {
            QMessageBox::warning(this, "메뉴 정보 오류", o["message"].toString());
        }
    };
    socket_->write(QJsonDocument(menuinfo).toJson(QJsonDocument::Compact));
}

void MainWindow::find_category_list() {
    category_set.clear();
    for (const menu& m : lst_menu) {
        if (m.category != "대표메뉴")
            category_set.insert(m.category);
    }
    category_list = category_set.values();

    for (const QString& cat : category_list)
        qDebug() << "[카테고리]" << cat;
}


void MainWindow::add_menu_head_widget(){
    // 기존 대표메뉴 삭제
    QLayoutItem* item;
    while ((item = ui->Layout_menu_head->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (const menu& m : lst_menu) {
        if (m.category != "대표메뉴") continue;

        menu_head* head = new menu_head(this, m.img, m.name, m.price);
        head->setMinimumSize(120, 150);
        head->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        // 시그널 연결
        connect(head, &menu_head::head_clicked, this, &MainWindow::onMenuHeadClicked);
        // 레이아웃에 추가
        ui->Layout_menu_head->addWidget(head);
    }
}

void MainWindow::add_menu_normal_widget() {
    // 기존 기타메뉴 제거
    QLayoutItem* item;
    while ((item = ui->layout_menu_normal->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // 카테고리별로 메뉴 정리
    QMap<QString, QList<menu>> cat_menus;
    for (const menu& m : lst_menu) {
        if (m.category == "대표메뉴") continue;
        cat_menus[m.category].append(m);
    }

    // 각 카테고리마다 추가
    for (const QString& cat : cat_menus.keys()) {
        // 카테고리 제목
        QLabel* label = new QLabel(cat);
        label->setStyleSheet("font-size: 14px; font-weight: bold; color: black;");
        ui->layout_menu_normal->addWidget(label);

        // 메뉴 묶을 컨테이너
        QVBoxLayout* layout = new QVBoxLayout;
        QWidget* container = new QWidget;
        container->setLayout(layout);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        // 메뉴 항목들
        for (const menu& m : cat_menus[cat]) {
            menu_normal* normal = new menu_normal(this, m.img, m.name, m.price, m.menu_info);
            connect(normal, &menu_normal::normal_clicked, this, &MainWindow::onMenuHeadClicked);
            layout->addWidget(normal);
        }

        // 카테고리 박스 추가
        ui->layout_menu_normal->addWidget(container);
    }
    ui->layout_menu_normal->addStretch();  // 끝에 여백 추가
}



void MainWindow::set_sel_menu_widget(){
    // 기존 UI 위젯 제거
    QLayoutItem *child;
    while ((child = ui->gridLayout_menu_sel->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    QString img = "경로/이미지.png";
    QString name = "메뉴명";
    QString price = "10000";

    menu_head* head = new menu_head(this, img, name, price);
    connect(head, &menu_head::head_clicked, this, &MainWindow::onMenuHeadClicked);
    ui->gridLayout_menu_sel->addWidget(head);
}


void MainWindow::onMenuHeadClicked(QString menu_name){
    // 메뉴 리스트에서 찾기
    for (const menu& m : lst_menu) {
        if (m.name == menu_name) {
            qDebug() << "찾은 메뉴:" << m.name;

            // 기존 위젯 삭제
            if (selectedMenuInfoWidget) {
                selectedMenuInfoWidget->deleteLater();
                selectedMenuInfoWidget = nullptr;
            }

            // store_info 찾기: brand_uid로 비교
            store_info matched_store;
            bool found = false;

            for (const store_info& store : lst_rst_info) {
                if (store.brand_uid == m.brand_uid) {
                    matched_store = store;
                    found = true;
                    break;
                }
            }

            if (!found) {
                qDebug() << "해당 메뉴의 가게 정보가 없습니다.";
                return;
            }

            // 새 위젯 생성, 스택드 위젯에 넣기
            selectedMenuInfoWidget = new sel_menu_info(this, m, matched_store);

            // 주문 완료 시그널 연결
            connect(selectedMenuInfoWidget, &sel_menu_info::orderCompleted, this, &MainWindow::menu_selected);
            selectedMenuInfoWidget->updateTotalPrice();

            // < 버튼 시그널 연결 페이지 4로
            connect(selectedMenuInfoWidget, &sel_menu_info::backButtonClicked, this, [=]() {
                ui->stackedWidget->setCurrentIndex(4);  // 메인화면으로 이동
            });

            // 페이지 전환
            int pageIndex = 17;
            ui->stackedWidget->insertWidget(pageIndex, selectedMenuInfoWidget);
            ui->stackedWidget->setCurrentIndex(pageIndex);

            break;
        }
    }
    set_Btn_menu_finish_select();

}

void MainWindow::menu_selected(const ORDER_DETAIL& selectedMenu) {
    // 동일한 메뉴 + 옵션이 있으면 수량만 증가
    for (ORDER_DETAIL& order : orderList) {
        if (order.MENU_NAME == selectedMenu.MENU_NAME &&
            order.OPT_NAME_ALL == selectedMenu.OPT_NAME_ALL) {
            order.MENU_CNT += selectedMenu.MENU_CNT;
            qDebug() << "메뉴 수량 증가:" << order.MENU_NAME << ", 현재 수량:" << order.MENU_CNT;
            update_ui();  // 총합, 수량 등 UI 갱신
            set_Btn_menu_finish_select();
            return;
        }
    }

    // 없으면 새로 추가
    orderList.append(selectedMenu);
    qDebug() << "메뉴 추가됨:" << selectedMenu.MENU_NAME;

    update_ui();  // 총합, 수량 등 UI 갱신
    set_Btn_menu_finish_select();
}

void MainWindow::menu_deleted(const QString& menu_name, const QString& opt_name_all) {
    for (int i = 0; i < orderList.size(); ++i) {
        if (orderList[i].MENU_NAME == menu_name &&
            orderList[i].OPT_NAME_ALL == opt_name_all) {
            orderList.removeAt(i);
            break;
        }
    }

    // UI 갱신
    Btn_menu_finish_select_clicked();  // 위젯 재생성
    set_Btn_menu_finish_select();      // 금액 재계산
    update_ui();
}

int MainWindow::calculate_total_sum(){
    int total = 0;
    for (const ORDER_DETAIL& d : orderList) {
        total += (d.MENU_PRICE + d.OPT_PRICE_ALL) * d.MENU_CNT;
    }
    return total;
}

void MainWindow::update_ui() {  //0620
    ui->groupBox_8->show();
    ui->pushButton_cnt->setText(QString::number(orderList.size()));
    ui->pushButton_cnt_2->setText(QString::number(orderList.size()));

    int total = calculate_total_sum();
    QString formatted = QLocale(QLocale::Korean).toString(total);
    ui->Btn_menu_finish_select->setText(formatted + "원 배달 주문하기");

    // 버튼 상태는 이 함수로 일원화
    set_Btn_menu_finish_select();
}

void MainWindow::set_Btn_menu_finish_select() {  //0620
    QLocale locale(QLocale::Korean);
    int total = calculate_total_sum();
    QString formattedSum = locale.toString(total);
    ui->Btn_menu_finish_select->setText(QString("%1원 배달 주문하기").arg(formattedSum));

    bool isEmpty = orderList.isEmpty();
    int currPage = ui->stackedWidget->currentIndex();
    bool show = !isEmpty && (currPage == GO_STORE_SELECT);

    ui->Btn_menu_finish_select->setVisible(show);
    ui->pushButton_cnt_2->setVisible(show);
    ui->frame_menu_finish_select->setVisible(show);

    ui->frame_menu_finish_select->setMaximumHeight(show ? 16777215 : 0);
    ui->frame_menu_finish_select->setMinimumHeight(show ? 50 : 0);
}

void MainWindow::Btn_menu_finish_select_clicked() {
    // 기존 위젯 제거
    QLayoutItem* item;
    while ((item = ui->verticalLayout_9->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    qDebug() << "orderList 개수:" << orderList.size();
    // orderList에 있는 주문 정보로 위젯 생성
    for (const ORDER_DETAIL& order : orderList) {
        int item_total = (order.MENU_PRICE + order.OPT_PRICE_ALL) * order.MENU_CNT;

        selected_menu_widget* widget = new selected_menu_widget(this, order, item_total);   //여기에 정보 다 들어있어서 보냄
        ui->verticalLayout_9->addWidget(widget);
        //21일
        ui->label_pnum->setText(user_info.phonenum);
        ui->label_where->setText(user_adr.adr_name);

        // 시그널 연결
        // 수량 변경 시 총금액 갱신
        connect(widget, &selected_menu_widget::orderUpdated, this, &MainWindow::handleOrderUpdated);
        // x버튼 누르면 삭제 처리
        connect(widget, &selected_menu_widget::orderDeleted, this, &MainWindow::menu_deleted);
    }

    set_Btn_menu_finish_select(); //금액 갱신

    // 총합 계산
    QLocale locale(QLocale::Korean);
    int total = 0;
    for (const ORDER_DETAIL& d : orderList) {
        total += (d.MENU_PRICE + d.OPT_PRICE_ALL) * d.MENU_CNT;
    }
    QString formattedSum = locale.toString(total);
    ui->button_order1->setText(QString("%1원 배달 결제하기").arg(formattedSum));
    ui->stackedWidget->setCurrentIndex(10);
    add_recommend_menu_widget(); // 함께먹으면 좋은 음식
    update_button_order1_enabled();  // 수량 변동 등 이후 활성화 상태 갱신
    ui->label_79->setText(QString("가게배달 %1 후 도착").arg(selected_store.delivery_time));
}

void MainWindow::update_button_order1_enabled() {   //t새로운함수
    int total = calculate_total_sum();
    int min_order_price = selected_store.min_order_price.toInt();
    ui->button_order1->setEnabled(total >= min_order_price);
}


void MainWindow::add_recommend_menu_widget(){
    QLayout* layout = ui->scrollAreaWidgetContents_20->layout();
    if (!layout) return;

    // 기존 위젯 제거
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // 현재 가게의 brand_uid
    QString brand_uid = selected_store.brand_uid;
    int count = 0;

    for (const menu& m : lst_menu) {
        if (m.brand_uid == brand_uid && m.category == "사이드") {
            // 위젯 생성 및 추가
            menu_head* widget = new menu_head(this, m.img, m.name, m.price);
            connect(widget, &menu_head::head_clicked, this, &MainWindow::onMenuHeadClicked);
            layout->addWidget(widget);
            count++;
        }
    }
    if (count == 0) {
        QLabel* label = new QLabel("추천할 사이드 메뉴가 없습니다.");
        label->setStyleSheet("color: gray; font-style: italic;");
        layout->addWidget(label);
    }

    layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void MainWindow::handleOrderUpdated(const ORDER_DETAIL& updatedOrder) {
    // orderList에서 해당 메뉴 갱신
    for (ORDER_DETAIL& order : orderList) {
        if (order.MENU_NAME == updatedOrder.MENU_NAME &&
            order.OPT_NAME_ALL == updatedOrder.OPT_NAME_ALL) {
            order.MENU_CNT = updatedOrder.MENU_CNT;
            break;
        }
    }

    int total = calculate_total_sum();
    QString formatted = QLocale(QLocale::Korean).toString(total);
    ui->button_order1->setText(QString("%1원 배달 결제하기").arg(formatted));

    // 최소 주문 금액 기준 버튼 활성화 여부 결정
    int min_order_price = selected_store.min_order_price.toInt();
    ui->button_order1->setEnabled(total >= min_order_price);
}


// 주문 요청
QString MainWindow::gen_order_id() {
    QString letters;
    for (int i = 0; i < 3; ++i)
        letters.append(QChar('A' + QRandomGenerator::global()->bounded(26)));

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1_%2").arg(letters, timestamp);
}

void MainWindow::send_order_to_db(){
    ORDER newOrder;
    this->user_rst_msg = ui->user_rst_msg->text();
    this->user_rider_msg = ui->user_rider_msg->text();
    clearScrollAreaWidgets();

    newOrder.ORDER_ID = this->gen_order_id();  // 주문번호 생성
    newOrder.UID = user_info.uid;
    newOrder.BRAND_UID = selected_store.brand_uid.toInt();
    newOrder.PLACE_UID = selected_store.place_uid.toInt();
    newOrder.TOTAL_PRICE = calculate_total_sum();
    newOrder.ADDRESS = DefaultAdr;
    newOrder.ADDRESS_DETAIL = selected_store.address_detail;
    newOrder.ORDER_MSG = user_rst_msg;
    newOrder.RIDER_MSG = user_rider_msg;
    newOrder.STATUS_TOGO = 1;                    // 배달
    newOrder.STATUS_DISPOSIBLE = true;           // 일회용 선택
    newOrder.STATUS_ORDER = 0;                   // 주문 접수 대기

    newOrder.ORDER_TIME = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // 주문 상세 항목 준비
    QList<ORDER_DETAIL> orderDetails = orderList;  // 기존 장바구니 복사
    for (ORDER_DETAIL& d : orderDetails) {
        d.ORDER_ID = newOrder.ORDER_ID;  // 주문번호 일괄 매핑
    }
    // 실제 DB insert
    db->db_send_order(newOrder, orderDetails);

    QMessageBox::information(this, "주문 완료", "주문이 접수되었습니다!");

    orderList.clear();
    update_ui();

    orders_all.append(newOrder);
    order_details_all.append(orderDetails);

    go_page_pay();
}

void MainWindow::send_order_request() {
    // QJsonObject req;
    // req["action"] = "10_0";
    // req["uid"]    = QString::number(user_info.uid);

    // responseHandler_ = [this](const QJsonObject& resp) {
    //     orders_all.clear();
    //     order_details_all.clear();

    //     // 주문 데이터 파싱
    //     for (auto v : resp["orders"].toArray()) {
    //         auto o = v.toObject();
    //         ORDER ord;
    //         ord.ORDER_ID        = o["order_id"].toString();
    //         ord.UID             = o["uid"].toInt();
    //         ord.BRAND_UID       = o["brand_uid"].toInt();
    //         ord.PLACE_UID       = o["place_uid"].toInt();
    //         ord.TOTAL_PRICE     = o["total_price"].toInt();
    //         ord.ORDER_TIME      = o["order_time"].toString();
    //         ord.ADDRESS         = o["address"].toString();
    //         ord.ADDRESS_DETAIL  = o["address_detail"].toString();
    //         ord.ORDER_MSG       = o["order_msg"].toString();
    //         ord.RIDER_MSG       = o["rider_msg"].toString();
    //         ord.STATUS_TOGO     = o["status_togo"].toInt();
    //         ord.STATUS_DISPOSIBLE = o["status_disposible"].toBool();
    //         ord.STATUS_ORDER    = o["status_order"].toInt();
    //         orders_all.append(ord);

    //         QList<ORDER_DETAIL> dl;
    //         for (auto dv : o["details"].toArray()) {
    //             auto d = dv.toObject();
    //             ORDER_DETAIL it;
    //             it.ORDER_ID      = d["order_id"].toString();
    //             it.MENU_NAME     = d["menu_name"].toString();
    //             it.MENU_PRICE    = d["menu_price"].toInt();
    //             it.MENU_CNT      = d["menu_cnt"].toInt();
    //             it.OPT_NAME_ALL  = d["opt_name_all"].toString();
    //             it.OPT_PRICE_ALL = d["opt_price_all"].toInt();
    //             dl.append(it);
    //         }
    //         order_details_all.append(dl);
    //     }
    //     show_order_list();
    // };
    // socket_->write(QJsonDocument(req).toJson(QJsonDocument::Compact));
}

// 주문 리스트 표시
void MainWindow::show_order_list() {
    // frame_orderlist 안의 레이아웃 가져오기
    QLayout* layout = ui->frame_orderlist->layout();

    // 먼저 기존 위젯 삭제 (중복 생성 방지)
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();  // 위젯 메모리 해제
        }
        delete item;
    }

    // 여백(margin)과 간격(spacing)을 최소화해서 위젯 사이 공간 줄이기
    layout->setContentsMargins(0, 0, 0, 0);  // 레이아웃 바깥 여백 제거
    layout->setSpacing(4);                  // 위젯 사이 간격 4픽셀로 설정

    // 위젯 동적 생성
    int count = qMin(orders_all.size(), order_details_all.size());
    for (int i = 0; i < count; ++i) {
        // 주문과 주문 상세 데이터로 위젯 생성
        order_list_widget *widget = new order_list_widget(this, orders_all[i], order_details_all[i]);

        // 레이아웃에 위젯 추가
        layout->addWidget(widget);

        // 시그널-슬롯 연결
        connect(widget, &order_list_widget::order_click_signal, this, &MainWindow::go_order_list_particular);
        connect(widget, &order_list_widget::order_reorder_signal, this, &MainWindow::order_reorder);
        connect(widget, &order_list_widget::order_review_signal, this, &MainWindow::write_review);
    }

    // 디버깅용 출력
    qDebug() << "orders_all size:" << orders_all.size();
    for (const auto& order : orders_all) {
        qDebug() << order.ORDER_ID << order.ORDER_TIME;
    }

    qDebug() << "order_details_all size:" << order_details_all.size();
    for (const auto& details_list : order_details_all) {
        for (const auto& detail : details_list) {
            qDebug() << detail.MENU_NAME << detail.MENU_CNT;
        }
        qDebug() << "-----"; // 주문별 구분선
    }
}

void MainWindow::gocart(){      //0620 진영 추가
    set_Btn_menu_finish_select(); //금액 갱신 추가 지닞니닌짖ㄴ
    // 총합 계산
    QLocale locale(QLocale::Korean);
    int total = 0;
    for (const ORDER_DETAIL& d : orderList) {
        total += (d.MENU_PRICE + d.OPT_PRICE_ALL) * d.MENU_CNT;
    }
    QString formattedSum = locale.toString(total);

    ui->button_order1->setText(QString("%1원 배달 결제하기").arg(formattedSum));

    ui->stackedWidget->setCurrentIndex(GO_CART);
}

//========================[0621]===============================
void MainWindow::go_page_order2() {
    // 페이지 이동
    ui->stackedWidget->setCurrentIndex(GO_CART2);

    // 총합 계산
    int total = 0;
    for (const ORDER_DETAIL& d : orderList) {
        total += (d.MENU_PRICE + d.OPT_PRICE_ALL) * d.MENU_CNT;
    }
    QString formattedSum = QLocale(QLocale::Korean).toString(total); //메뉴 총 금액

    // 배달비 포함
    int deliveryFee = selected_store.delivery_fee.toInt();  // 문자열 -> 정수 변환
    int finalTotal = total + deliveryFee;

    QString asd = QLocale(QLocale::Korean).toString(finalTotal);   //배달비랑 다 더한금액

    // 라벨과 버튼에 출력
    ui->label_totall_price1->setText(formattedSum + "원");   //메뉴 금액 출력
    int dfee = selected_store.delivery_fee.toInt(); //배달비 금액 출
    QString formattedFee = QLocale(QLocale::Korean).toString(dfee);
    ui->label_dfee->setText(formattedFee + "원");

    ui->label_totall_price2->setText(asd + "원");    //총 결제금액출력
    ui->pushButton_pay->setText(asd + "원 결제하기");    //결제버튼 출력
}

// 페이지 전환 함수들
//=============================[0621]===============================
void MainWindow::gohome() {
    ui->stackedWidget->setCurrentIndex(GO_HOME);

    ui->Btn_main_gohome->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/home2.png) 0 0 0 0 stretch stretch;""}"); /////////////////////////////////
    ui->Btn_main_heart->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/heart1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_my->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/my1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_order->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/order1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    clear_order_messages();  //주문 메시지 초기화
    selected_store = store_info(); //선택한 가게 정보 초기화
}
void MainWindow::goorder() {
    ui->stackedWidget->setCurrentIndex(GO_ORDER);

    ui->Btn_main_gohome->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/home1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_heart->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/heart1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_my->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/my1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_order->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/order2.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////

    send_order_request();
    show_order_list();
}

void MainWindow::goheart(){
    ui->stackedWidget->setCurrentIndex(GO_HEART);

    ui->Btn_main_gohome->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/home1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_heart->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/heart2.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_my->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/my1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_order->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/order1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
}
void MainWindow::gomy(){
    ui->stackedWidget->setCurrentIndex(GO_MY);

    ui->pushButton_nickname->setText(user_info.name);
    ui->pushButton_email->setText(user_info.Email_id);

    ui->Btn_main_gohome->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/home1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_heart->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/heart1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_my->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/my2.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
    ui->Btn_main_order->setStyleSheet("QPushButton {""border-image: url(:/image/main_img/order1.png) 0 0 0 0 stretch stretch;""}");/////////////////////////////////
}
void MainWindow::goaddress()                  { ui->stackedWidget->setCurrentIndex(GO_ADDRESS); ui->lineEdit_addressdetail->clear(); ui->lineEdit_set_adrname->clear();}
void MainWindow::go_order_list_particular(){
    ui->stackedWidget->setCurrentIndex(GO_ORDER_DETAIL);
    ui->label_my_phonenum->setText(user_info.phonenum);
}
void MainWindow::write_review() {
    ui->stackedWidget->setCurrentIndex(GO_WRITE_REVIEW);
    review_star();
}
void MainWindow::my_review()                  { ui->stackedWidget->setCurrentIndex(GO_MY_REVIEW); }
//결제 완료 3초 후 닫히고 주문완료 창
void MainWindow::clearScrollAreaWidgets() {
    // scrollArea_20이 가진 위젯
    QWidget* scrollContent = ui->scrollArea_20->widget();
    if (!scrollContent) return;

    // 그 위젯의 레이아웃이 horizontalLayout_12라고 가정
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(scrollContent->layout());
    if (!layout) return;

    // 모든 항목 제거
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();  // 안전하게 위젯 제거
        }
        delete item;
    }
}

void MainWindow::go_page_pay() {
    //==============[0621]=============
    // orderList.clear();                    // 주문 정보 초기화
    // selected_store = store_info();       // 현재 선택된 가게 정보도 초기화
    clearScrollAreaWidgets();
    Btn_menu_finish_select_clicked();    // 하단 버튼 UI 초기화
    add_recommend_menu_widget();         // 사이드 메뉴 초기화
    ui->pushButton_cnt->setText("0");    // 장바구니 수량 라벨 초기화
    ui->pushButton_cnt_2->setText("0");

    ui->stackedWidget->setCurrentIndex(GO_PAYING);

    // 프로그레스 바 설정
    ui->progressBar_2->setRange(0, 0);
    ui->progressBar_2->setTextVisible(false);
    ui->progressBar_2->setStyleSheet(R"(
        QProgressBar {
            border: 2px solid #FF2D6F;
            border-radius: 10px;
            background-color: #FFFFFF;
        }
        QProgressBar::chunk {
            background-color: #FF2D6F;
            margin: 1px;
        }
    )");

    // 3초 뒤 결제완료 페이지로 이동
    QTimer::singleShot(3000, this, [=]() {
        ui->stackedWidget->setCurrentIndex(GO_PAY_COMPLETE);
        ui->label_my_phonenum->setText(user_info.phonenum);

        int menu_total = 0;
        int delivery_fee = 0;
        int final_total = 0;

        if (!orders_all.isEmpty()) {
            const auto& order = orders_all.last();

            static const QStringList re = {"수저, 포크 X", "수저, 포크 O"};
            ui->label_disposable->setText(re.value(order.STATUS_DISPOSIBLE, ""));
            ui->label_user_msg->setText(order.ORDER_MSG);
            ui->label_rider_msg->setText(order.RIDER_MSG);
            ui->label_order_id->setText(order.ORDER_ID);
            ui->label_my_adress->setText(DefaultAdr);

            // 메뉴 총액은 구조체의 TOTAL_PRICE
            menu_total = order.TOTAL_PRICE;
            // 배달비는 selected_store 구조체에서 가져오며, 문자열일 수 있으므로 변환
            delivery_fee = selected_store.delivery_fee.toInt();
            final_total = menu_total + delivery_fee;

            QString formattedTotal = QLocale(QLocale::Korean).toString(final_total);
            ui->label_total_price->setText(QString("%1 원").arg(formattedTotal));

            // 주문 시간 표시 (AM/PM 변환)
            QString fullTime = order.ORDER_TIME;
            if (fullTime.length() >= 16) {
                QString hourStr = fullTime.mid(11, 2);
                QString minute  = fullTime.mid(14, 2);

                int hour = hourStr.toInt();
                QString ampm = (hour < 12) ? "AM" : "PM";
                int displayHour = (hour % 12 == 0) ? 12 : hour % 12;

                QString timeStr = QString("%1 %2:%3")
                                      .arg(ampm)
                                      .arg(displayHour, 2, 10, QChar('0'))
                                      .arg(minute);
                ui->label_order_time->setText(timeStr);
                home_order_open();
            } else {
                ui->label_order_time->setText("시간 정보 없음");
            }

        } else {
            ui->label_disposable->setText("정보 없음");
            ui->label_user_msg->setText("요청 없음");
            ui->label_rider_msg->setText("메시지 없음");
            ui->label_order_id->setText("ID 없음");
            ui->label_order_time->setText("시간 없음");
            ui->label_menu_price->setText("없음");
            ui->label_opt_price->setText("0원");
            ui->label_total_price->setText("총 결제금액 0 원");
        }

        // ----------- 메뉴 요약 텍스트 -----------
        QString summary = "없음";
        if (!order_details_all.isEmpty() && !order_details_all.last().isEmpty()) {
            const auto& details = order_details_all.last();
            summary = details[0].MENU_NAME;
            if (details.size() > 1)
                summary += QString(" 외 %1건").arg(details.size() - 1);
        }
        ui->label_menu_price->setText(summary);

        // ----------- 옵션 가격 총합 계산 -----------
        int opt_total = 0;
        if (!order_details_all.isEmpty() && !order_details_all.last().isEmpty()) {
            for (const auto& d : order_details_all.last()) {
                opt_total += d.OPT_PRICE_ALL * d.MENU_CNT;
            }
        }
        QString formattedOpt = QLocale(QLocale::Korean).toString(opt_total);
        ui->label_opt_price->setText(QString("%1 원").arg(formattedOpt));

        // 메시지 입력창 초기화
        clear_order_messages();

    });

    setting_order_history();  // 결제 완료 후 히스토리 갱신
}

void MainWindow::go_sale()                    { QMessageBox::information(this, "알림", "할인은 없습니다."); }
void MainWindow::order_cancel()               { QMessageBox::information(this, "알림", "취소 문의는 해당 가게로 문의해주세요!"); }
void MainWindow::order_reorder() {
    ui->stackedWidget->setCurrentIndex(GO_CART);
    ui->label_pnum->setText(user_info.phonenum);
    ui->label_where->setText(user_adr.adr_name);
    QMessageBox::information(this, "알림", "장바구니를 비우고 메뉴를 담았습니다.");}
void MainWindow::go_store_and_menu(){
    ui->stackedWidget->setCurrentIndex(GO_STORE_AND_MENU);
    send_rstinfo_request();
    set_AddressBasic_btn();
}
void MainWindow::go_selected_store(){
    ui->stackedWidget->setCurrentIndex(GO_STORE_SELECT);
    update_ui();
    set_rst_page();
    set_Btn_menu_finish_select();
}

// 리뷰 별점 초기화 및 클릭 처리
void MainWindow::review_star() {
    total_buttons  = { ui->btn_total1, ui->btn_total2, ui->btn_total3, ui->btn_total4, ui->btn_total5 };
    taste_buttons  = { ui->btn_taste1, ui->btn_taste2, ui->btn_taste3, ui->btn_taste4, ui->btn_taste5 };
    amount_buttons = { ui->btn_amount1, ui->btn_amount2, ui->btn_amount3, ui->btn_amount4, ui->btn_amount5 };

    // 전체 평점
    for (int i = 0; i < total_buttons.size(); ++i) {
        connect(total_buttons[i], &QPushButton::clicked, this, [=]() {
            score_total = i + 1;
            updateStars(total_buttons, score_total);
            qDebug() << "전체평점:" << score_total;
        });
    }

    // 맛 점수
    for (int i = 0; i < taste_buttons.size(); ++i) {
        connect(taste_buttons[i], &QPushButton::clicked, this, [=]() {
            score_taste = i + 1;
            updateStars(taste_buttons, score_taste);
            qDebug() << "맛 점수:" << score_taste;
        });
    }

    // 양 점수
    for (int i = 0; i < amount_buttons.size(); ++i) {
        connect(amount_buttons[i], &QPushButton::clicked, this, [=]() {
            score_amount = i + 1;
            updateStars(amount_buttons, score_amount);
            qDebug() << "양 점수:" << score_amount;
        });
    }
}

// ★☆ 토글용 헬퍼
void MainWindow::updateStars(const QList<QPushButton*>& buttons, int score) {
    for (int i = 0; i < buttons.size(); ++i) {
        buttons[i]->setText(i < score ? QStringLiteral("★") : QStringLiteral("☆"));
    }
}

// 결제 완료 후 히스토리 갱신 (간단히 재조회)
void MainWindow::setting_order_history() {
    send_order_request();
}

void MainWindow::home_order_open(){
    static const QStringList st = {
        "주문 확인 중", "주문 접수 완료 조리 중", "배달 중", "배달 완료", "주문 거절"
    };

    if (!orders_all.isEmpty()) {
        const auto& order = orders_all.last();
        QString statusText;

        if (order.STATUS_ORDER >= 0 && order.STATUS_ORDER < st.size()) {
            statusText = st[order.STATUS_ORDER];
            ui->pushButton_order_s_anstore->setText(statusText + " ");
        } else {
            statusText = "알 수 없는 상태";
        }
        // 프레임 보이기
        ui->frame_od->show();
    }
}


//11번째부터 20번째 버튼 숨기는 함수
void MainWindow::hideButtons(int from, int to) {
    for (int i = from; i <= to; ++i) {
        QString name = QString("pushButton__%1").arg(i);
        QPushButton* btn = findChild<QPushButton*>(name);
        if (btn) {
            btn->hide();
        }
    }
}
//모든 버튼 보여주는 함수
void MainWindow::showAllButtons() {
    for (int i = 1; i <= 20; ++i) {
        QString name = QString("pushButton__%1").arg(i);
        QPushButton* btn = findChild<QPushButton*>(name);
        if (btn) {
            btn->show();
            ui->pushButton__1->setFixedSize(65, 65);
            ui->pushButton__10->setStyleSheet("QPushButton {""border-image: url(:/image/MAIN_IMG/IMG_1247.png) 0 0 0 0 stretch stretch;""}");
        }
    }
}
//버튼 10번을 눌렀을 showAllButtons 함수 실행
void MainWindow::setupButtonPaging() {
    QPushButton* btn10 = findChild<QPushButton*>("pushButton__10");
    if (btn10) {
        connect(btn10, &QPushButton::clicked, this, [=]() {showAllButtons();});
    }
}
// 바텀 시트 위젯을 생성하고 초기화합니다.
void MainWindow::initCancelSheet()
{
    cancelSheet = new QWidget(this);
    cancelSheet->setObjectName("cancelSheet");
    cancelSheet->setStyleSheet(R"(
        QWidget#cancelSheet {
            background: white;
            border: 2px solid #FF2D6F;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
        }
    )");
    // 화면 크기에 맞춰 위치/크기 초기화
    cancelSheet->setGeometry(0, height(), width(), sheetHeight);

    QVBoxLayout *layout = new QVBoxLayout(cancelSheet);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    // 닫기 버튼
    QPushButton *closeBtn = new QPushButton("✕", cancelSheet);
    closeBtn->setFixedSize(24, 24);
    closeBtn->setStyleSheet("border:none; font-size:16px;");
    connect(closeBtn, &QPushButton::clicked,
            this, &MainWindow::toggleCancelBottomSheet);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    // 제목
    QLabel *title = new QLabel("<b>취소 문의</b>", cancelSheet);
    layout->addWidget(title);

    // 메시지
    QLabel *message = new QLabel(
        "주문 취소는 가게에 문의해주셔야 합니다.\n"
        "*조리 중이거나 배달이 시작된 주문은 취소가 어렵습니다.",
        cancelSheet);
    message->setWordWrap(true);
    layout->addWidget(message);

    // 작업 버튼
    QPushButton *callBtn = new QPushButton("가게 전화", cancelSheet);
    callBtn->setFixedHeight(40);
    callBtn->setStyleSheet(R"(
        QPushButton {
            border:1px solid #ccc;
            border-radius:4px;
            background:white;
        }
    )");
    connect(callBtn, &QPushButton::clicked,
            this, &MainWindow::toggleCancelBottomSheet);
    layout->addSpacing(12);
    layout->addWidget(callBtn);

    // 애니메이션 객체 초기화
    sheetAnim = new QPropertyAnimation(cancelSheet, "geometry", this);
    sheetAnim->setDuration(300);
    sheetAnim->setEasingCurve(QEasingCurve::OutCubic);

    // 연결: UI 버튼 → 시트 토글
    connect(ui->pushButton_cancel, &QPushButton::clicked,
            this, &MainWindow::showCancelBottomSheet);
}

// 바텀 시트를 생성(또는 초기화)하고 토글합니다.
void MainWindow::showCancelBottomSheet()
{
    if (!cancelSheet){
        initCancelSheet();
    }
    toggleCancelBottomSheet();
}

// 바텀 시트를 화면 위로 올리거나 숨깁니다.
void MainWindow::toggleCancelBottomSheet()
{
    if (!cancelSheet) return;

    // 기존 애니메이션 중지
    if (sheetAnim->state() == QAbstractAnimation::Running)
        sheetAnim->stop();

    QRect startRect = cancelSheet->geometry();
    QRect endRect;
    if (startRect.y() >= height()) {
        // 화면 아래 숨김 상태 -> 올려서 표시
        endRect = QRect(0, height() - sheetHeight, width(), sheetHeight);
    } else {
        // 표시 상태 -> 아래로 내려 숨김
        endRect = QRect(0, height(), width(), sheetHeight);
    }

    sheetAnim->setStartValue(startRect);
    sheetAnim->setEndValue(endRect);

    // 완료 후 숨김 처리
    connect(sheetAnim, &QPropertyAnimation::finished,
            this, [=]() {
                if (cancelSheet->geometry().y() >= height())
                    cancelSheet->hide();
            }, Qt::SingleShotConnection);

    cancelSheet->show();
    sheetAnim->start();
}
void MainWindow::resizeEvent(QResizeEvent *event)
{
    // 기본 동작 호출
    QMainWindow::resizeEvent(event);
    // 윈도우 크기 변경 시, 바텀 시트 크기/위치도 재조정
    if (cancelSheet) {cancelSheet->setGeometry(0, height(), width(), sheetHeight);
    }
}
// 로그인 후 광고창
void MainWindow::showAdPopup()
{
    // 1) 프레임 없는 모달 다이얼로그
    QDialog *dlg = new QDialog(this, Qt::Dialog | Qt::FramelessWindowHint);
    dlg->setModal(true);
    dlg->setAttribute(Qt::WA_TranslucentBackground);

    // 2) 전체 레이아웃 (여백 없이)
    QVBoxLayout *mainLay = new QVBoxLayout(dlg);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);

    // 3) 광고 이미지
    QLabel *img = new QLabel(dlg);
    QPixmap pix(":/image/main_img/gwanggo.png");
    img->setPixmap(pix.scaled(360, 480, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    img->setAlignment(Qt::AlignCenter);
    mainLay->addWidget(img);

    // ── 하단 “오늘은 그만 보기 | 닫기” 버튼 영역 ──
    QWidget *footer = new QWidget(dlg);
    footer->setAttribute(Qt::WA_TranslucentBackground);
    QHBoxLayout *fLay = new QHBoxLayout(footer);
    fLay->setContentsMargins(0, 8, 0, 8);
    fLay->setSpacing(8);

    QPushButton *btnOnce  = new QPushButton("오늘은 그만 보기", footer);
    QPushButton *btnClose = new QPushButton("닫기", footer);
    for (auto b : {btnOnce, btnClose}) {
        b->setFlat(true);
        b->setStyleSheet(
            "color:black;"
            "background:transparent;"
            "border:none;"
            "font-size:14px;"
            );
    }
    connect(btnOnce, &QPushButton::clicked, this, [this]() {please_adress();});
    connect(btnClose, &QPushButton::clicked, this, [this]() {please_adress();});
    QLabel *sep = new QLabel("|", footer);
    sep->setStyleSheet("color:white; font-size:14px;");

    fLay->addStretch();
    fLay->addWidget(btnOnce);
    fLay->addWidget(sep);
    fLay->addWidget(btnClose);
    fLay->addStretch();
    mainLay->addWidget(footer);
    // ────────────────────────────────────────────────

    // 5) 시그널 연결
    connect(btnOnce,  &QPushButton::clicked, dlg, &QDialog::accept);
    connect(btnClose, &QPushButton::clicked, dlg, &QDialog::accept);

    // 6) 팝업 실행
    dlg->exec();
}

// 주소 플리즈 바텀 시트 위젯을 생성하고 초기화합니다.
void MainWindow::please_adress() {
    int sheetHeight = 200;  // 원하는 높이로 조절 (멤버변수 사용 가능)

    // 이미 있는 cancelSheet가 있다면 중복 방지로 제거
    if (cancelSheet) {
        cancelSheet->deleteLater();
        cancelSheet = nullptr;
    }

    // 바텀시트 위젯 생성
    cancelSheet = new QWidget(this);
    cancelSheet->setObjectName("cancelSheet");
    cancelSheet->setStyleSheet(R"(
        QWidget#cancelSheet {
            background: white;
            border: 2px solid #FF2D6F;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
        }
    )");

    // 현재 창 아래에 붙도록 위치 조정
    int yPos = height() - sheetHeight;
    cancelSheet->setGeometry(0, yPos, width(), sheetHeight);

    // 레이아웃 설정
    QVBoxLayout *layout = new QVBoxLayout(cancelSheet);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    // 닫기 버튼
    QPushButton *closeBtn = new QPushButton("✕", cancelSheet);
    closeBtn->setFixedSize(24, 24);
    closeBtn->setStyleSheet("border:none; font-size:16px;");
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    // 제목
    QLabel *title = new QLabel("<b>주소가 입력 되지 않았습니다.</b>", cancelSheet);
    layout->addWidget(title);

    // 메시지
    QLabel *message = new QLabel("주소를 먼저 입력해주세요.", cancelSheet);
    message->setWordWrap(true);
    layout->addWidget(message);

    // 확인 버튼
    QPushButton *callBtn = new QPushButton("확인", cancelSheet);
    callBtn->setFixedHeight(40);
    callBtn->setStyleSheet(R"(
        QPushButton {
            border: 1px solid #ccc;
            border-radius: 4px;
            background: white;
        }
    )");
    layout->addWidget(callBtn);

    // 닫기 버튼에 닫기 연결 (숨기기 또는 삭제)
    connect(closeBtn, &QPushButton::clicked, cancelSheet, &QWidget::hide);
    connect(callBtn, &QPushButton::clicked, cancelSheet, &QWidget::hide);

    // 반드시 보여줘야 함
    cancelSheet->show();
}

void MainWindow::clear_order_messages() {
    ui->user_rst_msg->clear();
    ui->user_rider_msg->clear();
    user_rst_msg.clear();
    user_rider_msg.clear();
}

MainWindow::~MainWindow() {
    delete ui;
}
