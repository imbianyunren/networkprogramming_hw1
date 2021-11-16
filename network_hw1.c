#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/sendfile.h>

char web[8192] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                 "<!DOCTYPE html>\r\n"
                 "<html><head><title>midtern explosion</title></head>\r\n"
                 "<body><center><h1>蝦仁飯好吃</h1>"
                 "<img src=\"shrimp.jpeg\" alt=\"\" title=\"picture\" width=\"800px\">\r\n"
                 "<form method=\"post\" enctype=\"multipart/form-data\">\r\n"
                 "<p><input type=\"file\" name=\"upload\"></p>\r\n"
                 "<p><input type=\"submit\" value=\"submit\"></p>\r\n"
                 "</center>\r\n"
                 "</body></html>\r\n";

void sigchld(int signo)
{
    pid_t pid;

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        ;
}
void handler(int cli)
{
    char buf[65536];
    int img, len, tmpcnt = 0;
    FILE *f_inserv;
    char *ptr;
    char filename[128];


    memset(buf, 0, sizeof(buf));
    read(cli, buf, sizeof(buf));

    if (strncmp(buf, "GET /shrimp.jpeg", 16) == 0)
    {
    	//printf("%s",buf);
        img = open("shrimp.jpeg", O_RDONLY);
        sendfile(cli, img, NULL, 5000000);
        close(img);
    }
    else if (strncmp(buf, "POST /", 6) == 0)
    {
        ptr = strstr(buf, "filename");
        //printf("%s",buf);
        if (ptr)
        {
            len = strlen("filename");
            ptr += len + 2;
            while (*ptr != '\"')
            {
                filename[tmpcnt++] = *ptr++;
            }
            filename[tmpcnt] = '\0';

            while(!(*(ptr-4)=='\r' && *(ptr-3)=='\n' && *(ptr-2)=='\r' && *(ptr-1)=='\n')) ptr++; //文件開頭
            char *tail=buf+strlen(buf)-3;
				while(*tail!='\r') tail--; //文件結尾

            f_inserv = fopen(filename, "w");
            while(ptr!=tail){
                fputc(*ptr,f_inserv);
                ptr++;
            }
            fputc(*ptr,f_inserv);
            fclose(f_inserv);
            write(cli, web, sizeof(web));
        }
        else
            write(cli, web, sizeof(web));
    }
    else{
    	//printf("%s",buf);
        write(cli, web, sizeof(web));
        }
}
int main(void)
{
    int serv, cli;
    struct sockaddr_in server_addr;

    serv = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(serv, (struct sockaddr *)&server_addr, sizeof(server_addr));

    int listenfd = listen(serv, 10);

    signal(SIGCHLD, sigchld);

    while (1)
    {
        cli = accept(serv, NULL, NULL);

        pid_t child = fork();

        if (child == 0)
        {
            close(listenfd);
            handler(cli);
            exit(0);
        }
        else
        {
            close(cli);
        }
    }
    return 0;
}
