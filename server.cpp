/*
** server.cpp -- accept client connection and echo whaterver is recv - ed back to the client
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

#define BACKLOG 10 

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

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
    int sockfd, new_fd;
    int port_num;
    struct sigaction sa;
    socklen_t addr_size;
    struct sockaddr_storage their_addr;
    struct addrinfo hints;
    struct addrinfo *res, *p;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    char ipstr[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    int yes = 1;

    // checking host name
    // char* hostname;
    // gethostname(hostname, sizeof(hostname));
    // fprintf("host name is : %s", hostname);

    // getting addrinfo and binding to specific port
    if ((status = getaddrinfo(NULL, "3490", &hints, &res)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    };
     
    for(p = res; p!= NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1){
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(res);
    if(p==NULL){
        fprintf(stderr, "Failed to bind\n");
        exit(1);
    }

    char hostname[128];
    gethostname(hostname, sizeof hostname);
    printf("My hostname: %s\n", hostname);

    if(listen(sockfd, BACKLOG)==-1){
        perror("listen");
        exit(1);
    };
    // waiting for connection now
    printf("server waiting for connections ... \n");

    // now accept an incoming connection:

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    

    // now ready to communicate on socket new_fd
    while (1){
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello! welcome to echo server. Type something and I'll type the exact same thing back. How exciting!!", 256, 0) == -1)
                perror("send");
            int numbytes;
            char *buf[256];
            if ((numbytes = recv(sockfd, buf, 256, 0)) == -1) {
                perror("recv");
                exit(1);
            }

            buf[numbytes] = '\0';

            printf("sever: received '%s'\n",buf);

            int bytes_sent;
            if ((bytes_sent = send(sockfd, buf, 256, 0)) == -1) {
                perror("send");
                exit(1);
            }
            //close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
        
    }


    return 0;
}

