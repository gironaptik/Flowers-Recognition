#define _CRT_SECURE_NO_WARNINGS
#include "GLUT.h"
#include <math.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <time.h>
#include <Windows.h>

////Giron Aptik
////ID:307863258

const int W = 600; // window width
const int H = 600; // window height

const int TMPSZ = 512;
const int SCRSZ = 256;
const int INPUT_SZ = SCRSZ+1;
const int HIDDEN_SZ = INPUT_SZ / 2+1;
const int OUTPUT_SZ = 3;
const int REGSIZE = 15;
const int TEST = 10000;

unsigned char picture[TMPSZ][TMPSZ][3]; // for R,G,B
unsigned char screen[SCRSZ][SCRSZ][3]; // for R,G,B
unsigned char squares[SCRSZ][SCRSZ][3]; // for R,G,B

double input[INPUT_SZ];
double hidden[HIDDEN_SZ];
double output[OUTPUT_SZ];
double i2h[INPUT_SZ][HIDDEN_SZ];
double h2o[HIDDEN_SZ][OUTPUT_SZ];
double error[OUTPUT_SZ];
double delta_output[OUTPUT_SZ];
double delta_hidden[HIDDEN_SZ];
int network_digit = -1, tutor_digit = -1, kind = -1, number = -1, success = 0, failure =0;
double learning_rate = 0.1;

char flowersKinds[3][REGSIZE] = { "chrysantemum", "rose", "tolip" };
unsigned char* bmp;
char* flowerName;

void Clean();
void selfTest(int kind, int number);
void Backpropagation();
void HPF();
void FeedForward();


void LoadBitmap(  char * filename)
{
	free(bmp);
	int sz;
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;
	FILE* pf = fopen(filename, "rb"); // read binary file
	fread(&bf, sizeof(bf), 1, pf);
	fread(&bi, sizeof(bi), 1, pf);
	sz = bi.biHeight * bi.biWidth*3;
	bmp = (unsigned char*) malloc(sz);
	fread(bmp, 1, sz, pf);
	fclose(pf);
}

char* PickBitmap(char* name,int number)
{
	std::stringstream comb;
	comb << name[0] << number <<".bmp";
	std::string s = comb.str();
	flowerName = new char[s.length() + 1];
	std::copy(s.c_str(), s.c_str() + s.length() + 1, flowerName);
	std::cout << "Flower name:   " << flowerName << "\n";
	return flowerName;
}

void init()
{
	int k,sz = TMPSZ*TMPSZ *3;
	int i, j;
	srand(time(NULL));

	// set random weights
	for (i = 0; i < INPUT_SZ; i++)
		for (j = 0; j < HIDDEN_SZ; j++)
			i2h[i][j] = ((rand() % 1000) - 500) / 1000.0;

	// set random weights
	for (i = 0; i < HIDDEN_SZ; i++)
		for (j = 0; j < OUTPUT_SZ; j++)
			h2o[i][j] = ((rand() % 1000) - 500) / 1000.0;

	for (i = 0; i < TEST; i++) {
		kind = rand() % 3;
		number = rand() % (6) + 1;
		selfTest(kind, number);
		if(i >= TEST*0.9 && i<= TEST){
			if (tutor_digit == network_digit)
				success += 1;
			else
				failure += 1;
		}
	}
	std::cout << "The last 10% of the tests had: " << success << " Success and: " << failure << " of Failure";
	tutor_digit = -1;
	Clean();

	LoadBitmap("rose_test.bmp");
	for (k = 0,j=0,i=0; k < sz; k+=3)
	{
		picture[i][j][2] = bmp[k]; //blue
		picture[i][j][1] = bmp[k+1]; // green
		picture[i][j][0] = bmp[k+2]; // red
		j++;
		if (j == TMPSZ) // fill next line
		{
			j = 0;
			i++;
		}
	}
	// copy picture to screen
	for(i=0;i<SCRSZ;i++)
		for (j = 0; j < SCRSZ; j++)
		{
	screen[i][j][0] = (picture[i * 2][j * 2][0] + picture[i * 2][j * 2 + 1][0] +
		  			   picture[i * 2 + 1][j * 2][0] + picture[i * 2 + 1][j * 2 + 1][0])/4;
	screen[i][j][1] = (picture[i * 2][j * 2][1] + picture[i * 2][j * 2 + 1][1] +
		picture[i * 2 + 1][j * 2][1] + picture[i * 2 + 1][j * 2 + 1][1])/4;
	screen[i][j][2] = (picture[i * 2][j * 2][2] + picture[i * 2][j * 2 + 1][2] +
		picture[i * 2 + 1][j * 2][2] + picture[i * 2 + 1][j * 2 + 1][2])/4;

		}

	glClearColor(0.3, 0.3, 0.3, 0);
	glOrtho(-1, 1, -1, 1, -1, 1);
}

