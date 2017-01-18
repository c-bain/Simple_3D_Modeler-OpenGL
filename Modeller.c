//opengl cross platform includes
#include <stdio.h>
#include <stdlib.h>
//#include <vec3.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#  include <GLUT/glut.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/freeglut.h>
#endif

#include<iostream>
#include <fstream>
#include<ctime>
#include<cmath>
#include <string.h>

//the +/- from the cubes centre to the bounding planes
float BOUND_OFFSET=1;
using namespace std;
/* display function - GLUT display callback function
 *clears the screen, draws a square, and displays it
 */
//screen size
int width = 1024;
int height = 800;

//Lighting
int LightingMode = 0; // 0 - off, 1 - on

float light0x=0;
float light0y=5;
float light0z=1;
float light1x=0;
float light1y=5;
float light1z=1;


extern int MapSizeXY = 50;

 
//EYE LOCATION 
float eye[]={5,10,5};
float lookX = 0;
float lookZ = 0;
float deltaAngle = 0;
float deltaAngle2 = 0;
float Xorigin = 0;
float Yorigin = 0;
float angle = 0;
//switch lights and move
float switchlight = 0;


int mouseX = 0, mouseY = 0; //global vars to save mouse x/y coord

float eyeRotate[] = {0,0}; //angle, y - rotation, z - rotaion.


float Object[100][9];//object 0,1,2-position,3,4,5-rotation,6-scale factor,7-material,8-type,9-bounding volume

//0 - black plastic, 1 - cyan plastic, 2 - jade, 3 - chrome, 4 - yellow plastic
float Material[5][10] = {{0,0,0,0.01,0.01,0.01,0.5,0.5,0.5,0.25},
			{0,0.1,0.06,0.0,0.5,0.5,0.5,0.5,0.5,0.25},
			{0.135,0.2225,0.1575,0.54,0.89,0.63,0.316228,0.316228,0.316228,0.1},
			{0.25,0.25,0.25,0.4,0.4,0.4,0.774597,0.774597,0.774597,0.2},			
			{0,0,0,0.5,0.5,0,0.6,0.6,0.5,0.25}	
			};


int ObjectIndex = 0;
int Selected = 0;
int Deleted = 0;
bool intersect = false;
int willSelect=0;

int setNextMaterial = 0;


void ReadFile(void){
	FILE *fp;
	fp = fopen("Save&Load.txt", "r");
	char index[16]={0};	
	fgets(index, 255, (FILE*)fp);
	ObjectIndex=atoi(index);
	char selec[16]={0};	
	fgets(selec, 255, (FILE*)fp);
	Selected=atoi(selec);
	for(int i = 0; i < ObjectIndex; i++){
		for(int j = 0; j<9;j++){
			char Obj[16]={0};	
			fgets(Obj, 255, (FILE*)fp);
			Object[i][j]=atof(Obj);
		}
	}


}

void WriteFile(void){
	FILE *fp;
	fp = fopen("Save&Load.txt", "w+");
	

	char index[16] = {0}; 
	sprintf(index,"%i",ObjectIndex);
	fputs(index, fp);
	fputs("\n", fp);
	char SObject[16] = {0}; 
	sprintf(SObject,"%i", Selected);
	fputs(SObject, fp);
	fputs("\n", fp);
	
	for(int i = 0; i < ObjectIndex; i++){
		for(int j = 0; j<9;j++){
			char buffer[16] = {0}; 
			sprintf(buffer,"%f",Object[i][j]);
			fputs(buffer, fp);
			fputs("\n", fp);
		}
	}
	fclose(fp);

} 

void InitMaterial(void){
	for(int i = 0; i < 10; i++){
		Object[i][7] = -1;

	}

}
void InitType(void){
	for(int i = 0; i < 10; i++){
		Object[i][8] = -1;

	}

}

void InitScale(void){
	for(int i = 0; i < 10; i++){
		Object[i][6] = 0;

	}

}

