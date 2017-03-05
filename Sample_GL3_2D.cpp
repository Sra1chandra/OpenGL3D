#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct Block{
  VAO *cuboid;
  double length;      //x-axis
  double breadth;     //z-axis
  double height;      //y-axis
  double x_pos,y_pos,z_pos,angle;
  char key;
  int fall_status;
  int color;
  int x_destination,z_destination;
  glm::vec3 rotate_vector;
  double r_x,r_y,r_z;
  glm::mat4 rotation_matrix;
};

struct Board{
  VAO *tile[14][14];
  int tile_type[14][14];
  double tile_xpos[14][14],tile_ypos[14][14],tile_zpos[14][14];
  double angle[14][14];
  int tile_order[400];
  int tiles_created;
  int no_of_tiles;
};
struct Bridge{
  VAO *bridge[2];
  double x_pos[2];
  double z_pos[2];
  double angle;
  bool bridge_status;
  double height,length,breadth;
};
struct View{
  glm::mat4 generalview;
  glm::mat4 topview;
  glm::mat4 towerview;
  glm::mat4 follow_cam_view;
  glm::mat4 block_view;
  glm::mat4 helicopterview;
};
bool hang;
int LEVEL;
struct Block block;
struct Board board;
struct Bridge bridge[2];
GLuint programID;
GLFWwindow* window;
/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }
    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
float camera_rotation_angle = 90;
bool mouse_left=false;
int fbwidth,fbheight;
double x_pos,y_pos;
double x_pos1,y_pos1;
bool helicopterview,shift,cam_follow,ortho,block_view;
double x_direction,z_direction;
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void initialize_view(bool a,bool b,bool c,bool d)
{
  shift=a;ortho=b;cam_follow=c,block_view=d;
}
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.
    if (action == GLFW_RELEASE && !hang) {
        switch (key) {
            case GLFW_KEY_DOWN:
              if(block_view){x_direction=0;z_direction=1;}
              else{block.key='D';block.angle+=10;}
              break;
            case GLFW_KEY_LEFT:
              if(block_view){x_direction=-1;z_direction=0;}
              else{block.key='L';block.angle+=10;}
              break;
            case GLFW_KEY_RIGHT:
              if(block_view){x_direction=1;z_direction=0;}
              else{block.key='R';block.angle+=10;}
              break;
            case GLFW_KEY_UP:
              if(block_view){x_direction=0;z_direction=-1;}
              else{block.key='U';block.angle+=10;}
              break;
            default:
              break;
        }
    }
    else if (action == GLFW_PRESS) {
      helicopterview=false;
        switch (key) {
          case GLFW_KEY_G:
            Matrices.view=glm::lookAt(glm::vec3(-2,3,4), glm::vec3(0,0,0), glm::vec3(0,1,0));
            initialize_view(true,true,false,false);
            break;
          case GLFW_KEY_U:
            Matrices.view=glm::lookAt(glm::vec3(0,5,0),glm::vec3(0,0,0), glm::vec3(0,0,-1));
            initialize_view(true,false,false,false);
            break;
          case GLFW_KEY_T:
            Matrices.view=glm::lookAt(glm::vec3(-1,4,3), glm::vec3(0,0,0), glm::vec3(0,1,0));
            initialize_view(true,false,false,false);
            break;
          case GLFW_KEY_C:
          initialize_view(false,false,true,false);
            break;
          case GLFW_KEY_H:
            helicopterview=true;
            initialize_view(true,false,false,false);
            Matrices.view = glm::lookAt(glm::vec3(4*cos(camera_rotation_angle*M_PI/180.0f),4,4*sin(camera_rotation_angle*M_PI/180.0f)),
                              glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
            break;
          case GLFW_KEY_B:
          initialize_view(false,false,false,true);
            break;
          case GLFW_KEY_ESCAPE:
            quit(window);
            break;
          default:
            break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
          glfwGetCursorPos(window,&x_pos,&y_pos);
          x_pos=8*((x_pos-fbwidth/2)/fbwidth*1.0);
          y_pos=-8*((y_pos-fbheight/2)/fbheight*1.0);
          if(x_pos>3.3&&x_pos<4&&y_pos>-3.3&&y_pos<-2.7){block.key='R';block.angle+=10;}
          else if(x_pos>2&&x_pos<3.3&&y_pos>-3.3&&y_pos<-2.7){block.key='L';block.angle+=10;}
          else if(x_pos>2.7&&x_pos<3.3&&y_pos>-2.7&&y_pos<-2){block.key='U';block.angle+=10;}
          else if(x_pos>2.7&&x_pos<3.3&&y_pos>-4&&y_pos<-3.3){block.key='D';block.angle+=10;}
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE)
              mouse_left=false;
            else if(action == GLFW_PRESS)
            {
              mouse_left=true;
              glfwGetCursorPos(window,&x_pos,&y_pos);
              x_pos=8*((x_pos-fbwidth/2)/fbwidth*1.0);
              y_pos=-8*((y_pos-fbheight/2)/fbheight*1.0);
            }
            break;
        default:
            break;
    }
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
  fbwidth=width, fbheight=height;
  /* With Retina display on Mac OS X, GLFW's FramebufferSize
   is different from WindowSize */
  glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	 //glMatrixMode (GL_PROJECTION);
	   //glLoadIdentity ();
	   //gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0);
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views

    // Ortho projection for 2D views
}