void DrawSquares()
{
	double top = 0.35;
	double left = 0.1;
	double right = 0.2;
	double bottom = 0.25;

	for (int i = 0; i < OUTPUT_SZ; i++)
	{
		if (tutor_digit == i)
			glColor3d(0, 0.7, 0);
		else if (network_digit == i)
			glColor3d(0.7, 0.6, 0.2);
		 
		else glColor3d(0.5, 0.5, 0.5);

		glBegin(GL_POLYGON);
			glVertex2d(left, top);
			glVertex2d(right, top);
			glVertex2d(right, bottom);
			glVertex2d(left, bottom);
		glEnd();

		glColor3d(1, 1, 1);

		glRasterPos2d(left + 0.15, bottom + 0.02);

		for (int j = 0; j < REGSIZE; j++) {
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, flowersKinds[i][j]);
		}
		top -= 0.2;
		bottom -= 0.2;
	}
}
void HPF()
{
	int i, j;
	for (i = 1; i < SCRSZ - 1; i++)
		for (j = 1; j < SCRSZ - 1; j++)
			squares[i][j][0] = squares[i][j][1] = squares[i][j][2] =
			(int)fabs(4*screen[i][j][0] - screen[i-1][j][0] -
				screen[i+1][j][0] - screen[i][j-1][0]- screen[i][j+1][0]);
}


int MaxOutput()
{
	int i, max = 0;
	for (i = 1; i < OUTPUT_SZ; i++)
		if (output[i] > output[max])
			max = i;
	return max;
}

void Clean()
{
	int i, j;
	for (i = 0; i<SCRSZ; i++)
		for (j = 0; j < SCRSZ; j++)
		{
			screen[i][j][0] = 255;
			screen[i][j][1] = 255;
			screen[i][j][2] = 255;
		}
	for (i = 0; i<SCRSZ; i++)
		for (j = 0; j < SCRSZ; j++)
		{
			squares[i][j][0] = 255;
			squares[i][j][1] = 255;
			squares[i][j][2] = 255;
		}	
	//network_digit = -1;
}

void FeedForward()
{
	int i, j;
	// 1. setup input layer
	for(i=8;i<SCRSZ;i+=16)
		for (j = 8; j < SCRSZ; j += 16)
		{
			if (squares[i][j][0] == 0) // 
			{
				input[(i / 16) * 16 + (j / 16)] = 0;
				//printf("%c", 1);
			}
			else
			{
				input[(i / 16) * 16 + (j / 16)] = 1;
				//printf("%c", 2);
			}
			//if ((j / 16 + 1) % 16 == 0)
				//printf("\n");
		}

	input[INPUT_SZ - 1] = 1; // bias for input layer
	// 2. getting Hidden layer
	for (i = 0; i < HIDDEN_SZ; i++)
		hidden[i] = 0;
	for (i = 0; i < INPUT_SZ; i++)
		for (j = 0; j < HIDDEN_SZ; j++)
		{
			hidden[j] += input[i] * i2h[i][j];
		}
	// add sigmoid
	for (i = 0; i < HIDDEN_SZ; i++)
		hidden[i] = 1 / (1 + exp(-(hidden[i])));

	// set bias for hidden layer
	hidden[HIDDEN_SZ - 1] = 1;
	// 3. getting output layer
	for (i = 0; i < OUTPUT_SZ; i++)
		output[i] = 0;
	for (i = 0; i < HIDDEN_SZ; i++)
		for (j = 0; j < OUTPUT_SZ; j++)
		{
			output[j] += hidden[i] * h2o[i][j];
		}

	// add sigmoid
	for (i = 0; i < OUTPUT_SZ; i++) {
		output[i] = 1 / (1 + exp((output[i])));
	}

	// show it
	printf("\nOUTPUT\n");
	for (i = 0; i < OUTPUT_SZ; i++)
		printf("%.3lf ", output[i]);

	printf("\n");

	network_digit = MaxOutput();

}

