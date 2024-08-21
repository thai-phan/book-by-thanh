#include <windows.h>
#include <gl/soil.h>
#include <gl/glut.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <vector>
#define MAX 1000 // mac dinh toi da so luong trang co the co

using namespace std;

// cau truc diem
struct point2f{
	GLfloat x;
	GLfloat y;
	point2f(){	}
	point2f(GLfloat a, GLfloat b){
		x = a;
		y = b;
	}
};

static RECT rc;
static int width, height, window, menu_id, color_menu, shape_menu, value = 0; // cac tham so cho menu
static GLfloat deltaP = 0.01, deltaT = 0.001, deltaS = 0.002, deltaF = 0.05; // do day mong cua cuon sach
static GLuint bgTexture, frontCover, backCover; // texture luu anh nen, bia truoc sau
static GLuint texture[MAX][2]; // luu tru texture load tu hinh anh, 0 cho trang truoc va 1 cho trang sau
static GLint isPick[MAX][2] = {0}; // mang danh dau trang dc chon de chen anh, 0 cho trang trc va 1 cho trang sau
static GLfloat pageHeight = 16.0, pageWidth = 9.0; // do cao, rong cua trang sach
static GLfloat x_max = 10.0, y_max = 10.0, z_max = 10.0, depth = 0; // cac chi so cua khung nhin, bien luu do sau de ve trang
static GLint yrot[MAX] = {0}, count = 0, countC = 0; // bien quay, dem tong quat, va dem noi ham drawPage
static GLint numberOfPages; // bien toan cuc luu so trang sach nguoi dung can tao
static GLint xrot = 0, xdiff = 0, ydiff = 0; // cac bien tham so cho mouseMotion
static char* nameArray[MAX][2] = {NULL}; // luu duong dan cho texture
static int isPaging = 0, drawColor = 1; // flag lat trang va mau ve
static point2f pt1, pt2, pt3, pt4; // ve hinh tam thoi
static ofstream fout; // out stream
static ifstream fin; // in stream 
static bool mouseDown = false, curling = true, leftRight = false, drawing = false, line = false, point = false; // flag danh dau che do ve
static point2f p_root(0,0); // dung luu tru diem click chuot ban dau
static GLfloat order0 = 0, order1 = 0;


// ham dung windows api de mo cua so load ten file anh
static string GetFileName( const string & prompt ) { 
    const int BUFSIZE = 1024;
    char buffer[BUFSIZE] = {0};
    OPENFILENAME ofns = {0};
    ofns.lStructSize = sizeof( ofns );
    ofns.lpstrFile = buffer;
    ofns.nMaxFile = BUFSIZE;
    ofns.lpstrTitle = prompt.c_str();
    GetOpenFileName( & ofns );
    return buffer;
}

// ham lay duong dan thu muc chuong trinh dang duoc thuc thi
static string ExePath() {
    char buffer[MAX_PATH];
    GetModuleFileName( NULL, buffer, MAX_PATH );
    string::size_type pos = string( buffer ).find_last_of( "\\/" );
    return string( buffer ).substr( 0, pos);
}

// convert int to string
static string to_string ( int Number ){
	stringstream ss;
	ss << Number;
	return ss.str();
}

// ham dung thu vien soil de load texture tu hinh anh
static void loadTexture(GLuint& texture, char* fileName){
	texture = SOIL_load_OGL_texture //
    	(
        	fileName,
        	SOIL_LOAD_AUTO,
        	SOIL_CREATE_NEW_ID,
        	SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
    	);
}

// tai texture tu file texture.dat o thu muc hien tai
static void loadTextureFile(){
	SetCurrentDirectory(ExePath().c_str());
	fin.open("data\\texture.dat");
	if(fin.is_open()){
		string temp;
		for(int i = 1; i < numberOfPages+1; i++){
			getline(fin, temp);
			if(temp != "/n"){
				nameArray[i][0] = new char[temp.length() + 1];
				strcpy(nameArray[i][0], temp.c_str());
				loadTexture(texture[i][0], nameArray[i][0]);
				isPick[i][0] = 1;
			}
			getline(fin, temp);
			if(temp != "/n"){
				nameArray[i][1] = new char[temp.length() + 1];
				strcpy(nameArray[i][1], temp.c_str());
				loadTexture(texture[i][1], nameArray[i][1]);
				isPick[i][1] = 1;
			}
			
		}
	}
	else{
		cout << "Texture file does not exist!" << endl;
	}
	fin.close();	
}

// ham luu lai duong dan file anh da tai vao texture
static void saveTexture(){
	SetCurrentDirectory(ExePath().c_str());
	fout.open("data\\texture.dat", ios::out | ios::trunc);
	
	if(fout.is_open()){
		for(int i = 1; i < numberOfPages+1; i++){
			if(nameArray[i][0] != NULL)
				fout << nameArray[i][0] << endl;
			else
				fout << "\n";
			if(nameArray[i][1] != NULL)
				if(i < numberOfPages + 1)
					fout << nameArray[i][1] << endl;
				else
					fout << nameArray[i][1];
			else
				fout << "\n";
		}
	}
	else
		cout << "Save texture fail!" << endl;
	fout.close();
}

