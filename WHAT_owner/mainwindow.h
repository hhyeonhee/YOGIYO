#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>         // 메인 윈도우 기반
#include <QTcpSocket>          // TCP 소켓 사용
#include <QJsonDocument>       // JSON 처리
#include <QJsonObject>
#include <QDate>               // QDate
#include <QTime>               // QTime
#include <functional>          // std::function

//sql전용
#include <QtSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QSqlError>

#define SERVER_PORT 12345
// 구조체: 사장 정보
struct owner {
    QString type;                   // "owner"
    QString BRAND_UID;              // "1"
    QString PLACE_UID;              // "23"
    QString ADDRESS;                // "서울특별시 강남구"
    QString ADDRESS_DETAIL;         // "테헤란로 123"
    QDate   OPEN_DATE;              // "yyyy-MM-dd"
    QString PHONENUMBER;            // "01012345678"
    QString DELIVERY_TIM;           // "30~45분"
    QTime   OPEN_TIME;              // "hh:mm"
    QTime   CLOSE_TIME;             // "hh:mm"
    QString ORIGIN_INFO;            // "국내산"
    int     MIN_ORDER_PRICE;        // 15000
    int     DELIVERY_FEE;           // 3000
    int     FREE_DELIVERY_STANDARD; // 30000
    QString IMAGE_PASS;             // 빈 문자열 또는 파일 경로
    QString IMAGE_MAIN;             // 빈 문자열 또는 파일 경로
};


// 구조체: 주문 기본 정보
struct ORDER {
    QString ORDER_ID;
    int UID;
    int BRAND_UID;
    int PLACE_UID;
    int TOTAL_PRICE;
    QString ORDER_TIME;         // "yyyy-MM-dd hh:mm:ss"
    QString ADDRESS;            // 배달 주소
    QString ADDRESS_DETAIL;     // 상세 주소
    QString ORDER_MSG;          // 가게 요청사항
    QString RIDER_MSG;          // 라이더 요청사항
    int STATUS_TOGO;
    bool STATUS_DISPOSIBLE;
    int STATUS_ORDER;
};

// 구조체: 주문 상세 정보
struct ORDER_DETAIL {
    QString ORDER_ID;
    QString MENU_NAME;
    int MENU_PRICE;
    int MENU_CNT;
    QString OPT_NAME_ALL;
    int OPT_PRICE_ALL;
};


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    // 사장 주문 조회 버튼 클릭 시
    void on_check_orderlist_clicked();
    //sql접근
    void db_open();

    ~MainWindow();

private slots:
    void on_login_button_clicked();                       // 로그인 시도
    void on_connected();                                  // 연결 성공
    void on_error(QAbstractSocket::SocketError err);      // 에러 발생
    void sendLoginRequest();                              // 로그인 요청 보내기
    void handleOrderResponse(const QJsonObject &resp);    // 주문 응답 처리
    void handleLoginResponse(const QJsonObject &resp);    // 로그인 응답 처리
    void onSocketReadyRead();

    // 페이지 전환용 슬롯
    void go_to_page_login();        // 0
    void go_to_page_find_id();      // 1
    void go_to_page_find_pw();      // 2
    void go_to_page_sign_in();      // 3
    void go_to_page_main_menu();    // 4
    void go_to_page_order();        // 5


private:
    Ui::MainWindow *ui;                   // UI 객체
    QTcpSocket    *socket_;              // TCP 소켓 객체
    QString recv_buffer;       // ← 누적용 버퍼

    void populateOwnerTable();

    owner            owner_info;         // 로그인한 사장 정보
    ORDER            owner_order;        // 하나의 주문 정보
    ORDER_DETAIL     owner_order_d;      // 하나의 주문 상세 정보
};

#endif // MAINWINDOW_H