//calculate weather an intersection of our ray hits the teapot
void CalcIntersections(){
	//---Construct ray-----------------------------------------------------
	//construct Ray
	GLdouble R0[3], R1[3], Rd[3];
	GLdouble modelMat[16], projMat[16];
	GLint viewMat[4];

	//populate mpv matricies
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glGetIntegerv(GL_VIEWPORT, viewMat);

	//calculate near point
	gluUnProject(mouseX, mouseY, 0.0, modelMat, projMat, viewMat, &R0[0], &R0[1], &R0[2]);
	//calculate far point
	gluUnProject(mouseX, mouseY, 1.0, modelMat, projMat, viewMat, &R1[0], &R1[1], &R1[2]);

	//calcualte our ray from R0 and R1
	Rd[0] = R1[0] - R0[0];
	Rd[1] = R1[1] - R0[1];
	Rd[2] = R1[2] - R0[2];

	//turn ray Rd into unit ray 
	GLdouble m = sqrt(Rd[0]*Rd[0] + Rd[1]*Rd[1] + Rd[2]*Rd[2]);
	Rd[0] /= m;
	Rd[1] /= m;
	Rd[2] /= m;

	//---calculate intersection point now-----------------------------------
	//approx the teapot with a box of radius 1 centered around the teapot centered
	//goes against the xy plane to test the Intersection
	//NOTE: this is not the code from slides, but rather proof of concept
	//using assumtions which are true for this example only. 
	for(int i =0; i < 10; i ++){
	//calculate t value from z dir;
	double t = ((Object[i][2]) - R0[2])/Rd[2];

	//printf("t: %f | ", t);

	//use t value to find x and y of our intersection point
	double pt[3];
	pt[0] = R0[0] + t * Rd[0];
	pt[1] = R0[1] + t * Rd[1];
	pt[2] = Object[i][2];
	
	//printf("pt: %f, %f, %f | ", pt[0], pt[1], pt[2]);

	//now that we have our point on the xy plane at the level of the teapot,
	//use it to see if this point is inside a box centered at the teapots
	//location
	if(pt[0] > Object[i][0] - BOUND_OFFSET && pt[0] < Object[i][0] + BOUND_OFFSET &&
		pt[1] > Object[i][1] - BOUND_OFFSET && pt[1] < Object[i][1] + BOUND_OFFSET &&
		pt[2] > Object[i][2] - BOUND_OFFSET && pt[2] < Object[i][2] + BOUND_OFFSET){
		willSelect = i;
		intersect = true;
		BOUND_OFFSET = 1+Object[i][6];

		}
	else
		intersect = false;

	//printf("\n");

	}
}




//draw selected box
void SelectedObject(int type){
	glPushMatrix();
	float x = Object[type][0];
	float y = Object[type][1];
	float z = Object[type][2];
	glColor3f(0,1,0);
	glTranslatef(x,y,z);
		glRotatef(Object[type][3],1,0,0);		
		glRotatef(Object[type][4],0,1,0);
		glRotatef(Object[type][5],0,0,1);
	float s= 1+Object[type][6];
	//cout<<x<<" "<<s<<endl;
	glBegin(GL_LINES);		
		glVertex3f(-s, -s,-s);
		
		glVertex3f(-s, s,-s);		
			
		glVertex3f(-s, -s,-s);
		
		glVertex3f(s, -s,-s);		
		
		glVertex3f(-s, -s,-s);
		//current point
		glVertex3f(-s, -s,s);		
	
		glVertex3f(s, s,s);
		//current point
		glVertex3f(s, s,-s);		
		
		glVertex3f(s, s,s);
		//current point
		glVertex3f(s, -s,s);		
	
		glVertex3f(s, s,s);
		//current point
		glVertex3f(-s, s,s);		
	
		glVertex3f(-s, -s,s);
		//current point
		glVertex3f(-s, s,s);		
	
		glVertex3f(-s, -s,s);
		//current point
		glVertex3f(s, -s,s);		
		
		glVertex3f(-s, s,s);
		//current point
		glVertex3f(-s, s,-s);		
	
	
		glVertex3f(-s, s,-s);
		//current point
		glVertex3f(s, s,-s);		
	
		glVertex3f(s, s,-s);
	
		glVertex3f(s, -s,-s);		
		
		glVertex3f(s, -s,-s);
		//current point
		glVertex3f(s, -s,s);		
	glEnd();
	glPopMatrix();
}