// ham khoi tao
void init(void){
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glLineWidth(2);
	glPointSize(5);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    loadTexture(bgTexture, "image\\background.jpg"); // tai anh lam anh nen
    loadTexture(frontCover, "image\\frontcover.jpg"); // tai anh lam bia truoc
    loadTexture(backCover, "image\\backcover.jpg"); // tai anh lam bia sau
	loadTextureFile();
	if(CreateDirectory("data", NULL) || ERROR_ALREADY_EXISTS == GetLastError()){
		cout << "Init done!" << endl;
	}
	else{
		cout << "Failed to create data directory!" << endl;
	}
	
}

// ham ve line
void drawLine(point2f p1, point2f p2, int side){
	if(side == 0){ // ve cho mat truoc
		if(p1.x <= p2.x){
			if(p1.y <= p2.y){
				glBegin(GL_POLYGON);
					glVertex3f(p1.x*2.5,p1.y*1.2,depth+deltaS+order0);
					glVertex3f(p2.x*2.5,p2.y*1.2,depth+deltaS+order0);
					glVertex3f(p2.x*2.5-0.05,p2.y*1.2+0.05,depth+deltaS+order0);
					glVertex3f(p1.x*2.5-0.05,p1.y*1.2+0.05,depth+deltaS+order0);
				glEnd();
			}
			if(p1.y > p2.y){
				glBegin(GL_POLYGON);
					glVertex3f(p1.x*2.5,p1.y*1.2,depth+deltaS+order0);
					glVertex3f(p2.x*2.5,p2.y*1.2,depth+deltaS+order0);
					glVertex3f(p2.x*2.5+0.05,p2.y*1.2+0.05,depth+deltaS+order0);
					glVertex3f(p1.x*2.5+0.05,p1.y*1.2+0.05,depth+deltaS+order0);
				glEnd();			
			}
		}
		else {
			if(p1.y <= p2.y){
				glBegin(GL_POLYGON);
					glVertex3f(p1.x*2.5,p1.y*1.2,depth+deltaS+order0);
					glVertex3f(p2.x*2.5,p2.y*1.2,depth+deltaS+order0);
					glVertex3f(p2.x*2.5+0.05,p2.y*1.2+0.05,depth+deltaS+order0);
					glVertex3f(p1.x*2.5+0.05,p1.y*1.2+0.05,depth+deltaS+order0);
				glEnd();			
			}
			if(p1.y > p2.y){
				glBegin(GL_POLYGON);
					glVertex3f(p1.x*2.5,p1.y*1.2,depth+deltaS+order0);
					glVertex3f(p2.x*2.5,p2.y*1.2,depth+deltaS+order0);
					glVertex3f(p2.x*2.5-0.05,p2.y*1.2+0.05,depth+deltaS+order0);
					glVertex3f(p1.x*2.5-0.05,p1.y*1.2+0.05,depth+deltaS+order0);
				glEnd();			
			}
		}
	}
	if(side == 1){ // ve mat sau
		if(p1.x <= p2.x){
			if(p1.y <= p2.y){
				glBegin(GL_POLYGON);
					glVertex3f(-p1.x*2.5,p1.y*1.2,depth-deltaS-order1);
					glVertex3f(-p2.x*2.5,p2.y*1.2,depth-deltaS-order1);
					glVertex3f(-p2.x*2.5+0.05,p2.y*1.2+0.05,depth-deltaS-order1);
					glVertex3f(-p1.x*2.5+0.05,p1.y*1.2+0.05,depth-deltaS-order1);
				glEnd();
			}
			else{
				glBegin(GL_POLYGON);
					glVertex3f(-p1.x*2.5,p1.y*1.2,depth-deltaS-order1);
					glVertex3f(-p2.x*2.5,p2.y*1.2,depth-deltaS-order1);
					glVertex3f(-p2.x*2.5-0.05,p2.y*1.2+0.05,depth-deltaS-order1);
					glVertex3f(-p1.x*2.5-0.05,p1.y*1.2+0.05,depth-deltaS-order1);
				glEnd();			
			}
		}	
		else {
			if(p1.y <= p2.y){
				glBegin(GL_POLYGON);
					glVertex3f(-p1.x*2.5,p1.y*1.2,depth-deltaS-order1);
					glVertex3f(-p2.x*2.5,p2.y*1.2,depth-deltaS-order1);
					glVertex3f(-p2.x*2.5-0.05,p2.y*1.2+0.05,depth-deltaS-order1);
					glVertex3f(-p1.x*2.5-0.05,p1.y*1.2+0.05,depth-deltaS-order1);
				glEnd();			
			}
			else{
				glBegin(GL_POLYGON);
					glVertex3f(-p1.x*2.5,p1.y*1.2,depth-deltaS-order1);
					glVertex3f(-p2.x*2.5,p2.y*1.2,depth-deltaS-order1);
					glVertex3f(-p2.x*2.5+0.05,p2.y*1.2+0.05,depth-deltaS-order1);
					glVertex3f(-p1.x*2.5+0.05,p1.y*1.2+0.05,depth-deltaS-order1);
				glEnd();			
			}
		}
	}	
}

