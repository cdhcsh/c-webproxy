/* ===================================================================================
 * ================================= DEBUG SETTINGS  =================================
 * =================================================================================== */

/* 출력 기록 별도 파일에 저장 여부 */
#define _LOG_

#ifdef _LOG_
#define LOG_FILE_NAME "output.log"
#endif

/* ===================================================================================
 * ================================== DEFINE VALUES  =================================
 * =================================================================================== */

#include "csapp.h"
#include "cache.h"

/* 서버로부터의 결과값 MAX */
#define MAX_OBJECT_SIZE 102400
/* User-Agent 헤더 값 정의 */
static const char *user_agent_hdr =
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
        "Firefox/10.0.3\r\n";

/* ===================================================================================
 * ============================== FUNCTION PROTOTYPES ================================
 * =================================================================================== */

/* 처리 내역 출력 */
void print_log(const char *desc, const char *test);

/* 프록시 서비스 시작 */
void proxy_service(int fd);

/* 쓰레드 액션 */
void *thread_action(void *argp);

/* 프록시 서비스 실행 */
void process(int clientfd);

/* 클라이언트 요청 헤더 분리 */
void parse_request_hdr(rio_t *rio, char *method, char *uri, char *version);

/* 클라이언트 요청 헤더 읽기 */
void read_request_hdrs(rio_t *rp);

/* 클라이언트 요청 uri 분리 */
void parse_request_uri(char *uri, char *host, char *port, char *path);

/* 최종 서버 요청 헤더 생성 */
void build_server_request_hdrs(const char *method, const char *host, const char *port, const char *path, char *hdrs);

/* 클라이언트에게 최종 서버의 처리 결과 전송 */
void response_to_client(int clientfd, const char *host, const char *port, const char *hdrs);

/* 최종 서버 결과 반환 */
ssize_t get_response_from_server(const char *host, const char *port, const char *hdrs, char *response);

/* 최종 서버 요청 및 결과 저장 */
ssize_t request_to_server(const char *host, const char *port, const char *hdrs, char *response);

/* 에러 페이지 반환 */
void response_error(int fd, const char *cause, const char *err_num, const char *short_msg, const char *long_msg);

/* ===================================================================================
 * ============================== EXTERN VARIABLES ===================================
 * =================================================================================== */

cache_t *cache; // 캐쉬 구조체 선언
pthread_mutex_t cache_mutax; // 공유자원 잠금

/* ===================================================================================
 * =============================== MAIN FUNCTION =====================================
 * =================================================================================== */

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    cache = new_cache();
    pthread_mutex_init(&cache_mutax, NULL);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr,
                        &clientlen);  // line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                    0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        proxy_service(connfd);
    }
}


/* ===================================================================================
 * ================================== FUNCTIONS ======================================
 * =================================================================================== */

/* 처리 내역 출력 */
void print_log(const char *desc, const char *text) {
#ifdef _LOG_
    FILE *fp = fopen(LOG_FILE_NAME, "a");
    if (text[strlen(text) - 1] != '\n')
        fprintf(fp, "%s%s\n", desc, text);
    else
        fprintf(fp, "%s%s", desc, text);
    fclose(fp);
#else
    if (text[strlen(text) - 1] != '\n')
        printf("%s%s\n", desc, text);
    else
        printf("%s%s", desc, text);
#endif
}

/* 프록시 서비스 시작 */
void proxy_service(int fd) {
    pthread_t tid;
    void *argp = (void *) malloc(sizeof(int));
    *(int *) argp = fd;
    pthread_create(&tid, NULL, thread_action, argp);
}

/* 쓰레드 액션 */
void *thread_action(void *argp) {
    pthread_detach(pthread_self());
    int fd = *(int *) argp;
    process(fd);
    free(argp);
    Close(fd);
    return NULL;
}

/* 프록시 서비스 실행 */
void process(int clientfd) {
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], port[MAXLINE], path[MAXLINE];
    char hdrs[MAXLINE];
    rio_t rio;

    signal(SIGPIPE, SIG_IGN);  // Broken Pipe 방지
    Rio_readinitb(&rio, clientfd); // rio 초기화
    parse_request_hdr(&rio, method, uri, version); // 클라이언트 요청 헤더에서 method,uri,version 분리
    if (strstr(uri, "favicon.ico") > 0) return; // 브라우저로 접속시 예외 처리
    read_request_hdrs(&rio); // 클라이언트 요청 헤더 읽기
    parse_request_uri(uri, host, port, path); // 클라이언트 요청 uri에서 host,port,path 분리
    build_server_request_hdrs(method, host, port, path, hdrs); // 최종 서버에 요청할 헤더 생성
    response_to_client(clientfd, host, port, hdrs); // 최종 서버에 요청 및 결과를 클라이언트에 전송
}

