#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>


#define bufferSize 2048

void fortune();
void goOut();
void handlerSigint();
int execution(char** command, int inputfd[2]);
char* convertTime(double diff);
char** parse(char* command);
void freeParse(char** tabString);
int count(char** command);
int isTherePipe(char** command);


/* INFORMATIONS

Nous avons fait deux versions qui sont toutes deux partiellements fonctionnelles concernant les pipes

Dans cette version nous avons obtenons un pipe fonctionnel pour certaines commandes ( celles n'attendants pas la fin de la commande précedente)

En effet la difficulté ici est que les deux commande doivent s'executer en meme temps. On utilise la fonction pipe() pour creer un tuyau reliant 
le stdout de la première commande sur la seconde. Le problème est que nous ne sommes pas parvennu à transmettre le signal de fin de la première commande 
à la suivante.
Pour cette version on obtient donc un resultat fonctionnel pour des commandes tels que "ls | grep a" ou "cat /dev/urandom | hexdump" mais pour des commandes 
attendant la fin de la précedente nous n'auront aucun résultat: c'est notamment le cas de "ls | sort"

Pour envoyer le signal de fin nous pourrons proceder à un ctrl+C qui reviendra correctement au prompt.

Cette version utilise une récurrence qui pourrait permettre dans une version finie de gérer plusieurs pipe.

*/

const char* builtInFct[] = {"fortune"};
pid_t theson;

int	main()
{
	(void) signal(SIGINT, handlerSigint);

	ssize_t lecture; // Utilisé lors de la lecture

	struct timespec start={0,0};
	struct timespec stop={0,0};
	int firstCmd[2] = {-1,-1};

	char *buffer = NULL;

	buffer = (char *)calloc(sizeof(char), bufferSize);
	if (buffer == NULL) {
		perror("Allocation failure");
		return (EXIT_FAILURE);
	}




	write(1, "Bienvenue dans le Shell ENSEA.\n", 31);
	write(1, "Pour quitter, tapez 'exit'.\n", 29);
	write(1, "enseash % ", 10);


	while((lecture=read(1,buffer,bufferSize))>0){ 
		clock_gettime(CLOCK_MONOTONIC, &start);

		int status =0;

		char prompt[50] = "enseash ";
		char retour[10];

		if(!strncmp("exit", buffer,4) && lecture==5) goOut();

		else if (lecture>1){
			status=execution(parse(buffer), firstCmd);

			clock_gettime(CLOCK_MONOTONIC, &stop);
			double diff = (stop.tv_sec - start.tv_sec) * 1e9;
    		diff = (diff + (stop.tv_nsec - start.tv_nsec)) * 1e-9;


			if(WIFEXITED(status)) {
				strcpy(retour,"[exit:   ");
				if(WEXITSTATUS(status)>=100) retour[6]=(WEXITSTATUS(status)/100)+'0';
				if(WEXITSTATUS(status)>=10) retour[7]=((WEXITSTATUS(status)/10)%10)+'0';
				retour[8]=(WEXITSTATUS(status)%10)+'0';
				
			}
			else if (WIFSIGNALED(status)){
				strcpy(retour,"[sign:   ");
				if(WTERMSIG(status)>=100) retour[6]=(WTERMSIG(status)/100)+'0';
				if(WTERMSIG(status)>=10) retour[7]=((WTERMSIG(status)/10)%10)+'0';
				retour[8]=(WTERMSIG(status)%10)+'0';
			}

			strcat(prompt, retour);

			if(diff<=10e-3) {
				strcat(prompt, "] ");
			}
			else {
				char* res = convertTime(diff);	
				strcat(prompt,res);	
				free(res);
			}

		}

		strcat(prompt,"% ");
		write(1, prompt, strlen(prompt));
		memset(buffer,0,bufferSize);


	}

	free(buffer);
	write(1, "\n\nBye bye...\n", 13);
	
}


void fortune(int argc, char *argv[]){
	if(argc==1){
		for(int k=0;k<10000000;k++) for(int k=0;k<10;k++);
		write(1, "Today is what happened to yesterday.\n", 37);
		exit(0);
	}
	else if (argc==3 && !strcmp(argv[1],"-s") && !strcmp(argv[2],"osfortune")){

		write(1, "However, complexity is not always the enemy.\n", 45);
		write(1, "  -- Larry Wall (Open Sources, 1999 O'Reilly and Associates)\n", 61);
		exit(0);
		
	}
	else {
		write(1, "Mauvaise utilisation de la commande.\n", 37);
		exit(0);
	}

	
}

void goOut(){

	write(1, "\n\nBye bye...\n", 13);
	close(1);
}

void handlerSigint(){
	// Pour l'instant il ne se passe rien lors d'un ctrl+C
}

