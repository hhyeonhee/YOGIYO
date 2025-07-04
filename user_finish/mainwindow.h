#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtSql/qsqldatabase.h>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QList>
#include <functional>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QSet>
#include <QStringList>
#include <QDateTime>            //=======[0620]======
#include <QRandomGenerator>     //=======[0620]======
#include <QScrollArea>

#include <QGraphicsOpacityEffect>

#include <QPropertyAnimation>

#include "dbmanager.h"
#include "addressbtn.h"
#include "struct_and_enum.h"
#include "order_list_widget.h"
#include "rst_main_icon.h"
#include "menu_head.h"
#include "menu_normal.h"
#include "sel_menu_info.h"
#include "selected_menu_widget.h"
#include "dbmanager.h"        // =============[0620]========


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void fadeTransition(int fromIndex, int toIndex);
    void slideTransition(QStackedWidget* sw, int toIndex);

    ~MainWindow();

    // ---------- 페이지 전환 --------------
    void gohome();
    void goorder();
    void goheart();
    void gomy();
    void gocart();
    void go_page_order2();
    void goaddress();
    void go_order_list_particular();
    void write_review();
    void my_review();
    void go_page_pay();
    void go_sale();
    void order_cancel();
    void order_reorder();
    void go_selected_store();
    void go_to_menu_detail();


    // ---------- 페이지 전환 --------------
    void go_store_and_menu();
    void goToPage(int index);

    // ---------- 기능 함수 ----------
    void SetCategory();
    void set_addrBtn_widget();        // 주소 버튼 갱신
    void set_city_cbb();       // 시군구 콤보박스 갱신
    void set_AddressBasic_btn();
    void set_rst_widget();        //가게 목록 위젯 생성
    void set_rst_page(); //가게정보 및 메뉴 출력창 세팅
    void find_category_list();
    void set_sel_menu_widget();
    void add_menu_head_widget();
    void add_menu_normal_widget();
    void set_Btn_menu_finish_select();
    int calculate_total_sum();

    void add_recommend_menu_widget();

    QString DefaultAdr;
    //=================[0621]================
    QString DefaultAdr_city;
    QString DefaultAdr_detail;
    //=======================================
    QString selected_city_address;
    QSet<QString> category_set;
    QStringList category_list;

    void test_clicked();
    void home_order_open();
    void go_order_detail(const QList<int>& order_indexes);

    void please_adress();
    void update_button_order1_enabled();  // 0621 진영 추가

    QString gen_order_id();       //======[0620]===== 랜덤 주문번호 생성 서현
    void send_order_to_db();    //========[0620]======= 주문정보db전송 서현
    QString user_rst_msg;       //========[0620]======= 서현
    QString user_rider_msg;     //========[0620]======= 서
    void clear_order_messages();  //========[0621]=======
    void clearScrollAreaWidgets();

private slots:
    // ---------- 서버 통신 ----------

    void server_connect();
    void on_connected();
    void on_ready_read();
    void on_error(QAbstractSocket::SocketError err);

    void login_handling();
    void send_login_request();

    void address_handling();
    void send_address_request();  // 주소 + 시군구 요청
    void send_address_add();   //주소 추가 요청

    void send_order_request();
    void send_rstinfo_request(); //가게 정보 요청

    void send_menu_request();   //메뉴 정보 요청
    void onMenuHeadClicked(QString menu_name);

    // ---------- UI 동작 ----------
    void Address_clicked(const QString& address);
    void Address_delete_clicked(const QString& address_detail, QWidget* sender);

    //0617은비
    void hideButtons(int from, int to);
    void setupButtonPaging();
    void showAllButtons();

    void initCancelSheet();
    void showCancelBottomSheet(); // 바텀 시트 생성 및 토글 진입점
    void toggleCancelBottomSheet(); // 이미 생성된 시트를 보이거나 숨김 처리

    void Btn_menu_finish_select_clicked();  //0620
    void handleOrderUpdated(const ORDER_DETAIL& updatedOrder); //0620


private:
    Ui::MainWindow *ui;
    QTcpSocket *socket_;
    QByteArray recv_buffer;

    //--정보 저장
    DBManager * db;
    user user_info;
    address user_adr;
    store_info rst_info;
    store_info selected_store;  // 사용자가 선택한 가게 정보 저장
    menu menus;
    sel_menu_info* selectedMenuInfoWidget = nullptr;  // *******0619[진영]
    store_info storeData;       //******0619[진영]


    QList<ORDER> orders_all;
    QList<QList<ORDER_DETAIL>> order_details_all;

    QMap<category, QString> CategoryMap;  // 카테고리 한글 매핑
    QMap<QString, QStringList> cityToDistricts; // 시/군구 정보

    QList<address> lst_adr;               // 내 주소 리스트
    QList<addressBtn*> lst_adr_btn;       // 동적 버튼 리스트
    QList<store_info> lst_rst_info;      // 가게 리스트
    QList<menu> lst_menu;               //메뉴 리스트
    std::function<void(const QJsonObject&)> responseHandler_;

    // UI helper
    void show_selected_menu();
    void order_all_delete();
    void show_order_list();
    void setting_order_history();

    // 별점
    int score_total{0};
    int score_taste{0};
    int score_amount{0};
    QList<QPushButton*> total_buttons;
    QList<QPushButton*> taste_buttons;
    QList<QPushButton*> amount_buttons;
    void review_star();
    void updateStars(const QList<QPushButton*>& buttons, int score);

    // 페이드인,아웃
    QStackedWidget *stack;
    QWidget        *page1;
    QWidget        *page2;
    QGraphicsOpacityEffect *eff1;
    QGraphicsOpacityEffect *eff2;

    //카테고리나누는함수
    void showStoreIcons(int minBrandUid, int maxBrandUid);


    QWidget* cancelSheet   = nullptr;
    QPropertyAnimation* sheetAnim = nullptr;
    const int sheetHeight = 200;
    void showAdPopup(); // 광고 팝업 띄우는 함수

    void menu_selected(const ORDER_DETAIL& selected_menu);  //메뉴 선택 함수
    void menu_deleted(const QString& menu_name, const QString& opt_name_all);    //메뉴 삭제 함수
    QList<ORDER_DETAIL> orderList;  //장바구니, 메뉴담는 리스트진영

    void update_ui();

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H
