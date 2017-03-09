/*
 * Author: Kewen Gu
 * */


#define _XOPEN_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/select.h>

#include "nutella.h"
#include "msock.h"


/* Find the movie in a list of local movieList by its name */
int FindMovie(char *movieName, char movieList[][MAX_STR_LEN], int numMovies, char movieFilePath[]) {
  int i;
  char *tokens[4], tmp[MAX_STR_LEN];
  for (i = 0; i < numMovies; i++) {
    strcpy(tmp, movieList[i]);
    StrSlicing(tokens, movieList[i], ".");
    if (strcmp(movieName, tokens[0]) == 0) {
      strcpy(movieFilePath, "./movies/");
      strcat(movieFilePath, tmp);
      return 0;
    }
  }
  return 1;
}



/* Send operation for both client and server */
int TCPSend(char *sendBuf, int len, int servSock)
{
    int bytesSend;
    if ((bytesSend = send(servSock, sendBuf, (size_t)len, 0)) != len)
        DieWithError("send() failed");

    return bytesSend;
}


/* Receive operation for both client and server */
int TCPRecv(char *recvBuf, int len, int servSock)
{
    int n, bytesRcvd = 0;

    while (bytesRcvd < len)
    {
        if ((n = recv(servSock, recvBuf + bytesRcvd, (size_t)(len - bytesRcvd), 0)) < 0)
            DieWithError("recv() failed");

        bytesRcvd += n;
    }
    recvBuf[strlen(recvBuf)] = '\0';
    return bytesRcvd;
}


/* Slice the long string into tokens */
int StrSlicing(char *args[], char *buf, char *s)
{
	int i = 0;
	/* get the first token */
	char *token = strtok(buf, s);
	args[i] = token;

	/* walk through other tokens */
	while(token != NULL)
	{
	    token = strtok(NULL, s);
	    args[++i] = token;
	}
	return i;
}


/* Print the usage of this program */
void PrintUsage(FILE *fd, char *programName) {
    fprintf(fd, "usage: %s -t <nutella type> -s <server address> -m <multicast port> -u <unicast port>\n", programName);
    fprintf(fd, "\t<nutella type>  \t- run as 'server' or 'client'\n");
    fprintf(fd, "\t<server address>\t- the IP address of the server\n");
    fprintf(fd, "\t<multicast port>\t- the multicast port number\n");
    fprintf(fd, "\t<unicast port>  \t- the unicast port number\n");
    exit(1);
}


/* If error generated, print error message and die */
void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}
