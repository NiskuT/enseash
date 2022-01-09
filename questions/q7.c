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
int execution(char *command);
char* convertTime(double diff);
char** parse(char* command);
void freeParse(char** tabString);
int count(char** command);

const char* builtInFct[] = {"fortune"};

int	main()
{
	(void) signal(SIGINT, handlerSigint);

	ssize_t lecture; 

	struct timespec start={0,0};
	struct timespec stop={0,0};

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
			status=execution(buffer);
			// Appel à la fonction d'execution qui utilise fork pour executer la commande.

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

int execution(char *cmd){
	
	pid_t fpid =0;
	int status = 0;

	char** command = parse(cmd);


	fpid = fork();

	if(fpid==-1) perror("Fork impossible"); // En cas d'erreur lors du fork on en informe l'utilisateur

	else if (fpid>0){ // CODE DU PERE
		waitpid(fpid,&status,0);
		freeParse(command);
		kill(fpid, SIGTERM);
	}
	else{ // CODE DU FILS

		// DEBUT CODE REDIRECTION
		short inp = 0; // inp est un nombre qui sera mis à 1 si une redirection quelconque est detecté

		for(int k = 0; k< count(command);k++){ // On parcours les différents arguments de la commande
			
			if(!strcmp(command[k], "<")){ // Si on detecte < alors on ouvre le fichier dont le nom est l'argument suivant
				FILE* fichier= fopen(command[k+1], "r"); // Ouverture du fichier en mode lecture, nous obtiendrons le descripteur de fichier à l'aide de fileno()
				dup2(fileno(fichier),STDIN_FILENO); // La fonction dup2 va copier le descripteur de fichier recu en premier argument sur le second
				// Cela signifie que le contenu du fichier sera redirigé dans STDIN
				// Lorsqu'on sort du fork, le descripteur de fichier du pere correspondant à STDIN et STOUT n'a pas été changé et on reviens à un affichage classique
				fclose(fichier); // On peut ensuite fermer le fichier
				inp=1;
			}
			else if(!strcmp(command[k], ">")){
				FILE* fichier= fopen(command[k+1], "w"); // Cette fois ci on ouvre le fichier en mode ecriture
				dup2(fileno(fichier),STDOUT_FILENO); // La fonction dup2 va copier le descripteur de fichier renvoyé par fileno() sur STDOUT
				// Cela signifie que lorsque les fonctions renvoie du texte sur STDOUT, celui-ci sera en réalité écrit dans le fichier
				// Lorsqu'on sort du fork, le descripteur de fichier du pere correspondant à STDIN et STOUT n'a pas été changé et on reviens à un affichage classique
				fclose(fichier);
				inp=1;
			}
			if(inp) { // si une redirection est detecté, alors pour toutes les cases à partir du < ou > on libère la mémoire et on place à NULL
				free(command[k]);
				command[k]=NULL; 
			}
		}
		// FIN CODE REDIRECTION



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
	free(tabString);
}
