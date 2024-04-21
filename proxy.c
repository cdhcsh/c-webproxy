#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
        "Firefox/10.0.3\r\n";

void doit(int cliendfd);

void read_requesthdrs(rio_t *rp, char *buf);

void parse_uri(char *uri, char *host, char *port, char *ruri);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

void build_remote_requesthdrs(char *hdrs, char *method, char *host, char *port, char *ruri);

int request_remote_host(char *host, char *port, char *hdrs, char *buf);

void print_log(char *desc, char *text) {
    FILE *fp = fopen("output.log", "a");

    if (text[strlen(text) - 1] != '\n')
        fprintf(fp, "%s%s\n", desc, text);
    else
        fprintf(fp, "%s%s", desc, text);

    fclose(fp);
}
void sigchld_handler(int sig){
    while(waitpid(-1,0,WNOHANG) > 0)
        ;
    return;
}

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

    Signal(SIGCHLD,sigchld_handler);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr,
                        &clientlen);  // line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                    0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        int pid = Fork();
        if (pid < 0) {
            clienterror(connfd, "", "501", "Internal server error", "Internal server error");
        } else if (pid > 0) {
            Close(connfd);
            printf("Forked Child process [%d]\n", pid);
        } else { //자식 프로세스
            Close(listenfd);
            printf("\t- run\n");
            doit(connfd);   // line:netp:tiny:doit
            printf("\t- exit\n");
            exit(0);
        }
    }
}

void doit(int cliendfd) {
    char buf[MAX_OBJECT_SIZE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], port[MAXLINE], ruri[MAXLINE];
    char hdrs[MAXLINE];
    rio_t rio;
    int len;

    /* HTTP 요청 헤더 읽기 */
    Rio_readinitb(&rio, cliendfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strstr(uri, "favicon.ico") > 0) return;

    print_log("========== ClientRequest recieve ==========\n", buf);

    read_requesthdrs(&rio, buf);
    print_log("========== Client Request headers ==========\n", buf);


    parse_uri(uri, host, port, ruri);
    build_remote_requesthdrs(hdrs, method, host, port, ruri);

    print_log("========== Remote Request headers ==========\n", hdrs);

    print_log("Send Request to Remote...\n", "");
    if ((len = request_remote_host(host, port, hdrs, buf)) < 0) {
        print_log("Request send failed\n", "");
        clienterror(cliendfd, method, "502", "Bad Gateway", "Bad Gateway");
        return;
    }
    print_log("========== Remote response ==========\n", buf);
    Rio_writen(cliendfd, buf, len);

    Close(cliendfd);
}

int request_remote_host(char *host, char *port, char *hdrs, char *buf) {
    int remote_fd = open_clientfd(host, port);
    int len;
    if (remote_fd < 0) {
        return -1;
    }
    Rio_writen(remote_fd, hdrs, strlen(hdrs));
    len = Rio_readn(remote_fd, buf, MAX_OBJECT_SIZE);
    Close(remote_fd);
    return len;
}

void build_remote_requesthdrs(char *hdrs, char *method, char *host, char *port, char *ruri) {
    sprintf(hdrs, "%s /%s HTTP/1.0\r\n", method, ruri);
    sprintf(hdrs, "%sHost: %s:%s\r\n", hdrs, host, port);
    sprintf(hdrs, "%sConnection: close\r\n", hdrs);
    sprintf(hdrs, "%sProxy-a: close\r\n", hdrs);
    sprintf(hdrs, "%s%s\r\n", hdrs, user_agent_hdr);
}

/* 요청 헤더를 읽고 무시*/
void read_requesthdrs(rio_t *rp, char *output) {
    char buf[MAXLINE];

    // 출력 버퍼 초기화
    strcpy(output, "");

    Rio_readlineb(rp, buf, MAXLINE);

    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        strcat(output, buf);
    }
}

void parse_uri(char *uri, char *host, char *port, char *ruri) {

    char *p1, *p2;
    if ((p2 = strstr(uri, "http://")) > 0) {
        uri = p2 + 7;
    } else
        uri += 1;
    p1 = index(uri, ':');
    if (p1 > 0)
        *p1 = '\0';
    else {
        p1 = index(uri, '/');
        if (p1 > 0) *p1 = '\0';
    }
    strcpy(host, uri);
    p2 = index(p1 + 1, '/');
    if (p2 > 0) {
        *p2 = '\0';
        strcpy(ruri, ++p2);
    }
    strcpy(port, p1 + 1);
}

/* 에러 페이지 반환*/
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* HTTP response body 생성 */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* HTTP response 전송 */
    sprintf(buf, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type : text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "content-length: %d\r\n\r\n", (int) strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
