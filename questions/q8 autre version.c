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

const char* builtInFct[] = {"fortune"};

/* INFORMATIONS

Nous avons fait deux versions qui sont toutes deux partiellements fonctionnelles concernant les pipes
VOIR D'ABORD L'AUTRE VERSION

Dans cette version nous avons obtenons un retour au prompt correct mais sans l'affichage de la commande utilisant un pipe

Plutot qu'une recurrence nous utilisons ici deux fork(). Cela limite donc le nombre de pipe à 1.

*/

int	main()
{
	(void) signal(SIGINT, handlerSigint);

	ssize_t lecture;

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

	inputfd[0]=-1; // Nous placons ici -1 pour ne pas avoir d'erreur de unused parameter car nous n'utilisons pas dans cette version un argument pour
	// la transmission des fd

	// DEBUT CODE PIPE 1
	int pipefd[2] = {0,0};
	char** cmd1;

	int pipeN = isTherePipe(command);

	if(pipeN>=0){
		command[pipeN]=NULL;
		cmd1 = command;
		command = command+pipeN+1;
		
		if(pipe(pipefd)!=0) perror("Pipe impossible");
	} // Jusqu'ici le fonctionnement est le même que la première version

	// FIN CODE PIPE 1


	fpid = fork(); // on fork une première fois

	if(fpid==-1) perror("Fork impossible"); // En cas d'erreur lors du fork on en informe l'utilisateur

	else if (fpid>0){ //  CODE DU PERE
		
		// DEBUT CODE PIPE 3
		if(pipeN!=-1) {
			// Si jamais il y a un pipe on fork une seconde fois dans le pere
			pid_t fils = fork();

			if(fpid==-1) perror("Fork impossible");
			
			else if(fils>0){ 
				waitpid(fils, &status, WUNTRACED);
				kill(fils, SIGTERM);
				// On attend la terminaison du fils qui execute la commande 1
			}
			else{
				// Nous sommes ici dans le cas du fils executant la première commande, nous "branchons" donc la sortie de celle ci sur le tuyau anonyme
				dup2(pipefd[1],STDOUT_FILENO);
				close(pipefd[1]);
	        	close(pipefd[0]);

				execvp(cmd1[0],cmd1);
				// On appel execvp pour executer la première commande
			}

		}

		
		// FIN CODE PIPE 3

		waitpid(fpid,&status,0);

		//freeParse(command);
		//if(inputfd[0]==-1) free(command);

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

		if (pipeN!=-1){
			// A cet endroit nous sommes dans le cas du fils executant la seconde commande, nous utilisons donc le descripteur de fichier de lecure du pipe
			// pour le coller sur le descripteur de fichier de stdin
			dup2(pipefd[0],STDIN_FILENO);
			close(pipefd[1]);
			close(pipefd[0]);
		}

		
		// FIN CODE PIPE 2
		

		int numCom=-1;
		for(int i = 0; i<(int)(sizeof(builtInFct)/sizeof(char*)); i++) if(!strcmp(builtInFct[i], command[0])) numCom=i;

		
		switch(numCom){
			case 0: 
				fortune(count(command), command);
				break;

			default:
				write(1, "HEHOOO\n", 7);
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

	//for(int i = 1+nbChar; i<1+nbChar+3;i++) finalRes[i] = "] "[i-(3+nbChar)];

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