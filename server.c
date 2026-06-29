/*
-----------------------------------------------------------------------------------------------
MAIN :
 $ fd creation
 $ bind to local port
 $ listen
-----------------------------------------------------------------------------------------------
POLL :
 1.server FD
     $ accept
 2.client FDs
     $ Handle POLLIN,POLLHUP,POLLERR.
           if POLLIN --> recieve and send reply 
           if POLLHUP & POLLERR --> close all fds and shutdown the server
-----------------------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 8080
#define BACKLOG 5
#define MAX_CLIENTS 10
#define CLIENT_FD_STARTER 1



struct parser_array {

int slot;
char msg[100];

};

void close_fd(int fd){
    close(fd);
    }


void parser(char *buffer,int buffer_size,struct parser_array *pass){
  char *id=strtok(buffer," ");
  if(id !=NULL){
  pass->slot=atoi(id);
  }
  char *msg=strtok(NULL," ");
  if(msg!=NULL){
  strcpy(pass->msg,msg);
  }
  }


void broadcast(char *buffer,int size,struct pollfd *fds_no){
   int i;
   for(i=CLIENT_FD_STARTER;i<MAX_CLIENTS;i++){
   if(fds_no[i].fd!=-1){
   send(fds_no[i].fd,buffer,size, 0);
   }
   }
   }



int main() {
    int sfd, cfd;
    int opt=1,nfds=1;
    int i,j,activity;
    struct sockaddr_in server_addr, client_addr;
    struct pollfd fds[MAX_CLIENTS];
    struct parser_array client_array;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    char reply[1024];
    char broadcast_message[1024] , brodcast_message_for_new_client[40];
    // Creation of FD -- server fd
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Clear structure and fill details (INADDR_ANY --  all network cards)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    //Setting the flag for reusing server f
     if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) == -1){
     perror("setsockopt failed");
     exit(EXIT_FAILURE);
     }

    // Bind the socket to the port
    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }else{
    fds[0].fd=sfd;
    fds[0].events=POLLIN;
    listen(sfd,BACKLOG);
    }

    // setting the other fds to -1 --  for ignorance
    for(i=1;i<MAX_CLIENTS;i++){
    fds[i].fd = -1;
    }

/*------------------------
         POLL starts
 -------------------------
*/


    while(1){
 
    activity=poll(fds,MAX_CLIENTS,-1);  // Passing fds struct 

    if(activity<0){
    perror("Poll error");
    exit(EXIT_FAILURE);
    }

    if(fds[0].revents & POLLIN){
    cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_len);

    if (cfd == -1) {
    perror("Accept failed");
    exit(EXIT_FAILURE);
    }else{
    printf(" Accepted and fd created! \n");
    }

    for(i=1;i<MAX_CLIENTS;i++){
    if(fds[i].fd == -1){
    fds[i].fd=cfd;
    fds[i].events=POLLIN;
    printf("FD = %d created \n",i);

    snprintf(brodcast_message_for_new_client,sizeof(brodcast_message_for_new_client),"brodcast -- Clieant connected no : %d",i);
    broadcast(brodcast_message_for_new_client,sizeof(brodcast_message_for_new_client),fds);

    break;
    }
    }
    }//sfd function

    for(i=1;i<MAX_CLIENTS;i++){
    if(fds[i].fd == -1){ 
    continue;
    }else if(fds[i].revents & POLLIN){
     memset(buffer, 0, sizeof(buffer)); // clear the bucket
    recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
    printf("%d--Peer sent: %s\n",i, buffer);
    buffer[strcspn(buffer, "\n")] = '\0';


    parser(buffer,sizeof(buffer),&client_array);
    if(client_array.slot==0){
    strcpy(broadcast_message,client_array.msg);
    broadcast(broadcast_message,sizeof(broadcast_message),fds);
    }else if(client_array.slot<MAX_CLIENTS && client_array.slot>=CLIENT_FD_STARTER){
    send(fds[client_array.slot].fd,client_array.msg,sizeof(client_array.msg), 0);
    }else{fprintf(stderr,"invalid fd is given\n");}


    fgets(reply,sizeof(reply),stdin);
    if(!strcmp(reply,"exit\n")){
    printf("Closing...\n");
    for(j=0;j<MAX_CLIENTS;j++){
    if(fds[j].fd > -1){
    close_fd(fds[j].fd);

    }
    } // fd closing loop for EXIT reply 

    printf("Successfully closed\n");
    exit(EXIT_SUCCESS);
    }// EXIT string check  
     send(fds[i].fd, reply, strlen(reply), 0); // POLLIN handling
    }else if((fds[i].revents & POLLHUP) || (fds[i].revents & POLLERR)){
    close_fd(fds[i].fd);
    fds[i].fd=-1;
    printf("FD %d Closed sucesfully \n",i);
    }// PULLUP & POLLERR handling

    }//cfd function

    }//while loop
    return 0;
    }

