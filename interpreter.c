#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include "mat4.h"
#include "matmulti.h"
#include "parse_util.h"

#define BLACK 0
#define WHITE 1
#define RED 2
#define CYAN 3

#define NLONG 60
#define NLAT 40

#define PARALLEL 0
#define PERSPECTIVE 1

#define WIRE 0
#define SURFACE 1

#define AMBIENT 40


Mat4 *scale(Mat4 *coords, double sx, double sy, double sz);
Mat4 *rotatex(Mat4 *coords, double angle);
Mat4 *rotatey(Mat4 *coords, double angle);
Mat4 *rotatez(Mat4 *coords, double angle);
Mat4 *move(Mat4 *coords, double mx, double my, double mz);

void drawTriangle(double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3,double z3, float r, float g, float b, int view, int surfaceOrWire);
void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b);
void drawSurface(int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, int r, int g, int b);
void fillPixel(int x, int y, int z, int r, int g, int b);
Mat4 *worldToView(Mat4 *coords);
Mat4 *worldToPerspective(Mat4 *coords);
void callocPixels();
void varySplit(char *line);//Runs parse_split and replaces variables

struct Pixel {
  int r,g,b,z;
};

struct Color {
  float kr, kg, kb;  //Color constants for a surface
  int endcoord;//Initial and final triangles with these properties
};

struct Light {
  int r, g, b;
  int x, y, z;
};

typedef struct Pixel pixel; 
typedef struct Color color;
typedef struct Light light;  

pixel **pixels;
color colors[50];
light lights[50];
int calloced = 0;
Mat4 *coords;
char **splitline = NULL;
Mat4 *transform;
double spherecoords[NLONG][NLAT+1][4];

int colorNumber = -1;
int lightNumber = 0;

int width = 500, height = 500;
int xright = 250, ytop = 250;
int xleft = -250, ybottom = -250;

double ex = 0;
double ey = 0;
double ez = 1;

//Info about "vary" variables
char vnames[20][15];
double vvals[20];
int nvars = 0;

//Info about "save"d coordinate systems
char snames[20][15];
Mat4* svals[20];
int nsaves = 0;

int startFrame, endFrame;
int currentFrame = 0;

int numMatrices = 0;


