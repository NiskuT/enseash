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

int	main()
{
	(void) signal(SIGINT, handlerSigint);
	// Cette ligne permet de relier le signal généré par ctrl+C au handler sigint qui pour l'instant ne fait rien.
	// On peut donc entrer ctrl+c dans notre shell sans fermer celui-ci

	ssize_t lecture; // Utilisé lors de la lecture

	struct timespec start={0,0};
	struct timespec stop={0,0};
	int firstCmd[2] = {-1,-1};

	char *buffer = NULL;
	// void* calloc(size_t num_elements, size_t size) est le prototype de calloc (identique à malloc avec en plus initilalisation à 0)
	// On utilise cette fonction sur des char aux nombres de bufferSize. On verifie par la suite que l'allocation c'est bien passée.
	buffer = (char *)calloc(sizeof(char), bufferSize);
	if (buffer == NULL) {
		// Si le buffer est toujours NULL après allocation avec calloc alors il y a eu une erreur
		perror("Allocation failure");
		return (EXIT_FAILURE);
	}




	write(1, "Bienvenue dans le Shell ENSEA.\n", 31);
	write(1, "Pour quitter, tapez 'exit'.\n", 29);
	write(1, "enseash % ", 10);


	while((lecture=read(1,buffer,bufferSize))>0){ // read renvoie -1 si il y a une erreur, sinon le nombre d'octets lus
		clock_gettime(CLOCK_MONOTONIC, &start);

		//Creation d'une variable status pour connaitre le retour de la commande
		int status =0;

		char prompt[50] = "enseash ";
		char retour[10];

		if(!strncmp("exit", buffer,4) && lecture==5) goOut();
		// Ctrl+D n'est pas un signal, c'est EOF (End-Of-File). Il ferme le tuyau stdin.
		// En effet quand on fait un ctrl+D cela a le même effet que exit, c'est a dire sortir du will;

		else if (lecture>1){
			status=execution(parse(buffer), firstCmd);
			// Appel à la fonction d'execution qui utilise fork pour executer la commande.

			clock_gettime(CLOCK_MONOTONIC, &stop);
			double diff = (stop.tv_sec - start.tv_sec) * 1e9;
    		diff = (diff + (stop.tv_nsec - start.tv_nsec)) * 1e-9;


    		// Nous nous sommes rendu compte que la valeur maximale de WEXITSTATUS(status) était 255
    		// Nous avons donc ajusté l'affichage pour afficher jusqu'à 3 décimal
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
	// La fonction goOut fait appel à la fonction close qui ferme le descripteur de fichier et donc interrompt le while
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
	int pipefd[2] = {0,0};
	char** cmd2;

	int pipeN = isTherePipe(command);

	if(pipeN>=0){
		command[pipeN]=NULL;
		cmd2 = command+pipeN+1;
		if(pipe(pipefd)!=0) perror("Pipe impossible");
	}
	// FIN CODE PIPE 1


	fpid = fork();

	if(fpid==-1) perror("Fork impossible"); // En cas d'erreur lors du fork on en informe l'utilisateur

	else if (fpid>0){
		write(1, "Waiting...\n", 11);
		waitpid(fpid,&status,0);
		write(1, "End Waiting...\n", 15);
		freeParse(command);
		kill(fpid, SIGTERM);
		write(1, "Check 1\n", 8);
		// DEBUT CODE PIPE 3
		if(pipeN!=-1) execution(cmd2, pipefd);
		// FIN CODE PIPE 3
	}
	else{

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
		if(inputfd[0]!=-1){
			dup2(inputfd[0],STDIN_FILENO);
			close(inputfd[0]);
			write(1, "Check 2\n", 8);
		}

		if(pipeN!=-1){
			dup2(pipefd[1],STDOUT_FILENO);
			close(pipefd[1]);
		}
		// FIN CODE PIPE 2
		

		int numCom=-1;
		for(int i = 0; i<(int)(sizeof(builtInFct)/sizeof(char*)); i++) if(!strcmp(builtInFct[i], command[0])) numCom=i;

		write(1, "ready exec\n", 11);
		
		switch(numCom){
			case 0: 
				fortune(count(command), command);
				break;

			default:
				write(1, "Start exec\n", 11);
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
	free(tabString);
}