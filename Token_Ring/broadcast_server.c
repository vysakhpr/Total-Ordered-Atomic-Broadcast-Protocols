#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

clock_t start,end;
double tim;


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

char * writeData(char * msg, char * data , int token)
{
    int i=0,j=0;
    char* buff=malloc(sizeof(char)*1024);
    char tokenbuff[100];
    
    snprintf(tokenbuff, sizeof(tokenbuff),"%d" ,token);    
    while(msg[i]!='\0')
    {
        buff[j++]=msg[i++];
    }
    i=0;
    if(token!=-1)
    {
        while(tokenbuff[i]!='\0')
        {
            buff[j++]=tokenbuff[i++];
        }
        buff[j++]='~';
        i=0;
    }
    while(data[i]!='\0')
    {
        buff[j++]=data[i++];
    }
    buff[j++]='|';
    buff[j]='\0';
    return buff;
}

char * attachData(char * msg,char * current_msg ,int current_token, int previous_token)
{
    int i=0,j=0,k=1,t;
    char* buff=malloc(sizeof(char)*1024);
    char tokenbuff[100];
    char* data;
    snprintf(tokenbuff, sizeof(tokenbuff),"%d" ,current_token);    
    while(tokenbuff[i]!='\0')
    {
        buff[j++]=tokenbuff[i++];
    }
    buff[j++]='|';
    buff[j]='\0';
    if(current_msg[0]!='\0')
        buff=writeData(buff,current_msg,current_token);
    while(1)
    {
        data=extractData(msg,k);
        if(data[0]=='\0')
        {
            break;
        }
        t=extractToken(data);
        if(t<=previous_token)
        {
            if(previous_token!=0)
            {
                end=clock();
                tim=((double) (end - start)) / CLOCKS_PER_SEC;;
                printf("Time For Stability :%f\n", tim);
            }
            break;
        }
        buff=writeData(buff,data,-1);
        k++;
    }
    return buff;
}


char * getData(char * msg, int previous_token)
{
    int i=0,j=0,k=1,t;
    char* buff=malloc(sizeof(char)*1024);
    char* data;
    buff[0]='\0';
    while(1)
    {
        data=extractData(msg,k);
        if(data[0]=='\0')
        {
            break;
        }
        t=extractToken(data);
        if(t<previous_token)
        {

            break;
        }
        buff=writeData(buff,data,-1);
        k++;
    }
    return buff;
}



int main(int argc, char *argv[])
{	
	if(argc!=7 && argc!=8)
	{
		printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
	}

	int listenfd = 0, connfd = 0, token_listenfd=0,token_connfd=0,n,token=0,i=0,token_sendfd=0,previous_token,app_fd=0;
    fd_set fd;
    struct timeval tv;
    int broadcast=1;
    struct sockaddr_in serv_addr, broadcast_serv_addr, token_recv_addr, app_addr; 
    char recvBuff[1024],sendBuff[1024];
    char * tokenBuff=malloc(sizeof(char)*1025); 
    char * appBuff=malloc(sizeof(char)*1025);
    listenfd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
    token_listenfd = socket(AF_INET, SOCK_STREAM , 0);
    app_fd = socket(AF_INET, SOCK_DGRAM , 0);
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(&broadcast_serv_addr, '0', sizeof(broadcast_serv_addr));
    memset(recvBuff, '0', sizeof(recvBuff)); 
    memset(tokenBuff, '0', sizeof(tokenBuff));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[5])); 

    memset(&app_addr, '0', sizeof(serv_addr)); 
    app_addr.sin_family = AF_INET;
    app_addr.sin_port = htons(atoi(argv[6]));  //Broadcast Server Port
    inet_pton(AF_INET, "127.255.255.255", &app_addr.sin_addr);
    memset(app_addr.sin_zero, '\0', sizeof (app_addr.sin_zero));

    broadcast_serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &broadcast_serv_addr.sin_addr);
    broadcast_serv_addr.sin_port = htons(atoi(argv[2])); 
	token_recv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[3], &token_recv_addr.sin_addr);
    token_recv_addr.sin_port = htons(atoi(argv[4]));
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        printf("ERROR\n");     
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT,&(int){ 1 }, sizeof(int));
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    bind(token_listenfd, (struct sockaddr*)&broadcast_serv_addr, sizeof(broadcast_serv_addr)); 
    //listen(listenfd, 10); 
    listen(token_listenfd,10);



    while(1)
    {

                   //Receive Message from Application
        //connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        n=recv(listenfd,recvBuff,sizeof(recvBuff)-1,0);
        if(n == -1)
        {
            //printf("HAHAHA\n");
        	recvBuff[0]='\0';
        }
        else
        {
        	//n = read(connfd, recvBuff, sizeof(recvBuff)-1);
            //printf("HUHUHU\n");
        	recvBuff[n] = 0;
        	printf("%s",recvBuff);               
    
    	}
        close(connfd);


                
        //Receive Token Data from Broadcast Servers
    	if(argc == 8 && i==0)
    	{

    		token=0;
            previous_token=0;
            tokenBuff[0]='\0';
    		i=1;
    	}
    	else
    	{

    		token_connfd = accept(token_listenfd, (struct sockaddr*)NULL, NULL); 
        	n = read(token_connfd, tokenBuff, sizeof(char)*1025);
            //printf("n=%d\n", n);
        	close(token_connfd);
            tokenBuff[n] = '\0';
            token=atoi(extractData(tokenBuff,0));
            if(recvBuff[0]!='\0')
                token=token+1;
        }

        //printf("Received Data: %s\n",tokenBuff);

        //Process TokenData and Message to be send

        //printf("%s\n",tokenBuff);
        appBuff=getData(tokenBuff,previous_token);

        tokenBuff=attachData(tokenBuff,recvBuff,token,previous_token);


        
        printf("%s\n",appBuff);
    
        if(recvBuff[0]!='\0')
        {
            previous_token=token;
            start=clock();
        }

        

        //Send TokenData

        token_sendfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(token_sendfd, (struct sockaddr *)&token_recv_addr, sizeof(token_recv_addr));
        snprintf(sendBuff, sizeof(sendBuff),"%s" ,tokenBuff);
        
        write(token_sendfd, sendBuff, strlen(sendBuff));  
        close(token_sendfd);

        
        
        if(appBuff[0]!='\0')
        {
            // connect(app_fd, (struct sockaddr *)&app_addr, sizeof(app_addr));
            // snprintf(sendBuff, sizeof(sendBuff),"%s" ,appBuff);
            // printf("%s",sendBuff);
            // write(app_fd, sendBuff, strlen(sendBuff)); 
            // printf("%s",sendBuff);
            // close(app_fd);
            if(setsockopt(app_fd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof(broadcast))==-1)
                printf("ERROR\n");

            snprintf(sendBuff, sizeof(sendBuff),"%s" ,appBuff);
            n=sendto(app_fd, sendBuff, strlen(sendBuff), 0,(struct sockaddr *)&app_addr, sizeof(app_addr));
        }

         sleep(1);
     }
    return 0;
}