int main(int argc, char *argv[]) {
  FILE *fp, *fp2;
  char line[500];
  char newline[50];
  char inputFilename[100];
  char outputFilename[100];
  int i = 0, j = 0, k = 0;
  double angle;
  double* newcoords = malloc(sizeof(double) * 4);

  coords = mat4_create(0);
  transform = mat4_create_identity();

  char *newargv[4];
  newargv[0] = malloc(50 * sizeof(char));
  newargv[1] = malloc(50 * sizeof(char));
  newargv[2] = malloc(50 * sizeof(char));
  newargv[3] = malloc(50 * sizeof(char));
  
  strcpy(newargv[0], "convert");
  newargv[3] = NULL;

  strcpy(inputFilename, argv[1]);


  fp = fopen(inputFilename,"r");

  if (fp == NULL) {
    printf("File %s doesn't exist\n", argv[1]);
    return 0;
  }

  callocPixels();
  while(1) {
    if(!fgets(line, 1000, fp)) {
      printf("No frames command. Exiting...\n");
      return 0;
    }
    varySplit(line);

    if (!strcmp(splitline[0],"frames")) {
      startFrame = atoi(splitline[1]);
      currentFrame = startFrame;
      endFrame = atoi(splitline[2]);
      break;
    }
  }
  
  
  while(currentFrame <= endFrame) {
    nvars = 0;
    nsaves = 0;
    fp = fopen(inputFilename,"r");
    callocPixels();
    
    mat4_delete(transform);
    numMatrices--;
    transform = mat4_create_identity();
    numMatrices++;
    mat4_delete(coords);
    numMatrices--;
    coords = mat4_create(0);
    numMatrices++;


    while(1) {
      fgets(line, 1000, fp);
      if (line[0] == '\n') {
      } else {
	varySplit(line);
	//printf("%s\n", splitline[0]); //un-comment for debugging
	if (splitline[0][0] == '#') {
	} else if (!strcmp(splitline[0],"import")) {
	  FILE *import = fopen(splitline[1],"r");

	  double sx, sy, sz, rx, ry, rz, mx, my, mz;
	  Mat4 *tmptrans = mat4_create_identity();
	  numMatrices++;
	  Mat4 *tmp = mat4_create(0);
	  numMatrices++;

	  sx = atof(splitline[2]);
	  sy = atof(splitline[3]);
	  sz = atof(splitline[4]);
	  rx = atof(splitline[5]) * M_PI / 180;
	  ry = atof(splitline[6]) * M_PI / 180;
	  rz = atof(splitline[7]) * M_PI / 180;
	  mx = atof(splitline[8]);
	  my = atof(splitline[9]);
	  mz = atof(splitline[10]);

	  tmptrans = move(tmptrans, mx, my, mz);
	  tmptrans = rotatez(tmptrans, rz);      
	  tmptrans = rotatey(tmptrans, ry);
	  tmptrans = rotatex(tmptrans, rx);
	  tmptrans = scale(tmptrans, sx, sy, sz);

	  fgets(line, 1000, import);
	  while(line[0] == '#') {
	    fgets(line, 1000, import);
	  }
	  int numTris = atoi(line);
	  int currentTriNum = 0;
	  while(currentTriNum < numTris) {
	    fgets(line, 1000, import);
	    varySplit(line);

	    newcoords[0] = atof(splitline[0]);
	    newcoords[1] = atof(splitline[1]);
	    newcoords[2] = atof(splitline[2]);
	    newcoords[3] = 1;
	    mat4_add_column(tmp,newcoords);

	    newcoords[0] = atof(splitline[3]);
	    newcoords[1] = atof(splitline[4]);
	    newcoords[2] = atof(splitline[5]);
	    newcoords[3] = 1;
	    mat4_add_column(tmp,newcoords);

	    newcoords[0] = atof(splitline[6]);
	    newcoords[1] = atof(splitline[7]);
	    newcoords[2] = atof(splitline[8]);
	    newcoords[3] = 1;
	    mat4_add_column(tmp,newcoords);


	    currentTriNum++;
	  }
	  
	  tmptrans = premultiply(transform, tmptrans);
	  tmp = premultiply(tmptrans,tmp);
	  
	  for(i = 0; i < mat4_columns(tmp); i++) {
	    newcoords[0] = mat4_get(tmp,0,i);
	    newcoords[1] = mat4_get(tmp,1,i);
	    newcoords[2] = mat4_get(tmp,2,i);
	    newcoords[3] = mat4_get(tmp,3,i);
	    mat4_add_column(coords,newcoords);
	  }
	  
	  mat4_delete(tmptrans);
	  numMatrices--;
	  mat4_delete(tmp);
	  numMatrices--;
	  fclose(import);

	  //printf("After import, matrix count: %d\n",numMatrices);
	} else if (!strcmp(splitline[0],"light")) {
	  lights[lightNumber].r = atoi(splitline[1]);
	  lights[lightNumber].g = atoi(splitline[2]);
	  lights[lightNumber].b = atoi(splitline[3]);
	  lights[lightNumber].x = atoi(splitline[4]);
	  lights[lightNumber].y = atoi(splitline[5]);
	  lights[lightNumber].z = atoi(splitline[6]);
	  
	  lightNumber++;
	} else if (!strcmp(splitline[0],"surface-color")) {
	  colorNumber++;

	  if(colorNumber > 0) {
	    colors[colorNumber - 1].endcoord = mat4_columns(coords);
	  }

	  colors[colorNumber].kr = atof(splitline[1]);
	  colors[colorNumber].kg = atof(splitline[2]);
	  colors[colorNumber].kb = atof(splitline[3]);
	} else if (!strcmp(splitline[0],"frames")) {
	  startFrame = atoi(splitline[1]);
	  endFrame = atoi(splitline[2]);
	} else if (!strcmp(splitline[0],"vary")) {
	  double vstartFrame = atof(splitline[4]);
	  double vendFrame = atof(splitline[5]);
	  if(currentFrame >= vstartFrame && currentFrame <= vendFrame) {
	    strcpy(vnames[nvars],splitline[1]);
	    double initVal = atof(splitline[2]);
	    double finalVal = atof(splitline[3]);
	    vvals[nvars] = (currentFrame-vstartFrame)/(vendFrame - vstartFrame + 1) * (finalVal - initVal) + initVal;
	    nvars++;
	  }
	} else if (!strcmp(splitline[0],"save")) {
	  strcpy(snames[nsaves],splitline[1]);
	  svals[nsaves] = mat4_copy(transform);
	  numMatrices++;
	  nsaves++;
	} else if (!strcmp(splitline[0],"restore")) {
	  for(i = 0; i < nsaves; i++) {
	    if(!strcmp(splitline[1], snames[i])) {
	      mat4_delete(transform);
	      numMatrices--;
	      transform = mat4_copy(svals[i]);
	      numMatrices++;
	      break;
	    }
	  }
	} else if (!strcmp(splitline[0],"sphere-t")) {
	  int theta = 0, phi = 0;
	  double r = 1;
	  double x = 0, y = 0, z = 0;
	  double sx, sy, sz, rx, ry, rz, mx, my, mz;
	  Mat4 *tmptrans = mat4_create_identity();
	  numMatrices++;
	  Mat4 *tmp = mat4_create(0);
	  numMatrices++;
	  int nlong = NLONG;
	  int nlat = NLAT;
	  sx = atof(splitline[1]);
	  sy = atof(splitline[2]);
	  sz = atof(splitline[3]);
	  rx = atof(splitline[4]) * M_PI / 180;
	  ry = atof(splitline[5]) * M_PI / 180;
	  rz = atof(splitline[6]) * M_PI / 180;
	  mx = atof(splitline[7]);
	  my = atof(splitline[8]);
	  mz = atof(splitline[9]);
	  

	  if(sx < 0.2 || sy < 0.2 || sz < 0.2) {
	    nlong /= 6;
	    nlat /= 6;
	  }
	  
	  tmptrans = move(tmptrans, mx, my, mz);
	  tmptrans = rotatez(tmptrans, rz);      
	  tmptrans = rotatey(tmptrans, ry);
	  tmptrans = rotatex(tmptrans, rx);
	  tmptrans = scale(tmptrans, sx, sy, sz);
	  
	  
	  for(phi = 0; phi < nlong; phi++) {
	    for(theta = -1*nlat/2; theta <= nlat/2; theta++) {
	      spherecoords[phi][theta+(nlat/2)][0] = x + r * cos(M_PI/nlat*theta) * cos(M_PI/nlong*2*phi);
	      spherecoords[phi][theta+(nlat/2)][1] = y + r * cos(M_PI/nlat*theta) * sin(M_PI/nlong*2*phi);
	      spherecoords[phi][theta+(nlat/2)][2] = z + r * sin(M_PI/nlat*theta);
	      spherecoords[phi][theta+(nlat/2)][3] = 1;
	    }
	  }
	  
	  for(j = 0; j < nlong; j++) {
	    mat4_add_column(tmp,spherecoords[j][0]);
	    mat4_add_column(tmp,spherecoords[(j+1)%nlong][1]);
	    mat4_add_column(tmp,spherecoords[j][1]);
	    
	    for(i = 1; i < nlat - 1; i++) {
	      mat4_add_column(tmp,spherecoords[j][i]);
	      mat4_add_column(tmp,spherecoords[(j+1)%nlong][i]);
	      mat4_add_column(tmp,spherecoords[(j+1)%nlong][i+1]);
	      
	      mat4_add_column(tmp,spherecoords[j][i]);
	      mat4_add_column(tmp,spherecoords[(j+1)%nlong][i+1]);
	      mat4_add_column(tmp,spherecoords[j][i+1]);
	    }
	    
	    mat4_add_column(tmp,spherecoords[j][nlat-1]);
	    mat4_add_column(tmp,spherecoords[(j+1)%nlong][nlat-1]);
	    mat4_add_column(tmp,spherecoords[j][nlat]);
	  }
	  
	  tmptrans = premultiply(transform,tmptrans);
	  tmp = premultiply(tmptrans,tmp);
	  
	  for(i = 0; i < mat4_columns(tmp); i++) {
	    newcoords[0] = mat4_get(tmp,0,i);
	    newcoords[1] = mat4_get(tmp,1,i);
	    newcoords[2] = mat4_get(tmp,2,i);
	    newcoords[3] = mat4_get(tmp,3,i);
	    mat4_add_column(coords,newcoords);
	  }

	  mat4_delete(tmptrans);
	  numMatrices--;
	  mat4_delete(tmp);
	  numMatrices--;

	  //printf("After drawing sphere, matrix count: %d\n",numMatrices);
	} else if (!strcmp(splitline[0],"box-t")) {
	  double sx = atof(splitline[1]);
	  double sy = atof(splitline[2]);
	  double sz = atof(splitline[3]);
	  double rx = atof(splitline[4]);
	  double ry = atof(splitline[5]);
	  double rz = atof(splitline[6]);
	  double mx = atof(splitline[7]);
	  double my = atof(splitline[8]);
	  double mz = atof(splitline[9]);
	  double v1[4] = {0.5, 0.5, 0.5, 1};
	  double v2[4] = {0.5, -0.5, 0.5, 1};
	  double v3[4] = {-0.5, -0.5, 0.5, 1};
	  double v4[4] = {-0.5, 0.5, 0.5, 1};
	  double v5[4] = {-0.5, -0.5, -0.5, 1};
	  double v6[4] = {-0.5, 0.5, -0.5, 1};
	  double v7[4] = {0.5, -0.5, -0.5, 1};
	  double v8[4] = {0.5, 0.5, -0.5, 1};
	  Mat4 *tmp = mat4_create(0);
	  numMatrices++;
	  Mat4 *tmptrans = mat4_create_identity();
	  numMatrices++;	

	  tmptrans = move(tmptrans, mx, my, mz);
	  tmptrans = rotatez(tmptrans, rz * M_PI/180.0);      
	  tmptrans = rotatey(tmptrans, ry * M_PI/180.0);
	  tmptrans = rotatex(tmptrans, rx * M_PI/180.0);
	  tmptrans = scale(tmptrans, sx, sy, sz);
	
	  //Front Face
	  mat4_add_column(tmp,v1);
	  mat4_add_column(tmp,v3);
	  mat4_add_column(tmp,v2);
	  mat4_add_column(tmp,v1);
	  mat4_add_column(tmp,v4);
	  mat4_add_column(tmp,v3);
	
	  //Left Face
	  mat4_add_column(tmp,v4);
	  mat4_add_column(tmp,v5);
	  mat4_add_column(tmp,v3);
	  mat4_add_column(tmp,v4);
	  mat4_add_column(tmp,v6);
	  mat4_add_column(tmp,v5);
	
	  //Back Face
	  mat4_add_column(tmp,v6);
	  mat4_add_column(tmp,v7);
	  mat4_add_column(tmp,v5);
	  mat4_add_column(tmp,v6);
	  mat4_add_column(tmp,v8);
	  mat4_add_column(tmp,v7);
	
	  //Right Face
	  mat4_add_column(tmp,v8);
	  mat4_add_column(tmp,v2);
	  mat4_add_column(tmp,v7);
	  mat4_add_column(tmp,v8);
	  mat4_add_column(tmp,v1);
	  mat4_add_column(tmp,v2);
	
	  //Top Face
	  mat4_add_column(tmp,v8);
	  mat4_add_column(tmp,v4);
	  mat4_add_column(tmp,v1);
	  mat4_add_column(tmp,v8);
	  mat4_add_column(tmp,v6);
	  mat4_add_column(tmp,v4);
	
	  //Bottom Face
	  mat4_add_column(tmp,v2);
	  mat4_add_column(tmp,v5);
	  mat4_add_column(tmp,v7);
	  mat4_add_column(tmp,v2);
	  mat4_add_column(tmp,v3);
	  mat4_add_column(tmp,v5);
	
	  tmptrans = premultiply(transform, tmptrans);
	  tmp = premultiply(tmptrans,tmp);
	
	  for(i = 0; i < mat4_columns(tmp); i++) {
	    newcoords[0] = mat4_get(tmp,0,i);
	    newcoords[1] = mat4_get(tmp,1,i);
	    newcoords[2] = mat4_get(tmp,2,i);
	    newcoords[3] = mat4_get(tmp,3,i);
	    mat4_add_column(coords,newcoords);
	  }

	  mat4_delete(tmptrans);
	  numMatrices--;
	  mat4_delete(tmp);
	  numMatrices--;

	  //printf("After drawing box, matrix count: %d\n",numMatrices);
	} else if (!strcmp(splitline[0],"move")) {
	  transform = move(transform,atof(splitline[1]),atof(splitline[2]),atof(splitline[3]));
	} else if (!strcmp(splitline[0],"scale")) {
	  transform = scale(transform, atof(splitline[1]), atof(splitline[2]),atof(splitline[3]));
	} else if (!strcmp(splitline[0],"rotate-x")) {
	  angle = M_PI/180.0* atof(splitline[1]);
	  transform = rotatex(transform, angle);
	} else if (!strcmp(splitline[0],"rotate-y")) {
	  angle = M_PI/180.0* atof(splitline[1]);
	  transform = rotatey(transform, angle);
	} else if (!strcmp(splitline[0],"rotate-z")) {
	  angle = M_PI/180.0* atof(splitline[1]);
	  transform = rotatez(transform, angle);
	} else if (!strcmp(splitline[0],"screen")) {
	  xleft = atof(splitline[1]);
	  ybottom = atof(splitline[2]);
	  xright = atof(splitline[3]);
	  ytop = atof(splitline[4]);
	} else if (!strcmp(splitline[0],"pixels")) {
	  width = atof(splitline[1]);
	  height = atof(splitline[2]);
	  callocPixels();     
	} else if (!strcmp(splitline[0],"render-parallel")) {
	  colors[colorNumber].endcoord = mat4_columns(coords);
	  int tempColorNumber = 0;

	  for(i = 0; i < mat4_columns(coords); i += 3) {
	    while(i >= colors[tempColorNumber].endcoord) {
	      tempColorNumber++;
	    }

	    drawTriangle(mat4_get(coords,0,i),mat4_get(coords,1,i),mat4_get(coords,2,i),mat4_get(coords,0,i+1),mat4_get(coords,1,i+1),mat4_get(coords,2,i+1),mat4_get(coords,0,i+2),mat4_get(coords,1,i+2),mat4_get(coords,2,i+2), colors[tempColorNumber].kr, colors[tempColorNumber].kg, colors[tempColorNumber].kb, PARALLEL, WIRE);
	  }
	  
	  mat4_delete(coords);
	  numMatrices--;
	  coords = mat4_create(0);
	  numMatrices++;
	} else if (!strcmp(splitline[0],"render-surface-parallel")) {
	  colors[colorNumber].endcoord = mat4_columns(coords);
	  int tempColorNumber = 0;

	  //printf("Number of matrices before anything:%d\n",numMatrices);

	  for(i = 0; i < mat4_columns(coords); i += 3) {
	    while(i >= colors[tempColorNumber].endcoord) {
	      tempColorNumber++;
	    }

	    //printf("Number of matrices before drawing triangle:%d\n",numMatrices);

	    drawTriangle(mat4_get(coords,0,i),mat4_get(coords,1,i),mat4_get(coords,2,i),mat4_get(coords,0,i+1),mat4_get(coords,1,i+1),mat4_get(coords,2,i+1),mat4_get(coords,0,i+2),mat4_get(coords,1,i+2),mat4_get(coords,2,i+2), colors[tempColorNumber].kr, colors[tempColorNumber].kg, colors[tempColorNumber].kb, PARALLEL, SURFACE);
	    //printf("Number of matrices after drawing triangle:%d\n\n",numMatrices);
	  }
	  
	  mat4_delete(coords);
	  numMatrices--;
	  coords = mat4_create(0);
	  numMatrices++;
	} else if (!strcmp(splitline[0],"render-perspective-cyclops")) {
	  colors[colorNumber].endcoord = mat4_columns(coords);
	  int tempColorNumber = 0;

	  ex = atof(splitline[1]);
	  ey = atof(splitline[2]);
	  ez = atof(splitline[3]);

	  for(i = 0; i < mat4_columns(coords); i += 3) {
	    while(i >= colors[tempColorNumber].endcoord) {
	      tempColorNumber++;
	    }

	    drawTriangle(mat4_get(coords,0,i),mat4_get(coords,1,i),mat4_get(coords,2,i),mat4_get(coords,0,i+1),mat4_get(coords,1,i+1),mat4_get(coords,2,i+1),mat4_get(coords,0,i+2),mat4_get(coords,1,i+2),mat4_get(coords,2,i+2), colors[tempColorNumber].kr, colors[tempColorNumber].kg, colors[tempColorNumber].kb, PERSPECTIVE, WIRE);
	  }

	  
	  mat4_delete(coords);
	  numMatrices--;
	  coords = mat4_create(0);
	  numMatrices++;
	} else if (!strcmp(splitline[0],"render-surface-perspective-cyclops")) {
	  colors[colorNumber].endcoord = mat4_columns(coords);
	  int tempColorNumber = 0;

	  ex = atof(splitline[1]);
	  ey = atof(splitline[2]);
	  ez = atof(splitline[3]);

	  for(i = 0; i < mat4_columns(coords); i += 3) {
	    while(i >= colors[tempColorNumber].endcoord) {
	      tempColorNumber++;
	    }
	    if(tempColorNumber > colorNumber) {
	      printf("No surface color given or something\n");
	    }

	    drawTriangle(mat4_get(coords,0,i),mat4_get(coords,1,i),mat4_get(coords,2,i),mat4_get(coords,0,i+1),mat4_get(coords,1,i+1),mat4_get(coords,2,i+1),mat4_get(coords,0,i+2),mat4_get(coords,1,i+2),mat4_get(coords,2,i+2), colors[tempColorNumber].kr, colors[tempColorNumber].kg, colors[tempColorNumber].kb, PERSPECTIVE, SURFACE);
	  }

	  
	  mat4_delete(coords);
	  numMatrices--;
	  coords = mat4_create(0);
	  numMatrices++;
	} else if(!strcmp(splitline[0],"render-perspective-stereo")) {
	  colors[colorNumber].endcoord = mat4_columns(coords);
	  int tempColorNumber = 0;

	  ex = atof(splitline[1]);
	  ey = atof(splitline[2]);
	  ez = atof(splitline[3]);

	  for(i = 0; i < mat4_columns(coords); i += 3) {
	    while(i >= colors[tempColorNumber].endcoord) {
	      tempColorNumber++;
	    }

	    drawTriangle(mat4_get(coords,0,i),mat4_get(coords,1,i),mat4_get(coords,2,i),mat4_get(coords,0,i+1),mat4_get(coords,1,i+1),mat4_get(coords,2,i+1),mat4_get(coords,0,i+2),mat4_get(coords,1,i+2),mat4_get(coords,2,i+2), colors[tempColorNumber].kr, colors[tempColorNumber].kg, colors[tempColorNumber].kb, PERSPECTIVE, WIRE);        	
	  }
	  ex = atof(splitline[4]);
	  ey = atof(splitline[5]);
	  ez = atof(splitline[6]);
	  for(i = 0; i < mat4_columns(coords); i += 3) {
	    while(i >= colors[tempColorNumber].endcoord) {
	      tempColorNumber++;
	    }

	    drawTriangle(mat4_get(coords,0,i),mat4_get(coords,1,i),mat4_get(coords,2,i),mat4_get(coords,0,i+1),mat4_get(coords,1,i+1),mat4_get(coords,2,i+1),mat4_get(coords,0,i+2),mat4_get(coords,1,i+2),mat4_get(coords,2,i+2), colors[tempColorNumber].kr, colors[tempColorNumber].kg, colors[tempColorNumber].kb, PERSPECTIVE, WIRE);        	
	  }

	  mat4_delete(coords);
	  numMatrices--;
	  coords = mat4_create(0);
	  numMatrices++;
	} else if (!strcmp(splitline[0],"clear-triangles")) {
	  coords = mat4_create(0);
	  numMatrices++;
	} else if (!strcmp(splitline[0],"clear-pixels")) {
	  callocPixels();
	} else if (!strcmp(splitline[0],"files")) {
	  strcpy(outputFilename, splitline[1]);
	  strcpy(newargv[1], splitline[1]);
	  strcpy(newargv[2], splitline[1]);
	  char intString[3] = "000";
	  int length = strlen(outputFilename);
	  if(currentFrame <= 10) {
	    sprintf(intString, "%d", currentFrame - 1);
	    outputFilename[length] = '0';
	    outputFilename[length + 1] = '0';
	    outputFilename[length + 2] = '\0';
	    strcat(outputFilename,intString);
	    strcat(outputFilename,".ppm");
	    outputFilename[length + 7] = '\0';
	  } else if (currentFrame <= 100) {
	    sprintf(intString, "%d", currentFrame - 1);
	    outputFilename[length] = '0';
	    outputFilename[length + 1] = '\0';
	    strcat(outputFilename,intString);
	    strcat(outputFilename,".ppm");
	    outputFilename[length + 7] = '\0';
	  } else if (currentFrame <= 1000) {
	    sprintf(intString, "%d", currentFrame - 1);
	    outputFilename[length] = '\0';
	    strcat(outputFilename,intString);
	    strcat(outputFilename,".ppm");
	    outputFilename[length + 7] = '\0';
	  } else {
	    printf("Too many frames (Max is 1000)\n");
	    return 0;
	  }

	  fp2 = fopen(outputFilename, "w");
	
	  if (fp2 == NULL) {
	    printf("Failed to open output file...\n");
	    return 0;
	  }
	
	  char header[30];
	  sprintf(header, "P3\n%d %d\n255\n", width, height);
	  fputs(header, fp2);
	
	  for (i = 0; i < height; i++) {
	    for (j = 0; j < width; j++) {
	      sprintf(newline, "%d\t%d\t%d\t", pixels[i][j].r, pixels[i][j].g, pixels[i][j].b);
	      fwrite(newline, sizeof(char), strlen(newline), fp2);
	    }
	  }
	  
	  fclose(fp2);
	} else if (!strcmp(splitline[0],"export")) {
	  if (currentFrame == 1) {
	    FILE* exportp;
	    char exportname[20];
	    sprintf(exportname, "%s.3dt", splitline[1]);
	    
	    exportp = fopen(exportname, "w");
	    
	    if(exportp == NULL) {
	      printf("Failed to open file for exporting...\n");
	      return 0;
	    }
	    
	    int numTriangles = mat4_columns(coords) / 3;
	    sprintf(newline, "%d\n", numTriangles);
	    fputs(newline, exportp);
	    
	    for(i = 0; i < numTriangles; i++) {
	      for(j = 0; j < 3; j++) {
		for(k = 0; k < 3; k++) {
		  sprintf(newline, "%f ", mat4_get(coords, k, 3*i + j));
		  fwrite(newline, sizeof(char), strlen(newline), exportp);
		}
	      }
	      sprintf(newline, "\n");
	      fwrite(newline, sizeof(char), strlen(newline), exportp);
	    }
	    fclose(exportp);
	  }
	} else if (!strcmp(splitline[0],"end")) {
	  break;
	} else  {
	  printf("Invalid line\n");
	  return 0;
	}
      }
    }
    fclose(fp);

    for(i = 0; i < nsaves; i++) {
      mat4_delete(svals[i]);
      numMatrices--;
    }
    nsaves = 0;

    printf("Current Frame Number: %d \n", currentFrame);
    currentFrame++;
    colorNumber = -1;
    lightNumber = 0;
    //printf("Matrix count: %d\n\n", numMatrices);
  }

  strcat(newargv[1],"*.ppm");
  strcat(newargv[2],".gif");

  printf("Converting files to gif....\n");

  execvp("convert", newargv);

  return 1;
}

