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

#include "image-tagger.h"

static int user1 = -1;
static int user1_start = 0;
char user1_guesses[100][100];
int number_guesses_user1 = 0;

static int user2 = -1;
static int user2_start = 0;
char user2_guesses[100][100];
int number_guesses_user2 = 0;
static const char *filePath;
int gameover = 0;

char *username;
long size;
struct stat st;

int main(int argc, char *argv[])
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
                            newsockfd);
                    }
                }

                // a request is sent from the client
                else if (!manage_http_request(i))
                {
                    close(i);
                    FD_CLR(i, &masterfds);
                }
            }
    }
    return 0;
}

static bool manage_http_request(int sockfd)
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
    char *curr = buff;
    METHOD method = method = parseMethod(curr, method);

    //parse which html we need to load
    char *temp = buff;
    Page page = parseCorrectHtml(temp, page);

    // assume the only valid request URI is "/" but it can be modified to accept more files
    if (method == GET)
    {
        manage_GET_requests(n, sockfd, buff, page);
    }
    else if (method == POST)
    {
        if (manage_POST_requests(n, sockfd, buff, page) == false){
            return false;
        }
    }
    else
    {
        fprintf(stderr, "no other methods supported");
    }

    return true;
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
    else{
        method = UNKNOWN;
    }
    return method;
}
Page parseCorrectHtml(char *temp, Page page)
{
    if (strstr(temp, "guess=Guess") != NULL)
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
bool loadPOSTHtml(int n, int sockfd, char *buff, const char *pathname)
{
    // send the header first
    if (write(sockfd, buff, n) < 0)
    {
        perror("write");
        return false;
    }
    // read the content of the HTML file
    int filefd = open(filePath, O_RDONLY);
    n = read(filefd, buff, 2048);

    if (n < 0)
    {
        perror("read");
        close(filefd);
        return false;
    }
    close(filefd);
    return true;

}
void loadGETHtml(int n, int sockfd, char *buff, const char *pathname)
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

void addUserName(int sockfd, char *buff, char *username, long size)
{
    // move the trailing part backward
    int username_length = strlen(username);
    long added_length = username_length + 2;
    int p1, p2;
    for (p1 = size - 1, p2 = p1 - added_length; p1 >= size - 25; --p1, --p2)
        buff[p1] = buff[p2];
    ++p2;
    // put the separator
    buff[p2++] = '\n';
    buff[p2++] = ' ';
    // copy the username
    strncpy(buff + p2, username, username_length);
    if (write(sockfd, buff, size) < 0)
    {
        perror("write");
        return false;
    }
}

void manage_GET_requests(int n, int sockfd, char *buff, Page page)
{
    if (page == INTRO)
    {
        loadGETHtml(n, sockfd, buff, INTRO_PAGE);
    }
    else if (page == START)
    {
        if (sockfd == user1)
        {
            user1_start = 1;
        }
        else if (sockfd == user2)
        {
            user2_start = 1;
        }

        loadGETHtml(n, sockfd, buff, FIRSTURN_PAGE);
    }
}
bool manage_POST_requests(int n, int sockfd, char *buff, Page page)
{
    setup_up_users(sockfd);

    filePath = START_PAGE;
    // when Quit button is clicket, exits games
    if (page == QUIT)
    {
        load_quit_page(sockfd, st, buff);
    }
    else if (page == FIRST)
    {
        load_first_turn_page(sockfd, st, buff);
    }
    else if (page == ACCEPTED)
    {
        load_accepted_page(sockfd, st, buff);
    }
    if (loadPOSTHtml(n, sockfd, buff, filePath) == false)
    {
        return false;
    }

    //adding username
    if (page == FIRST && strlen(username) > 0)
    {
        addUserName(sockfd, buff, username, size);
    }
    else
    {
        if (write(sockfd, buff, st.st_size) < 0)
        {
            perror("write");
            return false;
        }
    }
    return true;
}

void setup_up_users(int sockfd)
{
    // setup the users
    if ((user1 == -1) && (sockfd != user2))
    {
        user1 = sockfd;
    }
    else if ((user2 == -1) && (sockfd != user1))
    {
        user2 = sockfd;
    }
}
void reset_users(int sockfd)
{
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
}

void load_quit_page(int sockfd, struct stat st, char *buff)
{
    filePath = GAMEOVER_PAGE;
    // Reset User
    reset_users(sockfd);
    stat(filePath, &st);
    n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
}
void load_first_turn_page(int sockfd, struct stat st, char *buff)
{
    filePath = START_PAGE;
    //parse the username
    username = strstr(buff, "user=") + 5;
    size = st.st_size + strlen(username) + 2;
    stat(filePath, &st);
    n = sprintf(buff, HTTP_200_FORMAT, size);
}

void load_accepted_page(int sockfd, struct stat st, char *buff)
{
    //parse the keyword
    char *keyword = strstr(buff, "keyword=") + 8;
    int keyword_length = strlen(keyword);
    keyword[keyword_length - 12] = '\0';

    //if play1 is connected and playing
    if (sockfd == user1)
    {
        //if player 2 is connected too
        if (user2_start == 1)
        {
            check_if_gusses_match(keyword, user1_guesses, number_guesses_user1, user2_guesses, number_guesses_user2);
        }
        else if (gameover == 1)
        {
            filePath = ENDGAME_PAGE;
            gameover = 0;
        }
        else
        { //when player tries to guess before other player starts
            filePath = DISCARDED_PAGE;
        }
    }
    else if (sockfd == user2)
    {
        //if player 1 is connected too
        if (user1_start == 1)
        {
            check_if_gusses_match(keyword, user2_guesses, number_guesses_user2, user1_guesses, number_guesses_user1);
        }
        else if (gameover == 1)
        {
            filePath = ENDGAME_PAGE;
            gameover = 0;
        }
        else
        { //when player tries to guess before other player starts
            filePath = DISCARDED_PAGE;
        }
    }

    stat(filePath, &st);
    n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
}

void check_if_gusses_match(char *keyword, char user2_guesses, int number_guesses_user2, char user1_guesses, int number_guesses_user1)
{
    filePath = ACCEPTED_PAGE;
    strcpy(user2_guesses[number_guesses_user2], keyword);
    printf("%s\n", user2_guesses[number_guesses_user2]);
    number_guesses_user2++;

    for (int i = 0; i < number_guesses_user1; i++)
    {
        if (strcmp(user1_guesses[i], keyword) == 0)
        {
            gameover = 1;
            user1 = -1;
            user2 = -1;
            number_guesses_user1 = 0;
            number_guesses_user2 = 0;
            user1_start = 0;
            user2_start = 0;
            for (int i = 0; i < 100; i++)
            {
                memset(user1_guesses[i], '\0', 100);
                memset(user2_guesses[i], '\0', 100);
            }
            filePath = ENDGAME_PAGE;
        }
    }
}