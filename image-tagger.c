/*
* Anita Naseri 
* Assignment 1, April 2019
*/

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// constants
static char const *const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";
static char const *const HTTP_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_400_LENGTH = 47;
static char const *const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_404_LENGTH = 45;

static char *filePath;

static int user1 = -1;
static int user1_start = 0;
char user1_guesses[20][100];
int number_guesses_user1 = 0;

static int user2 = -1;
static int user2_start = 0;
char user2_guesses[20][100];
int number_guesses_user2 = 0;

// represents the types of method
typedef enum
{
    GET,
    POST,
    UNKNOWN
} METHOD;

typedef enum
{
    INTRO,
    START,
    FIRST,
    ACCEPTED,
    DISCARDED,
    QUIT,
    GAMEOVER,
    NOTDEFINED,
    EMPTY
} Page;

void removeSubstring(char *s, const char *toremove)
{
    while((s = strstr(s, toremove))){
        memmove(s, s + strlen(toremove), 1 + strlen(s + strlen(toremove)));
    }   
        
}
METHOD parseMethod(char *curr, METHOD method)
{
    // parse the method
    if (strncmp(curr, "GET ", 4) == 0)
    {
        curr += 4;
        method = GET;
    }
    else if (strncmp(curr, "POST ", 5) == 0)
    {
        curr += 5;
        method = POST;
    }
    return method;
}
Page parseCorrectHtml(char *temp, Page page)
{
    if (strstr(temp, "guess=") != NULL)
    {
        page = ACCEPTED;
    }
    else if (strstr(temp, "?start=Start") != NULL && strstr(temp, "quit=Quit") == NULL)
    {
        page = START;
    }
    else if (strstr(temp, "quit=Quit") != NULL)
    {
        page = QUIT;
    }
    else if (strstr(temp, "user=") != NULL && strstr(temp, "?start=Start") == NULL)
    {
        page = FIRST;
    }
    else
    {
        page = INTRO;
    }
    return page;
}
void loadHtml(int n, int sockfd, char* buff, const char *pathname)
{
    // get the size of the file
    struct stat st;
    stat(pathname, &st);
    n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
    // send the header first
    if (write(sockfd, buff, n) < 0)
    {
        perror("write");
    }
    // send the file
    int filefd = open(pathname, O_RDONLY);
    do
    {
        n = sendfile(sockfd, filefd, NULL, 2048);
    } while (n > 0);
    close(filefd);
}

static bool manage_http_request(int sockfd, char *guesses)
{
    // try to read the request
    char buff[2049];
    int n = read(sockfd, buff, 2049);
    if (n <= 0)
    {
        if (n < 0)
            perror("read");
        else
            printf("socket %d close the connection\n", sockfd);
        return false;
    }

    // terminate the string
    buff[n] = 0;

    // parse the method
    char * curr = buff;
    METHOD method = UNKNOWN;
    method = parseMethod(curr, method);

    //parse which html we need to load
    char *temp = buff;
    Page page = EMPTY;
    page = parseCorrectHtml(temp, page);
    

    printf("%s", temp);
    // sanitise the URI
    while (*curr == '.' || *curr == '/')
        ++curr;
    // assume the only valid request URI is "/" but it can be modified to accept more files
    if (method == GET)
    {
        if (page == INTRO)
        {
            loadHtml(n, sockfd, buff, "html/1_intro.html");  
        }
        else if (page == START)
        {
            loadHtml(n, sockfd, buff, "html/3_first_turn.html");
            printf("%s", buff);
        }
    }
        
    else if (method == POST)
    {
        char *username;
        int username_length;
        long added_length;
        long size;
        // setup the users
        if ((user1 == -1) && (sockfd != user2))
        {
            user1 = sockfd;
        }
        else if ((user2 == -1) && (sockfd != user1))
        {
            user2 = sockfd;
        }

        struct stat st;
        if (method == QUIT)
        {
            filePath = = "html/7_gameover.html";
            stat(filePath, &st);
            // Reset User
            if (sockfd == user1)
            {
                user1 = -1;
                user1_start = 0;
            }
            else if (sockfd == user2)
            {
                user2 = -1;
                user2_start = 0;
            }
            else
            {
                printf("Sockfd not set for some reason..\n\n");
            }

            n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
            if (write(sockfd, buff, n) < 0)
            {
                perror("write");
                return false;
            }

            int filefd = open(webpage, O_RDONLY);
            n = read(filefd, buff, 2048);
            if (n < 0)
            {
                perror("read");
                close(filefd);
                return false;
            }
            close(filefd);
            stat(webpage, &st);
            if (write(sockfd, buff, st.st_size) < 0)
            {
                perror("write");
                return false;
            }
        }
        else if (method == FIRST)
        {
        }
        else if (method == ACCEPTED)
        {

        }
        else
        {
            printf("\n\n\nerror reading html...\n\n\n");
        }

        filePath
    }

    else{
        fprintf(stderr, "no other methods supported");
   }

    return true;

}



int main(int argc, char * argv[])
{ 
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s ip port\n", argv[0]);
        return 0;
    }

    // create TCP socket which only accept IPv4
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Could not create sockeet");
        exit(EXIT_FAILURE);
    }

    // reuse the socket if possible
    int const reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // create and initialise address we will listen on
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // if ip parameter is not specified
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // bind address to socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen on the socket
    listen(sockfd, 5);

    // initialise an active file descriptors set
    fd_set masterfds;
    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    // record the maximum socket number
    int maxfd = sockfd;
    char *guesses = malloc(sizeof(char) * 1000);
    while (1)
    {
        // monitor file descriptors
        fd_set readfds = masterfds;
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // loop all possible descriptor
        for (int i = 0; i <= maxfd; ++i)
            // determine if the current file descriptor is active
            if (FD_ISSET(i, &readfds))
            {
                // create new socket if there is new incoming connection request
                if (i == sockfd)
                {
                    struct sockaddr_in cliaddr;
                    socklen_t clilen = sizeof(cliaddr);
                    int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
                    if (newsockfd < 0)
                        perror("accept");
                    else
                    {
                        // add the socket to the set
                        FD_SET(newsockfd, &masterfds);
                        // update the maximum tracker
                        if (newsockfd > maxfd)
                            maxfd = newsockfd;
                        // print out the IP and the socket number
                        char ip[INET_ADDRSTRLEN];
                        printf(
                            "new connection from %s on socket %d\n",
                            // convert to human readable string
                            inet_ntop(cliaddr.sin_family, &cliaddr.sin_addr, ip, INET_ADDRSTRLEN),
                            newsockfd
                        );
                    }
                }
                
                // a request is sent from the client
                else if (!manage_http_request(i, guesses))
                {
                    close(i);
                    FD_CLR(i, &masterfds);
                }
            }
    }
    return 0;
}