void drawTriangle(double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3, double z3, float r, float g, float b, int view, int surfaceOrWire) {
  double vec1[3] = {x3 - x2, y3 - y2, z3 - z2};
  double vec2[3] = {x1 - x2, y1 - y2, z1 - z2};
  double cross[3] = {vec1[1]*vec2[2] - vec1[2]*vec2[1],
		     vec1[2]*vec2[0] - vec1[0]*vec2[2],
		     vec1[0]*vec2[1] - vec1[1]*vec2[0]};
  double dot;
  int cx = (x3 + x2 + x1)/3;
  int cy = (y3 + y2 + y1)/3;
  int cz = (z3 + z2 + z1)/3;

  if (view == PERSPECTIVE) {
    if (z1 >= ez || z2 >= ez || z3 >= ez) {
      dot = 0;
    } else {
      dot = cross[0]*(x1-ex) + cross[1]*(y1-ey) + cross[2]*(z1-ez);
    }
  } else {
    dot = -1 * cross[2];
  }

  if (dot >= 0)
    return;

  Mat4 *tmpcoords = mat4_create(3);
  numMatrices++;
  mat4_set(tmpcoords,0,0,x1);
  mat4_set(tmpcoords,0,1,x2);
  mat4_set(tmpcoords,0,2,x3);
  mat4_set(tmpcoords,1,0,y1);
  mat4_set(tmpcoords,1,1,y2);
  mat4_set(tmpcoords,1,2,y3);
  mat4_set(tmpcoords,2,0,z1);
  mat4_set(tmpcoords,2,1,z2);
  mat4_set(tmpcoords,2,2,z3);
  mat4_set(tmpcoords,3,0,1);
  mat4_set(tmpcoords,3,1,1);
  mat4_set(tmpcoords,3,2,1);

  if (view == PARALLEL) {
    tmpcoords = worldToView(tmpcoords);
  } else {
    tmpcoords = worldToPerspective(tmpcoords);
    tmpcoords = worldToView(tmpcoords);
  }

  x1 = mat4_get(tmpcoords,0,0) + 0.5;
  x2 = mat4_get(tmpcoords,0,1) + 0.5;
  x3 = mat4_get(tmpcoords,0,2) + 0.5;
  y1 = mat4_get(tmpcoords,1,0) + 0.5;
  y2 = mat4_get(tmpcoords,1,1) + 0.5;
  y3 = mat4_get(tmpcoords,1,2) + 0.5;
    
  if(x1 < -10 || x2 < -10 || x3 < -10 || y1 < -10 || y2 < -10 || y3 < -10 ||
     x1 > width + 10 || x2 > width + 10 || x3 > width + 10 || 
     y1 > height + 10 || y2 > height + 10 || y3 > height + 10) {
    mat4_delete(tmpcoords);
    numMatrices--;
    return;
  }

  int i = 0;
  int newr = AMBIENT;
  int newg = AMBIENT;
  int newb = AMBIENT;
  float cosine;

  int lvx, lvy, lvz;//Light vector components
  for(i = 0; i < lightNumber; i++) {
    lvx = cx / 3 - lights[i].x;
    lvy = cy / 3 - lights[i].y;
    lvz = cz / 3 - lights[i].z;
    cosine = ((float) (lvx*cross[0]+lvy*cross[1]+lvz*cross[2]))/
      ((float) sqrt(lvx*lvx+lvy*lvy+lvz*lvz)) / ((float) sqrt(cross[0]*cross[0]+cross[1]*cross[1]+cross[2]*cross[2]));

    if(cosine < 0) {
      newr += r * lights[i].r * -1 * cosine;
      newg += g * lights[i].g * -1 * cosine;
      newb += b * lights[i].b * -1 * cosine;
    }

    if(newr > 255) {
      newr = 255;
    }
    if(newg > 255) {
      newg = 255;
    }
    if(newb > 255) {
      newb = 255;
    }
  }

  if(surfaceOrWire == SURFACE) {
    z1 = mat4_get(tmpcoords,2,0) + 0.5;
    z2 = mat4_get(tmpcoords,2,1) + 0.5;
    z3 = mat4_get(tmpcoords,2,2) + 0.5;

    int xi, xf1, xf2;
    int yi, yf1, yf2;
    int zi, zf1, zf2;
    
    if (y1 <= y2 && y1 <= y3) {
      yi = y1;
      xi = x1;
      zi = z1;
      if(x2 < x3) {
	xf1 = x2;
	yf1 = y2;
	zf1 = z2;
	xf2 = x3;
	yf2 = y3;
	zf2 = z3;
      } else {
	xf1 = x3;
	yf1 = y3;
	zf1 = z3;
	xf2 = x2;
	yf2 = y2;
	zf2 = z2;
      }
    } else if (y2 <= y3 && y2 <= y1) {
      yi = y2;
      xi = x2;
      zi = z2;
      if(x1 < x3) {
	xf1 = x1;
	yf1 = y1;
	zf1 = z1;
	xf2 = x3;
	yf2 = y3;
	zf2 = z3;
      } else {
	xf1 = x3;
	yf1 = y3;
	zf1 = z3;
	xf2 = x1;
	yf2 = y1;
	zf2 = z1;
      }
    } else if (y3 <= y1 && y3 <= y2) {
      yi = y3;
      xi = x3;
      zi = z3;
      if(x1 < x2) {
	xf1 = x1;
	yf1 = y1;
	zf1 = z1;
	xf2 = x2;
	yf2 = y2;
	zf2 = z2;
      } else {
	xf1 = x2;
	yf1 = y2;
	zf1 = z2;
	xf2 = x1;
	yf2 = y1;
	zf2 = z1;
      }
    }

    drawSurface(xi,yi,zi,xf1,yf1,zf1,xf2,yf2,zf2,newr,newg,newb);
  } else {
    drawLine(x1,y1,x2,y2,newr,newg,newb);
    drawLine(x2,y2,x3,y3,r,g,b);
    drawLine(x3,y3,x1,y1,r,g,b);
  }

  mat4_delete(tmpcoords);
  numMatrices--;
}