VAO *triangle, *rectangle,*cube;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    1,0,0, // vertex 1
    0,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

// Creates the rectangle object used in this sample code
void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    0,0.5,0, // vertex 1
    -1,0.5,0, // vertex 2
    -1,-0.5,0, // vertex 3

    -1,-0.5,0, // vertex 3
    0,-0.5,0, // vertex 4
    0,0.5,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1
  };
  // create3DObject creates and returns a handle to a VAO that can be used later
  rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void CreateCuboid(float l,float h,float b,int c,VAO **object)
{
  int red=1,green=1,blue=1;
  if(c==0) red=0;
  if(c==1) green=0;
  if(c==2) blue=0;
  if(c==4) {green=0;red=0;}
  if(c==3) {green=0;blue=0;}
  GLfloat color_buffer_data [108];
  // GL3 accepts only Triangles. Quads are not supported

  GLfloat vertex_buffer_data [] = {
    //vertex3       //vertex4       //vertex1
    l/2,-h/2,-b/2,  l/2,-h/2,b/2,   l/2,h/2,b/2,
    //vertex1       //vertex2       //vertex3
    l/2,h/2,b/2,    l/2,h/2,-b/2,   l/2,-h/2,-b/2,

    //vertex1       //vertex2       //vertex3
    -l/2,h/2,b/2,    -l/2,h/2,-b/2,   -l/2,-h/2,-b/2,
    //vertex3       //vertex4       //vertex1
    -l/2,-h/2,-b/2,  -l/2,-h/2,b/2,   -l/2,h/2,b/2,

    //vertex1       //vertex2       //vertex3
    l/2,h/2,b/2,    l/2,h/2,-b/2,   -l/2,h/2,-b/2,
    //vertex3       //vertex4       //vertex1
    -l/2,h/2,-b/2,  -l/2,h/2,b/2,   l/2,h/2,b/2,

    -l/2,-h/2,-b/2,  -l/2,-h/2,b/2,   l/2,-h/2,b/2,
    //vertex3       //vertex4       //vertex1
    //vertex1       //vertex2       //vertex3
    l/2,-h/2,b/2,    l/2,-h/2,-b/2,   -l/2,-h/2,-b/2,

    //vertex3       //vertex4       //vertex1
    -l/2,-h/2,b/2,  -l/2,h/2,b/2,   l/2,h/2,b/2,
    //vertex1       //vertex2       //vertex3
    l/2,h/2,b/2,    l/2,-h/2,b/2,   -l/2,-h/2,b/2,


    //vertex1       //vertex2       //vertex3
    l/2,h/2,-b/2,    l/2,-h/2,-b/2,   -l/2,-h/2,-b/2,
    //vertex3       //vertex4       //vertex1
    -l/2,-h/2,-b/2,  -l/2,h/2,-b/2,   l/2,h/2,-b/2
  };
  for(int i=0;i<6;i++)
  {
  for(int j=0;j<6;j++)
  {
    if(j>0&&j<=3)
    {
      color_buffer_data[i*18+j*3]=red;
      color_buffer_data[i*18+j*3+1]=green;
      color_buffer_data[i*18+j*3+2]=blue;
    }
    else
    {
      color_buffer_data[i*18+j*3]=0;
      color_buffer_data[i*18+j*3+1]=0;
      color_buffer_data[i*18+j*3+2]=0;
    }
  }
}
  // create3DObject creates and returns a handle to a VAO that can be used later
  *object = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createBlock()
{
  block.length=0.5;
  block.breadth=0.5;
  block.height=1;
  block.y_pos=block.height/2+0.5;
  block.angle=0;block.fall_status=0;
  block.rotate_vector=glm::vec3(0,0,1);
  block.rotation_matrix=glm::mat4(1.0f);
  block.fall_status=2;block.color=5;
  CreateCuboid(block.length,block.height,block.breadth,block.color,&block.cuboid);
}
void createBridge(struct Bridge *bridge)
{
  if(LEVEL==3)
  {
    bridge->angle=0;
    bridge->length=0.5;bridge->height=0.1;bridge->breadth=0.5;
    CreateCuboid(bridge->length,bridge->height,bridge->breadth,1,&bridge->bridge[0]);
    CreateCuboid(bridge->length,bridge->height,bridge->breadth,1,&bridge->bridge[1]);
  }
}
void createTile(int i,int j)
{
  int color;
  board.tile_xpos[i][j]=i/2.0;
  board.tile_ypos[i][j]=-0.2/2.0;
  board.tile_zpos[i][j]=j/2.0;
  if(board.tile_type[i][j]==1) color=0;
  else color=board.tile_type[i][j];
  CreateCuboid(0.5,0.2,0.5,color,&board.tile[i][j]);
}
void moveBridge(struct Bridge bridge,glm::mat4 VP)
{
  glm::mat4 MVP;
  Matrices.model=glm::mat4(1.0f);
  glm::mat4 translateRectangle = glm::translate (glm::vec3(-bridge.length/2,-bridge.height/2,0));        // glTranslatef
  translateRectangle*=glm::translate (glm::vec3(bridge.x_pos[0]/2.0,-bridge.height/2.0,bridge.z_pos[0]/2.0));
  glm::mat4 rotateRectangle = glm::rotate((float)(bridge.angle*M_PI/180.0f),glm::vec3(0,0,-1)); // rotate about vector (-1,1,1)
  glm::mat4 translateRectangle1 = glm::translate (glm::vec3(bridge.length/2,bridge.height/2,0));
  Matrices.model *= (translateRectangle*rotateRectangle*translateRectangle1);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(bridge.bridge[0]);

  Matrices.model=glm::mat4(1.0f);
  translateRectangle = glm::translate (glm::vec3(bridge.length/2,-bridge.height/2,0));        // glTranslatef
  translateRectangle*=glm::translate (glm::vec3(bridge.x_pos[1]/2.0,-bridge.height/2.0,bridge.z_pos[1]/2.0));
  rotateRectangle = glm::rotate((float)(bridge.angle*M_PI/180.0f),glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  translateRectangle1 = glm::translate (glm::vec3(-bridge.length/2,bridge.height/2,0));
  Matrices.model *= (translateRectangle*rotateRectangle*translateRectangle1);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(bridge.bridge[1]);
}
void Check_Block_Pos()
{
  int x_pos,y_pos,z_pos;
  if(block.height==2*block.length)
  {
    x_pos=block.x_pos*2;
    z_pos=block.z_pos*2;
    if(board.tile_type[x_pos][z_pos]==0)
      block.fall_status=1;
    if(board.tile_type[x_pos][z_pos]==2)
    {
      block.fall_status=3;
      board.tile_type[x_pos][z_pos]=0;
    }
    if(x_pos==block.x_destination && z_pos==block.z_destination)
      block.fall_status=5;
    if(board.tile_type[x_pos][z_pos]==4)
    {
      if(bridge[1].angle==0)
        bridge[1].angle=5;
      else if(bridge[1].angle==90)
        bridge[1].angle=85;
    }
  }
  if(block.length==2*block.height)
  {
    x_pos=(block.x_pos-block.length/4.0)*2;z_pos=block.z_pos*2;
    if(board.tile_type[x_pos][z_pos]==0&&board.tile_type[x_pos+1][z_pos]==0)
      block.fall_status=1;
    else if(board.tile_type[x_pos][z_pos]==0)
    {
      block.key='L';
      block.angle+=25;
      block.fall_status=1;
    }
    else if(board.tile_type[x_pos+1][z_pos]==0)
    {
      block.key='R';
      block.angle+=25;
      block.fall_status=1;
    }
    else if(board.tile_type[x_pos][z_pos]==3)
    {
      if(bridge[0].angle==0)
        bridge[0].angle=5;
      else if(bridge[0].angle==90)
        bridge[0].angle=85;
    }
  }
  if(block.breadth==2*block.height)
  {
    z_pos=(block.z_pos-block.length/4.0)*2;x_pos=block.x_pos*2;
    if(board.tile_type[x_pos][z_pos]==0&&board.tile_type[x_pos][z_pos+1]==0)
      block.fall_status=1;
    else if(board.tile_type[x_pos][z_pos]==0)
    {
      block.key='U';
      block.angle+=25;
      block.fall_status=1;
    }
    else if(board.tile_type[x_pos][z_pos+1]==0)
    {
      block.key='D';
      block.angle+=25;
      block.fall_status=1;
    }
  }
}
void moveBlock(glm::mat4 VP)
{
  glm::mat4 MVP;
  Matrices.model = glm::mat4(1.0f);
  if(block.key=='R')
  {
    block.r_x=-block.length/2;block.r_y=block.height/2;block.r_z=0;
    block.rotate_vector=glm::vec3(0,0,-1);
  }
  if(block.key=='L')
  {
    block.r_x=block.length/2;block.r_y=block.height/2;block.r_z=0;
    block.rotate_vector=glm::vec3(0,0,1);
  }
  if(block.key=='U')
  {
    block.r_x=0;block.r_y=block.height/2;block.r_z=block.breadth/2;
    block.rotate_vector=glm::vec3(-1,0,0);
  }
  if(block.key=='D')
  {
    block.r_x=0;block.r_y=block.height/2;block.r_z=-block.breadth/2;
    block.rotate_vector=glm::vec3(1,0,0);
  }
  glm::mat4 translateRectangle = glm::translate (glm::vec3(-block.r_x,-block.r_y,-block.r_z));        // glTranslatef
  translateRectangle*=glm::translate (glm::vec3(block.x_pos,block.y_pos,block.z_pos));
  glm::mat4 rotateRectangle = glm::rotate((float)(block.angle*M_PI/180.0f), block.rotate_vector); // rotate about vector (-1,1,1)
  glm::mat4 translateRectangle1 = glm::translate (glm::vec3(block.r_x,block.r_y,block.r_z));
  Matrices.model *= (translateRectangle*rotateRectangle*translateRectangle1*block.rotation_matrix);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  if(block.cuboid!=NULL&&!block_view)
    draw3DObject(block.cuboid);
  if(block.angle>=90&&block.fall_status==0)
  {
    block.angle=0;
    hang=false;
    block.rotation_matrix=rotateRectangle*block.rotation_matrix;
    if(block.key=='R')
    {
      block.x_pos +=block.length/2+block.height/2;
      block.y_pos=block.length/2;
      swap(block.length,block.height);
    }
    if(block.key=='L')
    {
      block.x_pos-=block.length/2+block.height/2;
      block.y_pos=block.length/2;
      swap(block.length,block.height);
    }
    if(block.key=='U')
    {
      block.z_pos-=block.breadth/2+block.height/2;
      block.y_pos=block.breadth/2;
      swap(block.breadth,block.height);
    }
    if(block.key=='D')
    {
      block.z_pos+=block.breadth/2+block.height/2;
      block.y_pos=block.breadth/2;
      swap(block.breadth,block.height);
    }
    Check_Block_Pos();
  }
}
void moveBoard(glm::mat4 VP)
{
  glm::mat4 MVP;
  glm::mat4 translateTriangle;
  glm::mat4 rotateTriangle;
  int x,z;
  for(int i=0;i<board.tiles_created;i+=2)
  {
        x=board.tile_order[i];z=board.tile_order[i+1];
        Matrices.model = glm::mat4(1.0f);
        translateTriangle = glm::translate (glm::vec3(board.tile_xpos[x][z],board.tile_ypos[x][z],board.tile_zpos[x][z]));
        rotateTriangle = glm::rotate((float)(((board.tile_order[i]+board.tile_order[i+1])%2)*90*M_PI/180.0f),
                                        glm::vec3(0,1,0));
        Matrices.model *= translateTriangle*rotateTriangle;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(board.tile[x][z]);
    }
}
void draw_Arrow(glm::mat4 VP,double angle,VAO *object)
{
  glm::mat4 MVP;
  glm::mat4 translateTriangle;
  glm::mat4 rotateTriangle;
  Matrices.model = glm::mat4(1.0f);
  translateTriangle = glm::translate (glm::vec3(3+0.5*cos((float)(angle*M_PI/180.0f)),-3+0.5*sin((float)angle*M_PI/180.0f),0));
  rotateTriangle = glm::rotate((float)(angle*M_PI/180.0f),glm::vec3(0,0,1));
  glm::mat4 scale=glm::scale(glm::vec3(0.2,0.2,0.2));
  Matrices.model *= translateTriangle*rotateTriangle*scale;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(object);

}
//float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);
  glm::vec3 target (0, 0, 0);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 0, 0, 1 );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);
  glm::mat4 VP1=glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f)*glm::lookAt(eye,target,up);
  draw_Arrow(VP1,0,triangle);
  draw_Arrow(VP1,0,rectangle);
  draw_Arrow(VP1,90,triangle);
  draw_Arrow(VP1,90,rectangle);
  draw_Arrow(VP1,180,triangle);
  draw_Arrow(VP1,180,rectangle);
  draw_Arrow(VP1,270,triangle);
  draw_Arrow(VP1,270,rectangle);

  GLfloat fov = 90.0f;
  if(ortho)
  Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
  else
  Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);
  if(helicopterview&&mouse_left)
  {
    glfwGetCursorPos(window,&x_pos1,&y_pos1);
      x_pos1=8*((x_pos1-fbwidth/2)/fbwidth*1.0);
      y_pos1=-8*((y_pos1-fbheight/2)/fbheight*1.0);
      if(y_pos1>0)
        camera_rotation_angle+=(x_pos1-x_pos)*180.0/8.0;
      else
        camera_rotation_angle+=(x_pos-x_pos1)*180.0/8.0;
      Matrices.view = glm::lookAt(glm::vec3(4*cos(camera_rotation_angle*M_PI/180.0f),4,4*sin(camera_rotation_angle*M_PI/180.0f)),
                        glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
      x_pos=x_pos1;
      y_pos=y_pos1;
  }
  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!

  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);

  /* Render your scene */

  glm::mat4 translateTriangle = glm::translate (glm::vec3(0.0f, -0.025f, 0)); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  Matrices.model *= translateTriangle;
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  //draw3DObject(triangle);
  if(shift)
    translateTriangle=glm::translate (glm::vec3(-3.0f, 1.0f, -3.0));
  else
    translateTriangle=glm::translate (glm::vec3(0.0f, 0.0f, 0.0));

  glm::mat4 scale=glm::scale(glm::vec3(1.2,1.2,1.2));

  if(cam_follow)
  {
    Matrices.view=glm::lookAt(glm::vec3(block.x_pos-1,block.y_pos+1,block.z_pos+2),
                       glm::vec3(block.x_pos,block.y_pos,block.z_pos), glm::vec3(0.5,1,-1));
  }
  if(block_view)
  {
    Matrices.view=glm::lookAt(glm::vec3(block.x_pos,block.y_pos,block.z_pos),
                       glm::vec3(block.x_pos+x_direction,block.y_pos,block.z_pos+z_direction), glm::vec3(0,1,0));
  }
  moveBoard(VP*scale*translateTriangle);
  if(bridge[0].bridge[0]!=NULL)
  {
    moveBridge(bridge[0],VP*scale*translateTriangle);
    moveBridge(bridge[1],VP*scale*translateTriangle);
  }
  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  if(block.cuboid!=NULL&&!block_view)
    moveBlock(VP*scale*translateTriangle);

  // Increment angles
  //float increments = 1;

  //camera_rotation_angle++; // Simulating camera rotation
  //triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  //rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
     // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
  createRectangle();
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

  Matrices.view=glm::lookAt(glm::vec3(-2,3,4), glm::vec3(0,0,0), glm::vec3(0,1,0));
  ortho=true;
  x_direction=0;z_direction=1;
  reshapeWindow (window, width, height);

  // Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}