// chon mau ve
void chooseColor(int code){
	switch(code){
		case 1:{
			glColor3f(1.0, 0.0, 0.0);
			break;
		}
		case 2:{
			glColor3f(0.0, 1.0, 0.0);
			break;
		}
		case 3:{
			glColor3f(0.0, 0.0, 1.0);
			break;
		}
		case 4:{
			glColor3f(0.0, 0.0, 0.0);
			break;
		}
		case 5:{
			glColor3f(1.0, 1.0, 0.0);
			break;
		}
		case 6:{
			glColor3f(1.0, 0.0, 1.0);
			break;
		}
		case 7:{
			glColor3f(0.0, 1.0, 1.0);
			break;
		}
		case 8:{
			glColor3f(1.0, 1.0, 1.0);
			break;
		}
	}
}

// ham ve trang sach
void drawPage(){
	// bat dau ve
	glPushMatrix();
	glRotatef(yrot[countC], 0.0, 1.0, 0.0); // Rotate cho trang sach
	
	glDisable(GL_TEXTURE_2D); // tat che do texture de ve khong bi sai mau
	

	// ve trang sach mac dinh
	if(countC == 0 || countC == numberOfPages+1){ // ve mau cho phan ben trong cua trang bia
		 glColor3f(0.258824 ,0.435294 ,0.258824); // MediumSeaGreen
	}
	else{ // co the dung nhieu mau khac nhau cho trang sach, xoa cmt va cmt dong cu de thu nghiem
	//glColor3f(0.623529, 0.623529, 0.372549); // khaki
	//glColor3f(0.658824,0.658824,0.658824); // LightGray
	glColor3f(0.847059,0.847059,0.74902); // Wheat
	}
	glBegin(GL_POLYGON);
		glVertex3f(0, pageHeight/2, depth);
		glVertex3f(pageWidth, pageHeight/2, depth);
		glVertex3f(pageWidth, -pageHeight/2, depth);
		glVertex3f(0, -pageHeight/2, depth);
	glEnd();

	glEnable(GL_TEXTURE_2D); // bat che do texture
	
	// kiem tra bien isPick de dan texture hinh anh, neu = 1 tien hanh anh xa tu mang texture[]	
	
	if(countC > 0 && countC < numberOfPages + 2){ // kiem tra co phai la bia sach khong
		// kiem tra mat sau va anh xa
		if(isPick[countC][1] == 1){
			glBindTexture(GL_TEXTURE_2D, texture[countC][1]);
			glBegin(GL_QUADS);
			glTexCoord2f(1,1);glVertex3f(0, 2*pageHeight/6, depth-deltaT);
			glTexCoord2f(0,1);glVertex3f(pageWidth, 2*pageHeight/6, depth-deltaT);
			glTexCoord2f(0,0);glVertex3f(pageWidth, -2*pageHeight/6, depth-deltaT);
			glTexCoord2f(1,0);glVertex3f(0, -2*pageHeight/6, depth-deltaT);
			glEnd();
		}
	
		// kiem tra mat truoc va anh xa
		if(isPick[countC][0] == 1){
			glBindTexture(GL_TEXTURE_2D, texture[countC][0]);
			glBegin(GL_QUADS);
			glTexCoord2f(0,1);glVertex3f(0, 2*pageHeight/6, depth+deltaT);
			glTexCoord2f(1,1);glVertex3f(pageWidth, 2*pageHeight/6, depth+deltaT);
			glTexCoord2f(1,0);glVertex3f(pageWidth, -2*pageHeight/6, depth+deltaT);
			glTexCoord2f(0,0);glVertex3f(0, -2*pageHeight/6, depth+deltaT);
			glEnd();
		}
		
		glDisable(GL_TEXTURE_2D);
		// tai file hinh cho mat truoc
		fin.open((ExePath().append("\\data\\").append(to_string(countC)).append("0.dat")).c_str());
		if(fin.is_open()){
			int shapeNum = 0, color, shape;
			point2f t1,t2;
			fin >> shapeNum;
			if(shapeNum != 0){
				for(int i = 0; i < shapeNum; i++){
					fin >> color;
					chooseColor(color);
					fin >> shape;
					if(shape == 1){
						order0 += 0.0001;
						fin >> t1.x >> t1.y >> t2.x >> t2.y;
						glBegin(GL_POLYGON);
							glVertex3f(t1.x*2.5,t1.y*1.2,depth+deltaS+order0);
							glVertex3f(t2.x*2.5,t1.y*1.2,depth+deltaS+order0);
							glVertex3f(t2.x*2.5,t2.y*1.2,depth+deltaS+order0);
							glVertex3f(t1.x*2.5,t2.y*1.2,depth+deltaS+order0);
						glEnd();					
					}
					if(shape == 2){
						order0 += 0.0001;
						fin >> t1.x >> t1.y >> t2.x >> t2.y;
						drawLine(t1, t2, 0);
					}					
					if(shape == 3){
						order0 += 0.0001;
						fin >> t1.x >> t1.y;
						glBegin(GL_POLYGON);
							glVertex3f(t1.x*2.5-deltaF, t1.y*1.2+deltaF, depth+deltaS+order0);
							glVertex3f(t1.x*2.5+deltaF, t1.y*1.2+deltaF, depth+deltaS+order0);
							glVertex3f(t1.x*2.5+deltaF, t1.y*1.2-deltaF, depth+deltaS+order0);
							glVertex3f(t1.x*2.5-deltaF, t1.y*1.2-deltaF, depth+deltaS+order0);
						glEnd();
					}

				}				
			}
		}
		fin.close();
		
		// tai file hinh cho mat sau
		fin.open((ExePath().append("\\data\\").append(to_string(countC)).append("1.dat")).c_str());
		if(fin.is_open()){
			int shapeNum = 0, color, shape;
			point2f t1,t2;
			fin >> shapeNum;
			if(shapeNum != 0){
				for(int i = 0; i < shapeNum; i++){
					fin >> color;
					chooseColor(color);	
					fin >> shape;
					if(shape == 1){
						order1 += 0.0001;
						fin >> t1.x >> t1.y >> t2.x >> t2.y;
						glBegin(GL_QUADS);
							glVertex3f(-t1.x*2.5,t1.y*1.2,depth-deltaS-order1);
							glVertex3f(-t2.x*2.5,t1.y*1.2,depth-deltaS-order1);
							glVertex3f(-t2.x*2.5,t2.y*1.2,depth-deltaS-order1);
							glVertex3f(-t1.x*2.5,t2.y*1.2,depth-deltaS-order1);
						glEnd();					
					}
					if(shape == 2){
						order1 += 0.0001;
						fin >> t1.x >> t1.y >> t2.x >> t2.y;
						drawLine(t1, t2, 1);
					}					
					if(shape == 3){
						order1 += 0.000001;
						fin >> t1.x >> t1.y;
						glBegin(GL_POLYGON);
							glVertex3f(-t1.x*2.5-deltaF, t1.y*1.2+deltaF, depth-deltaS-order1);
							glVertex3f(-t1.x*2.5+deltaF, t1.y*1.2+deltaF, depth-deltaS-order1);
							glVertex3f(-t1.x*2.5+deltaF, t1.y*1.2-deltaF, depth-deltaS-order1);
							glVertex3f(-t1.x*2.5-deltaF, t1.y*1.2-deltaF, depth-deltaS-order1);
						glEnd();					
					}

				}				
			}
		}
		fin.close();
		
		// hien thi tam thoi cho cac hinh dang ve
		if(!(pt1.x == 0 && pt2.x == 0)){ // mat truoc
			chooseColor(drawColor);
			if(drawing){
				order0 += 0.0001;
				glBegin(GL_POLYGON);
					glVertex3f(pt1.x*2.5,pt1.y*1.2,depth+deltaS+order0);
					glVertex3f(pt2.x*2.5,pt1.y*1.2,depth+deltaS+order0);
					glVertex3f(pt2.x*2.5,pt2.y*1.2,depth+deltaS+order0);
					glVertex3f(pt1.x*2.5,pt2.y*1.2,depth+deltaS+order0);
				glEnd();				
			}
			if(line){
			order0 += 0.0001;
				drawLine(pt1, pt2, 0);
			}
		}
		if(!(pt3.x == 0 && pt4.x == 0)){ // mat sau
			chooseColor(drawColor);
			if(drawing){
				order1 += 0.0001;
				glBegin(GL_POLYGON);
					glVertex3f(-pt3.x*2.5,pt3.y*1.2,depth-deltaS-order1);
					glVertex3f(-pt4.x*2.5,pt3.y*1.2,depth-deltaS-order1);
					glVertex3f(-pt4.x*2.5,pt4.y*1.2,depth-deltaS-order1);
					glVertex3f(-pt3.x*2.5,pt4.y*1.2,depth-deltaS-order1);
				glEnd();				
			}
			if(line){
				order1 += 0.0001;
				drawLine(pt3, pt4, 1);
			}
		}
	}
	
	glEnable(GL_TEXTURE_2D);
	// ve bia truoc
	if(countC == 0){
		glBindTexture(GL_TEXTURE_2D, frontCover);
		glBegin(GL_QUADS);
		glTexCoord2f(0,1);glVertex3f(0, pageHeight/2, depth+deltaT);
		glTexCoord2f(1,1);glVertex3f(pageWidth, pageHeight/2, depth+deltaT);
		glTexCoord2f(1,0);glVertex3f(pageWidth, -pageHeight/2, depth+deltaT);
		glTexCoord2f(0,0);glVertex3f(0, -pageHeight/2, depth+deltaT);
		glEnd();
	}
	// ve bia sau
	if(countC == numberOfPages+1){
		glBindTexture(GL_TEXTURE_2D, backCover);
		glBegin(GL_QUADS);
		glTexCoord2f(1,1);glVertex3f(0, pageHeight/2, depth-deltaT);
		glTexCoord2f(0,1);glVertex3f(pageWidth, pageHeight/2, depth-deltaT);
		glTexCoord2f(0,0);glVertex3f(pageWidth, -pageHeight/2, depth-deltaT);
		glTexCoord2f(1,0);glVertex3f(0, -pageHeight/2, depth-deltaT);
		glEnd();		
	}
	glPopMatrix();
	// ket thuc ve
	
	// cap nhat bien noi ham countC va do sau cua trang can ve tiep theo
	countC ++;
	depth-=deltaP; // neu muon thay doi do day cua cuon sach, thay doi tham so lon hon
}