void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b) {
  int temp, dx, dy;
  int acc, x, y;


  if (x1 == x2 && y1 == y2) {
    fillPixel(x1,y1,0,r,g,b);
    return;
  }
  if (abs(x1-x2) >= abs(y1-y2)) {
    if (x1 > x2) {
      temp = x1;
      x1 = x2;
      x2 = temp;
      temp = y1;
      y1 = y2;
      y2 = temp;
    }
    dx = x2 - x1;
    dy = y2 - y1;
    x = x1;
    y = y1;
    acc = dx/2;
    while (x < x2) {
      x++;
      acc += abs(dy);
      if (acc >= dx) {
	if (dy > 0)
	  y++;
	else
	  y--;
	acc -= dx;
      }
      fillPixel(x,y,0,r,g,b);
    } 
  } else {
    if (y1 > y2) {
      temp = x1;
      x1 = x2;
      x2 = temp;
      temp = y1;
      y1 = y2;
      y2 = temp;
    }
    dy = y2 - y1;
    dx = x2 - x1;
    x = x1;
    y = y1;
    acc = dy/2;
    while (y < y2) {
      y++;
      acc += abs(dx);
      if (acc >= dy) {
	if (dx > 0)
	  x++;
	else
	  x--;
	acc -= dy;
      }
      fillPixel(x,y,0,r,g,b);
    } 
  }
}


