#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <pthread.h>

struct arg_struct
{
    int s_port;
    int r_port;
    int process_no;
};

char * extractData(char* str, int n)
{
    int i=n+1,j=0,k=0;
    char* buff=malloc(sizeof(char)*100);
    while(1)
    {
        while(str[j]!='|')
        {
            if(str[j]=='\0')
                return "\0";
            buff[k++]=str[j++];
        }
        if(i==1)
        {
            buff[k]='\0';
            return buff;
        }
        k=0;
        buff[k]='\0';
        j++;
        i--;
    }
}


int extractToken( char * str)
{
    int i=0,token;
    char tokenbuff[100];
    while(str[i]!='~')
    {
        tokenbuff[i]=str[i];
        i++;
    }
    tokenbuff[i]='\0';
    return atoi(tokenbuff);
}

char* extractTokenlessData(char * str)
{
    int i=0,j=0;
    char* buff=malloc(sizeof(char)*1024);

    while(str[i++]!='~');

    while(str[i]!='|')
    {
        buff[j++]=str[i++];
    }
    buff[j]='\0';
    return buff;
}

void printData(char * str)
{
    int k=0,token;
    char* data=malloc(sizeof(char)*1024);    
    char* msg=malloc(sizeof(char)*1024);    
    while(1)
    {
        data=extractData(str,k);
        if(data[0]=='\0')
            return;
        token=extractToken(str);
        msg=extractTokenlessData(str);
        printf("Token Number: %d ; %s\n",token, msg);
        k=k+1;
    }
}

int app_send(void*  arguments)
{
    int sockfd = 0, n = 0;
    char sendBuff[1024];
    struct sockaddr_in serv_addr; 
    struct arg_struct *args=arguments;
    int port= args->s_port;
    memset(sendBuff, '0',sizeof(sendBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr)); 
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);  //Broadcast Server Port

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 
    while(1)
    {
        scanf("%d",&n);
        if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\n Error : Connect Failed \n");
            return 1;
        }

        snprintf(sendBuff, sizeof(sendBuff), "Message From Process: %d",args->process_no); 
        write(sockfd, sendBuff, strlen(sendBuff)); 
        close(sockfd);
    }


}


int app_receive(void* arguments)
{
    int listenfd=0, connfd=0,n;
    char recvBuff[1025];
    struct sockaddr_in serv_addr; 
    struct arg_struct *args=arguments;
    int port= args->r_port;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(recvBuff, '0', sizeof(recvBuff)); 
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    listen(listenfd, 10); 


    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        printf("HUHUHU\n");
        n = read(connfd, recvBuff, sizeof(recvBuff)-1);
        recvBuff[n] = 0;
        printf("HAHAHA %s\n",recvBuff);
        printData(recvBuff);               
        close(connfd);
    }

}

int main(int argc, char* argv[])
{
	pthread_t sid,rid;
    void * res;
    struct arg_struct args;
    args.s_port=atoi(argv[1]);
    args.r_port=atoi(argv[2]);
    args.process_no=atoi(argv[3]);

    pthread_create(&sid, NULL, (void*)&app_send, (void *)&args);
    pthread_create(&rid, NULL, (void*)&app_receive, (void *)&args);
    pthread_join(sid,&res);
    pthread_join(rid,&res);
    return 0;
}