int execution(char** command, int inputfd[2]){
	
	pid_t fpid =0;
	int status = 0;


	// DEBUT CODE PIPE 1

	// Ci dessous se trouve le début du code utiliser pour la création du pipe.
	// Nous avons modifié le prototype de la fonction pour faire passer en argument les files descriptor du pipe
	int pipefd[2] = {0,0};
	// Ce tableau est envoyé à pipe() qui renvoie deux files descriptor: un pour la lecture et le second pour l'écriture

	char** cmd1;
	// Ce tableau sera utilisé pour contenir la première commande en cas de pipe

	int pipeN = isTherePipe(command);
	// La commande isTherePipe renvoie -1 si aucun pipe n'est détecté, sinon l'index du pipe

	if(pipeN>=0){
		command[pipeN]=NULL;
		cmd1 = command;
		command = command+pipeN+1;
		// La commande devient la deuxième commande (qui succède à |) et cmd1 contient la première commande
		
		if(pipe(pipefd)!=0) perror("Pipe impossible");
		// On appel pipe() qui crée un tuyau anonyme
	}
	// FIN CODE PIPE 1


	fpid = fork();
	if(inputfd[0]!=-1) theson=fpid;
	// Si jamais le file decriptor en entrée est -1, on est dans le cas de l'execution de la commande 2, alors on met fpid dans une variable globale

	if(fpid==-1) perror("Fork impossible"); // En cas d'erreur lors du fork on en informe l'utilisateur

	else if (fpid>0){ //  CODE DU PERE
		// DEBUT CODE PIPE 3

		if(pipeN!=-1) {
			// Si on arrive ici, on est dans le code du pere et il y a un pipe dans la commande entrée par l'utilisateur
			// On fait donc appel a execution avec la première commande, et les files descriptor du pipe
			// Cela doit permettre une recurrence dans le cas ou il y a plusieurs pipe
			execution(cmd1, pipefd);
		}
		if(inputfd[0]!=-1){
			/*pid_t fils =*/ wait(&status);
			// Si on arrive ici on est dans le cas du pere executant la commande 1, 
			// On attend la terminaison d'un fils quelconque et on envoie un signal de fin à la deuxième commande (ou la première selon différents essais)
			// Malheuresement cette partie ne fonctionne pas comme nous le souhaiterions

			if(WIFEXITED(status)/* && fils != theson*/) {
				//kill(fils, SIGQUIT);//WEXITSTATUS(status));
				kill(theson, SIGQUIT);
				close(inputfd[0]);
				close(inputfd[1]);
				// Ces deux fonctions close visent à fermer le tuyau anonyme
			}
		}

		// FIN CODE PIPE 3


		waitpid(fpid,&status,0);

		freeParse(command);
		if(pipeN==-1) free(command);
		// On a modifier le code de freeParse, en effet celui-ci s'arretant au premier argument null, il faut l'appeler sur les deux commandes
		// Cependant le tableau ne doit etre free qu'une fois

		kill(fpid, SIGTERM);

		
	}
	else{ // CODE DU FILS

		short inp = 0;
		for(int k = 0; k< count(command);k++){
			if(!strcmp(command[k], "<")){
				FILE* fichier= fopen(command[k+1], "r");
				dup2(fileno(fichier),STDIN_FILENO);
				fclose(fichier);
				inp=1;
			}
			else if(!strcmp(command[k], ">")){
				FILE* fichier= fopen(command[k+1], "w");
				dup2(fileno(fichier),STDOUT_FILENO);
				fclose(fichier);
				inp=1;
			}
			if(inp) { 
				free(command[k]);
				command[k]=NULL; 
			}
		}

		// DEBUT CODE PIPE 2
		// A cette endroit, selon le meme principe que pour les redirection, nous "branchons" la sortie de la commande 1 sur le pipe
		// et l'entrée de la commande 2 sur le pipe
		if(inputfd[1]!=-1){
			dup2(inputfd[1],STDOUT_FILENO);
		}

		if(pipeN!=-1){
			dup2(pipefd[0],STDIN_FILENO);
		}
		// FIN CODE PIPE 2
		

		int numCom=-1;
		for(int i = 0; i<(int)(sizeof(builtInFct)/sizeof(char*)); i++) if(!strcmp(builtInFct[i], command[0])) numCom=i;

		
		switch(numCom){
			case 0: 
				fortune(count(command), command);
				break;

			default:
				if(execvp(command[0],command)==-1) perror("Erreur d'execution de la commande");
				exit(EXIT_FAILURE);
				break;
		}
	}

	return status;
}

int isTherePipe(char** command){
	int i = 0;
	while(command[i]) {
		if (!strcmp(command[i], "|")) return i;
		i++;
	}
	return -1;
}

int count(char** command){

	int i =0;
	while(command[i]) i++;
	return i;
}

char* convertTime(double diff){

	const int CHAR_BUFF_SIZE = 20;
	
	char *p = NULL;
	char *finalRes = NULL;
	p = (char *)calloc(sizeof(char), CHAR_BUFF_SIZE);

	char *s = p + CHAR_BUFF_SIZE;

	int nbChar=1;
	
	if(diff>1){

		*--s = 's';
		int units=(int) diff;			

		while (units > 0) {
        	*--s = (units % 10) + '0';
        	units /= 10;
        	nbChar++;
    	}
	}
	else{
		diff*=1000;

		*--s = 's';
		*--s = 'm';
		nbChar++;
		int units=(int) diff;			

		while (units > 0) {
        	*--s = (units % 10) + '0';
        	units /= 10;
        	nbChar++;
    	}

	}
	// "enseash [" -> 9 caractères
	finalRes = (char *)calloc(sizeof(char), 3+nbChar);
	finalRes[0] = '|';

	for(int i = 1;i<1+nbChar;i++){
		finalRes[i]=*s;
		++s;
	}

	finalRes[nbChar+1]=']';
	finalRes[nbChar+2]=' ';  

	free(p);
	return finalRes;
}

char** parse(char* command){

	char** tabString=NULL;
	char* mot = NULL;
	int numArgument = 0;
	short first = 1;

	command[strlen(command)-1]=' ';

	do{
		if(first){ mot=strtok(command," "); first=0;}
		else mot=strtok(NULL," ");

		tabString=(char**) realloc(tabString, (numArgument+1)*sizeof(char*));

		if(mot){
			tabString[numArgument]=(char*) calloc(sizeof(char),strlen(mot));
			strcpy(tabString[numArgument],mot);
		}
		else{
			tabString[numArgument]=NULL;
		}
		numArgument++;

	}while(mot);

	return tabString;

}

void freeParse(char** tabString){
	int idx = 0;
	while(tabString[idx]!=NULL) {
		free(tabString[idx]);
		idx++;
	}
}