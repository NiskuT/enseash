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


int	main()
{
	(void) signal(SIGINT, handlerSigint);

	ssize_t lecture; // Utilisé lors de la lecture

	// Les deux structure initialisées ci dessous vont permettre la mesure du temps et ainsi un chronometrage
	// Elles sont composées de deux variables : un time_t et un long, qui correspondent à un nombre de seconde et un nombre de nano-seconde
	struct timespec start={0,0};
	struct timespec stop={0,0};

	char *buffer = NULL;
	buffer = (char *)calloc(sizeof(char), bufferSize);  // Allocation de la mémoire pour le buffer de lecture
	if (buffer == NULL) {
		perror("Allocation failure");
		return (EXIT_FAILURE);
	}


	write(1, "Bienvenue dans le Shell ENSEA.\n", 31);
	write(1, "Pour quitter, tapez 'exit'.\n", 29);
	write(1, "enseash % ", 10);


	while((lecture=read(1,buffer,bufferSize))>0){
		clock_gettime(CLOCK_MONOTONIC, &start);
		// L'appel à la fonction clock_gettime permet de placer l'heure actuelle d'après l'horloge CLOCK_MONOTONIC dans la strucure start
		// CLOCK_MONOTONIC fait référence à une horloge qui ne peut être modifier et qui représente le temps depuis un point de départ non spécifié

		int status =0;

		char prompt[50] = "enseash "; // On initialise un tableau statique de 50 char pour afficher le prompt
		char retour[10]; // On initialise également un tableau pour le retour obtenu avec WEXITSTATUS() ou WTERMSIG()


		if(!strncmp("exit", buffer,4) && lecture==5) goOut();

		else if (lecture>1){
			status=execution(buffer);
			// Appel à la fonction d'execution qui utilise fork pour executer la commande.

			// Une nouvelle fois nous faisons appel à clock_gettime pour définir l'heure de fin de la commande
			clock_gettime(CLOCK_MONOTONIC, &stop);
			double diff = (stop.tv_sec - start.tv_sec) * 1e9;
    		diff = (diff + (stop.tv_nsec - start.tv_nsec)) * 1e-9;
    		// On peut ensuite calculer en faisant la différence le temps d'execution ici contenu dans la variable diff


			if(WIFEXITED(status)) {
				strcpy(retour,"[exit:   "); // MODIFICATION : On utilise desormais strcpy pour ecrire "[exit:   " dans le tableau retour
				// Le code ci dessous prends desormais en charge toutes les possibilités de retour
				// Nous nous sommes rendu compte que la valeur maximale de WEXITSTATUS(status) était 255
    			// Nous avons donc ajusté l'affichage pour afficher jusqu'à 3 décimal
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
			// Si on arrive ici alors une commande a été executé et a généré un signal de retour contenu dans le tableau retour
			// on utilise strcat qui permet de concatener le string retour à la fin du string prompt

			if(diff<=10e-3) { // Si jamais le temps d'execution est inférieur à UNE milliseconde alors on ne souhaite pas l'afficher
				strcat(prompt, "] "); // On se contente donc de fermer l'accolade une fois encore à l'aide de strcat
			}
			else { // Si le temps d'execution est supérieur à une milliseconde nous souhaitons l'afficher
				char* res = convertTime(diff);	// La fonction convertTime va se charger de formater le temps dans une chaine de caractère qu'elle renvoie
				strcat(prompt,res);	// On concatène donc le string renvoyé 
				free(res); // et on oublie pas de libérer la memoire qui à été allouée dynamiquement dans convertTime
			}

		}

		strcat(prompt,"% "); // Il ne manque plus qu'à ajouter à la fin le % et afficher le prompt
		write(1, prompt, strlen(prompt));

	}

	free(buffer);
	write(1, "\n\nBye bye...\n", 13);
	
}


void fortune(){
	for(int k=0;k<10000000;k++) for(int k=0;k<10;k++); // Pour pouvoir obtenir un temps d'execution supérieur à une milliseconde on ajoute cette ligne
	write(1, "Today is what happened to yesterday.\n", 37);
	exit(0);
}

void goOut(){
	// La fonction goOut fait appel à la fonction close qui ferme le descripteur de fichier et donc interrompt le while
	write(1, "\n\nBye bye...\n", 13);
	close(1);
}

void handlerSigint(){
	// Pour l'instant il ne se passe rien lors d'un ctrl+C
}

int execution(char *command){
	
	pid_t fpid =0;
	int status = 0;

	fpid = fork();

	if(fpid==-1) perror("Fork impossible"); // En cas d'erreur lors du fork on en informe l'utilisateur

	else if (fpid>0){ // CODE DU PERE
		waitpid(fpid,&status,0);
		kill(fpid, SIGTERM);
	}
	else{ // CODE DU FILS
		if(!strncmp("fortune", command,7)) fortune();

		else exit(1);

	}
	return status;

}

char* convertTime(double diff){
	// Nous recevons en entrée un double contenant le temps d'execution en seconde

	const int CHAR_BUFF_SIZE = 20;
	
	char *p = NULL;
	char *finalRes = NULL;
	p = (char *)calloc(sizeof(char), CHAR_BUFF_SIZE); // On commence par allouer CHAR_BUFF_SIZE octets sur lequels nous allons travailler 
	// pour la mise en forme de la chaine de caractère

	char *s = p + CHAR_BUFF_SIZE;
	// On crée un nouveau pointeur s qui pointe sur la dernière case du tableau p

	int nbChar=1;
	// Cette variable permettra de connaitre la taille du string final à renvoyer
	
	if(diff>1){ // Si le temps d'execution dépasse une seconde

		*--s = 's'; // On place s (pour seconde) dans la dernière case de p et on décremente egalement le pointeur s
		// s pointe donc dorénavant sur l'avant dernière case de p
		
		int units=(int) diff; // Si le temps d'execution est supérieur ou égal à une seconde on affiche que les secondes	

		while (units > 0) {
        	*--s = (units % 10) + '0';
        	units /= 10;
        	nbChar++; // Pour chaque décimale en commencant par les unités, puis les dixaines, puis les centaines... On place le code ascii dans la 
        	// case pointée par s et on décremente s qui pointe maintenant une case avant
        	// Le temps max supporté par cette fonction est 10^19
    	}
	}
	else{ // Si l'on arrive ici, le temps doit donc être donné en millisecondes
		diff*=1000; // On le multiplie par 1000 pour le convertir en ms

		*--s = 's';
		*--s = 'm'; // On place ms dans les deux dernières cases
		nbChar++;
		int units=(int) diff;			

		while (units > 0) { // Le fonctionnement est ici identique aux secondes
        	*--s = (units % 10) + '0';
        	units /= 10;
        	nbChar++;
    	}

	}
	
	// nbChar contient ici le nombre de caractère pour le temps et l'unité à la fin
	// On alloue un tableau de la bonne dimension (nbChar + 3 car on ajoute le | qui sépare du code de retour et le '] ' à la fin)
	finalRes = (char *)calloc(sizeof(char), 3+nbChar);
	finalRes[0] = '|';

	for(int i = 1;i<1+nbChar;i++){
		finalRes[i]=*s;
		++s;
	} // Dans cette boucle for on recopie tout les caractères

	finalRes[nbChar+1]=']';
	finalRes[nbChar+2]=' ';  

	free(p); // On oublie pas de libérer la mémoire allouée pour p
	return finalRes; // La mémoire allouée pour finalRes sera libérée dans le while
}