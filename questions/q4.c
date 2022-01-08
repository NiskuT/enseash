#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>


#define bufferSize 2048

void fortune();
void goOut();
void handlerSigint();
int execution(char *command);


int	main()
{
	(void) signal(SIGINT, handlerSigint);

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


	while((lecture=read(1,buffer,bufferSize))>0){ 

		//Creation d'une variable status pour connaitre le retour de la commande saisie
		int status =0;

		char retour[20] = "enseash [    :  ] % ";
		// initialisation du tableau a afficher pour le prompt

		if(!strncmp("exit", buffer,4) && lecture==5) goOut();

		else if (lecture>1){
			status=execution(buffer);
			// Appel à la fonction d'execution qui utilise fork pour executer la commande.
			// La fonction renvoie un entier correspondant au statut du processus créé


			if(WIFEXITED(status)) {
				// Si jamais le statut correspond à un exit
				// Par la suite cet affichage sera amélioré pour mieux correspondre aux besoins et 
				// prendre en compte un code de retour allant jusqu'à 255
				retour[9]='e';retour[10]='x';retour[11]='i';retour[12]='t';
				if(WEXITSTATUS(status)>=10) retour[14]=(WEXITSTATUS(status)/10)+'0'; // En additionnant +'0' à la fin on ajoute le chiffre désiré aux code ascii de 0
				// Comme les chiffres se suivent dans la table ascii on obtient donc le code ascii du chiffre que l'on place dans la case adéquate du tableau retour
				retour[15]=(WEXITSTATUS(status)%10)+'0';
				
			}
			else if (WIFSIGNALED(status)){
				// Si jamais le statut correspond à un sign
				retour[9]='s';retour[10]='i';retour[11]='g';retour[12]='n';
				if(WTERMSIG(status)>=10) retour[14]=(WTERMSIG(status)/10)+'0';
				retour[15]=(WTERMSIG(status)%10)+'0';
			}
			write(1, retour, 20);
		}
		
		// Si aucune commande n'a été executé on affiche le prompt classique ci dessous
		else write(1, "enseash % ", 10);

	}

	free(buffer);
	write(1, "\n\nBye bye...\n", 13);
	
}


void fortune(){
	write(1, "Today is what happened to yesterday.\n", 37);
	// L'appel à exit(0) est primordiale pour ne pas que le pere reste bloqué
	exit(0);
}

void goOut(){
	write(1, "\n\nBye bye...\n", 13);
	close(1);
}

void handlerSigint(){
	// Pour l'instant il ne se passe rien lors d'un ctrl+C
}

int execution(char *command){

	// La fonction execution recoit en paramètre un string

	// Cette fonction va executer la commande entrée dans un nouveau processus
	
	pid_t fpid =0;
	int status = 0;

	fpid = fork();

	if(fpid==-1) perror("Fork impossible"); // En cas d'erreur lors du fork on en informe l'utilisateur

	else if (fpid>0){ // CODE DU PARENTS
		waitpid(fpid,&status,0);
		kill(fpid, SIGTERM);
	}
	else{ // CODE DU FILS
		if(!strncmp("fortune", command,7)) fortune();

		// L'appel à exit(1) ci dessous permet au pere de passer la ligne waitip();
		// On a également pris soin de rajouter un exit(0) dans la fonction fortune pour ne pas etre "bloqué"
		else exit(1);

	}
	return status;

}