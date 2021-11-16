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

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0);
}
void handler(int cli)
{
    char buf[65536];
    int img, len, tmpcnt = 0;
    FILE *f_inserv; //在伺服器上建立的檔案
    char *ptr;
    char filename[128];


    memset(buf, 0, sizeof(buf));
    read(cli, buf, sizeof(buf)); //讀取socketfd內容

    if (strncmp(buf, "GET /shrimp.jpeg", 16) == 0)
    {//讀取圖片時要做的開檔步驟
    	//printf("%s",buf);
        img = open("shrimp.jpeg", O_RDONLY);
        sendfile(cli, img, NULL, 5000000);
        close(img);
    }
    else if (strncmp(buf, "POST /", 6) == 0)
    {//上傳檔案要做的內容
        ptr = strstr(buf, "filename");
        //printf("%s",buf);
        if (ptr)
        {
            len = strlen("filename");//buffer有一段是filename="xxx.txt"(txt是舉例)
            ptr += len + 2;//+2是因為="
            while (*ptr != '\"')
            {
                filename[tmpcnt++] = *ptr++;//讀到檔案名稱尾巴(")
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
    else{//基本上就是處理GET / 的要求，然後將網頁內容輸出到客戶端
    	//printf("%s",buf);
        write(cli, web, sizeof(web));
        }
}
int main(void)
{
    int serv, cli;
    struct sockaddr_in server_addr;
    
    //create socket
    serv = socket(AF_INET, SOCK_STREAM, 0);
    
    //連線設定
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //使用任何在本機的對外ip
    
    //網路監視器
    bind(serv, (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    //監聽
    int listenfd = listen(serv, 10);

    signal(SIGCHLD, sigchld);

    while (1)
    {
        //waiting for connection
        cli = accept(serv, NULL, NULL);
        //fork出子行程
        pid_t child = fork();

        if (child == 0)
        {//child
            close(listenfd);
            handler(cli);
            exit(0);
        }
        else
        {//parent
            close(cli);
        }
    }
    return 0;
}
