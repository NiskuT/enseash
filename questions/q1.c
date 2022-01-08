#include <unistd.h>

int	main()
{

	write(1, "Bienvenue dans le Shell ENSEA.\n", 31);
	write(1, "Pour quitter, tapez 'exit'.\n", 29);
	write(1, "enseash % ", 10);



	write(1, "\n\nBye bye...\n", 13);
}