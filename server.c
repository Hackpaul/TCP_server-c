#define _GNU_SOURCE

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
#define BUFFER_SIZE 1024

struct parser_array {
    int slot;
    char msg[BUFFER_SIZE - 4];
};

void close_fd(int fd){
    if(fd > 0){
        close(fd);
    }
}

void parser(char *buffer, struct parser_array *pass){
    int slot;
    char temp_buffer[BUFFER_SIZE - 4];
    int parser_value = sscanf(buffer, "%d %1019[^\n]", &slot, temp_buffer);
    if(parser_value == 2){
        pass->slot = slot;
        snprintf(pass->msg, sizeof(pass->msg), "%s", temp_buffer);
    }
}

void broadcast(char *buffer, int size, struct pollfd *fds_no){
    int i;
    for(i = CLIENT_FD_STARTER; i < MAX_CLIENTS; i++){
        if(fds_no[i].fd != -1){
            send(fds_no[i].fd, buffer, size, 0);
        }
    }
}

void broadcast_client_tag(struct pollfd *fds_no){
    int i;
    char client_tag[4];
    for(i = CLIENT_FD_STARTER; i < MAX_CLIENTS; i++){
        if(fds_no[i].fd != -1){
            memset(client_tag, 0, sizeof(client_tag));
            snprintf(client_tag, sizeof(client_tag), "@%d : ", i);
            send(fds_no[i].fd, client_tag, strlen(client_tag), 0);
        }
    }
}

int main() {
    int sfd, cfd;
    int opt = 1, nfds = 1;
    int i, j, activity;
    struct sockaddr_in server_addr, client_addr;
    struct pollfd fds[MAX_CLIENTS];
    struct parser_array client_array;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024] = {0}, reply[1024] = {0}, broadcast_message[1024] = {0};
    char brodcast_message_for_new_client[40], client_tag[10], send_to_tag[10], reciever_tag[10];
    
    // Creation of FD -- server fd
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
         perror("Socket creation failed");
         exit(EXIT_FAILURE);
    }

    // Clear structure and fill details (INADDR_ANY -- all network cards)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Setting the flag for reusing server f
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    } else {
        fds[0].fd = sfd;
        fds[0].events = POLLIN;
        listen(sfd, BACKLOG);
    }

    // setting the other fds to -1 -- for ignorance
    for(i = 1; i < MAX_CLIENTS; i++){
        fds[i].fd = -1;
    }

    printf("Server initialized smoothly. Listening on port %d...\n", PORT);

    while(1){
        activity = poll(fds, MAX_CLIENTS, -1); // Passing fds struct 

        if(activity < 0){
            perror("Poll error");
            exit(EXIT_FAILURE);
        }
        if(fds[0].revents & POLLIN){
            cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_len);
            if (cfd == -1) {
                perror("Server :: Accept failed !! \n");
            } else {
                for(i = CLIENT_FD_STARTER; i < MAX_CLIENTS; i++){
                    if(fds[i].fd == -1){
                        fds[i].fd = cfd;
                        fds[i].events = POLLIN;
                        printf("server :: <FD=%d> created \n", i);

                        memset(brodcast_message_for_new_client, 0, sizeof(brodcast_message_for_new_client));
                        snprintf(brodcast_message_for_new_client, sizeof(brodcast_message_for_new_client), "Info : Client <%d> connected", i);

                        broadcast_client_tag(fds);
                        broadcast(brodcast_message_for_new_client, strlen(brodcast_message_for_new_client), fds);
                        broadcast("\n", 1, fds);
                        broadcast_client_tag(fds);
                        printf("server :: <Broadcast> : %s\n", brodcast_message_for_new_client);
                        break;
                    }
                }
            }
        } // sfd function

        for(i = CLIENT_FD_STARTER; i < MAX_CLIENTS; i++){
            if(fds[i].fd == -1){ 
                continue;
            } else if(fds[i].revents & POLLIN){
                memset(buffer, 0, sizeof(buffer)); // clear the bucket
                int byte_recv = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                if(byte_recv > 0){
                    memset(&client_array, 0, sizeof(client_array));
                    parser(buffer, &client_array);

                    memset(send_to_tag, 0, sizeof(send_to_tag));
                    memset(client_tag, 0, sizeof(client_tag));
                    memset(reciever_tag, 0, sizeof(reciever_tag));

                    snprintf(send_to_tag, sizeof(send_to_tag), "<%d> ", client_array.slot);
                    snprintf(client_tag, sizeof(client_tag), "@%d : ", i);
                    snprintf(reciever_tag, sizeof(reciever_tag), "@%d : ", client_array.slot);

                    if(client_array.slot == 0){
                        broadcast(send_to_tag, strlen(send_to_tag), fds);
                        broadcast(client_array.msg, strlen(client_array.msg), fds);
                        broadcast("\n", 1, fds); 
                        broadcast_client_tag(fds);
                        printf("Client :: <Broadcast> : %s\n", client_array.msg);
                    } else if(client_array.slot < MAX_CLIENTS && client_array.slot >= CLIENT_FD_STARTER){
                        send(fds[client_array.slot].fd, send_to_tag, strlen(send_to_tag), 0);
                        send(fds[client_array.slot].fd, client_array.msg, strlen(client_array.msg), 0);
                        send(fds[client_array.slot].fd, "\n", 1, 0);
                        send(fds[i].fd, reciever_tag, strlen(reciever_tag), 0);
                        send(fds[i].fd, client_tag, strlen(client_tag), 0);
                        printf("Client :: <%d> --> <%d> : %s\n", i, client_array.slot, client_array.msg);
                    } else {
                        fprintf(stderr, "invalid fd is given\n");
                    }
                } else {
                    printf("server :: FD %d disconnected\n", i);
                    close_fd(fds[i].fd);
                    fds[i].fd = -1;
                }
            } else if((fds[i].revents & POLLHUP) || (fds[i].revents & POLLERR)){
                close_fd(fds[i].fd);
                fds[i].fd = -1;
                printf("server :: FD %d disconnected\n", i);
            }
        } // cfd function
    } // while loop
    return 0;
}
