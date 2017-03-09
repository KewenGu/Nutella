/*
 * Author: Kewen Gu
 * */

#ifndef KGU_CS4513_PROJECT3_SERVER_H
#define KGU_CS4513_PROJECT3_SERVER_H

#define MULTICAST_ADDR "239.0.0.1"
#define MULTICAST_PORT 5123
#define UNICAST_PORT 5124
#define USLEEP_TIME 500000 // in microseconds
#define TIMEOUT 3 // in seconds
#define MAX_STR_LEN 84
#define BUF_SIZE 1024
#define NAME_LEN 32
#define MAX_NUM_MOVIES 16

int FindMovie(char *movieName, char movieList[][MAX_STR_LEN], int numMovies, char movieFilePath[]);
int TCPSend(char *sendBuf, int len, int sock);
int TCPRecv(char *recvBuf, int len, int sock);
int StrSlicing(char *args[], char *buf, char *s);
void PrintUsage(FILE *fd, char *programName);
void DieWithError(char *errorMessage);

#endif //KGU_CS4513_PROJECT3_SERVER_H
