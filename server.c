#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFLEN 256
#define NO_CLIENTS 9000

void error(char *msg)
{
    perror(msg);
    exit(1);
}

//structura cu datele userilor
struct  users_data {
	char lastname[12];
	char firstname[12];
	int card_no;
	int pin;
	char password[9];
	float sold;
};

//structura in care se retine ultimul socket in care un user a incercat sa se logheze
struct lock {
	int socket;
	int loged;
};

int main(int argc, char *argv[]) {
	
	//SOCKET TCP

	//Procesarea datelor clientilor - sunt retinute intr-un vector de structuri data
	FILE *users_data_file;
	users_data_file = fopen(argv[2], "r");

	int no_users, i;
	fscanf(users_data_file, "%d", &no_users);

	//vectorul de structuri in care sunt retinute datele clientilor
	struct users_data data[no_users];
	for (i = 0; i < no_users; i++) {
		fscanf(users_data_file, "%s %s %d %d %s %f", data[i].lastname, data[i].firstname,
			&data[i].card_no, &data[i].pin, data[i].password, &data[i].sold);
	}

	//se printeaza datele userilor din baza de date
	/*for (i = 0; i < no_users; i++) {
		printf("%s %s %d %d %s %f\n", data[i].lastname, data[i].firstname, data[i].card_no,
			data[i].pin, data[i].password, data[i].sold);
	}*/

	//inchiderea fisierului cu datele userilor
	fclose(users_data_file);

	//vector de structuri in care se retine ultimul socket in care un user a incercat sa se logheze
	struct lock locked[no_users];

	int log[no_users], sock[no_users], trans[no_users];
	for (i = 0; i < no_users; i++) {
		log[i] = 0;		//vector in care se tine evidenta autentificarilor
		sock[i] = 0;	// vector in care se tine evidenta pe ce socket este logat un user
		trans[i] = 0;	//vector in care se tine evidenta userilor care vor sa transfere bani
		locked[i].socket = 0;		//se retine socketul pe care incearca sa se logeze un user
		locked[i].loged = 0;	//se retine daca un user s-a logat cu succes - 1 =>succes; numar negativ => fail
	}


	int sockfd, newsockfd, portno, clilen;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n, j;

    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;		//multime folosita temporar 
    int fdmax;			//valoare maxima file descriptor din multimea read_fds

    if (argc < 2) {
        fprintf(stderr,"Usage : %s port\n", argv[0]);
        exit(1);
    }

    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
     
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
     
    portno = atoi(argv[1]);

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    serv_addr.sin_port = htons(portno);
     
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
            error("ERROR on binding");
     
    listen(sockfd, NO_CLIENTS);

    //adaugam stdin in multimea read_fds
	FD_SET(0, &read_fds);
    //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockfd, &read_fds);

    fdmax = sockfd;

    int l;
    float suma_trans = 0;
    int transfer = 0;

    //vector in care se retine daca pe un client este logat un user
    int occ_sock[9000];
    for(l = 0; l <= 9000; l++) {
    	occ_sock[l] = 0;
    }

    while (1) {

		//Primire date socket tcp

		tmp_fds = read_fds; 

		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
		
		if (FD_ISSET(0, &tmp_fds)) {
			int t;
			//citesc de la tastatura
		    memset(buffer, 0 , BUFLEN);
		    fgets(buffer, BUFLEN-1, stdin);
		    
		    //Serverul primeste comanda quit
		    if (strncmp(buffer, "quit", 4) == 0) {
		        for (t = 4; t <= fdmax; t++) {
		        	//trimit mesaj la clienti
			        n = send(t, "Server-ul se va inchide", 24, 0);
			        if (n < 0)
			          	error("ERROR writing to socket");
		        }
	        	close(sockfd);
	    	   	return 0;
	        }
	    }

		for(i = 0; i <= fdmax; i++) {

			if (FD_ISSET(i, &tmp_fds)) {

				if (i == sockfd) {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}

					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", 
						inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}
					
				else {
					//am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("server: socket %d hung up\n", i);
						} else {
							error("ERROR in recv");
						}
						close(i); 
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul
					} 
					
					else { //recv intoarce >0

						printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
						char command[6] = "";
						strncat(command, buffer, 5);
						
						int k;
						int t_card = 0;
						int t_pin = 0;
						int no_cl, trans_cl, block;

						for (k = 0; k < no_users; k++) {
							if (sock[k] == i) {
								no_cl = k;
							}
						}
						//Serverul primeste comanda de login
						if (strcmp(command, "login") == 0) {

							//daca pe socket-ul curent nu este logat niciun user, putem incerca sa ne logam
							if (occ_sock[i] == 0) {

								for (k = 0; k < no_users; k++) {
									char card[7], no_pin[5];
									sprintf(card, "%d", data[k].card_no);
									sprintf(no_pin, "%d", data[k].pin);

									//se verifica daca numarul de card este valid
									if (strncmp(buffer + 6, card, 6) == 0) {
										t_card = 1;
										block = k;
										//se verifica daca pinul este corect
										if (strncmp(buffer + 13, no_pin, 4) == 0) {
											no_cl = k;
											t_pin = 1;
											//daca pinul este corect se verifica daca cardul este blocat
											if (locked[no_cl].loged <= -3) {
												send(i, "IBANK> : -5 Card blocat\n", 25, 0);
											//daca cardul nu este blocat userul se poate loga
											} else {
												locked[k].socket = i;
												locked[k].loged = 1;
												log[k]++;
												occ_sock[i]++;	//clientul este logat pe socketul i
											}
											
										//daca pinul este gresit se scade 1 din loged pentru a marca operatia nereusita	
										} else {
											int t;

											/*daca se incearca logarea cu alt cont pe socketul curent, se reseteaza 
												loged pentru celelalte conturi*/
											for (t = 0; t < no_users; t++) {
												if (t != block && locked[t].socket == i && locked[t].loged > -3) {
													locked[t].loged = 0;
												}
											}	

											locked[k].socket = i;
											locked[k].loged--;

											//daca s-a gresit pinul de 3 ori consecutiv se blocheaza cardul
											if (locked[k].loged <= -3) {
												send(i, "IBANK> : -5 Card blocat\n", 25, 0);
											}
											
										}
									}
									
								}
								//printf("loged %d", locked[block].loged);
								//daca numarul de card este gresit cod de eroare -4
								if (t_card == 0) { 
									send(i, "IBANK> -4 : Numar card inexistent\n", 35, 0);
								//daca pinul este gresit si cardul nu este blocat cod de eroare -3
								} else if (t_pin == 0 && locked[block].loged > -3) {
									send(i, "IBANK> -3 : Pin gresit\n", 24, 0);
								//daca clientul este deja logat cod de eroare -2
								} else if (log[no_cl] > 1) {
									send(i, "IBANK> -2 : Sesiune deja deschisa\n", 36, 0);
								//daca credentialele sunt corecte clientul este logat
								} else if (t_card == 1 && t_pin == 1 && locked[no_cl].loged > -3) {
									char buf[40];
									sock[no_cl] = i;
									sprintf(buf, "IBANK:> Welcome %s %s\n", 
										data[no_cl].lastname, data[no_cl].firstname);
									send(i, buf, strlen(buf), 0);
								}

							//Pe socket-ul curent este logat deja un user
							} else {
								send(i, "IBANK> -2 : Sesiune deja deschisa\n", 36, 0);
							}
						}

						//Serverul primeste comada de logout
						if (strcmp(command, "logou") == 0) {

							//daca userul este logat pe socketul curent acesta este deconectat
							if (log[no_cl] == 1 && occ_sock[i] == 1) {
								log[no_cl] = 0;
								occ_sock[i] = 0;
								sock[no_cl] = 0;
								locked[no_cl].socket = 0;
								locked[no_cl].loged = 0;
								send(i, "IBANK> Clientul a fost deconectat\n", 35, 0);
							//daca userul nu este logat cod de eroare -1
							} else {
								send(i, "IBANK> -1 : Clientul nu a fost autentificat\n", 44, 0);
							}
						}

						//Serverul primeste comanda listsold
						if (strcmp(command, "lists") == 0) {

							//daca userul este logat se afiseaza soldul
							if (log[no_cl] == 1 && occ_sock[i] == 1) {
								char buff[40];
								sprintf(buff, "IBANK> %.2f\n", data[no_cl].sold);
								send (i, buff, strlen(buff), 0);
							//daca userul nu este logat cod de eroare -1
							} else {
								send(i, "IBANK> -1 : Clientul nu a fost autentificat\n", 44, 0);
							}
						}

						//userul curent doreste sa transfere bani
						if (trans[no_cl] == 1) {

							//daca primeste o comanda ce incepe cu y trensferul este realizat cu succes
							if (strncmp(command, "y", 1) == 0) {
								//se scade suma de transferat din contul lui
								data[no_cl].sold -= suma_trans;
								//se aduna suma de transferat la clientul la care se face transferul
								data[trans_cl].sold += suma_trans;
								trans[no_cl] = 0;
								send(i, "IBANK> Transfer realizat cu succes\n", 36, 0);
							//daca comanda nu incepe cu y cod de eroare -9
							} else {
								trans[no_cl] = 0;
								send(i, "IBANK> -9 : Operatie anulata\n", 30, 0);
							}
						}
						
						//Serverul primeste comanda transfer
						if (strcmp(command, "trans") == 0) {
							
							for (k = 0; k < no_users; k++) {
								if (sock[k] == i) {
									no_cl = k;
								}
							}

							int no_card = 0;
							char card[7], temp_card[7];

							//se verifica daca userul a fost autentificat
							if (log[no_cl] == 1 && occ_sock[i] == 1) {

								for (k = 0; k < no_users; k++) {
									sprintf(temp_card, "%d", data[k].card_no);

									//se verifica daca numarul de card este valid
									if (strncmp(buffer + 9, temp_card, 6) == 0) {
										sprintf(card, "%d", data[k].card_no);
										no_card = data[k].card_no;
										trans_cl = k;
									}
								}

								//numarul de card este valid
								if (no_card != 0) {

									//se verifica daca fondurile sunt suficiente
									if (atof(buffer + 16) < data[no_cl].sold) {
										suma_trans = atof(buffer + 16);
										transfer = 1;
										trans[no_cl] = 1;
										char bufff[200];
										//se cere confirmarea transferului
										sprintf(bufff, "IBANK> Transfer %s catre %s %s? [y/n]", 
											buffer + 16, data[trans_cl].lastname, data[trans_cl].firstname);						
										send(i, bufff, strlen(bufff), 0);
									//fonduri insuficiente
									} else {
										send(i, "IBANK> -8 : Fonduri insuficiente\n", 33, 0);
									}
								//numarul de card nu este valid
								} else {
									send(i, "IBANK> -4 : Numar card inexistent\n", 35, 0);
								}
							//pe clientul curent nu este autentificat niciun user
							} else {
								send(i, "IBANK> -1 : Clientul nu a fost autentificat\n", 44, 0);
							}
						}

						//daca clientul vrea sa se inchida este scos file descriptorul din multimea de citire
						if (strncmp(command, "quit", 4) == 0){
							FD_CLR(i, &read_fds);
						}
					}
				} 
			}
		}
    }

    //se inchide socketul
    close(sockfd);

	return 0;
}