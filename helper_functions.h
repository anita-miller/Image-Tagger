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

//add the username to html file dynamically
bool addUserName(int sockfd, char *buff, char *username, long size);

// set up the user's value thats playing to sockfd
void setup_up_users(int sockfd);

//reset users to -1 when game is quited
void reset_users(int sockfd);

void loadPOSTHtml(int n, int sockfd, char *buff, const char *pathname);

void loadGETHtml(int n, int sockfd, char *buff, const char *pathname);

int load_quit_page(int n, int sockfd,char *buff);

int load_start_page(int n, int sockfd, char *buff);

int load_accepted_page(int n, int sockfd, char *buff);

void manage_GET_requests(int n, int sockfd, char *buff, Page page);

bool manage_POST_requests(int n, int sockfd, char *buff, Page page);
