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

#include "helper_functions.h"

// constants
static char const *const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";

static const char *INTRO_PAGE = "html/1_intro.html";
static const char *START_PAGE = "html/2_start.html";
static const char *FIRSTURN_PAGE = "html/3_first_turn.html";
static const char *ACCEPTED_PAGE = "html/4_accepted.html";
static const char *DISCARDED_PAGE = "html/5_discarded.html";
static const char *ENDGAME_PAGE = "html/6_endgame.html";
static const char *GAMEOVER_PAGE = "html/7_gameover.html";

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

bool addUserName(int sockfd, char *buff, char *username, long size)
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


void loadPOSTHtml(int n, int sockfd, char *buff, const char *pathname)
{
    // send the header first
    if (write(sockfd, buff, n) < 0)
    {
        perror("write");
    }
    // read the content of the HTML file
    int filefd = open(filePath, O_RDONLY);
    n = read(filefd, buff, 2048);
    
    if (n < 0)
    {
        perror("read");
        close(filefd);
    }
    close(filefd);
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

int load_quit_page(int n, int sockfd, char *buff)
{
    filePath = GAMEOVER_PAGE;
    // Reset User
    reset_users(sockfd);
    stat(filePath, &st);
    n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
    return n;
}
int load_start_page(int n, int sockfd, char *buff)
{
    stat(filePath, &st);
    filePath = START_PAGE;
    //parse the username
    username = strstr(buff, "user=") + 5;
    size = st.st_size + strlen(username) + 2;
    n = sprintf(buff, HTTP_200_FORMAT, size);
    return n;
}

int load_accepted_page(int n, int sockfd, char *buff)
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
            filePath = ACCEPTED_PAGE;
            strcpy(user1_guesses[number_guesses_user1], keyword);
            printf("%s\n", user1_guesses[number_guesses_user1]);
            number_guesses_user1++;

            for (int i = 0; i < number_guesses_user2; i++)
            {
                if (strcmp(user2_guesses[i], keyword) == 0)
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
    return n;
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
        n = load_quit_page(n, sockfd, buff);
    }
    else if (page == FIRST)
    {
        n = load_start_page(n, sockfd, buff);
        
    }
    else if (page == ACCEPTED)
    {
        n = load_accepted_page(n, sockfd, buff);
    }
    loadPOSTHtml(n, sockfd, buff, filePath);

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
