#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define bufferSize 2048

void fortune();


int	main()
{

	ssize_t lecture; // Utilisé lors de la lecture pour determiner le nombre d'octets lus

	char *buffer = NULL;
	// On crée un pointeur sur un caractère (chaine de caractère)

	// void* calloc(size_t num_elements, size_t size) est le prototype de calloc (meme fonctionnement que malloc avec en plus initilalisation à 0)
	// On utilise cette fonction pour alouer bufferSize caractère en mémoire et initialiser tout à 0. On verifie par la suite que l'allocation c'est bien passée.
	buffer = (char *)calloc(sizeof(char), bufferSize);
	if (buffer == NULL) {
		// Si le buffer est toujours NULL après allocation avec calloc alors il y a eu une erreur
		perror("Allocation failure");
		return (EXIT_FAILURE);
	}


	write(1, "Bienvenue dans le Shell ENSEA.\n", 31);
	write(1, "Pour quitter, tapez 'exit'.\n", 29);
	write(1, "enseash % ", 10);


	while((lecture=read(1,buffer,bufferSize))>0){ // read() renvoie -1 si il y a une erreur, sinon le nombre d'octets lus


		if(!strncmp("fortune", buffer,7) && lecture==8) fortune();
		// strncmp compare selon l'ordre lexicographique (0 si les deux chaines sont identiques, >0 sinon)
		// On verifie egalement que le nombre d'octets lus correspond à la taille de "fortune" pour eviter que fortunex soit capté par exemple
		// Attention : le nombre d'octets lus est comparé à 8 et non à 7 car à la fin de la chaine il y a le caractère \n
		// Par la suite nous devrons mettre en place un parsing de la commande d'entrée.

		write(1, "enseash % ", 10);
		// On affiche le prompt

	}


	free(buffer); // On oublie pas de libérer la mémoire alloué dynamiquement à l'aide de free()
	write(1, "\n\nBye bye...\n", 13);
}


void fortune(){
	write(1, "Today is what happened to yesterday.\n", 37);
}