void Backpropagation()
{
	int i, j, k;
	// 1. Compute error E = (t(i)-y(i))
	for (i = 0; i < OUTPUT_SZ; i++)
	{
		if (i == tutor_digit)
			error[i] = (1 - output[i]);
		else
			error[i] = -output[i];
	}
	// 2. compute delta of output layer
	for (i = 0; i < OUTPUT_SZ; i++)
	{
		delta_output[i] = output[i] * (1 - output[i])*error[i];
	}
	// 3.  compute delta of hidden layer
	for (j = 0; j < HIDDEN_SZ; j++)
	{
		double tmp = 0;
		for (k = 0; k < OUTPUT_SZ; k++)
			tmp += delta_output[k] * h2o[j][k];

		delta_hidden[j] = hidden[j] * (1 - hidden[j])*tmp;
	}
	// 4. update weights in h2o
	for (i = 0; i < HIDDEN_SZ; i++)
		for (j = 0; j < OUTPUT_SZ; j++)
			h2o[i][j] -= learning_rate * hidden[i] * delta_output[j];
	// 5. update weights in i2h
	for (i = 0; i < INPUT_SZ; i++)
		for (j = 0; j < HIDDEN_SZ; j++)
			i2h[i][j] -= learning_rate * input[i] * delta_hidden[j];

	Clean();

}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	DrawSquares();
	// show screen
	glRasterPos2d(-0.95, 0);
	glDrawPixels(SCRSZ, SCRSZ, GL_RGB,
		GL_UNSIGNED_BYTE, screen);
	// show squares
	glRasterPos2d(-0.95, -0.95);
	glDrawPixels(SCRSZ, SCRSZ, GL_RGB,
		GL_UNSIGNED_BYTE, squares);
	glutSwapBuffers();// show what was drawn in "frame buffer"
}

void idle()
{
	glutPostRedisplay();// calls indirectly to display
}



void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		if (x > W / 40 && x<W / 40 + SCRSZ && y >  11* H / 20 && y < 11 * H / 20 + SCRSZ) // click in bottom screen
		{
			HPF();
			FeedForward();
		}
		if (x >= 11 * W / 20 && x <= 11 * W / 20+ W / 20 && y > H / 3 && y < 7 * H / 12)
		{
			tutor_digit = y / (H / 10) - 3;
			// start Backpropagation
			Backpropagation();
		}

	}
}

void selfTest(int kind, int number) {
	int k, sz = TMPSZ * TMPSZ * 3;
	int i, j;
	LoadBitmap(PickBitmap(flowersKinds[kind], number));
	for (k = 0, j = 0, i = 0; k < sz; k += 3)
	{
		picture[i][j][2] = bmp[k]; //blue
		picture[i][j][1] = bmp[k + 1]; // green
		picture[i][j][0] = bmp[k + 2]; // red
		j++;
		if (j == TMPSZ) // fill next line
		{
			j = 0;
			i++;
		}
	}
	// copy picture to screen
	for (i = 0; i < SCRSZ; i++)
		for (j = 0; j < SCRSZ; j++)
		{
			screen[i][j][0] = (picture[i * 2][j * 2][0] + picture[i * 2][j * 2 + 1][0] +
				picture[i * 2 + 1][j * 2][0] + picture[i * 2 + 1][j * 2 + 1][0]) / 4;
			screen[i][j][1] = (picture[i * 2][j * 2][1] + picture[i * 2][j * 2 + 1][1] +
				picture[i * 2 + 1][j * 2][1] + picture[i * 2 + 1][j * 2 + 1][1]) / 4;
			screen[i][j][2] = (picture[i * 2][j * 2][2] + picture[i * 2][j * 2 + 1][2] +
				picture[i * 2 + 1][j * 2][2] + picture[i * 2 + 1][j * 2 + 1][2]) / 4;
		}
	HPF();
	FeedForward();
	tutor_digit = kind;
	Backpropagation();

}

void main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(200, 100);
	glutCreateWindow("Flowers Recognition: "); 
	glutDisplayFunc(display); // refresh function
	glutIdleFunc(idle); // idle: when nothing happens
	//glutMotionFunc(drag); // 
	glutMouseFunc(mouse);
	init();
	glutMainLoop();
}