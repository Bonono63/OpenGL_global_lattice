#include <stdio.h>
#include <stdlib.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <fstream>
#include <filesystem>


// free out!
int readfile(const char * path, char** out)
{
        std::ifstream file (path);
        if (file.is_open())
        {
                long long file_size = std::filesystem::file_size(path);
                
                *out = (char*) malloc((file_size+1)*sizeof(char));
                for (long long i = 0 ; i < file_size; i++)
                {
                        char c;
                        file.get(c);
                        *(*out+i) = c;
                }
                // terminating byte because final character isn't one?
                *(*out+file_size) = '\0';
                file.close();
        }
        else
        {
                printf("Unable to read file at: %s\n",path);
                file.close();
                return -1;
        }
        return 0;
}

unsigned int loadShader(const char* vertexShaderPath, const char* fragmentShaderPath)
{
        // VERTEX
        char * vertex_source;
        int vertex_file = readfile(vertexShaderPath, &vertex_source);
        if (vertex_file != 0)
        {
                printf("unable to compile shader. vertex shader couldn't be found.");
                return -1;
        }
	
	    unsigned int vertexShader;
	    vertexShader = glCreateShader(GL_VERTEX_SHADER);
	    glShaderSource(vertexShader, 1, (const char* const *)&vertex_source, NULL);
	    glCompileShader(vertexShader);

	    int vertex_success;
	    char vertexInfoLog[512];
	    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertex_success);
	    if(!vertex_success)
	    {
	    	glGetShaderInfoLog(vertexShader, 512, NULL, vertexInfoLog);
	    	printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s\n",vertexInfoLog);
	    }
	    free(vertex_source);

        // FRAGMENT

        char * fragment_source;
        int fragment_file = readfile(fragmentShaderPath, &fragment_source);
        if (fragment_file != 0)
        {
                printf("unable to compile shader. fragment shader couldn't be found.");
                return -1;
        }

	    unsigned int fragmentShader;
	    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	    glShaderSource(fragmentShader, 1, (const char* const *)&fragment_source, NULL);
	    glCompileShader(fragmentShader);
	    
	    int fragment_success;
	    char fragmentInfoLog[512];
	    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragment_success);
	    if(!fragment_success)
	    {
	    	glGetShaderInfoLog(fragmentShader, 512, NULL, fragmentInfoLog);
	    	printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: %s\n",fragmentInfoLog);
	    }
	    free(fragment_source);

	    //SHADER PROGRAM
	    unsigned int shader = glCreateProgram();
	    glAttachShader(shader, vertexShader);
	    glAttachShader(shader, fragmentShader);
	    glLinkProgram(shader);

	    int shader_success;
	    char shaderInfoLog[512];
	    glGetProgramiv(shader, GL_LINK_STATUS, &shader_success);
	    if(!shader_success)
	    {
	    	glGetProgramInfoLog(shader, 512, NULL, shaderInfoLog);
	    	printf("ERROR::SHADER::PROGRAM::COMPILATION_FAILED: %s\n",shaderInfoLog);
	    }
	    
	    glDeleteShader(vertexShader);
	    glDeleteShader(fragmentShader);

	    return shader;
}


int loadTexture(const char* path, unsigned int* out)
{
        int width,height,nrChannels;
        unsigned int texture;
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        if (data)
        {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
                stbi_image_free(data);
                printf("Couldn't load file at %s\n",path);
                return -1;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(data);   
        glBindTexture(GL_TEXTURE_2D, 0);
        *out = texture;

        return 0;
}


float vertex_data[] = {
        // VERTEX           COLOR           UV
        -1.0f, -1.0f, 0.0f,  1.0f,1.0f,1.0f, 0.0f,0.0f,
        -1.0f, 1.0f, 0.0f,  1.0f,0.0f,0.0f, 0.0f,1.0f,
         1.0f, 1.0f, 0.0f,  0.0f,0.0f,0.0f, 1.0f,1.0f,

        -1.0f, -1.0f, 0.0f, 1.0f,1.0f,1.0f, 0.0f,0.0f,
         1.0f, -1.0f, 0.0f, 0.0f,1.0f,1.0f, 1.0f,0.0f,
         1.0f,  1.0f, 0.0f, 0.0f,1.0f,1.0f, 1.0f,1.0f
};


// Update the framebuffer size to the window size 
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
        glViewport(0,0,width,height);
}


class Mesh
{
public:
        unsigned int VBO_id;
        unsigned int boundVAO;
        unsigned int Shader_id;

        Mesh(const char* vertexPath, const char* fragmentPath, unsigned int VAO_id, float* vertexData, int vertexDataSize)
        {
                boundVAO = VAO_id;
                // Create new individual Vertex Buffer Object  
                glGenBuffers(1, &VBO_id);
                // Unbind the current buffers just in case
                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // Bind the requested VAO
                glBindVertexArray(VAO_id);
                
                // set the current VBO
                glBindBuffer(GL_ARRAY_BUFFER, VBO_id);
                // set the vertex data
	            glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, GL_STATIC_DRAW);

                //  Configure the vertex data attributes

	            //Vertex Postion
	            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
	            glEnableVertexAttribArray(0);

                //Color Postion
                glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
                glEnableVertexAttribArray(1);

	            //UV Postion
	            glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(5*sizeof(float)));
	            glEnableVertexAttribArray(2);

	            // unbind/release the currently set buffers 
                glBindBuffer(GL_ARRAY_BUFFER, 0);
	            glBindVertexArray(0);

                Shader_id = loadShader(vertexPath, fragmentPath);
        }
};


int main(int argc, char* argv[])
{
        GLFWwindow* window;
	    int width,height;
	    width = 640;
	    height = 480;

	    //Check if glfw has initialized
	    if (!glfwInit())
		        return -1;

        
	    window = glfwCreateWindow(width,height, "global lattice test", NULL, NULL);
	    if(!window)
	    {
	    	glfwTerminate();
	    	printf("unable to open a Window context.");
	    	return -1;
	    }

	    glfwMakeContextCurrent(window);
	    gladLoadGL(glfwGetProcAddress);

        printf("%s\n",glGetString(GL_VERSION));

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


	    unsigned int VAO;
	    glGenVertexArrays(1, &VAO);

	    //Add new meshes
        Mesh test = Mesh("resources/genericVertex.glsl", "resources/genericFragment.glsl", VAO, vertex_data, sizeof(vertex_data));

	    if (argc == 2){
	    	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    }

        glBindVertexArray(VAO);

        printf("loaded properly");

        while(!glfwWindowShouldClose(window))
        {
		        float time = glfwGetTime();

                glClearColor(0.4f,0.5f,0.6f,1.0f);
		        glClear(GL_COLOR_BUFFER_BIT);

		        glUseProgram(test.Shader_id);
	        
                glDrawArrays(GL_TRIANGLES, 0, 6);

		        glfwSwapBuffers(window);

		        glfwPollEvents();
	    }

        glfwTerminate();

        return 0;
}