void addObject(int type){//0 - teapot, 1 - sphere
	
	Selected = ObjectIndex;	
	Object[ObjectIndex][8] = type;
	Object[ObjectIndex][0] = MapSizeXY/2;
	Object[ObjectIndex][1] = 1;
	Object[ObjectIndex][2] = MapSizeXY/2;
	ObjectIndex++;
	

}

void deleteObject(void){
	for(int i = Deleted; i < ObjectIndex; i ++ ){
		for(int j = 0; j < 9; j++){
		Object[i][j] = Object[i+1][j];
		}
	}
	for(int k = 0; k < 9; k++){
	Object[ObjectIndex][k] = 0;
	}
	if(Deleted <= Selected){
		Selected = Selected -1;
	}


	ObjectIndex = ObjectIndex -1;

}


void SelectedScaleUp(void){
	Object[Selected][6]+=0.1;

}

void SelectedScaleDown(void){
	Object[Selected][6]-=0.1;


}

void SelectedRotateYL(void){
	Object[Selected][4]+=1;
}

void SelectedRotateYR(void){
	Object[Selected][4]-=1;


}

void SelectedRotateXL(void){
	Object[Selected][3]+=1;
}
void SelectedRotateXR(void){
	Object[Selected][3]-=1;
}

void SelectedRotateZL(void){
	Object[Selected][5]+=1;
}
void SelectedRotateZR(void){
	Object[Selected][5]-=1;
}

void addMaterial(int m){
float m_ambient[4] ={ Material[m][0], Material[m][1], Material[m][2],1.0f };
float m_diffuse[4] ={ Material[m][3],  Material[m][4], Material[m][5], 1.0f };
float m_specular[4] ={ Material[m][6],  Material[m][7], Material[m][8], 1.0f };
float shine[1] = {Material[m][9]};

			 glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_ambient);
			 glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_diffuse);
 			 glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m_specular);
			 glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shine);


}


void drawObject(void){
	for(int i = 0; i < ObjectIndex; i++){
		//glEnable(GL_COLOR_MATERIAL);	
		glDisable(GL_COLOR_MATERIAL);	
		glPushMatrix();	
		float xloca = Object[i][0];
		float yloca = Object[i][1];
		float zloca = Object[i][2];
		glTranslatef(xloca,yloca,zloca);
		glRotatef(Object[i][3],1,0,0);		
		glRotatef(Object[i][4],0,1,0);
		glRotatef(Object[i][5],0,0,1);
		glScalef(1+Object[i][6],1+Object[i][6],1+Object[i][6]);		
		
		if(i == Selected){
			glColor3f(1,0,0);
		}
		else{
			glColor3f(0,1,1);
		}
		if(Object[i][7]!=-1){
			addMaterial(Object[i][7]);
		}
		
		if(Object[i][8] == 0){
			glutSolidTeapot(1);
		}
		else if(Object[i][8] == 1)
			glutSolidSphere(1, 100, 100);
		else if(Object[i][8] == 2)
			glutSolidCube(1);
		else if(Object[i][8] == 3)
			glutSolidCone(1, 1, 100, 100);
		else if(Object[i][8] == 4)
			glutSolidTorus(0.5,1,100,100);
		glPopMatrix();
		
			
	}
}

