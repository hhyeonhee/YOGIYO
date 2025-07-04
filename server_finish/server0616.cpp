#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <arpa/inet.h>
#include "cJSON.h"
#include "cJSON.c"
#include <cstring>
#include <fstream>  // 파일 입출력 헤더 추가
#include <mysql/mysql.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024
using namespace std;

// 라이더 id/pw 확인 함수 추가 json 폴더 오픈
bool rider_check_credentials(const char* phonenumber, const char* pw) {
    ifstream file("rider_info.json");
    if (!file.is_open()) {
        cerr << "rider_info.json 파일을 열 수 없습니다.\n";
        return false;
    }

    string json_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    cJSON* root = cJSON_Parse(json_str.c_str());
    if (!root) {
        cerr << "rider_info.json 파싱 실패\n";
        return false;
    }

    int array_size = cJSON_GetArraySize(root);
    for (int i = 0; i < array_size; ++i) {
        cJSON* item = cJSON_GetArrayItem(root, i);
        cJSON* json_id = cJSON_GetObjectItem(item, "phonenumber");
        cJSON* json_pw = cJSON_GetObjectItem(item, "pw");

        cout << i << (json_id ? json_id->valuestring : "ID 없음") << ", " << (json_pw ? json_pw->valuestring : "PW 없음") << endl;

        if (cJSON_IsString(json_id) && cJSON_IsString(json_pw)) {
            if (strcmp(json_id->valuestring, phonenumber) == 0 && strcmp(json_pw->valuestring, pw) == 0) {
                cJSON_Delete(root);
                return true;  // 아이디/비번 일치
            }
        }
    }
    cJSON_Delete(root);
    return false; // 일치하는 아이디/비번 없음
}

// 라이더 요청 처리 함수
void rider_handle_client(int* client_sock) {
    int sock = *client_sock;
    delete client_sock;  // 동적 할당된 소켓 포인터 해제

    while (true) {
        char buffer[BUFFER_SIZE] = {0};

        int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            cout << "클라이언트 연결 종료 또는 오류, recv 반환값: " << bytes_received << endl;
            break;
        }

        buffer[bytes_received] = '\0';
        cout << "recv 반환값: " << bytes_received << ", 받은 데이터: " << buffer << endl;

        cJSON* root = cJSON_Parse(buffer);
        if (!root) {
            cerr << "JSON 파싱 실패\n";
            continue;
        }

        const cJSON* action = cJSON_GetObjectItem(root, "action");
        const cJSON* phonenumber = cJSON_GetObjectItem(root, "phonenumber");
        const cJSON* pw = cJSON_GetObjectItem(root, "pw");

        // 로그인 처리
        if (cJSON_IsString(action) && strcmp(action->valuestring, "1000_0") == 0) {
            cout << "로그인 요청\n";

            bool login_success = false;
            if (phonenumber && pw) {
                login_success = rider_check_credentials(phonenumber->valuestring, pw->valuestring);
            }

            cJSON* resp = cJSON_CreateObject();
            if (login_success) {
                cJSON_AddStringToObject(resp, "action", "1000_1");
                cJSON_AddStringToObject(resp, "message", "로그인 성공");

                // rider_info.json 파일에서 사용자 정보 검색
                ifstream file("rider_info.json");
                string json_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
                file.close();

                cJSON* all_users = cJSON_Parse(json_str.c_str());
                int array_size = cJSON_GetArraySize(all_users);

                for (int i = 0; i < array_size; ++i) {
                    cJSON* user = cJSON_GetArrayItem(all_users, i);
                    cJSON* json_id = cJSON_GetObjectItem(user, "phonenumber");

                    if (cJSON_IsString(json_id) && strcmp(json_id->valuestring, phonenumber->valuestring) == 0) {
                        cJSON_AddStringToObject(resp, "phonenumber", json_id->valuestring);
                        cJSON_AddStringToObject(resp, "city", cJSON_GetObjectItem(user, "city")->valuestring);
                        cJSON_AddStringToObject(resp, "vehicle", cJSON_GetObjectItem(user, "vehicle")->valuestring);
                        cJSON_AddStringToObject(resp, "name", cJSON_GetObjectItem(user, "name")->valuestring);
                        cJSON_AddStringToObject(resp, "birth", cJSON_GetObjectItem(user, "birth")->valuestring);
                        cJSON_AddStringToObject(resp, "account_number", cJSON_GetObjectItem(user, "account_number")->valuestring);
                        break;
                    }
                }

                cJSON_Delete(all_users);
            } else {
                cJSON_AddStringToObject(resp, "action", "1000_2");
                cJSON_AddStringToObject(resp, "message", "로그인 실패");
            }

            // JSON 문자열 생성 + <<END>> 추가
            string response_str = cJSON_PrintUnformatted(resp);
            response_str += "<<END>>";  // 메시지 끝 신호

            // 클라이언트에 전송
            send(sock, response_str.c_str(), response_str.length(), 0);
            cJSON_Delete(resp);
        }


        // 배달 목록 요청 처리
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "1001_0") == 0) {
            cout << "배달 목록 요청\n";

            // owner_info.json 파일 열기
            FILE* fp = fopen("owner_info.json", "r");
            if (!fp) {
                perror("owner_info.json 열기 실패");
                cJSON_Delete(root);
                continue;
            }

            // 파일 크기 구하기
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            // 파일 내용 읽기
            char* json_data = (char*)malloc(size + 1);
            fread(json_data, 1, size, fp);
            json_data[size] = '\0';
            fclose(fp);

            // JSON 파싱
            cJSON* owner_root = cJSON_Parse(json_data);
            free(json_data);

            if (!owner_root || !cJSON_IsArray(owner_root)) {
                perror("owner_info.json 파싱 실패 or 배열 아님");
                cJSON_Delete(root);
                continue;
            }

            // DB 연결 초기화
            MYSQL* conn = mysql_init(NULL);
            if (!conn) {
                perror("mysql_init 실패");
                cJSON_Delete(root);
                cJSON_Delete(owner_root);
                continue;
            }

            // DB 연결
            if (!mysql_real_connect(conn, "10.10.20.99", "JIN", "1234", "WHAT", 3306, NULL, 0)) {
                perror("DB 연결 실패");
                mysql_close(conn);
                cJSON_Delete(root);
                cJSON_Delete(owner_root);
                continue;
            }

            // 쿼리 : BRAND_UID, PLACE_UID, ADDRESS, ADDRESS_DETAIL, RIDER_MSG 가져오기 (컬럼명 맞춤)
            const char* query = "SELECT BRAND_UID, PLACE_UID, ADDRESS, ADDRESS_DETAIL, RIDER_MSG FROM ORDER_";

            if (mysql_query(conn, query)) {
                perror("쿼리 실행 실패");

                cJSON* err_resp = cJSON_CreateObject();
                cJSON_AddStringToObject(err_resp, "action", "1001_2");
                cJSON_AddStringToObject(err_resp, "message", "쿼리 실행 실패");

                string err_str = cJSON_PrintUnformatted(err_resp);
                err_str += "\n<<END>>";
                send(sock, err_str.c_str(), err_str.length(), 0);

                cJSON_Delete(err_resp);
                mysql_close(conn);
                cJSON_Delete(root);
                cJSON_Delete(owner_root);
                continue;
            }

            MYSQL_RES* result = mysql_store_result(conn);
            if (!result) {
                perror("결과 저장 실패");
                mysql_close(conn);
                cJSON_Delete(root);
                cJSON_Delete(owner_root);
                continue;
            }

            // 응답 JSON 객체 생성
            cJSON* resp = cJSON_CreateObject();
            cJSON_AddStringToObject(resp, "action", "1001_1");
            cJSON* order_list = cJSON_CreateArray();

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                string brand_uid = row[0] ? row[0] : "";
                string place_uid = row[1] ? row[1] : "";

                string user_full_address = string(row[2] ? row[2] : "") + " " + (row[3] ? row[3] : "");
                string rider_msg = row[4] ? row[4] : "";

                std::string owner_full_address = "";

                int owner_size = cJSON_GetArraySize(owner_root);
                for (int j = 0; j < owner_size; ++j) {
                    cJSON* owner = cJSON_GetArrayItem(owner_root, j);
                    cJSON* owner_brand_uid = cJSON_GetObjectItem(owner, "BRAND_UID");
                    cJSON* owner_place_uid = cJSON_GetObjectItem(owner, "PLACE_UID");
                    cJSON* addr = cJSON_GetObjectItem(owner, "ADDRESS");
                    cJSON* detail = cJSON_GetObjectItem(owner, "ADDRESS_DETAIL");

                    if (cJSON_IsString(owner_brand_uid) && cJSON_IsString(owner_place_uid) &&
                        cJSON_IsString(addr) && cJSON_IsString(detail)) {
                        if (brand_uid == owner_brand_uid->valuestring &&
                            place_uid == owner_place_uid->valuestring) {
                            owner_full_address = string(addr->valuestring) + " " + detail->valuestring;
                            break;
                        }
                    }
                }

                if (owner_full_address != "") {
                    cJSON* order = cJSON_CreateObject();
                    cJSON_AddStringToObject(order, "owner_full_address", owner_full_address.c_str());
                    cJSON_AddStringToObject(order, "user_full_address", user_full_address.c_str());
                    cJSON_AddStringToObject(order, "RIDER_MSG", rider_msg.c_str());
                    cJSON_AddItemToArray(order_list, order);
                }
            }

            cJSON_AddItemToObject(resp, "orders", order_list);

            string response_str = cJSON_PrintUnformatted(resp);
            response_str += "\n<<END>>";

            send(sock, response_str.c_str(), response_str.length(), 0);

            cJSON_Delete(resp);
            cJSON_Delete(owner_root);
            mysql_free_result(result);
            mysql_close(conn);
        }


        cJSON_Delete(root);
    }
}





