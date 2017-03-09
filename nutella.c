/*
 * Author: Kewen Gu
 * */


#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/select.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nutella.h"
#include "msock.h"


int main(int argc, char *argv[]) {

  if (argc < 3 || argc > 8)
    PrintUsage(stderr, argv[0]);


  int multiSock, uniSock, multiPort, uniPort, servSock, clntSock, clntLen, childpid;
  struct sockaddr_in sAddr, cAddr;
  char *multiAddr, *type, servIP[MAX_STR_LEN], servAddr[MAX_STR_LEN], servPort[MAX_STR_LEN];
  int opt, ret, cnt, numMovies, bytes;
  char movieName[MAX_STR_LEN], movieList[MAX_NUM_MOVIES][MAX_STR_LEN], movieListCopy[MAX_NUM_MOVIES][MAX_STR_LEN], movieFilePath[MAX_STR_LEN];
  char request[MAX_STR_LEN], response[MAX_STR_LEN], readBuf[BUF_SIZE], sendBuf[BUF_SIZE], streamBuf[BUF_SIZE], input[MAX_STR_LEN], message[MAX_STR_LEN];
  char *tokens[4];
  FILE *fp;
  DIR *dir;
  fd_set fdSet;
  struct timeval timer;
  struct dirent *ent;
  struct ifaddrs *addrs, *tmp;
  struct sockaddr_in *pAddr;
  struct addrinfo hints, *servinfo, *res;


  multiAddr = MULTICAST_ADDR;
  multiPort = MULTICAST_PORT;
  uniPort = UNICAST_PORT;
  clntLen = sizeof(cAddr);

  while ((opt = getopt(argc, argv, "t:s:m:u:h")) != -1) {
    switch (opt) {
      case 't':
        type = argv[optind - 1];
        break;
      case 's':
        multiAddr = argv[optind - 1];
        break;
      case 'm':
        multiPort = atoi(argv[optind - 1]);
        break;
      case 'u':
        uniPort = atoi(argv[optind - 1]);
        break;
      case 'h':
        PrintUsage(stdout, argv[0]);
        break;
    }
  }


  if ((dir = opendir ("./movies/")) != NULL) {
    /* print all the files and directories within directory */
    numMovies = 0;
    printf("List of local movies: \n");
    while ((ent = readdir (dir)) != NULL) {
      if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0)) {
        strcpy(movieList[numMovies], ent->d_name);
        printf ("%d. %s\n", numMovies + 1, movieList[numMovies]);
        numMovies++;
      }
    }
    printf("\n");
    closedir(dir);
  }
  /* could not open directory */
  else
    DieWithError("readdir() failed");

  getifaddrs(&addrs);
  tmp = addrs;
  while (tmp)
  {
    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
      pAddr = (struct sockaddr_in *)tmp->ifa_addr;
      if (strcmp(tmp->ifa_name, "eth0") == 0) {
        strcpy(servIP, inet_ntoa(pAddr->sin_addr));
        break;
      }
    }
    tmp = tmp->ifa_next;
  }

  freeifaddrs(addrs);

  sprintf(servPort, "%d", uniPort);
  strcpy(servAddr, servIP);
  strcat(servAddr, ":");
  strcat(servAddr, servPort);

  /* Create the socket */
  if ((uniSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError("socket() failed");
  /* Configure server address */
  memset(&sAddr, 0, sizeof(sAddr));
  sAddr.sin_family      = AF_INET;
  sAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sAddr.sin_port        = htons(uniPort);
  /* Bind to the address */
  if (bind(uniSock, (struct sockaddr*) &sAddr, sizeof(sAddr)))
    DieWithError("bind() failed");
  /* Listen to the socket */
  if (listen(uniSock, 1) < 0)
    DieWithError("listen() failed");



  if (strcmp(type, "server") == 0) {

    if ((multiSock = msockcreate(SERVER, multiAddr, multiPort)) < 0)
      DieWithError("msocketcreate() failed");

    printf("Server: listening to requests from peers......\n");

    while (1) {
      bzero(request, MAX_STR_LEN);
      if (mrecv(multiSock, request, MAX_STR_LEN) < 0)
        DieWithError("mrecv() failed");

      printf("Server: received '%s'\n", request);

      memcpy(movieListCopy, movieList, MAX_STR_LEN * MAX_NUM_MOVIES);
      if (!FindMovie(request, movieListCopy, numMovies, movieFilePath)) {

        printf("Server: found the movie path %s\n", movieFilePath);
        printf("Server: sending stream address to client\n");

        if (msend(multiSock, servAddr, MAX_STR_LEN) < 0)
          DieWithError("msend() failed");

        /* Accept the incoming connection */
        if ((clntSock = accept(uniSock, (struct sockaddr *) &cAddr, (socklen_t *)&clntLen)) < 0)
          DieWithError("accept() failed");

        if ((childpid = fork()) < 0)
          DieWithError("fork() failed");

        else if (childpid == 0) {
          printf("Server: TCP connection accepted, ready to stream\n");
          close(uniSock);
          if ((fp = fopen(movieFilePath, "r")) == NULL)
            DieWithError("fopen() failed");

          bzero(sendBuf, BUF_SIZE);
          bzero(readBuf, BUF_SIZE);
          while (fgets(readBuf, BUF_SIZE, fp) != NULL) {
            if (strcmp(readBuf, "end\n") != 0) {
              strcat(sendBuf, readBuf);
            }
            else {
              // printf("%s\n", sendBuf);
              TCPSend(sendBuf, BUF_SIZE, clntSock);
              usleep((useconds_t) USLEEP_TIME);
              bzero(sendBuf, BUF_SIZE);
            }
            bzero(readBuf, BUF_SIZE);
          }
          TCPSend("DONE", BUF_SIZE, clntSock);
          TCPRecv(message, BUF_SIZE, clntSock);

          while (strcmp(message, "REPEAT") == 0) {
            printf("Server: received repeat request\n");
            fseek(fp, 0, SEEK_SET);
            while (fgets(readBuf, BUF_SIZE, fp) != NULL) {
              if (strcmp(readBuf, "end\n") != 0) {
                strcat(sendBuf, readBuf);
              }
              else {
                // printf("%s\n", sendBuf);
                TCPSend(sendBuf, BUF_SIZE, clntSock);
                usleep((useconds_t) USLEEP_TIME);
                bzero(sendBuf, BUF_SIZE);
              }
              bzero(readBuf, BUF_SIZE);
            }
            TCPSend("DONE", BUF_SIZE, clntSock);
            TCPRecv(message, BUF_SIZE, clntSock);
          }
          printf("\n");
          fclose(fp);
          exit(0);
        }
        else {
          close(clntSock);
        }
      }
      else
        printf("Server: movie not found\n\n");
    }
  }


  else if (strcmp(type, "client") == 0) {

    if ((multiSock = msockcreate(CLIENT, multiAddr, multiPort)) < 0)
      DieWithError("msocketcreate() failed");

    while (1) {
      bzero(movieName, MAX_STR_LEN);
      printf("Enter movie name: ");
      fflush(stdout);
      if (read(STDIN_FILENO, movieName, MAX_STR_LEN) < 0)
        DieWithError("read() failed");
      if (strcmp(movieName, "\n") == 0) {
        close(multiSock);
        exit(0);
      }

      movieName[strlen(movieName) - 1] = '\0';

      if (msend(multiSock, movieName, MAX_STR_LEN) < 0)
        DieWithError("msend() failed");

      printf("Client: searching for movies\n");

      timer.tv_sec = TIMEOUT;
      timer.tv_usec = 0;

      FD_ZERO(&fdSet);
      FD_SET(multiSock, &fdSet);

      bzero(response, MAX_STR_LEN);
      if ((ret = select(multiSock + 1, &fdSet, NULL, NULL, &timer)) < 0)
        DieWithError("select() failed");
      else if (ret) {
        if ((cnt = mrecv(multiSock, response, MAX_STR_LEN)) < 0)
          DieWithError("mrecv() failed");
        else if (cnt) {
          printf("Client: received stream address %s\n", response);

          StrSlicing(tokens, response, ":");

          /* Configure the server address */
        	memset(&hints, 0, sizeof(hints));
        	hints.ai_family = AF_INET;
        	hints.ai_socktype = SOCK_STREAM;

        	/* Obtain the server address info */
        	if(getaddrinfo(tokens[0], tokens[1], &hints, &servinfo) != 0)
          	DieWithError("getaddrinfo() failed");

        	/* Try the obtained address one by one until connected */
        	for (res = servinfo; res; res = res->ai_next)
        	{
        		/* Create a relaible, stream socket using TCP */
        		if ((servSock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
        			DieWithError("socket() failed");

        		/* Establish the connection to the echo server */
        		if (connect(servSock, res->ai_addr, res->ai_addrlen) < 0)
        			DieWithError("connect() failed");
        	}

          printf("Client: connected to stream server\n");

          printf("\n");
          while((bytes = recv(servSock, streamBuf, BUF_SIZE, 0)) > 0) {
            if (strcmp(streamBuf, "DONE") == 0)
              break;
            printf("\033[2J");
            printf("\033[0;0f");
        		printf("%s", streamBuf);
            fflush(stdout);
            bzero(streamBuf, BUF_SIZE);
        	}
        	if (bytes == -1)
        		DieWithError("recv() failed");
          printf("\n");

          while (1) {
            printf("Play it again? (y/N): ");
            fflush(stdout);
            if (read(STDIN_FILENO, input, MAX_STR_LEN) < 0)
              DieWithError("read() failed");
            if ((strstr(input, "yes") == NULL) && (strstr(input, "y") == NULL) && (strstr(input, "Y") == NULL)) {
              TCPSend("NONREPEAT", BUF_SIZE, servSock);
              break;
            }
            else {
              TCPSend("REPEAT", BUF_SIZE, servSock);
              printf("\n");
              while((bytes = recv(servSock, streamBuf, BUF_SIZE, 0)) > 0) {
                if (strcmp(streamBuf, "DONE") == 0)
                  break;
                printf("\033[2J");
                printf("\033[0;0f");
            		printf("%s", streamBuf);
                fflush(stdout);
                bzero(streamBuf, BUF_SIZE);
            	}
            	if (bytes == -1)
            		DieWithError("recv() failed");
              printf("\n");
            }
          }
        }
      }
      else
        printf("Client: time out, movie not found\n");
    }
    close(servSock);
  }

  return 0;
}
