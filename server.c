/*
 *  UDP server
 */

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/*
 *  サーバ情報を格納する
 */
struct server_info {
    unsigned short sv_port;     /* サーバのポート番号 */

    int sd;                     /* ソケットディスクリプタ */
    struct sockaddr_in sv_addr; /* サーバのアドレス構造体 */
};
typedef struct server_info sv_info_t;

struct timespec ts;

/*!
 * @brief      受信したメッセージをそのままクライアントに返す
 * @param[in]  info   クライアント接続情報
 * @param[out] buff   受信したメッセージ
 * @param[out] errmsg エラーメッセージ格納先
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static int
udp_echo_process(sv_info_t *info, char *buff, char *errmsg)
{
    int rc = 0;
    int recv_msglen = 0;
    int ts_len = sizeof(struct timespec);
    struct sockaddr_in cl_addr = {0};
    unsigned int cl_addr_len = 0;

    char *temp;
    temp = (char *)malloc(2 * ts_len);

    /* クライアントからメッセージを受信するまでブロックする */
    cl_addr_len = sizeof(cl_addr);
    recv_msglen = recvfrom(info->sd, buff, BUFSIZ, 0,
                           (struct sockaddr *)&cl_addr, &cl_addr_len);
    if(recv_msglen < 0){
        sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
        return(-1);
    }

    memcpy(temp, buff, ts_len);
    // パケットを TCP で送信
    clock_gettime(CLOCK_REALTIME, &ts);
    memcpy(temp+ts_len, &ts, ts_len);

    /* 応答メッセージを送信する */
    rc = sendto(info->sd, temp, 2 * ts_len, 0, 
                (struct sockaddr *)&cl_addr, sizeof(cl_addr));
    if(rc != 2 * ts_len){
        /* sendto()が正しく送信できていないエラー */
        sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
        return(-1);
    }
    fprintf(stdout, "[client: %s]\n", inet_ntoa(cl_addr.sin_addr));

    return(0);
}

/*!
 * @brief      UDPメッセージを受信する
 * @param[in]  info   クライアント接続情報
 * @param[out] errmsg エラーメッセージ格納先
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static int
udp_echo_server(sv_info_t *info, char *errmsg)
{
    int rc = 0;
    int exit_flag = 1;
    char buff[BUFSIZ];

    while(exit_flag){
        rc = udp_echo_process(info, buff, errmsg);
        if(rc != 0) return(-1);

        /* 受信メッセージを標準出力する */
        // fprintf(stdout, "Received: %s\n", buff);

        /* クライアントからのサーバ終了命令を確認する */
        if(strcmp(buff, "terminate") == 0){
            exit_flag = 0;
        }
    }

    return( 0 );
}

/*!
 * @brief      ソケットの初期化
 * @param[in]  info   クライアント接続情報
 * @param[out] errmsg エラーメッセージ格納先
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static int
socket_initialize(sv_info_t *info, char *errmsg)
{
    int rc = 0;

    /* ソケットの生成 : UDPを指定する */
    info->sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(info->sd < 0){
        sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
        return(-1);
    }

    /* サーバのアドレス構造体を作成する */
    info->sv_addr.sin_family = AF_INET;
    info->sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    info->sv_addr.sin_port = htons(info->sv_port);

    /* ローカルアドレスへバインドする */
    rc = bind(info->sd, (struct sockaddr *)&(info->sv_addr),
              sizeof(info->sv_addr));
    if(rc != 0){
        sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
        return(-1);
    }

    return(0);
}

/*!
 * @brief      ソケットの終期化
 * @param[in]  info   クライアント接続情報
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static void
socket_finalize(sv_info_t *info)
{
    /* ソケット破棄 */
    if(info->sd != 0) close(info->sd);

    return;
}

/*!
 * @brief      UDPサーバ実行
 * @param[in]  info   クライアント接続情報
 * @param[out] errmsg エラーメッセージ格納先
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static int
udp_server(sv_info_t *info, char *errmsg)
{
    int rc = 0;

    /* ソケットの初期化 */
    rc = socket_initialize(info, errmsg);
    if(rc != 0) return(-1);

    /* 文字列を受信する */
    rc = udp_echo_server(info, errmsg);

    /* ソケットの終期化 */
    socket_finalize(info);

    return(rc);
}

/*!
 * @brief      初期化処理。待受ポート番号を設定する。
 * @param[in]  argc   コマンドライン引数の数
 * @param[in]  argv   コマンドライン引数
 * @param[out] info   サーバ情報
 * @param[out] errmsg エラーメッセージ格納先
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static int
initialize(int argc, char *argv[], sv_info_t *info, char *errmsg)
{
    /* if(argc != 2){
        sprintf(errmsg, "Usage: %s <port>\n", argv[0]);
        return(-1);
    } */

    memset(info, 0, sizeof(sv_info_t));
    info->sv_port = 2964;

    return(0);
}

/*!
 * @brief   main routine
 * @return  成功ならば0、失敗ならば-1を返す。
 */
int
main(int argc, char *argv[])
{
    int rc = 0;
    sv_info_t info = {0};
    char errmsg[256];

    rc = initialize(argc, argv, &info, errmsg);
    if(rc != 0){
        fprintf(stderr, "Error: %s\n", errmsg);
        return(-1);
    }

    rc = udp_server(&info, errmsg);
    if(rc != 0){
        fprintf(stderr, "Error: %s\n", errmsg);
        return(-1);
    }

    return(0);
}