// 오너 id/pw 확인 함수 추가 json 폴더 오픈
bool owner_check_credentials(const char* phonenumber, const char* type) {
    ifstream file("owner_info.json");
    if (!file.is_open()) {
        cerr << "owner_info.json 파일을 열 수 없습니다.\n";
        return false;
    }

    string json_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    cJSON* root = cJSON_Parse(json_str.c_str());
    if (!root) {
        cerr << "owner_info.json 파싱 실패\n";
        return false;
    }

    int array_size = cJSON_GetArraySize(root);
    for (int i = 0; i < array_size; ++i) {
        cJSON* item = cJSON_GetArrayItem(root, i);
        cJSON* json_id = cJSON_GetObjectItem(item, "PHONENUMBER");
        cJSON* json_pw = cJSON_GetObjectItem(item, "type");

        cout << i << (json_id ? json_id->valuestring : "ID 없음") << ", "
        << (json_pw ? json_pw->valuestring : "PW 없음") << endl;

        if (cJSON_IsString(json_id) && cJSON_IsString(json_pw)) {
            if (strcmp(json_id->valuestring, phonenumber) == 0 && strcmp(json_pw->valuestring, type) == 0) {
                cJSON_Delete(root);
                return true;  // 아이디/비번 일치
            }
        }
    }
    cJSON_Delete(root);
    return false; // 일치하는 아이디/비번 없음
}

