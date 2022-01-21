#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <net/if.h>
#include <sys/ioctl.h>

unsigned long long int charToInt(int bytes, ...);
struct ifreq ifr;

int main(int argc, char** argv)
{
    int sd;
    int acc_sd;
    struct sockaddr_in addr;
    struct timespec ts;
    struct timespec sync_ts;

    socklen_t sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in from_addr;
 
    unsigned char buf[2048];
 
   // 受信バッファの初期化
    memset(buf, 0, sizeof(buf));
 
    // IPv4 TCP のソケットを作成
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    strncpy(ifr.ifr_name, "enp1s0", IFNAMSIZ-1);
    ioctl(sd, SIOCGIFHWADDR, &ifr);
 
    // 待ち受けるIPとポート番号を設定
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2964);
    addr.sin_addr.s_addr = INADDR_ANY;
 
    // バインドする
    if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }
    
    while(1) {
        // パケット受信待ち状態とする
        // 待ちうけキューを１０としている
        if(listen(sd, 10) < 0) {
            perror("listen");
            return -1;
        }
    
        // クライアントからコネクト要求が来るまで停止する
        // 以降、サーバ側は acc_sd を使ってパケットの送受信を行う
        if((acc_sd = accept(sd, (struct sockaddr *)&from_addr, &sin_size)) < 0) {
            perror("accept");
            return -1;
        }
  
        // パケット受信。パケットが到着するまでブロック
        if(recv(acc_sd, buf, sizeof(buf), 0) < 0) {
            perror("recv");
            return -1;
        }
        
        char *temp;
        temp = (char *)malloc(8 * sizeof(char));
        // パケットを TCP で送信
        clock_gettime(CLOCK_REALTIME, &ts);
        for (int i = 3; i >= 0; i--) temp[(3-i)] = (unsigned char)(ts.tv_sec >> (i * 8)) % 256;
        for (int i = 3; i >= 0; i--) temp[4+(3-i)] = (unsigned char)(ts.tv_nsec >> (i * 8)) % 256;

        if(send(acc_sd, temp, 8, 0) < 0) {
            perror("send");
            return -1;
        }
   
        sync_ts.tv_sec = charToInt(4, buf[0], buf[1], buf[2], buf[3]);
        sync_ts.tv_nsec = charToInt(4, buf[4], buf[5], buf[6], buf[7]);
        // 出力
        printf("Received:\ttv_sec=%ld  tv_nsec=%ld\n",sync_ts.tv_sec,sync_ts.tv_nsec);
        printf("My time:\ttv_sec=%ld  tv_nsec=%ld\n",ts.tv_sec,ts.tv_nsec);
        // 受信データの出力
        // printf("%s\n", buf);
    }
    return 0;
}

unsigned long long int charToInt(int bytes, ...) {
	va_list list;
	unsigned long long int temp = 0; 

	va_start(list, bytes);
	for(int i = 0; i < bytes; i++) {
		temp += va_arg(list, int);
		if (bytes - i > 1) temp = temp << 8;
	}
	va_end(list);

	return temp;
}