// ham xu ly menu chung
void menu(int num){
	switch(num){
		case 0:{ // quit
			glutDestroyWindow(window);
			exit(0);
		}
		case 1:{ // lat trang
			curling = true;
			drawing = false;
			point = false;
			line = false;
			glutPostRedisplay();
			break;
		}
		case 2:{ // tai hinh anh
			if(leftRight){ // trang phai
					if(count == numberOfPages+2) // neu da lat qua trang cuoi thi break
						break;
					string filename = GetFileName("Select picture: ");
					nameArray[count][0] = new char[filename.length() + 1];
					strcpy(nameArray[count][0], filename.c_str());
					loadTexture(texture[count][0], nameArray[count][0]);
					isPick[count][0] = 1; // danh dau mat truoc cua trang da dc chon
					saveTexture();
					glutPostRedisplay(); // ve lai				
			}
			
			if(!leftRight){ // trang trai
					if(count <= 1) // neu chua co trang nao duoc lat thi break
						break;
					string filename = GetFileName("Select picture: ");
					nameArray[count-1][1] = new char[filename.length() + 1];
					strcpy(nameArray[count-1][1], filename.c_str());
					loadTexture(texture[count-1][1], nameArray[count-1][1]);					
					isPick[count-1][1] = 1; // danh dau mat sau cua trang da duoc chon
					saveTexture();
					glutPostRedisplay(); // ve lai			
			}
			break;
		}
		case 3:{ // xoa anh
			if(leftRight){ // trang phai
				if(count == numberOfPages+2 || count == 0)
					break;
				nameArray[count][0] = NULL;
				isPick[count][0] = 0;
				saveTexture();
				glutPostRedisplay();
			}
			if(!leftRight){ // trang trai
				if(count <= 1 || count == numberOfPages + 2)
					break;
				nameArray[count-1][1] = NULL;
				isPick[count-1][1] = 0;
				saveTexture();
				glutPostRedisplay();
			}			
			break;
		}
		case 4:{ // xoa hinh ve
			if(leftRight){ // trai phai
					fout.open((ExePath().append("\\data\\").append(to_string(count)).append("0.dat")).c_str());
					if(fout.is_open()){
						fout << "";
					}
					fout.close();
					glutPostRedisplay();		
			}
			if(!leftRight){ // trang trai
					fout.open((ExePath().append("\\data\\").append(to_string(count-1)).append("1.dat")).c_str());
					if(fout.is_open()){
						fout << "";
					}
					fout.close();	
					glutPostRedisplay();	
			}
			break;
		}

		default:
			break;
	}
}