// 오너 요청 처리 //수정중
void owner_handle_client(int* client_sock) {
    int sock = *client_sock;
    delete client_sock;
    char buffer[BUFFER_SIZE] = {0}; // 사용자가 보낸 문자열 저장

    while (true) {
        int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            cout << "클라이언트 연결 종료 또는 에러 발생\n";
            break;
        }
        buffer[bytes_received] = '\0';
        cout << "recv 반환값: " << bytes_received 
             << ", 받은 데이터: " << buffer << endl;

        // JSON 파싱
        cJSON* root = cJSON_Parse(buffer);
        if (!root) {
            cerr << "JSON 파싱 실패\n";
            continue;
        }

        // action 필드 추출
        const cJSON* action = cJSON_GetObjectItem(root, "action");

        // 오너 로그인 처리
        if (cJSON_IsString(action) && strcmp(action->valuestring, "100_0") == 0) {
            const cJSON* phonenumber = cJSON_GetObjectItem(root, "PHONENUMBER");
            const cJSON* type        = cJSON_GetObjectItem(root, "type");
            cout << "로그인 요청\n";
            cout << "PHONENUMBER: " << (phonenumber ? phonenumber->valuestring : "NULL") << "\n";
            cout << "type: " << (type ? type->valuestring : "NULL") << "\n";

            bool login_success = false;
            if (phonenumber && type) {
                login_success = owner_check_credentials(
                    phonenumber->valuestring,
                    type->valuestring
                );
            }

            cJSON* resp = cJSON_CreateObject();
            if (login_success) {
                cJSON_AddStringToObject(resp, "action", "100_1");
                cJSON_AddStringToObject(resp, "message", "로그인 성공");

                // owner_info.json에서 정보 읽어서 추가
                ifstream file("owner_info.json");
                string json_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
                file.close();

                cJSON* all_users = cJSON_Parse(json_str.c_str());
                int count = cJSON_GetArraySize(all_users);
                for (int i = 0; i < count; ++i) {
                    cJSON* user = cJSON_GetArrayItem(all_users, i);
                    cJSON* json_id = cJSON_GetObjectItem(user, "PHONENUMBER");
                    if (cJSON_IsString(json_id) && strcmp(json_id->valuestring, phonenumber->valuestring) == 0) {

                        // 응답에 사장 정보 모두 추가
                        cJSON_AddStringToObject(resp, "BRAND_UID", cJSON_GetObjectItem(user, "BRAND_UID")->valuestring);
                        cJSON_AddStringToObject(resp, "PLACE_UID", cJSON_GetObjectItem(user, "PLACE_UID")->valuestring);
                        cJSON_AddStringToObject(resp, "NAME", cJSON_GetObjectItem(user, "NAME")->valuestring);
                        cJSON_AddStringToObject(resp, "ADDRESS", cJSON_GetObjectItem(user, "ADDRESS")->valuestring);
                        cJSON_AddStringToObject(resp, "ADDRESS_DETAIL", cJSON_GetObjectItem(user, "ADDRESS_DETAIL")->valuestring);
                        cJSON_AddStringToObject(resp, "OPEN_DATE", cJSON_GetObjectItem(user, "OPEN_DATE")->valuestring);
                        cJSON_AddStringToObject(resp, "PHONENUMBER", json_id->valuestring);
                        cJSON_AddStringToObject(resp, "type", cJSON_GetObjectItem(user, "type")->valuestring);
                        cJSON_AddStringToObject(resp, "DELIVERY_TIM", cJSON_GetObjectItem(user, "DELIVERY_TIM")->valuestring);
                        cJSON_AddStringToObject(resp, "OPEN_TIME", cJSON_GetObjectItem(user, "OPEN_TIME")->valuestring);
                        cJSON_AddStringToObject(resp, "CLOSE_TIME", cJSON_GetObjectItem(user, "CLOSE_TIME")->valuestring);
                        cJSON_AddStringToObject(resp, "ORIGIN_INFO", cJSON_GetObjectItem(user, "ORIGIN_INFO")->valuestring);
                        cJSON_AddStringToObject(resp, "MIN_ORDER_PRICE", cJSON_GetObjectItem(user, "MIN_ORDER_PRICE")->valuestring);
                        cJSON_AddStringToObject(resp, "DELIVERY_FEE", cJSON_GetObjectItem(user, "DELIVERY_FEE")->valuestring);
                        cJSON_AddStringToObject(resp, "FREE_DELIVERY_STANDARD", cJSON_GetObjectItem(user, "FREE_DELIVERY_STANDARD")->valuestring);
                        cJSON_AddStringToObject(resp, "IMAGE_MAIN", cJSON_GetObjectItem(user, "IMAGE_MAIN")->valuestring);
                        cJSON_AddStringToObject(resp, "IMAGE_PASS", cJSON_GetObjectItem(user, "IMAGE_PASS")->valuestring);
                        break;
                    }
                }
                cJSON_Delete(all_users);
            } else {
                cJSON_AddStringToObject(resp, "action", "100_2");
                cJSON_AddStringToObject(resp, "message", "로그인 실패");
            }

            string response_str = cJSON_PrintUnformatted(resp);
            response_str += "\n<<END>>"; // 구분자 추가
            cout << "서버 -> 사장: " << response_str << endl;
            send(sock, response_str.c_str(), response_str.length(), 0);
            cJSON_Delete(resp);
        }
////////////////////////////////////////////

        // 사장 주문 조회 요청 처리
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "110_0") == 0) {
            const cJSON* place_uid = cJSON_GetObjectItem(root, "place_uid");
            cout << "▶ 서버: 사장 주문 조회 요청 받음, place_uid="
                 << (place_uid ? place_uid->valuestring : "NULL") << endl;

            // DB 연결
            MYSQL* conn = mysql_init(nullptr);
            if (!conn || !mysql_real_connect(conn, "10.10.20.99", "JIN", "1234", "WHAT", 3306, nullptr, 0)) {
                perror("DB 연결 실패");

                // 실패 응답
                cJSON* err_resp = cJSON_CreateObject();
                cJSON_AddStringToObject(err_resp, "action", "110_2");
                cJSON_AddStringToObject(err_resp, "message", "DB 연결 실패");

                char* err_str = cJSON_PrintUnformatted(err_resp);
                string full_str = string(err_str) + "\n<<END>>"; // 구분자 추가
                send(sock, full_str.c_str(), full_str.length(), 0);
                free(err_str);
                cJSON_Delete(err_resp);

                if (conn) mysql_close(conn);
                cJSON_Delete(root);
                break;
            }

            // 쿼리 ORDER_ 테이블
                char esc_pid[32];
                unsigned long pid_len = mysql_real_escape_string(conn, esc_pid,
                place_uid->valuestring,
                strlen(place_uid->valuestring));
                esc_pid[pid_len] = '\0';
                char q1[1024];
                snprintf(q1, sizeof(q1),
                    "SELECT ORDER_ID, BRAND_UID, PLACE_UID, TOTAL_PRICE, "
                    "DATE_FORMAT(ORDER_TIME,'%%Y-%%m-%%d %%H:%%i:%%s') AS ORDER_TIME, "
                    "ADDRESS, ADDRESS_DETAIL, "
                    "IFNULL(ORDER_MSG,'') AS ORDER_MSG, IFNULL(RIDER_MSG,'') AS RIDER_MSG, "
                    "STATUS_TOGO, STATUS_DISPOSIBLE, STATUS_ORDER "
                    "FROM `ORDER_` WHERE PLACE_UID='%s'", esc_pid);
                if (mysql_query(conn, q1)) {
                    fprintf(stderr, "쿼리 실패: %s\n", mysql_error(conn));

                    // --- 실패 응답 추가 ---
                    cJSON* err_resp = cJSON_CreateObject();
                    cJSON_AddStringToObject(err_resp, "action", "110_2");
                    cJSON_AddStringToObject(err_resp, "message", "주문 조회 쿼리 실패");

                    char* err_str = cJSON_PrintUnformatted(err_resp);
                    string full_str = string(err_str) + "\n<<END>>"; // 구분자 추가
                    send(sock, full_str.c_str(), full_str.length(), 0);
                    free(err_str);
                    cJSON_Delete(err_resp);

                    mysql_close(conn);
                    cJSON_Delete(root);
                    break;
                }
                MYSQL_RES* res1 = mysql_store_result(conn);

                // 응답 JSON 만들기
                cJSON* resp2 = cJSON_CreateObject();
                cJSON_AddStringToObject(resp2, "action", "110_1");
                cJSON* arr2 = cJSON_AddArrayToObject(resp2, "orders");

                MYSQL_ROW r1;
                while ((r1 = mysql_fetch_row(res1))) {
                    cJSON* o = cJSON_CreateObject();
                    // r1 배열의 인덱스가 SELECT 순서와 1:1 매핑되도록
                    cJSON_AddStringToObject(o, "order_id",       r1[0] ? r1[0] : "");
                    cJSON_AddNumberToObject(o, "brand_uid",      r1[1] ? atoi(r1[1]) : 0);
                    cJSON_AddNumberToObject(o, "place_uid",      r1[2] ? atoi(r1[2]) : 0);
                    cJSON_AddNumberToObject(o, "total_price",    r1[3] ? atoi(r1[3]) : 0);
                    cJSON_AddStringToObject(o, "order_time",     r1[4] ? r1[4] : "");
                    cJSON_AddStringToObject(o, "address",        r1[5] ? r1[5] : "");
                    cJSON_AddStringToObject(o, "address_detail", r1[6] ? r1[6] : "");
                    cJSON_AddStringToObject(o, "order_msg",      r1[7] ? r1[7] : "");
                    cJSON_AddStringToObject(o, "rider_msg",      r1[8] ? r1[8] : "");
                    cJSON_AddNumberToObject(o, "status_togo",     r1[9]  ? atoi(r1[9])  : 0);
                    cJSON_AddNumberToObject(o, "status_disposible",r1[10] ? atoi(r1[10]) : 0);
                    cJSON_AddNumberToObject(o, "status_order",     r1[11] ? atoi(r1[11]) : 0);


                // ORDER_DETAIL 조회
                char q2[256];
                snprintf(q2, sizeof(q2),
                    "SELECT MENU_NAME, MENU_PRICE, MENU_CNT, OPT_NAME_ALL, OPT_PRICE_ALL "
                    "FROM ORDER_DETAIL WHERE ORDER_ID='%s'", r1[0]);
                if (!mysql_query(conn, q2)) {
                    MYSQL_RES* res2 = mysql_store_result(conn);
                    cJSON* det = cJSON_AddArrayToObject(o, "details");
                    MYSQL_ROW r2;
                    while ((r2 = mysql_fetch_row(res2))) {
                        cJSON* d = cJSON_CreateObject();
                        cJSON_AddStringToObject(d, "menu_name", r2[0] ? r2[0] : "");
                        cJSON_AddNumberToObject(d, "menu_price", r2[1] ? atoi(r2[1]) : 0);
                        cJSON_AddNumberToObject(d, "menu_cnt",   r2[2] ? atoi(r2[2]) : 0);
                        cJSON_AddStringToObject(d, "opt_name_all",  r2[3] ? r2[3] : "");
                        cJSON_AddNumberToObject(d, "opt_price_all", r2[4] ? atoi(r2[4]) : 0);
                        cJSON_AddItemToArray(det, d);
                    }
                    mysql_free_result(res2);
                }
                cJSON_AddItemToArray(arr2, o);
            }
            // 마무리
            mysql_free_result(res1);
            mysql_close(conn);

            char* out2 = cJSON_PrintUnformatted(resp2);
            string full_resp = string(out2) + "\n<<END>>"; // 구분자 추가
            send(sock, full_resp.c_str(), full_resp.length(), 0);
            cout << "서버: 사장 주문 응답(110_1) 전송 완료\n";
            free(out2);
            cJSON_Delete(resp2);
        }
        cJSON_Delete(root); // 메모리 해제
    }
}