void drawSurface(int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3, int z3, int r, int g, int b) {
  int dx, dy, dz;
  int acc, zacc, x, y, z;

  int dx2, dy2, dz2;
  int acc2, zacc2, xr, zr;

  int i;

  float slope;

  if (x1 == x2 && y1 == y2) {
    return;
  }

  dy = y2 - y1;
  dx = x2 - x1;
  dz = z2 - z1;
  dy2 = y3 - y1;
  dx2 = x3 - x1;
  dz2 = z3 - z1;
  x = x1;
  y = y1;
  z = z1;
  xr = x1;
  zr = z1;
  acc = dy/2;
  acc2 = dy2/2;
  zacc = dy/2;
  zacc2 = dy2/2;

  
  fillPixel(x,y,z,r,g,b);

  while (y < y2 && y < y3) {  
    y++;
    acc += abs(dx);
    zacc += abs(dz);
    acc2 += abs(dx2);
    zacc2 += abs(dz2);
    while (acc >= dy && dy != 0) {
      if (dx > 0)
	x++;
      else
	x--;
      acc -= dy;
    }
    while (zacc >= dy && dy != 0) {
      if (dz > 0)
	z++;
      else
	z--;
      zacc -= dy;
    }
    while (acc2 >= dy2 && dy2 != 0) {
      if (dx2 > 0)
	xr++;
      else
	xr--;
      acc2 -= dy2;
    }
    while (zacc2 >= dy2 && dy2 != 0) {
      if (dz2 > 0)
	zr++;
      else
	zr--;
      zacc2 -= dy2;
    }
    if(xr - x) {
      slope = ( ((float) zr) - ((float) z) )/( ((float) xr) - ((float) x) );
      for(i = x; i <= xr; i++) {
	fillPixel(i,y,slope * (i-x) + z,r,g,b);
      }
      for(i = xr; i <= x; i++) {
	fillPixel(i,y,slope * (i-xr) + zr,r,g,b);
      }
    } else {
      if(z > zr) {
	fillPixel(x,y,z,r,g,b);
      } else {
	fillPixel(x,y,zr,r,g,b);
      }
    }
  }
  if(y == y2) {
    x = x2;
    z = z2;
    dy = y3 - y;
    dx = x3 - x;
    dz = z3 - z;
    acc = dy/2;
    zacc = dy/2;
  } else {
    xr = x3;
    zr = z3;
    dy2 = y2 - y;
    dx2 = x2 - xr;
    dz2 = z2 - zr;
    acc2 = dy2/2;
    zacc2 = dy2/2;
  }
  while (y < y2 || y < y3) {
    if(xr - x) {
      slope = ( (float) zr - (float) z )/( (float) xr - (float) x );
      for(i = x; i <= xr; i++) {
	fillPixel(i,y,slope * (i-x) + z,r,g,b);
      }
      for(i = xr; i <= x; i++) {
	fillPixel(i,y,slope * (i-xr) + zr,r,g,b);
      }
    } else {
      if(z > zr) {
	fillPixel(x,y,z,r,g,b);
      } else {
	fillPixel(x,y,zr,r,g,b);
      }
    }

    y++;
    acc += abs(dx);
    zacc += abs(dz);
    acc2 += abs(dx2);
    zacc2 += abs(dz2);
    
    while (acc >= dy && dy != 0) {
      if (dx > 0)
	x++;
      else
	x--;
      acc -= dy;
    }
    while (zacc >= dy && dy != 0) {
      if (dz > 0)
	z++;
      else
	z--;
      zacc -= dy;
    }
    while (acc2 >= dy2 && dy2 != 0) {
      if (dx2 > 0)
	xr++;
      else
	xr--;
      acc2 -= dy2;
    }
    while (zacc2 >= dy2 && dy2 != 0) {
      if (dz2 > 0)
	zr++;
      else
	zr--;
      zacc2 -= dy2;
    }
  }
}


