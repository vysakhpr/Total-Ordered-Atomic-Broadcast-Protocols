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


struct arg_struct
{
    char* token_self_ip;
    int token_self_port;
    char* token_sender_ip;
    int token_sender_port;
    char* broadcast_ip;
    int broadcast_port;
    int application_sender_port;
    int application_receiver_port;
    int is_starter;
};


char * writeData(char * data , int token)
{
    int i=0,j=0;
    char* buff=malloc(sizeof(char)*1024);
    char tokenbuff[100];
    
    snprintf(tokenbuff, sizeof(tokenbuff),"%d" ,token);    
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


void token_ring(void * arguments)
{
    struct arg_struct *args=arguments;
    int listenfd = 0, connfd = 0, token_listenfd=0,token_connfd=0,n,token=0,i=0,token_sendfd=0,broadcastfd=0;
    int broadcast=1;
    struct sockaddr_in serv_addr, broadcast_serv_addr, token_recv_addr, broadcast_addr; 
    char recvBuff[1024],sendBuff[1024];
    char * tokenBuff=malloc(sizeof(char)*1025); 
    listenfd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
    token_listenfd = socket(AF_INET, SOCK_STREAM , 0);
    broadcastfd=socket(AF_INET,SOCK_DGRAM,0);
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(&broadcast_serv_addr, '0', sizeof(broadcast_serv_addr));
    memset(&broadcast_addr,'0',sizeof(broadcast_addr));
    


    memset(recvBuff, '0', sizeof(recvBuff)); 
    memset(tokenBuff, '0', sizeof(tokenBuff));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(args->application_sender_port); 

    broadcast_addr.sin_family=AF_INET;
    broadcast_addr.sin_port=htons(args->broadcast_port);
    inet_pton(AF_INET,args->broadcast_ip,&broadcast_addr.sin_addr);
    memset(broadcast_addr.sin_zero,'\0',sizeof(broadcast_addr.sin_zero));


    broadcast_serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, args->token_self_ip, &broadcast_serv_addr.sin_addr);
    broadcast_serv_addr.sin_port = htons(args->token_self_port); 

    token_recv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, args->token_sender_ip, &token_recv_addr.sin_addr);
    token_recv_addr.sin_port = htons(args->token_sender_port);

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        printf("ERROR\n");     
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT,&(int){ 1 }, sizeof(int));
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    bind(token_listenfd, (struct sockaddr*)&broadcast_serv_addr, sizeof(broadcast_serv_addr)); 
    listen(token_listenfd,10);



    while(1)
    {
        n=recv(listenfd,recvBuff,sizeof(recvBuff)-1,0);
        if(n == -1)
            recvBuff[0]='\0';
        else
        {
            recvBuff[n] = 0;
            printf("%s",recvBuff);               
    
        }
        close(connfd);
       
        if(args->is_starter == 1 && i==0)
        {
            token=0;
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
            token=atoi(tokenBuff);
            if(recvBuff[0]!='\0')
                token=token+1;
        }


        if(recvBuff[0]!='\0')
        {
            if(setsockopt(broadcastfd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof(broadcast))==-1)
                printf("ERROR\n");
            tokenBuff=writeData(recvBuff,token);
            snprintf(sendBuff, sizeof(sendBuff),"%s" ,tokenBuff);
            n=sendto(broadcastfd, sendBuff, strlen(sendBuff), 0,(struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
        }


        token_sendfd = socket(AF_INET, SOCK_STREAM, 0);
        connect(token_sendfd, (struct sockaddr *)&token_recv_addr, sizeof(token_recv_addr));
        snprintf(sendBuff, sizeof(sendBuff),"%d" ,token);
        printf("%s\n",sendBuff);
        write(token_sendfd, sendBuff, strlen(sendBuff));  
        close(token_sendfd);


         sleep(1);
    }

}


void receive_stability(void * arguments)
{
    clock_t start,end;
    double tim;
    int listenfd=0, connfd=0,n,i=0;
    char recvBuff[1025];
    struct sockaddr_in serv_addr; 
    struct arg_struct *args=arguments;
    int port= 9000;
    int set_option_on=1;
    listenfd = socket(AF_INET, SOCK_DGRAM, 0);


    memset(recvBuff, '0', sizeof(recvBuff)); 
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*) &set_option_on, sizeof(set_option_on))==-1)
        printf("ERROR\n");
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    while(1)
    {
        n=recv(listenfd,recvBuff,sizeof(recvBuff)-1,0);
        if (n>0){
            if(i==0)
            {
                start=clock();
            }
            i++;
        }

        if(i==1)
        {
            end=clock();
            tim=((double)(end-start))/CLOCKS_PER_SEC;
            printf("Time For Stability :%f\n", tim);
            i=0;
        }
    }
}