// menu chon mau sac
void colorMenu(int num){
	switch(num){
		case 1:{ // mau do
			if(!curling){
				drawColor = 1;
			}
			glutPostRedisplay();
			break;
		}
		case 2:{ // mau luc
			if(!curling){
				drawColor = 2;				
			}
			glutPostRedisplay();
			break;
		}
		case 3:{ // mau lam
			if(!curling){
				drawColor = 3;				
			}
			glutPostRedisplay();
			break;
		}		
		case 4:{ // mau den
			if(!curling)
				drawColor = 4;
			glutPostRedisplay();
			break;
		}
		case 5:{ // mau vang
			if(!curling)
				drawColor = 5;
			glutPostRedisplay();
			break;
		}		
		case 6:{ // mau hong canh sen
			if(!curling)
				drawColor = 6;
			glutPostRedisplay();
			break;
		}
		case 7:{ // mau xanh lo
			if(!curling)
				drawColor = 7;
			glutPostRedisplay();
			break;
		}
		case 8:{ // mau trang
			if(!curling)
				drawColor = 8;
			glutPostRedisplay();
			break;
		}
	}
}

// menu chon hinh ve
void shapeMenu(int num){
	switch(num){
		case 1:{ // ve tu giac
			curling = false;
			line = false;
			drawing = true;
			point = false;
			glutPostRedisplay();
			break;
		}
		case 2:{ // ve duong thang
			curling = false;
			drawing = false;
			line = true;
			point = false;
			glutPostRedisplay();
			break;
		}
		case 3:{ // ve diem
			curling = false;
			drawing = false;
			line = false;
			point = true;
			glutPostRedisplay();
			break;
		}
	}
}

// tao menu
void createMenu(void){
	color_menu = glutCreateMenu(colorMenu);
	glutAddMenuEntry("Red", 1);
	glutAddMenuEntry("Green", 2);
	glutAddMenuEntry("Blue", 3);
	glutAddMenuEntry("Black", 4);
	glutAddMenuEntry("Yellow", 5);
	glutAddMenuEntry("Magenta", 6);
	glutAddMenuEntry("Cyan", 7);
	glutAddMenuEntry("White", 8);
	shape_menu = glutCreateMenu(shapeMenu);
	glutAddMenuEntry("Rectangle", 1);
	glutAddMenuEntry("Line", 2);
	glutAddMenuEntry("Point", 3);
	menu_id = glutCreateMenu(menu);
	glutAddMenuEntry("Curling", 1);
	glutAddSubMenu("Draw", shape_menu);
	glutAddSubMenu("Color", color_menu);
	glutAddMenuEntry("Clear", 4);
	glutAddMenuEntry("Add Picture", 2);
	glutAddMenuEntry("Clear Picture", 3);
    glutAddMenuEntry("Quit", 0);
}

