/*
* Anita Naseri 
* Assignment 1, April 2019
*/

#include <stdio.h>


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
    EMPTY
} Page;

//parsing method type from buffer
METHOD parseMethod(char *curr, METHOD method);

//parsing which html type to load from buffer
Page parseCorrectHtml(char *temp, Page page);

bool addUserName(int sockfd, char *buff, char *username, long size);
void setup_up_users(int sockfd);
void reset_users(int sockfd);

void loadPOSTHtml(int n, int sockfd, char *buff, const char *pathname);
void loadGETHtml(int n, int sockfd, char *buff, const char *pathname);

void check_if_gusses_match(char *keyword, char user2_guesses[100][100], int number_guesses_user2, char user1_guesses[100][100], int number_guesses_user1);

int load_quit_page(int n, int sockfd,char *buff);
int load_start_page(int n, int sockfd, char *buff);
int load_accepted_page(int n, int sockfd, char *buff);

void manage_GET_requests(int n, int sockfd, char *buff, Page page);

bool manage_POST_requests(int n, int sockfd, char *buff, Page page);

//static bool manage_http_request(int sockfd);