//enable the light sources
void EnableLight(void){

	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1); 	

	float light_pos[]={light0x,light0y,light0z,0};
	float amb0[4]  = {0.1, 0.1, 0.2, 1}; 
	float diff0[4] = {0.4, 0.2, 0.2, 1}; 
	float spec0[4] = {0.5, 0.3, 0.1, 1}; 
	
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb0); 
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diff0); 
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec0); 
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glDisable(GL_COLOR_MATERIAL);

	float light_pos1[]={light1x,light1y,light1z,0};
	float amb1[4]  = {0, 0.2, 0.2, 1}; 
	float diff1[4] = {0, 0.4, 0.4, 1}; 
	float spec1[4] = {0.3, 0.3, 0.3, 1}; 
	
    	glLightfv(GL_LIGHT1, GL_AMBIENT, amb1); 
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diff1); 
	glLightfv(GL_LIGHT1, GL_SPECULAR, spec1); 
	glLightfv(GL_LIGHT1, GL_POSITION, light_pos1);
glDisable(GL_COLOR_MATERIAL);


}




//initialize
void init(void)
{
	glClearColor(0, 0, 0, 0);
	glColor3f(1, 1, 1);

    	glMatrixMode(GL_PROJECTION);

	glFrustum(-10,10,-10,10,0,100);
	gluPerspective(45,1,1,1000);

}

void Reset(void){
	for(int i = 0; i < sizeof(Object)/9/4; i++){
		for(int j = 0; j<9;j++){
			Object[i][j] = 0;
		}	
	
	}
	ObjectIndex = 0;
	Selected = 0;
	intersect = false;
	willSelect=0;
	switchlight = 0;
	setNextMaterial = 0;
	InitType();
	InitScale();
	InitMaterial();
	eyeRotate[1] = 0;


}
void SetNextMaterial(int j){
	if((ObjectIndex)!=sizeof(Object)/4/9){
		Object[ObjectIndex][7] = j;
	}



}

//keyboard quit 
void keyboard(unsigned char key, int xIn, int yIn){
	switch(key){
		case 'Q':
		case 'q':
		case 27:
			exit(0);
			break;
		case '1':
			if(setNextMaterial == 1)
				SetNextMaterial(0);
			else
				Object[Selected][7] = 0; //add material
			setNextMaterial = 0;
			break;
		case '2':
			if(setNextMaterial == 1)
				SetNextMaterial(1);
			else
				Object[Selected][7] = 1; //add material
			setNextMaterial = 0;			
			break;
		case '3':
			if(setNextMaterial == 1)
				SetNextMaterial(2);
			else
				Object[Selected][7] = 2; //add material
			setNextMaterial = 0;
			break;
		case '4':
			if(setNextMaterial == 1)
				SetNextMaterial(3);
			else
				Object[Selected][7] = 3; //add material
			setNextMaterial = 0;
			break;
		case '5':
			if(setNextMaterial == 1)
				SetNextMaterial(4);
			else
				Object[Selected][7] = 4; //add material
			setNextMaterial = 0;
			break;

		case 'z':
		case 'Z':
			addObject(0); //adding teapot
			break;	
		case 'x':
		case 'X':
			addObject(1); //adding sphere
			break;
		case 'c':
		case 'C':
			addObject(2);
			break;
		case 'v':
		case 'V':
			addObject(3);
			break;
		case 'b':
		case 'B':
			addObject(4);
			break;
		//move around the selected object
		case 'w':

			Object[Selected][0] += 0.3;
			break;
		case 'W':
			Object[Selected][0] -= 0.3;
			break;
		case 'd':
		case 'D':
			Object[Selected][2] += 0.3;
			break;
		case 'a':
		case 'A':
			Object[Selected][2] -= 0.3;
			break;
		case 'e':
			Object[Selected][1] += 0.3;
			break;
		case 'E':
			Object[Selected][1] -= 0.3;
			break;
		//scale up and down
		case 'o':
		case 'O':
			SelectedScaleUp();
			break;
		case 'p':
		case 'P':
			SelectedScaleDown();
			break;
		//rotation
		case 'h':
			SelectedRotateXL();
			break;
		case 'H':
			SelectedRotateXR();
			break;
		case 'j':
			SelectedRotateYL();
			break;
		case 'J':
			SelectedRotateYR();
			break;
		case 'k':
			SelectedRotateZL();
			break;
		case 'K':
			SelectedRotateZR();
			break;
		case 'r':
		case 'R':
			Reset();
			break;
		case 's':
		case 'S':
			WriteFile();
			break;
		case 'l':
		case 'L':
			ReadFile();
			break;
		case 'f':
			switchlight=0;
			break;
		case 'F':
			switchlight = 1;
			break;
		//move light
		case 't':
			if(switchlight == 0)
				light0x -= 0.1;
			else if(switchlight == 1)
				light1x -= 0.1;
			break;
		case 'T':
			if(switchlight == 0)
				light0x += 0.1;
			else if(switchlight == 1)
				light1x += 0.1;
			break;
		case 'y':
			if(switchlight == 0)
				light0y -= 0.1;
			else if(switchlight == 1)
				light1y -= 0.1;
			break;
		case 'Y':
			if(switchlight == 0)
				light0y += 0.1;
			else if(switchlight == 1)
				light1y += 0.1;
			break;
		case 'm':
		case 'M':
			setNextMaterial = 1;
			break;

	}
	glutPostRedisplay();
}