// ham display
void display(void){
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor3f(0.65,0.5,0.39);
	glDisable(GL_TEXTURE_2D); // tat che do texture de ve dung mau
	// ve duong chi gay sach
	glPushMatrix();
	glColor3f(0.52, 0.37, 0.26);
	glBegin(GL_LINES);
		glVertex3f(0, -pageHeight/2+0.25, 1);
		glVertex3f(0, pageHeight/2-0.25, 1);
	glEnd();
	
	glDisable(GL_TEXTURE_2D);
	
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();
	// ve anh nen
	glBindTexture(GL_TEXTURE_2D, bgTexture);
	glBegin(GL_POLYGON);
		glTexCoord2f(1,1);glVertex3f(20, 15, -2);
		glTexCoord2f(0,1);glVertex3f(-20, 15, -2);
		glTexCoord2f(0,0);glVertex3f(-20, -15, -2);
		glTexCoord2f(1,0);glVertex3f(20, -15, -2);
	glEnd();
	glPopMatrix();	
	
	// ve tung trang sach voi so luong nguoi dung nhap
	for(int i = 0; i < numberOfPages+2; i++){
		drawPage();
	}
	glPopMatrix();
	
	glDisable(GL_TEXTURE_2D);
	// ve nut chon mau sac
	glPushMatrix();
		if(!curling){
			chooseColor(drawColor);
			glRectf(11,y_max+2.2,12,y_max+1.2);
		}
	glPopMatrix();

	// khoi tao lai bien noi ham countC va do sau depth cho nhung lan ve lai
	countC = 0;
	depth = 0;
	order0 = 0;
	order1 = 0;
	glutSwapBuffers();
	createMenu();
}

// kiem tra xem chuot click vao mot toa do nao do, x1y1 la toa do diem tren trai, x2y2 la toa do diem duoi phai
static bool isClick(point2f p, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2){
	if(p.x > x1 && p.x < x2 && p.y > y2 && p.y < y1)
		return true;
	else
		return false;
}

// kiem tra toa do mot diem thuoc rightPage
static bool rightPage(point2f p){
	if(count > numberOfPages+1){ // neu da lat het sang trai thi bo qua
		return false;
	}
	if(isClick(p, 0, 6.7, 3.6, -6.7)){
		return true;
	}
	else
		return false;
}

// kiem tra toa do mot diem thuoc leftPage
static bool leftPage(point2f p){
	if(count < 1 || count > numberOfPages + 2) // neu chua co trang nao dc lat sang trai thi bo qua
		return false;
	if(isClick(p, -3.6, 6.7, 0, -6.7)){
		return true;
	}
	else
		return false;
}

// tinh toan toa do hien tai cua chuot theo he toa do cua minh, tra ve diem do
static point2f getMousePos(int x, int y){
	point2f p(x/(glutGet(GLUT_WINDOW_WIDTH)/(x_max*2)) - x_max,
				(glutGet(GLUT_WINDOW_HEIGHT)-y)/(glutGet(GLUT_WINDOW_HEIGHT)/(y_max*2)) - y_max);
	return p;
}

// ham doc file ve hinh
void readFile(int stt, int side, int &n, string &str){
	if(side == 0)
		fin.open((ExePath().append("\\data\\").append(to_string(stt)).append("0.dat")).c_str());
	else
		fin.open((ExePath().append("\\data\\").append(to_string(stt)).append("1.dat")).c_str());
	string temp;			
	if(fin.is_open()){						
		fin >> n;
		if(n != 0){
		while(getline(fin,temp)){
			str.append(temp).append("\n");
			}							
		}
		else{
			n = 0;
			str = "\n";
		}
	}
	else{
		n = 0;
		str = "\n";
	}
	fin.close();
}

// ham ghi file hinh ve
void writeFile(int stt, int side, int n, string str, int shape){
	if(side == 0)
		fout.open((ExePath().append("\\data\\").append(to_string(stt)).append("0.dat")).c_str());
	else
		fout.open((ExePath().append("\\data\\").append(to_string(stt)).append("1.dat")).c_str());
	if(fout.is_open()){
		fout << n+1;
		fout << str;
		fout << drawColor << " " << shape;
		if(shape == 1){
			fout << endl << p_root.x << " " << p_root.y << endl;
			if(side == 0)
				fout << pt2.x << " " << pt2.y;
			else
				fout << pt4.x << " " << pt4.y;
		}
		if(shape == 2){
			fout << endl << p_root.x << " " << p_root.y << endl;
			if(side == 0)
				fout << pt2.x << " " << pt2.y;
			else
				fout << pt4.x << " " << pt4.y;
		}
		if(shape == 3){
			if(side == 0){
				fout << " " << pt2.x << " " << pt2.y << endl;		
			}
			else
				fout << " " << pt4.x << " " << pt4.y << endl;
		}
	}
	fout.close();	
}

