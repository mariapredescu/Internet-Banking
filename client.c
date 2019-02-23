#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <unistd.h>
#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {

	//SOCKET TCP
	int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    fd_set read_fds;    //multimea de citire folosita in select()
    fd_set tmp_fds;    //multime folosita temporar 
    int fdmax;
    
    char buffer[BUFLEN];
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }  

    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    //name este numele fisierului de log al noului client
    int pid = getpid();
    char name[100];
    sprintf(name, "client-%d.log", pid);

    //se deschide fisierul pentru scriere
    FILE *file;
    file = fopen(name, "w");  
    
    //adaugam stdin in multimea read_fds
    FD_SET(0, &read_fds);
    //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;
    
    while(1) {
        tmp_fds = read_fds;

        if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
            error("ERROR in select");

        if (FD_ISSET(0, &tmp_fds)) {
            //citesc de la tastatura
            memset(buffer, 0 , BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);
            fflush(file);
            //scriu ceea ce citesc in fisierul clientului
			fprintf(file, "%s", buffer);

            //trimit mesaj la server
        	n = send(sockfd, buffer, strlen(buffer), 0);
        	if (n < 0) 
            	error("ERROR writing to socket");
            
            //daca mesajul citit este quit clientul anunta serverul ca inchide conexiunea
            if (strncmp(buffer, "quit", 4) == 0) {
            	close(sockfd);
            	FD_CLR(sockfd, &read_fds);
            	fclose(file);
            	return 0;
            }
        }

        if (FD_ISSET(sockfd, &tmp_fds)) {
            memset(buffer, 0, BUFLEN);
            recv(sockfd, buffer, BUFLEN - 1, 0);
            fflush(file);
            
            //se scrie in fisier mesajul primit de la server
            fprintf(file, "%s\n", buffer);
            printf("%s\n", buffer);

            //daca mesajul spune ca Server-ul se va inchide clientul se inchide
            if (strcmp(buffer, "Server-ul se va inchide") == 0) {
            	close(sockfd);
            	FD_CLR(sockfd, &read_fds);
            	fclose(file);
            	return 0;
            }
            
        }
    }
    close(sockfd);
    fclose(file);
    return 0;
}