//회원 회원가입 함수 json 폴더 오픈
bool user_register_user(const char* type, const char* uid, const char* email_id, const char* pw, const char* name, const char* birth, const char* phonenumber, const char* pass, const char* point) {    //()에 있는거 사용자로부터 받은 아이디 비번 bool로 반환해서 성공실패 여부
    ifstream file("user_info.json");  //json파일 열어보기
    string json_str;

    if (file.is_open()) {   //잘 열렸는지 확인
        // 열렸으면 json_str에 한줄로 다 저장
        json_str.assign((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();
    } else {
        // 파일이 없으면 빈 배열 생성
        json_str = "[]";    //회원이 아무도 없는 상태
    }

    cJSON* root = cJSON_Parse(json_str.c_str()); //문자열로 된 JSON을 CJSON 객체로 파싱
    //c_str이게 string객체를 const char* 형태로 변환해줌
    if (!root) {
        std::cerr << "user_info.json 파싱 실패\n";
        return false;
    }

    // ID 중복 검사
    int array_size = cJSON_GetArraySize(root);
    for (int i = 0; i < array_size; ++i) {          //아이디 존재만큼 for문 돌리기
        cJSON* item = cJSON_GetArrayItem(root, i);  //회원 한명의 정보를 담음
        cJSON* json_id = cJSON_GetObjectItem(item, "email_id");
        if (cJSON_IsString(json_id) && strcmp(json_id->valuestring, email_id) == 0) { //입력한거랑 json에서 가져온거랑 같은지
            cerr << "이미 존재하는 ID입니다.\n";
            cJSON_Delete(root);
            return false;
        }
    }

    // 새 유저 추가
    cJSON* new_user = cJSON_CreateObject();
    cJSON_AddStringToObject(new_user, "type", type);
    cJSON_AddStringToObject(new_user, "uid", uid);
    cJSON_AddStringToObject(new_user, "email_id", email_id);
    cJSON_AddStringToObject(new_user, "pw", pw);
    cJSON_AddStringToObject(new_user, "name", name);
    cJSON_AddStringToObject(new_user, "birth", birth);
    cJSON_AddStringToObject(new_user, "phonenumber", phonenumber);
    cJSON_AddStringToObject(new_user, "pass", pass);
    cJSON_AddStringToObject(new_user, "point", point);
    cJSON_AddItemToArray(root, new_user);

    // 파일로 저장
    ofstream outfile("user_info.json");   //outfile로 파일에 쓰기 가능해짐
    if (!outfile.is_open()) {
        cerr << "파일 쓰기 실패\n";
        cJSON_Delete(root);
        return false;
    }

    char* new_json_str = cJSON_Print(root); //cjson_print로 root 전체 json 구조를 문자열로 출력
    outfile << new_json_str;    //그 문자열을 파일에 저장함
    outfile.close();
    cJSON_free(new_json_str);
    cJSON_Delete(root);

    return true;
}

// 회원 id/pw 확인 함수 추가
bool user_check_credentials(const char* email_id, const char* pw) {
    ifstream file("user_info.json");
    if (!file.is_open()) {
        cerr << "user_info.json 파일을 열 수 없습니다.\n";
        return false;
    }

    string json_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    cJSON* root = cJSON_Parse(json_str.c_str());
    if (!root) {
        cerr << "user_info.json 파싱 실패\n";
        return false;
    }

    int array_size = cJSON_GetArraySize(root);
    for (int i = 0; i < array_size; ++i) {
        cJSON* item = cJSON_GetArrayItem(root, i);
        cJSON* json_id = cJSON_GetObjectItem(item, "email_id");
        cJSON* json_pw = cJSON_GetObjectItem(item, "pw");

        cout << i << (json_id ? json_id->valuestring : "email_id 없음") << ", "
        << (json_pw ? json_pw->valuestring : "PW 없음") << endl;

        if (cJSON_IsString(json_id) && cJSON_IsString(json_pw)) {
            if (strcmp(json_id->valuestring, email_id) == 0 && strcmp(json_pw->valuestring, pw) == 0) {
                cJSON_Delete(root);
                return true;  // 아이디/비번 일치
            }
        }
    }
    cJSON_Delete(root);
    return false; // 일치하는 아이디/비번 없음
}

//회원정보 요청 처리
void user_handle_client(int* client_sock) {
    int sock = *client_sock;
    delete client_sock;

    char buffer[BUFFER_SIZE] = {0};

    while (true) {
        // 클라이언트로부터 데이터 수신
        int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0); //수신된 바이트 수
        if (bytes_received <= 0) {
            cout << "클라이언트 연결 종료\n";
            break;
        }

        buffer[bytes_received] = '\0';
        cout << "recv 반환값: " << bytes_received << ", 받은 데이터: " << buffer << endl;

        // JSON 파싱
        cJSON* root = cJSON_Parse(buffer);
        if (!root) {
            cerr << "JSON 파싱 실패\n";
            continue;
        }

        // JSON 객체에서 필드 추출
        const cJSON* action = cJSON_GetObjectItem(root, "action");
        const cJSON* type = cJSON_GetObjectItem(root, "type");
        const cJSON* uid = cJSON_GetObjectItem(root, "uid");
        const cJSON* email_id = cJSON_GetObjectItem(root, "email_id");
        const cJSON* pw = cJSON_GetObjectItem(root, "pw");
        const cJSON* name = cJSON_GetObjectItem(root, "name");
        const cJSON* birth = cJSON_GetObjectItem(root, "birth");
        const cJSON* phonenumber = cJSON_GetObjectItem(root, "phonenumber");
        const cJSON* pass = cJSON_GetObjectItem(root, "pass");
        const cJSON* point = cJSON_GetObjectItem(root, "point");

        // 로그인 요청 처리
        if (cJSON_IsString(action) && strcmp(action->valuestring, "1_0") == 0) {
            cout << "로그인 요청\n";
            cout << "email_id: " << (email_id ? email_id->valuestring : "NULL") << "\n";
            cout << "pw: " << (pw ? pw->valuestring : "NULL") << "\n";

            bool login_success = false;
            if (email_id && pw) {
                login_success = user_check_credentials(email_id->valuestring, pw->valuestring);
            }

            // 응답 JSON 생성
            cJSON* resp = cJSON_CreateObject();
            if (login_success) {
                cJSON_AddStringToObject(resp, "action", "1_1");
                cJSON_AddStringToObject(resp, "message", "로그인 성공");

                // user_info.json 파일에서 해당 유저 정보 추출
                ifstream file("user_info.json");
                string json_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
                file.close();

                cJSON* all_users = cJSON_Parse(json_str.c_str());
                int array_size = cJSON_GetArraySize(all_users);

                for (int i = 0; i < array_size; ++i) {
                    cJSON* user = cJSON_GetArrayItem(all_users, i);
                    cJSON* json_id = cJSON_GetObjectItem(user, "email_id");

                    if (cJSON_IsString(json_id) && strcmp(json_id->valuestring, email_id->valuestring) == 0) {
                        cJSON_AddStringToObject(resp, "uid", cJSON_GetObjectItem(user, "uid")->valuestring);
                        cJSON_AddStringToObject(resp, "email_id", cJSON_GetObjectItem(user, "email_id")->valuestring);
                        cJSON_AddStringToObject(resp, "name", cJSON_GetObjectItem(user, "name")->valuestring);
                        cJSON_AddStringToObject(resp, "birth", cJSON_GetObjectItem(user, "birth")->valuestring);
                        cJSON_AddStringToObject(resp, "phonenumber", cJSON_GetObjectItem(user, "phonenumber")->valuestring);
                        cJSON_AddStringToObject(resp, "pass", cJSON_GetObjectItem(user, "pass")->valuestring);
                        cJSON_AddStringToObject(resp, "point", cJSON_GetObjectItem(user, "point")->valuestring);
                        break;
                    }
                }
                cJSON_Delete(all_users);
            } else {
                cJSON_AddStringToObject(resp, "action", "1_2");
                cJSON_AddStringToObject(resp, "message", "로그인 실패");
            }

            // 응답 전송 - 끝에 "\n<<END>>" 추가
            char* resp_str = cJSON_PrintUnformatted(resp);
            std::string send_str = std::string(resp_str) + "\n<<END>>";
            send(sock, send_str.c_str(), send_str.size(), 0);

            cout << "서버 -> 클라이언트: " << send_str << endl;

            free(resp_str);
            cJSON_Delete(resp);
        }


        // 회원가입 요청 처리
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "2_0") == 0) {
            cout << "회원가입 요청\n";
            cout << "type: " << (type ? type->valuestring : "NULL") << "\n";
            cout << "uid: " << (uid ? uid->valuestring : "NULL") << "\n";
            cout << "email_id: " << (email_id ? email_id->valuestring : "NULL") << "\n";
            cout << "pw: " << (pw ? pw->valuestring : "NULL") << "\n";
            cout << "name: " << (name ? name->valuestring : "NULL") << "\n";
            cout << "birth: " << (birth ? birth->valuestring : "NULL") << "\n";
            cout << "phonenumber: " << (phonenumber ? phonenumber->valuestring : "NULL") << "\n";
            cout << "point: " << (point ? point->valuestring : "NULL") << "\n";

            bool register_success = false;
            if (email_id && pw) {
                register_success = user_register_user(
                    type ? type->valuestring : "", 
                    uid ? uid->valuestring : "", 
                    email_id->valuestring, pw->valuestring,
                    name ? name->valuestring : "", 
                    birth ? birth->valuestring : "", 
                    phonenumber ? phonenumber->valuestring : "", 
                    pass ? pass->valuestring : "", 
                    point ? point->valuestring : ""
                );
            }

            // 응답 JSON 생성
            cJSON* resp = cJSON_CreateObject();
            if (register_success) {
                cJSON_AddStringToObject(resp, "action", "2_1");
                cJSON_AddStringToObject(resp, "message", "회원가입 성공");
            } else {
                cJSON_AddStringToObject(resp, "action", "2_2");
                cJSON_AddStringToObject(resp, "message", "회원가입 실패 (중복 ID)");
            }
            // 응답 전송 - 끝에 "\n<<END>>" 추가
            char* resp_str = cJSON_PrintUnformatted(resp);
            std::string send_str = std::string(resp_str) + "\n<<END>>";
            send(sock, send_str.c_str(), send_str.size(), 0);

            cout << "서버 -> 클라이언트: " << send_str << endl;

            free(resp_str);
            cJSON_Delete(resp);
        }

        // 주소 요청 처리
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "7_0") == 0) {
            cout << "주소 요청\n";

            if (uid && cJSON_IsString(uid)) {
                // DB 연결
                MYSQL* conn = mysql_init(NULL);
                if (!conn) {
                    perror("mysql_init 실패");
                    continue;
                }

                if (!mysql_real_connect(conn, "10.10.20.99", "JIN", "1234", "WHAT", 3306, NULL, 0)) {
                    perror("DB 연결 실패");
                    mysql_close(conn);
                    continue;
                }

                // 쿼리 실행    
                char escaped_uid[128];  
                mysql_real_escape_string(conn, escaped_uid, uid->valuestring, strlen(uid->valuestring));
                // 사용자 주소 db에서 가져옴
                char query[256];
                snprintf(query, sizeof(query),
                        "SELECT ADDRESS_NAME, ADDRESS, ADDRESS_DETAIL, ADDRESS_TYPE, ADDRESS_BASIC FROM USER_ADDRESS WHERE uid = '%s'", escaped_uid);

                if (mysql_query(conn, query)) {
                    perror("쿼리 실행 실패");
                    mysql_close(conn);
                    continue;
                }

                // uid 값을 이용해서 해당 사용자 주소 찾는 쿼리문 작성함
                MYSQL_RES* res = mysql_store_result(conn);
                MYSQL_ROW row;
                cJSON* response = cJSON_CreateObject();
                cJSON* address_array = cJSON_CreateArray(); // 주소 여러개 저장할 json 배열 준비

                // uid 값 추출
                const cJSON* uid = cJSON_GetObjectItem(root, "uid");
                
                // DB에서 가져온 각 주소를 JSON으로 만들어서 배열에 추가함
                while ((row = mysql_fetch_row(res))) {
                    cJSON* address_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(address_obj, "address_name", row[0] ? row[0] : ""); //널값 들어가면 터지니까 뒤에 붙이기
                    cJSON_AddStringToObject(address_obj, "address", row[1] ? row[1] : "");
                    cJSON_AddStringToObject(address_obj, "address_detail", row[2] ? row[2] : "");
                    cJSON_AddStringToObject(address_obj, "address_type", row[3] ? row[3] : "");
                    cJSON_AddStringToObject(address_obj, "address_basic", row[4] ? row[4] : "");

                    cJSON_AddItemToArray(address_array, address_obj);                    // 배열에 추가
                }

                // 성공했을때
                if (cJSON_GetArraySize(address_array) > 0) {
                    cJSON_AddStringToObject(response, "action", "7_1");                  // 응답 코드
                    cJSON_AddStringToObject(response, "result", "주소 보내준");         // 응답 결과
                    cJSON_AddItemToObject(response, "address_list", address_array);     // 주소 배열 추가
                    cout << "주소 조회 성공 - 개수: " << cJSON_GetArraySize(address_array) << endl;
                } 
                else {
                    // 주소가 없는 경우
                    cJSON_AddStringToObject(response, "action", "7_2");
                    cJSON_AddStringToObject(response, "result", "fail");
                    cJSON_AddStringToObject(response, "address", "해당 uid 없음");
                    cJSON_Delete(address_array);  // 안 쓰는 배열 삭제
                    cout << "주소 조회 실패 UID 없음\n";
                }

                // address_base.json 파일 열기
                FILE* fp = fopen("address_base.json", "r");
                if (fp) {
                    fseek(fp, 0, SEEK_END);
                    long filesize = ftell(fp);
                    rewind(fp);

                    char* buffer = (char*)malloc(filesize + 1);
                    fread(buffer, 1, filesize, fp);
                    buffer[filesize] = '\0';
                    fclose(fp);

                    cJSON* area_array = cJSON_Parse(buffer);
                    if (area_array) {
                        cJSON_AddItemToObject(response, "address_base", area_array);  // 지역 리스트 통째로 추가
                    } else {
                        cout << "address_base.json 파싱 실패\n";
                    }
                    free(buffer);
                } 
                else {
                    cout << "address_base.json 파일 열기 실패\n";
                }

                // district 필드 추가 (예시: "서귀포시")
                // cJSON_AddStringToObject(response, "district", "서귀포시");

                // 응답 전송 전에 JSON 문자열 생성 및 끝에 "\n<<END>>" 추가
                char* response_str = cJSON_PrintUnformatted(response);
                string send_str = string(response_str) + "\n<<END>>";

                send(sock, send_str.c_str(), send_str.size(), 0);

                free(response_str);
                cJSON_Delete(response);
                mysql_free_result(res);
                mysql_close(conn);
            } 
            else {
                // uid가 없는 경우
                cJSON* response = cJSON_CreateObject();
                cJSON_AddStringToObject(response, "action", "7_2");
                cJSON_AddStringToObject(response, "result", "실패 아이디없음");
                cJSON_AddStringToObject(response, "address", "유효한 uid");

                char* response_str = cJSON_PrintUnformatted(response);
                string send_str = string(response_str) + "\n<<END>>";

                send(sock, send_str.c_str(), send_str.size(), 0);
                cout << "서버 -> 클라이언트 전송완료" << endl;

                free(response_str);
                cJSON_Delete(response);
            }
        }


        ///////////////////////////////////////
        // 회원 주소 추가 요청
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "8_0") == 0) {
            cout << "주소 추가 요청\n";

            // JSON에서 값 추출
            const cJSON* uid = cJSON_GetObjectItem(root, "uid");
            const cJSON* address_name = cJSON_GetObjectItem(root, "address_name");  // 선택적
            const cJSON* new_address = cJSON_GetObjectItem(root, "new_address");
            const cJSON* new_address_detail = cJSON_GetObjectItem(root, "new_address_detail");
            const cJSON* new_address_type = cJSON_GetObjectItem(root, "new_address_type");
            const cJSON* new_address_basic = cJSON_GetObjectItem(root, "new_address_basic");

            // 필수값 체크: uid, new_address
            if (cJSON_IsString(uid) && cJSON_IsString(new_address)) {
                MYSQL* conn = mysql_init(NULL);
                if (!conn) {
                    perror("mysql_init 실패");
                    return;
                }

                if (!mysql_real_connect(conn, "10.10.20.99", "JIN", "1234", "WHAT", 3306, NULL, 0)) {
                    perror("DB 연결 실패");
                    mysql_close(conn);
                    return;
                }

                // address_name은 선택적 -> 값 없으면 빈 문자열 처리
                const char* addr_name_str = "";
                if (cJSON_IsString(address_name)) {
                    addr_name_str = address_name->valuestring;
                }

                // new_address_type 문자열 또는 숫자로 들어오므로 둘 다 체크함
                int addr_type_int = 0;
                if (cJSON_IsString(new_address_type)) {
                    addr_type_int = atoi(new_address_type->valuestring);
                } else if (cJSON_IsNumber(new_address_type)) {
                    addr_type_int = new_address_type->valueint;
                }

                // new_address_basic 문자열 또는 숫자로 들어오므로 둘 다 체크함
                int addr_basic_int = 0;
                if (cJSON_IsString(new_address_basic)) {
                    addr_basic_int = atoi(new_address_basic->valuestring);
                } else if (cJSON_IsNumber(new_address_basic)) {
                    addr_basic_int = new_address_basic->valueint;
                }

                // 디버깅 로그
                cout << "address_type: " << addr_type_int << ", address_basic: " << addr_basic_int << endl;

                // 입력값 escape 처리
                char arr_uid[128], arr_name[128], full_addr[512], addr_detail[512];
                mysql_real_escape_string(conn, arr_uid, uid->valuestring, strlen(uid->valuestring));
                mysql_real_escape_string(conn, arr_name, addr_name_str, strlen(addr_name_str));
                mysql_real_escape_string(conn, full_addr, new_address->valuestring, strlen(new_address->valuestring));

                if (cJSON_IsString(new_address_detail)) {
                    mysql_real_escape_string(conn, addr_detail, new_address_detail->valuestring, strlen(new_address_detail->valuestring));
                } else {
                    addr_detail[0] = '\0';  // 값 없으면 빈 문자열
                }

                // 쿼리문 작성
                char query[1024];
                snprintf(query, sizeof(query),
                    "INSERT INTO USER_ADDRESS (uid, ADDRESS_NAME, ADDRESS, ADDRESS_DETAIL, ADDRESS_TYPE, ADDRESS_BASIC) "
                    "VALUES ('%s', '%s', '%s', '%s', %d, %d)",
                    arr_uid, arr_name, full_addr, addr_detail, addr_type_int, addr_basic_int);

                // 응답 객체 생성
                cJSON* response = cJSON_CreateObject();
                if (mysql_query(conn, query) == 0) {
                    cJSON_AddStringToObject(response, "action", "8_1");
                    cout << "주소 추가 성공\n";
                } else {
                    cJSON_AddStringToObject(response, "action", "8_2");
                    cJSON_AddStringToObject(response, "result", "주소 추가 실패");
                    cout << "쿼리 실행 오류: " << mysql_error(conn) << endl;
                }

                // 주소 목록 재조회
                char select_query[512];
                snprintf(select_query, sizeof(select_query),
                    "SELECT ADDRESS_NAME, ADDRESS, ADDRESS_DETAIL, ADDRESS_TYPE, ADDRESS_BASIC FROM USER_ADDRESS WHERE uid = '%s'",
                    arr_uid);

                if (mysql_query(conn, select_query) == 0) {
                    MYSQL_RES* res = mysql_store_result(conn);
                    MYSQL_ROW row;
                    cJSON* address_array = cJSON_CreateArray();

                    while ((row = mysql_fetch_row(res))) {
                        cJSON* address_obj = cJSON_CreateObject();
                        cJSON_AddStringToObject(address_obj, "address_name", row[0] ? row[0] : "");
                        cJSON_AddStringToObject(address_obj, "address", row[1] ? row[1] : "");
                        cJSON_AddStringToObject(address_obj, "address_detail", row[2] ? row[2] : "");
                        cJSON_AddStringToObject(address_obj, "address_type", row[3] ? row[3] : "");
                        cJSON_AddStringToObject(address_obj, "address_basic", row[4] ? row[4] : "");
                        cJSON_AddItemToArray(address_array, address_obj);
                    }

                    cJSON_AddItemToObject(response, "address_list", address_array);
                    mysql_free_result(res);
                    cout << "주소 조회 개수: " << cJSON_GetArraySize(address_array) << endl;
                } else {
                    cJSON_AddStringToObject(response, "address_list", "조회 실패");
                    cout << "주소 조회 실패: " << mysql_error(conn) << endl;
                }

                // 응답 전송
                char* response_str = cJSON_PrintUnformatted(response);
                std::string send_str = std::string(response_str) + "\n<<END>>";
                send(sock, send_str.c_str(), send_str.size(), 0);
                cout << "서버 -> 클라이언트: " << send_str << endl;

                free(response_str);
                cJSON_Delete(response);
                mysql_close(conn);
            } else {
                cout << "필수 파라미터 누락\n";
            }
        }

        // 주소 삭제 요청
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "9_0") == 0) {
            cout << "주소 삭제 요청\n";

            // JSON에서 값 추출
            const cJSON* uid = cJSON_GetObjectItem(root, "uid");        // 해당 아이디
            const cJSON* full_address_detail = cJSON_GetObjectItem(root, "del_address_detail");    // 유저가 삭제하고싶은 상세주소

            if (cJSON_IsString(uid) && cJSON_IsString(full_address_detail)) {
                MYSQL* conn = mysql_init(NULL);
                if (!conn) {
                    perror("mysql_init 실패");
                    return;
                }

                if (!mysql_real_connect(conn, "10.10.20.99", "JIN", "1234", "WHAT", 3306, NULL, 0)) {
                    perror("DB 연결 실패");
                    mysql_close(conn);
                    return;
                }

                // 입력값 escape 처리
                char arr_uid[128], arr_full_addr_detail[512];
                mysql_real_escape_string(conn, arr_uid, uid->valuestring, strlen(uid->valuestring));
                mysql_real_escape_string(conn, arr_full_addr_detail, full_address_detail->valuestring, strlen(full_address_detail->valuestring));

                // 상세주소 기준 주소 삭제 쿼리
                char del_query[1024];
                snprintf(del_query, sizeof(del_query),
                    "DELETE FROM USER_ADDRESS WHERE uid='%s' AND ADDRESS_DETAIL='%s'",
                    arr_uid, arr_full_addr_detail);
                cout << "삭제 쿼리: " << del_query << endl;

                // 삭제 응답
                cJSON* response = cJSON_CreateObject();
                if (mysql_query(conn, del_query) == 0) {
                    cJSON_AddStringToObject(response, "action", "9_1");
                    cJSON_AddStringToObject(response, "result", "주소 삭제 성공");
                    cout << "주소 삭제 성공\n";
                } else {
                    cJSON_AddStringToObject(response, "action", "9_2");
                    cJSON_AddStringToObject(response, "result", "주소 삭제 실패");
                    cout << "쿼리 실행 오류: " << mysql_error(conn) << endl;
                }

                // 삭제 이후 주소 목록 재조회
                char select_query[512];
                snprintf(select_query, sizeof(select_query),
                    "SELECT ADDRESS_NAME, ADDRESS, ADDRESS_DETAIL, ADDRESS_TYPE, ADDRESS_BASIC FROM USER_ADDRESS WHERE uid = '%s'",
                    arr_uid);

                if (mysql_query(conn, select_query) == 0) {
                    MYSQL_RES* res = mysql_store_result(conn);
                    MYSQL_ROW row;
                    cJSON* address_array = cJSON_CreateArray();

                    while ((row = mysql_fetch_row(res))) {
                        cJSON* address_obj = cJSON_CreateObject();
                        cJSON_AddStringToObject(address_obj, "address_name", row[0] ? row[0] : "");
                        cJSON_AddStringToObject(address_obj, "address", row[1] ? row[1] : "");
                        cJSON_AddStringToObject(address_obj, "address_detail", row[2] ? row[2] : "");
                        cJSON_AddStringToObject(address_obj, "address_type", row[3] ? row[3] : "");
                        cJSON_AddStringToObject(address_obj, "address_basic", row[4] ? row[4] : "");
                        cJSON_AddItemToArray(address_array, address_obj);
                    }

                    cJSON_AddItemToObject(response, "address_list", address_array);
                    mysql_free_result(res);
                    cout << "주소 조회 개수: " << cJSON_GetArraySize(address_array) << endl;
                } else {
                    cJSON_AddStringToObject(response, "address_list", "조회 실패");
                    cout << "주소 조회 실패: " << mysql_error(conn) << endl;
                }

                // 응답 전송 - 끝에 "\n<<END>>" 추가
                char* response_str = cJSON_PrintUnformatted(response);
                std::string send_str = std::string(response_str) + "\n<<END>>";
                send(sock, send_str.c_str(), send_str.size(), 0);
                cout << "서버 -> 클라이언트: " << send_str << endl;

                free(response_str);
                cJSON_Delete(response);
                mysql_close(conn);
            }
        }

        /////////////////////////////// 현희님 코드
        // 주문 조회 요청 10_0 처리
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "10_0") == 0) {
            cout << "주문 조회 요청\n";
            cout << "uid: " << (uid ? uid->valuestring : "NULL") << "\n";

            if (uid && cJSON_IsString(uid)) {
                // 1) DB 연결
                MYSQL* conn = mysql_init(nullptr);
                if (!conn) {
                    perror("mysql_init 실패");
                    return;
                }
                if (!mysql_real_connect(conn, "10.10.20.99", "JIN", "1234", "WHAT", 3306, nullptr, 0)) {
                    fprintf(stderr, "DB 연결 실패: %s\n", mysql_error(conn));
                    mysql_close(conn);
                    return;
                }

                // 2) uid 이스케이프
                char esc_uid[32];
                mysql_real_escape_string(conn, esc_uid, uid->valuestring, strlen(uid->valuestring));

                // 3) ORDER_ 테이블 조회
                char q1[512];
                snprintf(q1, sizeof(q1),
                    "SELECT "
                    "ORDER_ID, BRAND_UID, PLACE_UID, TOTAL_PRICE, "
                    "DATE_FORMAT(ORDER_TIME,'%%Y-%%m-%%d %%H:%%i:%%s') AS ORDER_TIME, "
                    "ADDRESS, ADDRESS_DETAIL, "
                    "IFNULL(ORDER_MSG,'') AS ORDER_MSG, "
                    "IFNULL(RIDER_MSG,'') AS RIDER_MSG, "
                    "STATUS_TOGO, STATUS_DISPOSIBLE, STATUS_ORDER "
                    "FROM `ORDER_` "
                    "WHERE UID=%s",
                    esc_uid);
                if (mysql_query(conn, q1)) {
                    fprintf(stderr, "쿼리 실패(ORDER_): %s\n", mysql_error(conn));
                    mysql_close(conn);
                    return;
                }
                cout << "쿼리성공\n";
                MYSQL_RES* res1 = mysql_store_result(conn);

                // 4) JSON 응답 생성
                cJSON* resp = cJSON_CreateObject();
                cJSON_AddStringToObject(resp, "action", "10_1");
                cout << "10_1전송\n";
                cJSON* arr = cJSON_AddArrayToObject(resp, "orders");

                MYSQL_ROW r1;
                while ((r1 = mysql_fetch_row(res1))) {
                    cJSON* o = cJSON_CreateObject();
                    // r1 배열의 인덱스가 SELECT 순서와 1:1 매핑되도록
                    cJSON_AddStringToObject(o, "order_id",       r1[0] ? r1[0] : "");
                    cJSON_AddNumberToObject(o, "brand_uid",      r1[1] ? atoi(r1[1]) : 0);
                    cJSON_AddNumberToObject(o, "place_uid",      r1[2] ? atoi(r1[2]) : 0);
                    cJSON_AddNumberToObject(o, "total_price",    r1[3] ? atoi(r1[3]) : 0);
                    cJSON_AddStringToObject(o, "order_time",     r1[4] ? r1[4] : "");
                    cJSON_AddStringToObject(o, "address",        r1[5] ? r1[5] : "");
                    cJSON_AddStringToObject(o, "address_detail", r1[6] ? r1[6] : "");
                    cJSON_AddStringToObject(o, "order_msg",      r1[7] ? r1[7] : "");
                    cJSON_AddStringToObject(o, "rider_msg",      r1[8] ? r1[8] : "");
                    cJSON_AddNumberToObject(o, "status_togo",     r1[9]  ? atoi(r1[9])  : 0);
                    cJSON_AddNumberToObject(o, "status_disposible",r1[10] ? atoi(r1[10]) : 0);
                    cJSON_AddNumberToObject(o, "status_order",     r1[11] ? atoi(r1[11]) : 0);


                    // 5) ORDER_DETAIL 조회
                    char q2[256];
                    snprintf(q2, sizeof(q2),
                        "SELECT MENU_NAME, MENU_PRICE, MENU_CNT, OPT_NAME_ALL, OPT_PRICE_ALL "
                        "FROM ORDER_DETAIL WHERE ORDER_ID='%s'", r1[0]);
                    if (!mysql_query(conn, q2)) {
                        MYSQL_RES* res2 = mysql_store_result(conn);
                        cJSON* det = cJSON_AddArrayToObject(o, "details");
                        MYSQL_ROW r2;
                        while ((r2 = mysql_fetch_row(res2))) {
                            cJSON* d = cJSON_CreateObject();
                            cJSON_AddStringToObject(d, "menu_name",
                                r2[0] ? r2[0] : "");

                            // NULL 체크 후 atoi
                            cJSON_AddNumberToObject(d, "menu_price",
                                r2[1] ? atoi(r2[1]) : 0);
                            cJSON_AddNumberToObject(d, "menu_cnt",
                                r2[2] ? atoi(r2[2]) : 0);

                            cJSON_AddStringToObject(d, "opt_name_all",
                                r2[3] ? r2[3] : "");

                            cJSON_AddNumberToObject(d, "opt_price_all",
                                r2[4] ? atoi(r2[4]) : 0);

                            cJSON_AddItemToArray(det, d);
                        }
                        mysql_free_result(res2);
                    }

                    cJSON_AddItemToArray(arr, o);
                }

                // 6) 정리 및 전송
                mysql_free_result(res1);
                mysql_close(conn);

                char* out = cJSON_PrintUnformatted(resp);
                string response_with_delim = string(out) + "\n<<END>>"; // 응답 끝 구분자 추가
                send(sock, response_with_delim.c_str(), response_with_delim.length(), 0);
                cout << "데이터전송성공\n";
                free(out);
                cJSON_Delete(resp);
            }
            else {
                // uid 누락 오류 처리
                cJSON* err = cJSON_CreateObject();
                cJSON_AddStringToObject(err, "action", "10_2");
                cJSON_AddStringToObject(err, "message", "유효하지 않은 uid");
                char* em = cJSON_PrintUnformatted(err);
                string err_with_delim = string(em) + "\n<<END>>"; // 구분자 추가
                send(sock, err_with_delim.c_str(), err_with_delim.length(), 0);
                free(em);
                cJSON_Delete(err);
            }
        }

        // 기본 주소 설정 요청
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "11_0") == 0) {
            cout << "기본 주소 요청\n";

            // JSON에서 값 추출
            const cJSON* uid = cJSON_GetObjectItem(root, "uid");                // 해당 아이디
            const cJSON* address_detail = cJSON_GetObjectItem(root, "address_detail");  // 기본주소로 설정할 상세주소

            if (cJSON_IsString(uid) && cJSON_IsString(address_detail)) {
                MYSQL* conn = mysql_init(NULL);
                if (!conn) {
                    perror("mysql_init 실패");
                    return;
                }

                if (!mysql_real_connect(conn, "10.10.20.99", "JIN", "1234", "WHAT", 3306, NULL, 0)) {
                    perror("DB 연결 실패");
                    mysql_close(conn);
                    return;
                }

                // 입력값 escape 처리
                char arr_uid[128], arr_address_detail[512];
                mysql_real_escape_string(conn, arr_uid, uid->valuestring, strlen(uid->valuestring));
                mysql_real_escape_string(conn, arr_address_detail, address_detail->valuestring, strlen(address_detail->valuestring));

                // 응답 객체 생성
                cJSON* response = cJSON_CreateObject();

                // 기존에 TYPE 1 인 걸 모두 0으로 초기화
                char reset_query[512];
                snprintf(reset_query, sizeof(reset_query),
                    "UPDATE USER_ADDRESS SET ADDRESS_BASIC = 0 WHERE uid = '%s'", arr_uid);

                if (mysql_query(conn, reset_query) != 0) {
                    cJSON_AddStringToObject(response, "action", "11_2");
                    cJSON_AddStringToObject(response, "result", "기존 기본주소 초기화 실패");
                    cout << "쿼리 실행 오류(초기화): " << mysql_error(conn) << endl;
                }
                else {
                    // 선택한 주소를 TYPE 1로 설정
                    char set_query[512];
                    snprintf(set_query, sizeof(set_query),
                        "UPDATE USER_ADDRESS SET ADDRESS_BASIC = 1 WHERE uid = '%s' AND ADDRESS_DETAIL = '%s'",
                        arr_uid, arr_address_detail);

                    if (mysql_query(conn, set_query) == 0) {
                        cJSON_AddStringToObject(response, "action", "11_1");
                        cJSON_AddStringToObject(response, "result", "기본 주소 설정 성공");
                        cout << "기본 주소 설정 성공\n";
                    }
                    else {
                        cJSON_AddStringToObject(response, "action", "11_2");
                        cJSON_AddStringToObject(response, "result", "기본 주소 설정 실패");
                        cout << "쿼리 실행 오류(설정): " << mysql_error(conn) << endl;
                    }
                }

                // 변경 후 주소 목록 다시 보내줌
                char select_query[512];
                snprintf(select_query, sizeof(select_query),
                    "SELECT ADDRESS_NAME, ADDRESS, ADDRESS_DETAIL, ADDRESS_BASIC, ADDRESS_TYPE FROM USER_ADDRESS WHERE uid = '%s'",
                    arr_uid);

                if (mysql_query(conn, select_query) == 0) {
                    MYSQL_RES* res = mysql_store_result(conn);
                    MYSQL_ROW row;
                    cJSON* address_array = cJSON_CreateArray();

                    while ((row = mysql_fetch_row(res))) {
                        cJSON* address_obj = cJSON_CreateObject();
                        cJSON_AddStringToObject(address_obj, "address_name", row[0] ? row[0] : "");
                        cJSON_AddStringToObject(address_obj, "address", row[1] ? row[1] : "");
                        cJSON_AddStringToObject(address_obj, "address_detail", row[2] ? row[2] : "");
                        cJSON_AddStringToObject(address_obj, "address_basic", row[3] ? row[3] : "");
                        cJSON_AddNumberToObject(address_obj, "address_type", row[4] ? atoi(row[4]) : 0);
                        cJSON_AddItemToArray(address_array, address_obj);
                    }

                    cJSON_AddItemToObject(response, "address_list", address_array);
                    mysql_free_result(res);
                    cout << "주소 조회 개수: " << cJSON_GetArraySize(address_array) << endl;
                }
                else {
                    cJSON_AddStringToObject(response, "address_list", "조회 실패");
                    cout << "주소 조회 실패: " << mysql_error(conn) << endl;
                }

                // 응답 전송 (끝에 \n<<END>> 붙임)
                char* response_str = cJSON_PrintUnformatted(response);
                string send_str = string(response_str) + "\n<<END>>";
                send(sock, send_str.c_str(), send_str.size(), 0);
                cout << "서버 -> 클라이언트: " << send_str << endl;

                free(response_str);
                cJSON_Delete(response);
                mysql_close(conn);
            }
        }

        // 기본 주소 설정 요청
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "12_0") == 0) {
            cout << "기본 주소 요청\n";
            
            //json요청에서 address 가져옴 존재안하면 에러
            const cJSON* address = cJSON_GetObjectItem(root, "address"); // 소문자로 유지
            if (!cJSON_IsString(address)) {
                cerr << "address가 존재하지 않거나 문자열이 아님\n";
                cJSON* resp = cJSON_CreateObject();
                cJSON_AddStringToObject(resp, "action", "12_2");
                cJSON_AddStringToObject(resp, "message", "주소 파라미터 오류");
                string err_str = cJSON_PrintUnformatted(resp);
                err_str += "\n<<END>>";
                send(sock, err_str.c_str(), err_str.length(), 0);
                cJSON_Delete(resp);
                continue;
            }

            // JSON 파일 열기
            ifstream file("owner_info.json");
            if (!file.is_open()) {
                cerr << "owner_info.json 파일 열기 실패\n";
                continue;
            }

            //파일 내용을 json_str 문자열로 읽음
            string json_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            file.close();

            //json 문자열 파싱해서 객체 생성
            cJSON* data = cJSON_Parse(json_str.c_str());
            if (!data) {
                cerr << "owner_info.json 파싱 실패\n";
                continue;
            }

            //address가 일치하는거 저장할 배열 만듬
            cJSON* matching_list = cJSON_CreateArray();

            int count = cJSON_GetArraySize(data);
            int num = 0;    //디버깅용 몇개가는지
            for (int i = 0; i < count; ++i) {
                cJSON* item = cJSON_GetArrayItem(data, i);
                cJSON* addr = cJSON_GetObjectItem(item, "ADDRESS");

                if (cJSON_IsString(addr) && strcmp(addr->valuestring, address->valuestring) == 0) {
                    cJSON_AddItemToArray(matching_list, cJSON_Duplicate(item, 1));
                }
                num++;
            }

            cJSON* resp = cJSON_CreateObject();
            cJSON_AddStringToObject(resp, "action", "12_1");
            cJSON_AddItemToObject(resp, "address_list", matching_list); 

            string response_str = cJSON_PrintUnformatted(resp);
            response_str += "\n<<END>>";

            cout << "보낸 가게 수 총" << num << "개" << endl;
            send(sock, response_str.c_str(), response_str.length(), 0);

            cJSON_Delete(resp);
            cJSON_Delete(data);
        }

        //////////////////////////////////////////// 0617 진영
        // 해당 가게 메뉴 요청
        else if (cJSON_IsString(action) && strcmp(action->valuestring, "13_0") == 0) {
            cout << "가게 메뉴 요청\n";

            const cJSON* brand_uid = cJSON_GetObjectItem(root, "brand_uid");
            if (!cJSON_IsString(brand_uid)) {
                cerr << "brand_uid가 없음 또는 문자열 아님\n";
                cJSON* err_resp = cJSON_CreateObject();
                cJSON_AddStringToObject(err_resp, "action", "13_2");
                cJSON_AddStringToObject(err_resp, "message", "brand_uid 누락 또는 잘못된 형식");
                string err_str = cJSON_PrintUnformatted(err_resp);
                err_str += "\n<<END>>";
                send(sock, err_str.c_str(), err_str.length(), 0);
                cJSON_Delete(err_resp);
                continue;
            }

            // brand_uid에 따라 열 파일 결정
            string filename;
            if (strcmp(brand_uid->valuestring, "1") == 0) {
                filename = "theventi.json";
            } 
            else if (strcmp(brand_uid->valuestring, "2") == 0) {
                filename = "bulk.json";
            } 
            else if (strcmp(brand_uid->valuestring, "6") == 0) {
                filename = "bhc.json";
            } 
            else {
                // 유효하지 않은 brand_uid 처리
                cerr << "유효하지 않은 brand_uid: " << brand_uid->valuestring << "\n";
                cJSON* err_resp = cJSON_CreateObject();
                cJSON_AddStringToObject(err_resp, "action", "13_2");
                cJSON_AddStringToObject(err_resp, "message", "존재하지 않는 브랜드 UID");
                string err_str = cJSON_PrintUnformatted(err_resp);
                err_str += "\n<<END>>";
                send(sock, err_str.c_str(), err_str.length(), 0);
                cJSON_Delete(err_resp);
                continue;
            }

            // 해당 JSON 파일 열기
            ifstream file(filename);
            if (!file.is_open()) {
                cerr << filename << " 파일 열기 실패\n";
                continue;
            }

            string json_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            file.close();

            cJSON* menu_data = cJSON_Parse(json_str.c_str());
            if (!menu_data) {
                cerr << "JSON 파싱 실패\n";
                continue;
            }

            // 일치하는 brand_uid의 메뉴들을 저장
            int num = 0;
            cJSON* menu_list = cJSON_CreateArray();
            int count = cJSON_GetArraySize(menu_data);
            for (int i = 0; i < count; ++i) {
                cJSON* item = cJSON_GetArrayItem(menu_data, i);
                cJSON* item_brand_uid = cJSON_GetObjectItem(item, "BRAND_UID");

                if (cJSON_IsString(item_brand_uid) && strcmp(item_brand_uid->valuestring, brand_uid->valuestring) == 0) {
                    cJSON_AddItemToArray(menu_list, cJSON_Duplicate(item, 1));
                }
                num++;
            }

            cJSON* resp = cJSON_CreateObject();
            cJSON_AddStringToObject(resp, "action", "13_1");
            cJSON_AddItemToObject(resp, "menu_list", menu_list);

            string response_str = cJSON_PrintUnformatted(resp);
            response_str += "\n<<END>>";

            cout << "메뉴 보내기 성공\n메뉴 총" << num << "개" << endl;
            send(sock, response_str.c_str(), response_str.length(), 0);

            cJSON_Delete(resp);
            cJSON_Delete(menu_data);
        }