void fillPixel(int x, int y, int z, int r, int g, int b) {
  if(x >= 0 && x < width && y >= 0 && y < height && z > pixels[y][x].z) {
    pixels[y][x].r = r;
    pixels[y][x].g = g;
    pixels[y][x].b = b;
    pixels[y][x].z = z;
  }
}


Mat4 *worldToView(Mat4 *coords) {
  //Mat4 *result = mat4_create(mat4_columns(coords));
  //numMatrices++;
  double screenw = xright - xleft;
  double screenh = ytop - ybottom;
  Mat4 *tmp = mat4_create_identity();
  numMatrices++;
  Mat4 *tmp2 = mat4_create_identity();
  numMatrices++;

  mat4_set(tmp, 0, 0, ((double) width)/screenw);
  mat4_set(tmp, 1, 1, ((double) -height)/screenh);
  mat4_set(tmp, 2, 2, ((double) width)/screenw);

  mat4_set(tmp2, 0, 3, width/2);
  mat4_set(tmp2, 1, 3, height/2);
  tmp = premultiply(tmp2,tmp);
  
  Mat4 *result = multiply(tmp, coords);
  numMatrices++;

  mat4_delete(tmp);
  numMatrices--;

  mat4_delete(tmp2);
  numMatrices--;

  Mat4 *deleter = coords;
  mat4_delete(deleter);
  numMatrices--;

  return result;
}

