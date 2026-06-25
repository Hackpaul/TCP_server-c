//1.bind 
//2.listen
//3.accept
//4.send
//5.recv
//6.close 


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
void close_fd(int fd_1){
    close(fd_1);
    }

int main() {
    int sfd, cfd;
    int opt=1,nfds=1;
    int i,j,activity;
    struct sockaddr_in server_addr, client_addr;
    struct pollfd fds[MAX_CLIENTS];
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    char reply[1024];
   
    // 1. Create TCP Internet Socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Clear structure and fill details (INADDR_ANY binds to all network cards)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY; 

    //setting the flag for reusing 
     if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) == -1){
     perror("setsockopt failed");
     exit(EXIT_FAILURE);
     }

    // 3. Bind the socket to the port
    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }else{
    fds[0].fd=sfd;
    fds[0].events=POLLIN;
    listen(sfd,BACKLOG);
    }


    for(i=1;i<MAX_CLIENTS;i++){
    fds[i].fd = -1;
    }

    while(1){
    
    
    activity=poll(fds,MAX_CLIENTS,-1);

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
    nfds+=1;
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
    printf("Peer sent: %s\n", buffer);
    printf("Enter to reply:\n");
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