void initialize(int *tile_pos,int n)
{
  for(int i=0;i<14;i++)
    for(int j=0;j<14;j++)
      board.tile_type[i][j]=0;
  for(int i=0;i<400;i++)
    board.tile_order[i]=0;
  board.no_of_tiles=n/2;
  for(int i=0;i<n;i++)
    board.tile_order[i]=tile_pos[i];
  for(int i=0;i<n;i+=2)
      board.tile_type[tile_pos[i]][tile_pos[i+1]]=1;
}
void level_init(int level)
{
  switch (level)
  {
    case 1:
    {
      int tile_pos[]={3,3,3,4,3,5,4,5,5,5,4,6,5,6,6,6,6,5,6,4,7,5,7,4,7,3,8,3,8,2,9,3,9,2,10,3,10,4,10,5,
                      9,5,11,5,10,6,10,7,10,8,9,7,9,8,9,9,8,9,7,9,7,10,6,10,5,10,5,9,5,8,6,8,7,8};
      initialize(tile_pos,74);
      block.x_pos=3/2.0;block.z_pos=5/2.0;
      block.x_destination=6;block.z_destination=9;
      board.tile_type[6][9]=1;
      break;
    }
    case 2:
    {
      int tile_pos[]={4,10,5,10,5,11,4,11,3,11,3,10,3,9,4,9,2,10,5,9,6,10,7,10,8,10,9,10,8,9,9,9,8,8,9,8,10,8,10,7,
        10,6,10,5,10,4,11,4,9,4,9,3,10,3,11,3,9,2,10,2,11,2,8,3,8,4,7,3,7,4,6,4,5,4,4,4,4,5,4,6,3,5,3,6,2,5,2,6,
        2,4,2,3,3,3,4,3};
      initialize(tile_pos,96);
      board.tile_type[10][4]=2;board.tile_type[11][4]=2;board.tile_type[9][4]=2;board.tile_type[9][3]=2;
      board.tile_type[10][3]=2;board.tile_type[11][3]=2;board.tile_type[9][2]=2;board.tile_type[10][2]=2;
      block.x_pos=4/2.0;block.z_pos=10/2.0;
      block.x_destination=3;block.z_destination=4;
      board.tile_type[3][4]=1;
      break;
    }
    case 3:
    {
      int tile_pos[]={4,4,4,5,3,5,3,4,3,3,4,3,2,4,5,3,5,4,5,5,/*6,4,7,4,*/8,4,9,4,8,5,9,5,9,6,8,6,7,6,7,7,8,7,9,7,9,8,
        8,8,7,8,7,9,8,9,9,9,/*6,8,5,8,*/4,8,4,9,3,9,2,9,2,8,2,7,3,7,4,7
      };
      initialize(tile_pos,68);
      bridge[0].x_pos[0]=6;bridge[0].z_pos[0]=4;bridge[0].x_pos[1]=7;bridge[0].z_pos[1]=4;
      bridge[1].x_pos[0]=5;bridge[1].z_pos[0]=8;bridge[1].x_pos[1]=6;bridge[1].z_pos[1]=8;
      block.x_pos=4/2.0;block.z_pos=4/2.0;
      block.x_destination=3;block.z_destination=8;
      board.tile_type[3][8]=1;
      board.tile_type[2][4]=3;
      board.tile_type[9][7]=4;
      break;
    }
  }
}
void toggleBridge(struct Bridge *bridge)
{
  if(bridge->bridge_status&&bridge->angle>0)
  {
    bridge->angle+=5;
    hang=true;
    if(bridge->angle==90)
    {
      board.tile_type[(int)bridge->x_pos[0]][(int)bridge->z_pos[0]]=0;
      board.tile_type[(int)bridge->x_pos[1]][(int)bridge->z_pos[1]]=0;
      bridge->bridge_status=false;
      hang=false;
    }
  }
  else if(!bridge->bridge_status&&bridge->angle<90)
  {
    bridge->angle-=5;
    hang=true;
    if(bridge->angle==0)
    {
      board.tile_type[(int)bridge->x_pos[0]][(int)bridge->z_pos[0]]=1;
      board.tile_type[(int)bridge->x_pos[1]][(int)bridge->z_pos[1]]=1;
      bridge->bridge_status=true;
      hang=false;
    }
  }
}
int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;
  fbwidth=width;fbheight=height;
  hang=false;
  LEVEL=1;
  shift=true;

    GLFWwindow* window = initGLFW(width, height);

	  initGL (window, width, height);
    double last_update_time = glfwGetTime(),create_tile_time=glfwGetTime(), current_time;
    /* Draw in loop */
    level_init(LEVEL);
    board.tiles_created=0;
    bridge[0].bridge[0]=NULL;
    while (!glfwWindowShouldClose(window)) {
        // OpenGL Draw commands
        draw();
        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if(block.cuboid==NULL&&board.tiles_created==2*board.no_of_tiles)
          createBlock();
        if(board.tiles_created<2*board.no_of_tiles&&current_time-create_tile_time>=0.1)
        {
          createTile(board.tile_order[board.tiles_created],board.tile_order[board.tiles_created+1]);
          board.tiles_created+=2;
          create_tile_time=current_time;
        }
        if(board.tiles_created==2*board.no_of_tiles&&bridge[0].bridge[0]==NULL)
        {
          createBridge(&bridge[0]);
          bridge[0].bridge_status=true;
          createBridge(&bridge[1]);
          bridge[1].bridge_status=true;
          bridge[0].angle=5;
          bridge[1].angle=5;
        }
        if ((current_time - last_update_time) >= 0.05) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            if(bridge[0].bridge[0]!=NULL)
            {
              toggleBridge(&bridge[0]);
              toggleBridge(&bridge[1]);
            }
            if(block.fall_status==1)
            {
                block.angle+=10;
                if(block.angle>360)
                  block.angle-=360;
                block.y_pos-=0.5;
            }
            else if(block.fall_status==2)
            {
              if(block.y_pos>block.height/2.0)
                block.y_pos-=0.05;
              else
              {
                hang=false;
                block.fall_status=0;
              }
            }
            else if(block.fall_status==3)
            {
              block.y_pos-=0.25;
              board.tile_ypos[(int)(block.x_pos*2)][(int)(block.z_pos*2)]-=0.25;
            }
            if(block.fall_status==4)
            {
                block.angle+=10;
                if(block.angle>=90)
                {
                  block.angle=0;
                  block.fall_status=1;
                }
                if(block.angle>360)
                  block.angle-=360;
            }
            else if(block.fall_status==5)
              block.y_pos-=0.25;
            else if(block.angle!=0)
            {
                block.angle+=10;
                hang=true;
              }
            last_update_time = current_time;
        }
        if(block.y_pos<-3)
        {
          if(LEVEL==4)
            quit(window);
          if(block.fall_status==1||block.fall_status==3)
              level_init(LEVEL);
          if(block.fall_status==5)
            level_init(++LEVEL);
          if(block.fall_status==1||block.fall_status==3||block.fall_status==5)
          {
            block.angle=0;
            hang=false;
            create_tile_time=glfwGetTime();
            block.cuboid=NULL;
            bridge[0].bridge[0]=NULL;
            bridge[1].bridge[0]=NULL;
            block.fall_status=0;
            board.tiles_created=0;
          }
        }
    }
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