////////////////////////////////////////


        
        cJSON_Delete(root);
    }
}


int main() {
    // 소켓 생성
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        cerr << "소켓 생성 실패\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 주소 설정 및 바인드
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERVER_PORT);

    //소켓을 지정된 주소/포트에 묶어줌
    if (bind(server_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "바인드 실패\n";
        return 1;
    }
    
    //클라이언트 연결을 대기상태로 만듬 최대 5 대기까지
    if (listen(server_fd, 5) < 0) {
        cerr << "리스닝 실패\n";
        return 1;
    }
    cout << "서버 실행 중... (포트: " << SERVER_PORT << ")\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
    
        int client_sock = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_sock >= 0) {
            // 클라이언트로부터 타입 정보 수신
            char type_buf[16] = {0};
            int len = recv(client_sock, type_buf, sizeof(type_buf) - 1, 0);  // 널 종료 위해 -1
    
            if (len <= 0) {
                close(client_sock);
                continue;
            }
    
            type_buf[len] = '\0';  // 명시적으로 널 종료
            cout << "클라이언트 수신: " << type_buf << std::endl;
    
            // 소켓을 동적 할당해서 개별 쓰레드에 안전하게 전달
            int* pclient = new int(client_sock);
    
            if (strcmp(type_buf, "user") == 0) {
                cout << "user로 잘 들어감." << endl;
                thread(user_handle_client, pclient).detach();
            } 
            else if (strcmp(type_buf, "owner") == 0) {
                cout << "owner로 잘 들어감." << endl;
                thread(owner_handle_client, pclient).detach();
            }
            else if (strcmp(type_buf, "rider") == 0) {
                cout << "rider 잘 들어감." << endl;
                thread(rider_handle_client, pclient).detach();
            } 
            else {
                cout << "알 수 없음: " << type_buf << std::endl;
                close(client_sock);
                delete pclient;
            }
        }
    }
    
    // close(server_fd);
    return 0;
}