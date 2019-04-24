/*
* Anita Naseri 
* Assignment 1, April 2019
*/

#include <stdio.h>

// constants
static char const *const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";

static char const *const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_404_LENGTH = 45;

static const char *INTRO_PAGE = "html/1_intro.html";
static const char *START_PAGE = "html/2_start.html";
static const char *FIRSTURN_PAGE = "html/3_first_turn.html";
static const char *ACCEPTED_PAGE = "html/4_accepted.html";
static const char *DISCARDED_PAGE = "html/5_discarded.html";
static const char *ENDGAME_PAGE = "html/6_endgame.html";
static const char *GAMEOVER_PAGE = "html/7_gameover.html";

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

//parsing method type from buffer
METHOD parseMethod(char *curr, METHOD method);

//parsing which html type to load from buffer
Page parseCorrectHtml(char *temp, Page page);


void loadPOSTHtml(int n, int sockfd, char *buff, const char *pathname);

void loadGETHtml(int n, int sockfd, char *buff, const char *pathname);


void addUserName(int sockfd, char *buff, char *username, long size);

void manage_POST_requests(int n, int sockfd, char *buff, Page page); 
void manage_POST_requests(int n, int sockfd, char *buff, Page page);

void setup_up_users(int sockfd);

static bool manage_http_request(int sockfd);

int main(int argc, char * argv[]);