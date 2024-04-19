/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "../csapp.h"

int main(int argc, char **argv) {
    int cliendfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    }
    host = argv[1];
    port = argv[2];

    cliendfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, cliendfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(cliendfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(cliendfd);
    exit(0);
}
/* $end adder */
