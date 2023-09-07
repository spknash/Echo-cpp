/*
** client.cpp -- connect to server and be able to send messages which are echoed by server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXDATASIZE 257

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char const *argv[])
{
    // declarations of needed variables
    int status;
    int sockfd;
    int numbytes;

    char *buf[100];
    
    struct addrinfo hints;
    struct addrinfo *res, *p;
    socklen_t sin_size;

    char s[INET6_ADDRSTRLEN];
    // initialize sockaddr hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    // checking host name
    // char* hostname;
    // gethostname(hostname, sizeof(hostname));
    // fprintf("host name is : %s", hostname);
    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }
    // getting addrinfo and binding to specific port
    if ((status = getaddrinfo(argv[1], "3490", &hints, &res)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    };
     
    for(p = res; p!= NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1){
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    freeaddrinfo(res);
    if(p==NULL){
        fprintf(stderr, "client failed to connect\n");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);
    //free(s);
    //freeaddrinfo(res); // all done with this structure

    

    while (1) {
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';

        printf("client: received '%s'\n",buf);
        char buffer[256];
        printf("Enter a message: ");
        
        // Read a line of input from the user
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Error reading input.\n");
            continue;
        }

        // Remove the trailing newline character, if any
        if (buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }

        // Exit the loop if the user types "exit"
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
        printf("%s", buffer);

        // send the message
        int bytes_sent;
        if ((bytes_sent = send(sockfd, buffer, 256, 0)) == -1) {
            perror("send");
            exit(1);
        }
    }

    close(sockfd);

    return 0;



}