//save our mouse coords when they change
void mouse(int btn, int state, int x, int y){
	mouseX = x;
	mouseY = height - y;


	if(btn == GLUT_LEFT_BUTTON && state == GLUT_UP){
			Selected = willSelect;
	}
	else if(btn == GLUT_RIGHT_BUTTON && state == GLUT_UP){
		
			Deleted = willSelect;		
			deleteObject();
	}
	CalcIntersections();
}

void motion(int x, int y){
	mouseX = x;
	mouseY = height - y;
}


void passive(int x, int y){
	mouseX = x;
	mouseY = height - y;
}

//arrow keys control camera rotation
void special(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_LEFT: 

			eyeRotate[1] +=1;

			break; 
		case GLUT_KEY_RIGHT: 

			eyeRotate[1] -=1;

			break;

		 
		
    }
	glutPostRedisplay();
}

//draw the map with quads
void drawMapQuad(int row, int column){
	glEnable(GL_COLOR_MATERIAL);

	glColor3f(0.6,0.6,0.6);


	


		for (int i=0; i<row; i++) {
			glColor3f(0.3,0.3,0.3);
			for (int j=0; j<column; j++) {
					glNormal3f(i,0,j);
					glFrontFace(GL_CCW);
					glBegin(GL_QUAD_STRIP);						

						glVertex3f(i,0,j+1);
									
						glVertex3f(i+1,0,j+1);
						
						glVertex3f(i,0,j);
						
						glVertex3f(i+1,0,j);
					glEnd();
				
					
			}
			
		}

				for(int i = 0; i < row+1;i++){	
			for(int k = 1; k < column+1; k++){
		
			glBegin(GL_LINES);

				//previous point
				glVertex3f(k-1, 0,i);
				//current point
				glVertex3f(k,0,i);		
			glEnd();
			}	
		}
	

		for(int j = 0; j < column+1;j++){
			for(int l = 1; l < column+1; l++){
			glBegin(GL_LINES);
				//previous point
				glVertex3f(j,0,l-1);
				//current point
				glVertex3f(j,0,l);		
			glEnd();
			}	
		} 

}

void printText(int x, int y, char *string)
{
	//set the position of the text in the window using the x and y coordinates
	glRasterPos2f(x,y);
	//get the length of the string to display
	int len = (int) strlen(string);
	glColor3f(0,1,0);
	//loop to display character by character
	for (int i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,string[i]);
	}
}