// ham quan li su kien chuot
static void mouse(int button, int state, int x, int y){
	switch(button){
		case GLUT_LEFT_BUTTON:
			if(state == GLUT_DOWN){
				p_root = getMousePos(x,y); // lay vi tri hien thoi cua chuot theo toa do chuan
				if(curling){ // kiem tra dang o che do lat trang
					if((count != 0 && count != numberOfPages+2) && (yrot[count] != 0 || yrot[count-1] != -180)){
						mouseDown = false;
					}
					else{
						if(rightPage(p_root)){ // kiem tra click vao trang ben phai
							mouseDown = true;
							if(!leftRight)
								leftRight = !leftRight;
							xdiff = x - yrot[count];
						}
						if(leftPage(p_root)){ // kiem tra click vao trang ben trai
							mouseDown = true;
							if(leftRight)
								leftRight = !leftRight;
							xdiff = x - yrot[count-1];
						}
					}			
				}

				if(!curling){ // kiem tra che do ve
					if(rightPage(p_root) && count >= 1){
						mouseDown = true;
						leftRight = true;
					}
					if(leftPage(p_root) && count < numberOfPages + 2){
						mouseDown = true;
						leftRight = false;
					}
				}
			}
			else
				mouseDown = false;
				
			if(state == GLUT_UP){ 
				if(curling){
					// neu tha chuot khi chua lat den 90 do thi tro ve cho cu
					// neu da qua 90 do thi lat sang luon
					if(leftRight){ // trang ben phai
						if(yrot[count] > -90 && yrot[count] != 0){
							isPaging = 1;
						}
						if(yrot[count] <= -90 && yrot[count] != -180){
							isPaging = 1;
							count++;
						}
					}
					else{ // trang ben trai
						if(yrot[count-1] <= -90 && yrot[count-1] != -180){
							isPaging = 1;
						}
						if(yrot[count-1] > -90 && yrot[count-1] != 0){
							isPaging = 1;
							count--;
						}
					}				
				}
				if(drawing){ // ghi toa do ve tu giac
					if(leftRight && rightPage(p_root) && count < numberOfPages + 1){
						int n = 0;
						string str;
						readFile(count, 0, n, str);
						
						writeFile(count, 0, n, str, 1);
					}
					if(!leftRight && leftPage(p_root) && count < numberOfPages + 2){
						int n = 0;
						string str;
						readFile(count-1, 1, n, str);
						
						writeFile(count-1, 1, n, str, 1);
					}
					pt1.x = 0; pt2.x = 0; pt3.x = 0; pt4.x = 0;
					glutPostRedisplay();
				}
				if(point){ // ghi toa do ve diem
					if(leftRight && rightPage(p_root) && count < numberOfPages + 1){
						int n = 0;
						string str;
						readFile(count, 0, n, str);
						pt2 = p_root;
						writeFile(count, 0, n, str, 3);
					}
					if(!leftRight && leftPage(p_root) && count < numberOfPages + 2){
						int n = 0;
						string str;
						readFile(count-1, 1, n, str);
						pt4 = p_root;
						writeFile(count-1, 1, n, str, 3);
					}
					pt1.x = 0; pt2.x = 0; pt3.x = 0; pt4.x = 0;
					glutPostRedisplay();					
				}
				if(line){ // ghi toa do ve duong thang 
					if(leftRight && rightPage(p_root) && count < numberOfPages + 1){
						int n = 0;
						string str;
						readFile(count, 0, n, str);
						writeFile(count, 0, n, str, 2);
					}
					if(!leftRight && leftPage(p_root) && count < numberOfPages + 2){
						int n = 0;
						string str;
						readFile(count-1, 1, n, str);
						writeFile(count-1, 1, n, str, 2);
					}
					pt1.x = 0; pt2.x = 0; pt3.x = 0; pt4.x = 0;
					glutPostRedisplay();					
				}
			}	
		default:
			break;
	}
}

