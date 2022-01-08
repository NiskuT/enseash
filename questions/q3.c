#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


#define bufferSize 2048

void fortune();
void goOut();
void handlerSigint();


int	main()
{
	(void) signal(SIGINT, handlerSigint);
	// Cette ligne permet de relier le signal généré par ctrl+C au handler sigint qui pour l'instant ne fait rien.
	// On peut donc entrer ctrl+c dans notre shell sans fermer celui-ci
	// Nous pourrons par la suite relier d'autres signaux si nous le souhaitons

	ssize_t lecture; // Utilisé lors de la lecture

	char *buffer = NULL;
	buffer = (char *)calloc(sizeof(char), bufferSize); // Allocation de la mémoire pour le buffer de lecture
	if (buffer == NULL) {
		perror("Allocation failure");
		return (EXIT_FAILURE);
	}


	write(1, "Bienvenue dans le Shell ENSEA.\n", 31);
	write(1, "Pour quitter, tapez 'exit'.\n", 29);
	write(1, "enseash % ", 10);


	while((lecture=read(1,buffer,bufferSize))>0){ // read renvoie -1 si il y a une erreur, sinon le nombre d'octets lus

		if(!strncmp("exit", buffer,4) && lecture==5) goOut();
		// Si l'utilisateur à entrée exit alors on execute la fonction goOut() qui à l'aide d'un appel à close(1) ferme le tuyau stdin

		// Ctrl+D n'est pas un signal, c'est EOF (End-Of-File). Il ferme le tuyau stdin.
		// En effet quand on fait un ctrl+D cela a le même effet que exit, c'est a dire sortir du while;

		if(!strncmp("fortune", buffer,7) && lecture==8) fortune();


		write(1, "enseash % ", 10);

	}

	free(buffer);
	write(1, "\n\nBye bye...\n", 13);
	
}


void fortune(){
	write(1, "Today is what happened to yesterday.\n", 37);
}

void goOut(){
	// La fonction goOut fait appel à la fonction close qui ferme le descripteur de fichier et donc interrompt le while
	write(1, "\n\nBye bye...\n", 13);
	close(1);
}

void handlerSigint(){
	// Pour l'instant il ne se passe rien lors d'un ctrl+C
}