Mat4 *worldToPerspective(Mat4 *coords) {
  Mat4 *result = mat4_create(mat4_columns(coords));
  numMatrices++;

  double i, x, y, z;
  for (i = 0; i < mat4_columns(coords); i++) {
    x = mat4_get(coords, 0, i);
    y = mat4_get(coords, 1, i);
    z = mat4_get(coords, 2, i);
    x = ex - (ez * (x - ex) / (z - ez));
    y = ey - (ez * (y - ey) / (z - ez));
    mat4_set(result, 0, i, x);
    mat4_set(result, 1, i, y);
    mat4_set(result, 2, i, z);
    mat4_set(result, 3, i, 1);
  }

  Mat4 *deleter = coords;
  mat4_delete(deleter);
  numMatrices--;

  return result;
}

void callocPixels() {
  int i,j;
  if (calloced) {
    int length = sizeof(pixels)/sizeof(pixels[i]);
    for(i = 0; i < length; i++) {
      free(pixels[i]);
      pixels[i] = NULL;
    }
    free(pixels);
    pixels = NULL;
  }
  
  pixels = calloc(height, sizeof(pixel*));
  
  if(pixels == NULL) {
    printf("Initial calloc failed\n");
  }
  
  for(i = 0; i < height; i++) {
    pixels[i] = calloc(width, sizeof(pixel));
    if(pixels[i] == NULL) {
      printf("Calloc failed\n");
    }
    for(j = 0; j < width; j++) {
      pixels[i][j].z = INT_MIN;
      pixels[i][j].r = 0;
      pixels[i][j].g = 0;
      pixels[i][j].b = 0;
    }
  }
  calloced = 1;
}