// ham xu ly chuot di chuyen
static void mouseMotion(int x, int y){
	if(mouseDown && curling){ // kiem tra nut chuot van dang duoc an, va dang trong che do lat trang
		if(leftRight){
			yrot[count] = (x - xdiff);
			if(yrot[count] > 0) // kiem tra de khong bi lat qua muc
				yrot[count] = 0;
			if(yrot[count] <= -170)
				mouseDown = false;
			if(yrot[count] == -180){ // neu trang sach da duoc lat qua 130 do thi lat sang trai luon
				mouseDown = false; // dung bat su kien chuot khi lat xong
				count++;
			}
		}
		else{
			yrot[count-1] = (x - xdiff);
			if(yrot[count-1] < -180) // kiem tra de khong bi lat qua muc
				yrot[count-1] = -180;
			if(yrot[count-1] >= -10)
				mouseDown = false;
			if(yrot[count-1] == 0){ // neu trang sach da duoc lat qua 130 do thi lat sang phai luon
				mouseDown = false; // dung bat su kien chuot khi lat xong
				count--;
			}
		}
		glutPostRedisplay();
	}
	if(mouseDown && !curling){ // phan hien thi hinh ve tam thoi
		if(!point){ // neu khong phai la ve diem
			if(leftRight && count != 0 && count < numberOfPages + 1){
				pt1 = p_root;
				pt2 = getMousePos(x,y);
				if(!rightPage(pt2) || !rightPage(pt1)){
					if(rightPage(pt1) && !rightPage(pt2)){
						if(pt2.x > 3.5) pt2.x = 3.5;
						if(pt2.x < 0) pt2.x = 0;
						if(pt2.y > 6.7) pt2.y = 6.5;
						if(pt2.y < -6.5) pt2.y = -6.5;
					}
					else{
						pt2.x = 0;
						pt1.x = 0;					
					}
				}
				glutPostRedisplay();		
			}
			if(!leftRight && count > 1 && count < numberOfPages + 2){
				pt3 = p_root;
				pt4 = getMousePos(x,y);
				if(!leftPage(pt4) || !leftPage(pt3)){
					if(leftPage(pt3) && !leftPage(pt4)){
						if(pt4.x < -3.5) pt4.x = -3.5;
						if(pt4.x > 0) pt4.x = 0;
						if(pt4.y > 6.6)	pt4.y = 6.6;
						if(pt4.y < -6.6) pt4.y = -6.6;
					}
					else{
						pt4.x = 0;
						pt3.x = 0;					
					}
				}
				glutPostRedisplay();
			}		
		}
		else{ // neu la ve diem
			if(leftRight && count != 0 && count < numberOfPages + 1){
				int n = 0;
				string str;
				pt1 = p_root;
				pt2 = getMousePos(x,y);
				if(!rightPage(pt2) || !rightPage(pt1)){
					if(rightPage(pt1) && !rightPage(pt2)){
						if(pt2.x > 3.5) pt2.x = 3.5;
						if(pt2.x < 0) pt2.x = 0;
						if(pt2.y > 6.7) pt2.y = 6.5;
						if(pt2.y < -6.5) pt2.y = -6.5;
					}
					else{
						pt2.x = 0;
						pt1.x = 0;					
					}
				}
				readFile(count, 0, n, str);
				writeFile(count, 0, n, str, 3);
				glutPostRedisplay();		
			}
			if(!leftRight && count > 1 && count < numberOfPages + 2){
				int n = 0;
				string str;
				pt3 = p_root;
				pt4 = getMousePos(x,y);
				if(!leftPage(pt4) || !leftPage(pt3)){
					if(leftPage(pt3) && !leftPage(pt4)){
						if(pt4.x < -3.5) pt4.x = -3.5;
						if(pt4.x > 0) pt4.x = 0;
						if(pt4.y > 6.6)	pt4.y = 6.6;
						if(pt4.y < -6.6) pt4.y = -6.6;
					}
					else{
						pt4.x = 0;
						pt3.x = 0;					
					}
				}
				readFile(count-1, 1, n, str);
				writeFile(count-1, 1, n, str, 3);
				glutPostRedisplay();
			}				
		}
	}
}

// ham kiem tra vi tri chuot de bat menu
static void mousePass(int x, int y){
	point2f mp = getMousePos(x, y);
	if(rightPage(mp) && count != 0 && count!= numberOfPages + 1){
		leftRight = true;
		glutAttachMenu(GLUT_RIGHT_BUTTON);
	}
	else if(leftPage(mp) && count != 1 && count != numberOfPages + 2){
		leftRight = false;
		glutAttachMenu(GLUT_RIGHT_BUTTON);
	}
	else{
		glutDetachMenu(GLUT_RIGHT_BUTTON);
	}
}

// ham xu ly trang sach khi tha chuot 
static void animation(){
	if (isPaging == 0 || mouseDown || !curling) return;
	Sleep(17);
	static float deltat = 0.1;
	static float tweent=0;
	static int vect;
	tweent = tweent + deltat;
	int curlPage=-1;
	if (yrot[count]==0||yrot[count]==-180)
		curlPage = count-1;
	else
		curlPage = count;
	if (yrot[curlPage] > -90) 
		vect = 1;
	if (yrot[curlPage] <= -90){
		vect = -1;
	}
	yrot[curlPage] += vect*tweent;
	if (yrot[curlPage]>=0){
		isPaging = 0;
		yrot[curlPage] = 0;
		tweent = 1;
	}
	if (yrot[curlPage]<=-180){
		isPaging = 0;
		yrot[curlPage]=-180;
		tweent = 1;
	}
	glutPostRedisplay();
}

// ham reshape
void reshape(int w, int h){
	if( w != width || h != height )
	{
		glutReshapeWindow( width, height);
	}
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (GLfloat) w/(GLfloat) h, 10.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glOrtho(-x_max,x_max,-y_max,y_max,-z_max,z_max);
    
	glTranslatef(0.0, 0.0, -30.0);
}

int main(int argc, char** argv)
{
	// cho nguoi dung nhap so trang sach can tao
	cout << "Nhap so to cua trang sach muon tao: ";
	cin >> numberOfPages;
	GetWindowRect(GetDesktopWindow(), &rc);
	width = rc.right - 20; height = rc.bottom - 80;
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(0,0);
	window = glutCreateWindow("Book");
	
	createMenu(); 
	init();
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	glutPassiveMotionFunc(mousePass);
	glutIdleFunc(animation);
	glutMainLoop();
	return 0;
}