void drawHud(void){

 
 
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, width, height, 0.0, -1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	//glPushMatrix();        ----Not sure if I need this
	glLoadIdentity();
	glDisable(GL_CULL_FACE);

	glClear(GL_DEPTH_BUFFER_BIT);

	char x[100] = "Object: ";
	char y[100] = "Position: ";
	char yx[100] = "x: ";
	char yxpos[6];	 
	snprintf (yxpos, 6, "%f", Object[Selected][0]);
	char yy[100] = "y: ";
	char yypos[6];	 
	snprintf (yypos, 6, "%f", Object[Selected][1]);
	char yz[100] = "z: ";
	char yzpos[6];	 
	snprintf (yzpos, 6, "%f", Object[Selected][2]);
	char z[100] = "Rotation: ";
	char zx[100] = "x: ";
	char zxpos[6];	 
	snprintf (zxpos, 6, "%f", Object[Selected][3]);
	char zy[100] = "y: ";
	char zypos[6];	 
	snprintf (zypos, 6, "%f", Object[Selected][4]);
	char zz[100] = "z: ";
	char zzpos[6];	 
	snprintf (zzpos, 6, "%f", Object[Selected][5]);
	//0 - black plastic, 1 - cyan plastic, 2 - jade, 3 - chrome, 4 - yellow plastic
	char mat[100] = "Material: ";
	if(Object[Selected][7] == 0){
			strcat(mat,"black plastic");
	}
	else if(Object[Selected][7] == 1){
			strcat(mat,"cyan plastic");
	}
	else if(Object[Selected][7] == 2){
			strcat(mat,"jade");
	}
	else if(Object[Selected][7] == 3){
			strcat(mat,"chrome");
	}
	else if(Object[Selected][7] == 4){
			strcat(mat,"yellow plastic");
	}

	if(Object[Selected][8] == 0){
			strcat(x,"teapot");
	}
	else if(Object[Selected][8] == 1){
			strcat(x,"sphere");
	}
	else if(Object[Selected][8] == 2){
			strcat(x,"cube");
	}
	else if(Object[Selected][8] == 3){
			strcat(x,"cone");
	}
	else if(Object[Selected][8] == 4){
			strcat(x,"torus");
	}
	

	glColor3f(0,1,0);
	//object type
	printText(10,30,x);
	printText(170,30,mat);
	//position
	printText(10,60,y);
	printText(10,90,yx);
	printText(30,90,yxpos);
	printText(100,90,yy);
	printText(120,90,yypos);
	printText(190,90,yz);
	printText(210,90,yzpos);
	//rotation
	printText(10,120,z);
	printText(10,150,zx);
	printText(30,150,zxpos);
	printText(100,150,zy);
	printText(120,150,zypos);
	printText(190,150,zz);
	printText(210,150,zzpos);

	// Making sure we can render 3d again
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

}


//display the windows and the content
void display(void)
{	
	
	glClearColor(0.5,0.5,0.5,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	EnableLight();
	//ignore hidden faces
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glPushMatrix();
	glRotatef(eyeRotate[0],1,0,0);
	
	gluLookAt(eye[0], eye[1], eye[2], MapSizeXY/2+lookX, 0, MapSizeXY/2+lookZ, 0, 1, 0);


	glPushMatrix();

	glTranslatef( MapSizeXY/2, 0, MapSizeXY/2);
	
	glRotatef(eyeRotate[1],0,1,0);
	glTranslatef( -MapSizeXY/2, 0, -MapSizeXY/2);


	drawMapQuad(MapSizeXY,MapSizeXY);
	SelectedObject(Selected);
	drawObject();
	glDisable(GL_LIGHTING);	
	drawHud();
	glEnable(GL_LIGHTING);
	glPopMatrix();




	
	glutSwapBuffers();
}

 
//make our animation run at 60fps
void FPS(int val){
	glutPostRedisplay();
	glutTimerFunc(17, FPS, 0); // 1sec = 1000, 60fps = 1000/60 = ~17
}

//callbacks registry
void callBackInit(){
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutSpecialFunc(special);
	glutPassiveMotionFunc(passive);
	//glutReshapeFunc(reshape);
	glutTimerFunc(0, FPS, 0);
}


/* main function - program entry point */
int main(int argc, char** argv)
{		

	InitType();
	InitScale();
	InitMaterial();
	glutInit(&argc, argv);		//starts up GLUT
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(width,height);
	glutInitWindowPosition(0,0);
		
	glutCreateWindow("Assignment 3");	//creates the window

	callBackInit();
	init();
	glEnable(GL_DEPTH_TEST);
	glutMainLoop();				//starts the event loop

	return(0);					//return may not be necessary on all compilers
}