Mat4 *scale(Mat4 *coords, double sx, double sy, double sz) {
  Mat4 *tmp = mat4_create_identity();
  numMatrices++;

  mat4_set(tmp,0,0,sx);
  mat4_set(tmp,1,1,sy);
  mat4_set(tmp,2,2,sz);
  Mat4 *result = multiply(coords, tmp);
  numMatrices++;

  mat4_delete(tmp);
  numMatrices--;

  Mat4 *deleter = coords;

  mat4_delete(deleter);
  numMatrices--;

  return result;
}

Mat4 *rotatex(Mat4 *coords, double angle) {
  Mat4 *tmp = mat4_create_identity();
  numMatrices++;

  mat4_set(tmp,1,1,cos(angle));
  mat4_set(tmp,1,2,-sin(angle));
  mat4_set(tmp,2,1,sin(angle));
  mat4_set(tmp,2,2,cos(angle));
  Mat4 *result = multiply(coords, tmp);
  numMatrices++;

  mat4_delete(tmp);
  numMatrices--;

  Mat4 *deleter = coords;

  mat4_delete(deleter);
  numMatrices--;

  return result;
}

Mat4 *rotatey(Mat4 *coords, double angle) {
  Mat4 *tmp = mat4_create_identity();
  numMatrices++;

  mat4_set(tmp,2,2,cos(angle));
  mat4_set(tmp,2,0,-sin(angle));
  mat4_set(tmp,0,2,sin(angle));
  mat4_set(tmp,0,0,cos(angle));

  Mat4 *result = multiply(coords, tmp);
  numMatrices++;

  mat4_delete(tmp);
  numMatrices--;
  
  Mat4 *deleter = coords;

  mat4_delete(deleter);
  numMatrices--;

  return result;
}

Mat4 *rotatez(Mat4 *coords, double angle) {
  Mat4 *tmp = mat4_create_identity();
  numMatrices++;
    
  mat4_set(tmp,0,0,cos(angle));
  mat4_set(tmp,0,1,-sin(angle));
  mat4_set(tmp,1,0,sin(angle));
  mat4_set(tmp,1,1,cos(angle));

  Mat4 *result = multiply(coords, tmp);
  numMatrices++;

  mat4_delete(tmp);
  numMatrices--;


  Mat4 *deleter = coords;

  mat4_delete(deleter);
  numMatrices--;

  return result;
}

Mat4 *move(Mat4 *coords, double mx, double my, double mz) {
  Mat4 *tmp = mat4_create_identity();
  numMatrices++;

  mat4_set(tmp,0,3,mx);
  mat4_set(tmp,1,3,my);
  mat4_set(tmp,2,3,mz);
  Mat4 *result = multiply(coords, tmp);
  numMatrices++;

  mat4_delete(tmp);
  numMatrices--;  

  Mat4 *deleter = coords;
  mat4_delete(deleter);
  numMatrices--;

  return result;
}

void varySplit(char *line) {
  char floatString[50];
  int i = -1;
  int j = 0;

  splitline = parse_split(line);

  while(splitline[++i] != NULL) {
    for(j = 0; j < nvars; j++) {
      if(!strcmp(splitline[i],vnames[j])) {
	sprintf(floatString, "%f", vvals[j]);
	int length = strlen(splitline[i]);
	strncpy(splitline[i],floatString, length);
	break;
      }
    }
  }
}
 
