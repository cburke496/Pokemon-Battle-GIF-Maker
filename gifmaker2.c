#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
  FILE *newFile = fopen("commands", "w");
  FILE *reader = fopen("base2", "r");
  char line[200];
  int i = 0;
  char *newargv[3];


  srand(time(NULL));
  int winner = rand() % 2;

  if(argc != 3) {
    printf("Usage: ./gifmaker2 [pokemon_name_1] [pokemon_name_2]\n");
    return 0;
  }
  
  for(i = 0; i < 3; i++) {
    if(i) {
      fclose(reader);
      
      if(argv[i][0] >= 'A' && argv[i][0] <= 'Z') {
	argv[i][0] += ('a' - 'A');
      }
      
      reader = fopen(argv[i],"r");
      if(reader == NULL) {
	printf("Invalid Pokemon name: %s\n",argv[i]);
      }
    }

    while(1) {
      if(!fgets(line, 199, reader))
	break;
      
      if(!strcmp(line, "restore origin\n") && i == 1) {
	strcpy(line, "restore origin1\n");
      }
      if(!strcmp(line, "save origin\n") && i == 1) {
	strcpy(line, "save origin1\n");
      }
      if(!strcmp(line, "restore origin\n") && i == 2) {
	strcpy(line, "restore origin2\n");
      }
      if(!strcmp(line, "save origin\n") && i == 2) {
	strcpy(line, "save origin2\n");
      }

      fputs(line, newFile);
    }

    if(!i) {
      sprintf(line, "\nmove -4 0 0\n");
      fputs(line, newFile);
      sprintf(line, "\nsurface-color 0.7 0.7 0.7\n");
      fputs(line, newFile);
      sprintf(line, "\nimport cylinder.3dt 3 50 3 0 0 0 0 -50.5 0\n");
      fputs(line, newFile);
      sprintf(line, "\nsurface-color 0.6 0.6 0.6\n");
      fputs(line, newFile);
      sprintf(line, "\nimport cylinder.3dt 3.5 50 3.5 0 0 0 0 -50.6 0\n");
      fputs(line, newFile);
      sprintf(line, "\nrotate-y 90\n");
      fputs(line, newFile);
      if(!winner) {
	sprintf(line, "\nmove 0 0 endingmove\nscale ending ending ending\n");
	fputs(line, newFile);
      }
    }
    if(i == 1) {
      sprintf(line, "\nrestore origin\n");
      fputs(line, newFile);
      sprintf(line, "\nmove 4 0 0\n");
      fputs(line, newFile);
      sprintf(line, "\nsurface-color 0.6 0.6 0.6\n");
      fputs(line, newFile);
      sprintf(line, "\nimport cylinder.3dt 3.5 50 3.5 0 0 0 0 -50.6 0\n");
      fputs(line, newFile);
      sprintf(line, "\nsurface-color 0.7 0.7 0.7\n");
      fputs(line, newFile);
      sprintf(line, "\nimport cylinder.3dt 3 50 3 0 0 0 0 -50.5 0\n");
      fputs(line, newFile);
      sprintf(line, "\nrotate-y -90\n");
      fputs(line, newFile);
      if(winner == 1) {
	sprintf(line, "\nmove 0 0 endingmove\nscale ending ending ending\n");
	fputs(line, newFile);
      }
    }
  }

  sprintf(line, "\nrender-surface-perspective-cyclops 0 0 eyez\nfiles battle\nend");
  fputs(line, newFile);

  fclose(newFile);

  newargv[0] = malloc(20 * sizeof(char));
  newargv[1] = malloc(20 * sizeof(char));
  newargv[2] = malloc(20 * sizeof(char));
  strcpy(newargv[0], "./interpreter");
  strcpy(newargv[1], "commands");
  newargv[2] = NULL;

  execvp("./interpreter\0",newargv);
}