/* 클라이언트 요청 헤더 분리*/
void parse_request_hdr(rio_t *rio, char *method, char *uri, char *version) {
    char buf[MAXLINE];
    Rio_readlineb(rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    print_log("========== ClientRequest recieve ==========\n", buf);
}

/* 클라이언트 요청 헤더 읽기 */
void read_request_hdrs(rio_t *rp) {
    char buf[MAXLINE];
    // 저장위치 초기화
    Rio_readlineb(rp, buf, MAXLINE);
    print_log("========== Client Request headers ==========\n", buf);
}

/* 클라이언트 요청 uri 분리 */
void parse_request_uri(char *uri, char *host, char *port, char *path) {
    char *host_p, *port_p, *path_p;
    host_p = strstr(uri, "://") > 0 ? (uri + 7) : (uri + 1); // Http:// 건너뛰기
    port_p = strchr(host_p, ':');
    path_p = strchr(host_p, '/');
    if (path_p != NULL) { // path가 있다면
        *path_p = '\0';
        strcpy(path, path_p + 1); //path 저장
    } else
        strcpy(path, ""); // port가 있다면
    if (port_p != NULL) {
        *port_p = '\0';
        strcpy(port, port_p + 1); //port 저장
    } else
        strcpy(port, "80"); // 기본 http 포트
    host = strcpy(host, host_p); // hostname 저장
}

/* 최종 서버 요청 헤더 생성*/
void build_server_request_hdrs(const char *method, const char *host,
                               const char *port, const char *path, char *hdrs) {
    sprintf(hdrs, "%s /%s HTTP/1.0\r\n", method, path);
    sprintf(hdrs, "%sHost: %s:%s\r\n", hdrs, host, port);
    sprintf(hdrs, "%sConnection: close\r\n", hdrs);
    sprintf(hdrs, "%sProxy-Connection: close\r\n", hdrs);
    sprintf(hdrs, "%s%s\r\n", hdrs, user_agent_hdr);
    print_log("========== Remote Request headers ==========\n", hdrs);
}

/* 클라이언트에게 서버의 요청 결과 전송*/
void response_to_client(int clientfd, const char *host, const char *port, const char *hdrs) {
    char response[MAX_OBJECT_SIZE];
    ssize_t len;
    pthread_mutex_lock(&cache_mutax);
    if ((len = get_response_from_server(host, port, hdrs, response)) > 0) {
        Rio_writen(clientfd, response, len);
    } else {
        print_log("Request send failed\n", "");
        response_error(clientfd, "GET", "502", "Bad Gateway", "Bad Gateway");
    }
    pthread_mutex_unlock(&cache_mutax);
}

/* 최종 서버 결과 반환*/
ssize_t get_response_from_server(const char *host, const char *port, const char *hdrs, char *response) {
    node_t *node;
    char *cache_key, *cache_value;
    ssize_t len;
    node = cache_get(cache, hdrs);
    if (node != NULL) { // 캐싱
        memcpy(response, node->value, node->value_len);
        return node->value_len;
    } else {
        print_log("Send Request to Remote...\n", "");
        len = request_to_server(host, port, hdrs, response); // 최종 서버에게 요청 및 결과를 buf 저장
        if (len < 0) { // 최종 서버 연결 실패
            return -1;
        }

        cache_key = (char *) malloc(MAXLINE);
        memcpy(cache_key, hdrs, MAXLINE);
        cache_value = (char *) malloc(len);
        memcpy(cache_value, response, len);
        cache_add(cache, cache_key, cache_value, len); // 캐쉬 저장

        return len;
    }
}

/* 최종 서버 요청 및 결과 저장*/
ssize_t request_to_server(const char *host, const char *port, const char *hdrs, char *response) {
    int remote_fd = Open_clientfd(host, port); // 최종 서버와 연결
    ssize_t len;
    if (remote_fd < 0) { // 연결 실패
        return -1;
    }
    Rio_writen(remote_fd, hdrs, strlen(hdrs)); // 최종 서버에 헤더 전송
    len = Rio_readn(remote_fd, response, MAX_OBJECT_SIZE); // 최종 서버에서 결과를 받아서 저장
    Close(remote_fd); // 연결 닫기
    return len; // 결과 길이 반환
}

/* 에러 페이지 반환*/
void response_error(int fd, const char *cause, const char *err_num, const char *short_msg, const char *long_msg) {
    char buf[MAXLINE], body[MAXBUF];

    /* HTTP response body 생성 */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, err_num, short_msg);
    sprintf(body, "%s<p>%s: %s\r\n", body, long_msg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    /* HTTP response 전송 */
    sprintf(buf, "HTTP/1.1 %s %s\r\n", err_num, short_msg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type : text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "content-length: %d\r\n\r\n", (int) strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