void receive_broadcast(void * arguments)
{
    int listenfd=0,app_fd=0, connfd=0,n,broadcast=1,stab_fd=0;
    char recvBuff[1025],sendBuff[1025],stabBuff[1025];
    char * appBuff=malloc(sizeof(char)*1025);
    struct sockaddr_in serv_addr,app_addr,stab_addr; 
    struct arg_struct *args=arguments;
    int port= args->broadcast_port;
    int set_option_on=1;
    int len;
    listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    app_fd = socket(AF_INET, SOCK_DGRAM , 0);


    memset(stabBuff, '0', sizeof(stabBuff)); 
    memset(recvBuff, '0', sizeof(recvBuff)); 
    memset(sendBuff, '0', sizeof(sendBuff)); 
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(&app_addr, '0', sizeof(serv_addr)); 
    memset(&stab_addr, '0', sizeof(serv_addr)); 


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    app_addr.sin_family = AF_INET;
    app_addr.sin_port = htons(args->application_receiver_port);  //Broadcast Server Port
    inet_pton(AF_INET, "127.255.255.255", &app_addr.sin_addr);
    memset(app_addr.sin_zero, '\0', sizeof (app_addr.sin_zero));


    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*) &set_option_on, sizeof(set_option_on))==-1)
        printf("ERROR\n");
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    while(1)
    {
        len=sizeof(stab_addr);
        n=recvfrom(listenfd,recvBuff,sizeof(recvBuff)-1,0,(struct sockaddr *)&stab_addr,(socklen_t *)&len);
        if (n>0){
            recvBuff[n] = 0;
            snprintf(sendBuff, sizeof(sendBuff),"1");
            stab_addr.sin_port=htons(9000);
            n=sendto(stab_fd, sendBuff, strlen(sendBuff), 0,(struct sockaddr *)&stab_addr, sizeof(stab_addr));               
            if(setsockopt(app_fd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof(broadcast))==-1)
                printf("ERROR\n");
            snprintf(sendBuff, sizeof(sendBuff),"%s" ,recvBuff);
            n=sendto(app_fd, sendBuff, strlen(sendBuff), 0,(struct sockaddr *)&app_addr, sizeof(app_addr));   


        }
    }
}


int main(int argc, char *argv[])
{	
	if(argc!=9 && argc!=10)
	{
		printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
	}


    struct arg_struct args;
    pthread_t sid,rid,kid;

    args.token_self_ip=argv[1];
    args.token_self_port=atoi(argv[2]);
    args.token_sender_ip=argv[3];
    args.token_sender_port=atoi(argv[4]);
    args.broadcast_ip=argv[5];
    args.broadcast_port=atoi(argv[6]);
    args.application_sender_port=atoi(argv[7]);
    args.application_receiver_port=atoi(argv[8]);
    if(argc==10)
        args.is_starter=1;
    else
        args.is_starter=0;


    pthread_create(&sid, NULL, (void*)&token_ring, (void *)&args);
    pthread_create(&rid, NULL, (void*)&receive_broadcast, (void *)&args);
    //pthread_create(&kid, NULL, (void*)&receive_stability, (void *)&args);
    pthread_join(sid,NULL);
    pthread_join(rid,NULL);
    //pthread_join(kid,NULL);
    

	return 0;
}