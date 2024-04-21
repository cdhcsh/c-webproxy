#include "csapp.h"
#include "cache.h"


/* Recommended max cache and object sizes */
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
        "Firefox/10.0.3\r\n";

const void print_log(char *desc, char *test);

void proxy_service(int clientfd);

void *thread_action(void *vargp);

void doit(int cliendfd);

void read_requesthdrs(rio_t *rp, char *buf);

void parse_uri(char *uri, char *host, char *port, char *path);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

void build_remote_requesthdrs(char *hdrs, char *method, char *host, char *port, char *ruri);

int request_remote_host(char *host, char *port, char *hdrs, char *buf);

/* ===================================================================================
 * ==================================== MAIN FUNCTION ====================================
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
    cache_init();

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
 * ==================================== FUNCTIONS ====================================
 * =================================================================================== */


const void print_log(char *desc, char *text) {
    FILE *fp = fopen("output.log", "a");

    if (text[strlen(text) - 1] != '\n')
        fprintf(fp, "%s%s\n", desc, text);
    else
        fprintf(fp, "%s%s", desc, text);

    fclose(fp);
}

void proxy_service(int clientfd) {
    pthread_t tid;
    void *argp = (void *) malloc(sizeof(int));
    *(int *) argp = clientfd;
    pthread_create(&tid, NULL, thread_action, argp);
}

void *thread_action(void *vargp) {
    pthread_detach(pthread_self());
    int connfd = *(int *) vargp;
    doit(connfd);
    free(vargp);
    Close(connfd);
    return NULL;
}

void doit(int cliendfd) {
    char buf[MAX_OBJECT_SIZE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], port[MAXLINE], ruri[MAXLINE];
    char hdrs[MAXLINE];
    rio_t rio;
    ssize_t len;

    signal(SIGPIPE, SIG_IGN);  // Broken Pipe 방지
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

    char *cache_key,*cache_value;
    node_t *node;
    node = cache_get(hdrs);
    if(node > 0){
        print_log("Caching~\n","");
        sprintf(port,"%d",node->value_len);
        print_log("size: ",port);
        print_log("value : \n", node->value);
        Rio_writen(cliendfd, node->value, node->value_len);
    }
    else {
        print_log("Send Request to Remote...\n", "");
        if ((len = request_remote_host(host, port, hdrs, buf)) < 0) {
            print_log("Request send failed\n", "");
            clienterror(cliendfd, method, "502", "Bad Gateway", "Bad Gateway");
            return;
        }
        print_log("========== Remote response ==========\n", buf);

        cache_key = (char *) malloc(MAXLINE);
        memcpy(cache_key,hdrs,MAXLINE);
        cache_value = (char *) malloc(len);
        memcpy(cache_value,buf,len);
        cache_add(cache_key,cache_value,len);

        Rio_writen(cliendfd, buf, len);
    }
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
    sprintf(hdrs, "%sProxy-Connection: close\r\n", hdrs);
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

void parse_uri(char *uri, char *host, char *port, char *path) {
    char *host_p, *port_p, *path_p;
    host_p = strstr(uri, "://") > 0 ? (uri + 7) : (uri + 1);
    port_p = strstr(host_p, ":");
    path_p = strstr(host_p, "/");
    if (path_p > 0) {
        *path_p = '\0';
        strcpy(path, path_p + 1);
    } else
        strcpy(path, "/");
    if (port_p > 0) {
        *port_p = '\0';
        strcpy(port, port_p + 1);
    } else
        strcpy(port, "80");
    host = strcpy(host, host_p);
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
