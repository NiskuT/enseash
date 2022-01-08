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
// Nous commençons par creer un tableau contenant le nom de toutes nos fonctions built in ( il y en a qu'une ...)

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
	// La fonction fortune prend maintenant en argument argc et argv que nous allons utiliser pour des utilisations plus complexes

	if(argc==1){ // Le premier argument est toujours le nom de la fonction ("fortune"). Dans ce cas on fait un affichage classique
		for(int k=0;k<10000000;k++) for(int k=0;k<10;k++);
		write(1, "Today is what happened to yesterday.\n", 37);
		exit(0);
	}
	// Si il y a trois arguments et que le deux et le trois sont -s osfortune alors on est dans le cas ci-dessous
	else if (argc==3 && !strcmp(argv[1],"-s") && !strcmp(argv[2],"osfortune")){

		write(1, "However, complexity is not always the enemy.\n", 45);
		write(1, "  -- Larry Wall (Open Sources, 1999 O'Reilly and Associates)\n", 61);
		exit(0);
		
	}
	else { // Sinon l'utilisation n'est pas correcte
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
	// La fonction d'execution commence par appeler la fonction parse qui va renvoyer un tableau de string contenant tout les arguments
	// (qui étaient séparés par un espace) et en derniere case du tableau: NULL

	//char* command[] = {"ls","-la",NULL};
	// Ci dessus se trouve un exemple de ce que pourrait contenir command

	fpid = fork();

	if(fpid==-1) perror("Fork impossible"); // En cas d'erreur lors du fork on en informe l'utilisateur

	else if (fpid>0){ // CODE DU PERE
		waitpid(fpid,&status,0);
		freeParse(command);
		// Le tableau de string command étant alloué dynamiquement il faut le libérer à l'aide de cette fonction qui se trouve plus bas dans le code
		kill(fpid, SIGTERM);
	}
	else{ // CODE DU FILS

		int numCom=-1;
		for(int i = 0; i<(int)(sizeof(builtInFct)/sizeof(char*)); i++) if(!strcmp(builtInFct[i], command[0])) numCom=i;
		// La ligne ci-dessous parcours le tableau builtInFct et compare les strings a command[0] qui contient le nom de la fonction
		// Si à la fin numCom de vaut plus -1 alors la fonction est built IN
		
		switch(numCom){
			case 0: // Fonction built in zéro (fortune)
				fortune(count(command), command);
				break;

			default: // Sinon on est dans le cas d'une fonction non built in
				if(execvp(command[0],command)==-1) perror("Erreur d'execution de la commande");
				// Pour executer la comand nous utilisons execvp
				// v : les arguments sont passés en une fois dans un (char **) dont le dernier élément est (char*)NULL
				// p : l’executable donné en 1er est recherché dans le PATH
				// La fonction exec va remplacer le code du processus courant par celui d’un programme compilé présent sur le système de fichier.
				
				exit(EXIT_FAILURE);// Si nous arrivons à cette endroit il y a donc eu une erreur
				break;
		}
	}

	return status;
}

int count(char** command){

	int i =0;
	while(command[i]) i++;
	return i;
	// La fonction count parcours le tableau de string recu en paramètre jusqu'à rencontrer NULL
	// elle renvoie l'iindex auquel elle l'a rencontré
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
	// Cette commande recoit en entrée une chaine de caractère et renvoie un tableau de chaine de caractère

	char** tabString=NULL;
	char* mot = NULL;
	int numArgument = 0;
	short first = 1;

	command[strlen(command)-1]=' '; // tout d'abord nous placons un espace à la place du \n de fin car sinon les fonctions aurons une erreur d'arguments

	do{
		if(first){ mot=strtok(command," "); first=0;} // La fonction strtok renvoie un pointeur sur la chaine de caractère allant jusqu'au délimiteur "espace"
		else mot=strtok(NULL," "); // Lors du premier appel cette fonction prend en argument la chaine de caractère puis NULL les fois suivantes
		// La fonction renverra un pointeur NULL lorsqu'elle n'aura plus de token a renvoyer
		// Il est important de savoir que cette fonction modifie la chaine de caractère command

		tabString=(char**) realloc(tabString, (numArgument+1)*sizeof(char*));
		
		/*Cette fonction permet de réallouer un bloc de mémoire dans le tas (le heap en anglais). 
		Cela veut dire que si l'espace mémoire libre qui suit le bloc à réallouer est suffisament grand, 
		le bloc de mémoire est simplement agrandi. Par contre si l'espace libre n'est pas suffisant, un nouveau 
		bloc de mémoire sera alloué, le contenu de la zone d'origine recopié dans la nouvelle zone et 
		le bloc mémoire d'origine sera libéré automatiquement. */

		// Pour chaque nouvel argument nous réallouons donc de la mémoire pour contenir un pointeur sur un char

		if(mot){
			// Si nous n'en sommes pas au dernier argument nous allouons de la mémoire pour contenir le string correspondant à l'argument
			tabString[numArgument]=(char*) calloc(sizeof(char),strlen(mot));
			strcpy(tabString[numArgument],mot); // Nous utilisons ensuite strcpy pour copier le string dans la mémoire allouée
		}
		else{
			// Si il s'agit du dernier argument nous n'allouons pas de mémoire mais nous placons simplement NULL dans cette cernière case
			tabString[numArgument]=NULL;
		}
		numArgument++;

	}while(mot);

	return tabString;

}

void freeParse(char** tabString){
	int idx = 0;
	while(tabString[idx]!=NULL) {
		// Cette fonction commence par parcourir le tableau de string alloué dynamiquement, et libérer chacun d'entre eux
		free(tabString[idx]);
		idx++;
	}
	// à la fin il faut également liberer le tableau lui même
	free(tabString);
}