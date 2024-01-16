#include "glm/fwd.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/gtc/type_ptr.hpp"

#include <fstream>
#include <filesystem>

// 20 bytes per voxel
// this only meant to be stored in a block palette
// or a something like that, we need to avoid instancing it for every single instance
typedef struct Voxel
{
        float r;
        float g;
        float b;
        float a;
        int temperature;
}Voxel;


typedef struct Lattice
{
        int width;
        int height;
        int depth;
        unsigned int vbo;
        unsigned int vao;
        unsigned int shader_program;
        int vbo_size;
        glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,1.0f));
}Lattice;


typedef struct Camera
{
        const float fov = 70.0f;
        float speed = 0.25f;
        const float sensitivity = 0.05f;
        float yaw=90.f,pitch;
        glm::vec3 position = glm::vec3(0.0f,0.0f,-1.0f);
        //glm::vec3 target = glm::vec3(0.0f,0.0f,0.0f);
        glm::vec3 direction;// = glm::normalize(position - target);
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection;
}Camera;


// free out!
int read_file(const char * path, char** out)
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


void set_shader_value_float(const char * loc, float value, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                return;//printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform1f(location, value);
}


void set_shader_value_vec2(const char * loc, glm::vec2 value, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                return;//printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform2f(location, value.x, value.y);
}


void set_shader_value_float_array(const char * loc, float* value, int size, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                return;//printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform1fv(location,size,value);
}


void set_shader_value_matrix4(const char * loc, glm::mat4 value, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                return;//printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}


unsigned int load_shader(const char* vertex_shaderPath, const char* fragment_shaderPath)
{
        // VERTEX
        char * vertex_source;
        int vertex_file = read_file(vertex_shaderPath, &vertex_source);
        if (vertex_file != 0)
        {
                printf("unable to compile shader. vertex shader couldn't be found.\n");
                return -1;
        }
	
	    unsigned int vertex_shader;
	    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	    glShaderSource(vertex_shader, 1, (const char* const *)&vertex_source, NULL);
	    glCompileShader(vertex_shader);

	    int vertex_success;
	    char vertex_info_log[512];
	    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_success);
	    if(!vertex_success)
	    {
	    	glGetShaderInfoLog(vertex_shader, 512, NULL, vertex_info_log);
	    	printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s\n",vertex_info_log);
	    }
	    free(vertex_source);

        // FRAGMENT

        char * fragment_source;
        int fragment_file = read_file(fragment_shaderPath, &fragment_source);
        if (fragment_file != 0)
        {
                printf("unable to compile shader. fragment shader couldn't be found.\n");
                return -1;
        }

	    unsigned int fragment_shader;
	    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	    glShaderSource(fragment_shader, 1, (const char* const *)&fragment_source, NULL);
	    glCompileShader(fragment_shader);
	    
	    int fragment_success;
	    char fragment_info_log[512];
	    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_success);
	    if(!fragment_success)
	    {
	    	glGetShaderInfoLog(fragment_shader, 512, NULL, fragment_info_log);
	    	printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: %s\n",fragment_info_log);
	    }
	    free(fragment_source);

	    //SHADER PROGRAM
	    unsigned int shader = glCreateProgram();
	    glAttachShader(shader, vertex_shader);
	    glAttachShader(shader, fragment_shader);
	    glLinkProgram(shader);

	    int shader_success;
	    char shader_info_log[512];
	    glGetProgramiv(shader, GL_LINK_STATUS, &shader_success);
	    if(!shader_success)
	    {
	    	glGetProgramInfoLog(shader, 512, NULL, shader_info_log);
	    	printf("ERROR::SHADER::PROGRAM::COMPILATION_FAILED: %s\n",shader_info_log);
	    }
	    
	    glDeleteShader(vertex_shader);
	    glDeleteShader(fragment_shader);

	    return shader;
}

/*
// creates the textures to be displayed on chunk lattices
void texture_packer(int** chunk_data, int chunk_data_size, int chunk_width, int chunk_height, int chunk_depth, unsigned int* out_texture_id)
{
        glGenTextures(1, out_texture_id);
        glBindTexture(GL_TEXTURE_2D, *out_texture_id);

        int texture_width = (2*chunk_width)*(2*chunk_depth);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        
        int width,height,nr_channels;
        unsigned char* data_test = stbi_load("resources/dirt.png", &width, &height, &nr_channels, 0);

        unsigned char* data = (unsigned char*) malloc(4*sizeof(char));
        *data = 0xFF;
        *(data+1) = 0x00;
        *(data+2) = 0x00;
        *(data+3) = 0xFF;

        for (int i = 0 ; i < 4 ; i++)
        {
                printf("%d %p 0x%02x\n",i,data+i,*(data+i));
        }

        for (int i = 0 ; i < 16 ; i++)
        {
                printf("%d %p 0x%02x\n",i,data_test+i,*(data_test+i));
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

        //free(data);
        //stbi_image_free(data_test);
        glBindTexture(GL_TEXTURE_2D,0);
}
*/

/*
float vertex_data[] = {
        // VERTEX           UV
        -1.0f, -1.0f, 0.0f, 1.0f,1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,0.0f,
         1.0f,  1.0f, 0.0f, 0.0f,0.0f,

         1.0f,  1.0f, 0.0f, 0.0f,0.0f,
         1.0f, -1.0f, 0.0f, 0.0f,1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,1.0f
};*/


// Update the framebuffer size to the window size 
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
        glViewport(0,0,width,height);
}


// global client camera
struct Camera camera;


void camera_process(struct Camera* camera)
{
        camera->front = glm::normalize(camera->direction);

        camera->right = glm::normalize(glm::cross(glm::vec3(0.0f,1.0f,0.0f), camera->direction));
        camera->up = glm::cross(camera->direction, camera->right);

        camera->view = glm::lookAt(camera->position, camera->position+camera->front, camera->up);
}


double last_x,last_y;
bool first_mouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
        if (first_mouse)
        {
                last_x = xpos;
                last_y = ypos;
                first_mouse = false;
        }

        float x_offset = xpos - last_x;
        float y_offset = ypos - last_y;
        last_x = xpos;
        last_y = ypos;

        x_offset *= camera.sensitivity;
        y_offset *= camera.sensitivity;

        camera.yaw += x_offset;
        camera.pitch -= y_offset;

        if (camera.pitch > 89.0f)
                camera.pitch = 89.0f;
        if (camera.pitch < -89.0f)
                camera.pitch = -89.0f;
        
        camera.direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.direction.y = sin(glm::radians(camera.pitch));
        camera.direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
}


void input_process(GLFWwindow* window, struct Camera* camera, float frame_delta)
{
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camera->position += camera->speed * frame_delta * camera->front;
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camera->position -= camera->speed * frame_delta * camera->front;
        if(glfwGetKey(window, GLFW_KEY_A))
                camera->position -= glm::normalize(glm::cross(camera->front, camera->up)) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_D))
                camera->position += glm::normalize(glm::cross(camera->front, camera->up)) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                camera->position -= glm::vec3(0.0,1.0,0.0) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_SPACE))
                camera->position += glm::vec3(0.0,1.0,0.0) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                camera->speed = 8.0;
        else
                camera->speed = 1.0f;
}


void draw_lattice(GLFWwindow* window, struct Lattice* lattice, struct Camera* camera)
{
        glBindVertexArray(lattice->vao);

        // TODO set the texture buffer once the texture packer is done
        glUseProgram(lattice->shader_program);

        set_shader_value_float("TIME", (float) glfwGetTime(), lattice->shader_program);

        int width,height;
        glfwGetWindowSize(window, &width, &height);
        glm::vec2 resolution = glm::vec2(width, height);
        set_shader_value_vec2("RESOLUTION", resolution, lattice->shader_program);
        
        camera->projection = glm::perspective(glm::radians(camera->fov), (float)width/height, 0.001f, 300.0f);

        set_shader_value_matrix4("model", lattice->model_matrix, lattice->shader_program);
        set_shader_value_matrix4("view", camera->view, lattice->shader_program);
        set_shader_value_matrix4("projection", camera->projection, lattice->shader_program);

        glDrawArrays(GL_TRIANGLES, 0, lattice->vbo_size);
}


struct Lattice create_lattice(const char* vertexPath, const char* fragmentPath, float* vbo_data, size_t vbo_size)
{
        struct Lattice result;

        glGenVertexArrays(1,&result.vao);
        result.vbo_size = vbo_size;
        
        // Create new individual Vertex Buffer Object  
        glGenBuffers(1, &result.vbo);

        // Bind the requested VAO
        glBindVertexArray(result.vao);
        
        // set the current VBO
        glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
        // set the vertex data
        glBufferData(GL_ARRAY_BUFFER, vbo_size, vbo_data, GL_STATIC_DRAW);
        
        //  Configure the vertex data attributes
        
        //Vertex Postion
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);
               
        //UV Postion
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // unbind/release the currently set buffers 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        result.shader_program = load_shader(vertexPath, fragmentPath);
        if (result.shader_program == -1)
        {
                printf("shader program didn't compile correctly\n");
        }

        return result;
}


void create_lattice_mesh_data(int size, float voxel_scale, float** out, size_t* out_size)
{
        //number of floats per vertex
        const int vertex_stride = 6;
        //number of vertices per index / number of vertices required for a face
        const int index_stride = 6;
        
        long long int vertex_offset = 0;

        printf("size: %d\n",size);
        // 6 floats per vertex, width*2 + height*2 + depth*2 = face count, face_count * 6 * 6 * sizeof(float) = byte count
        size_t face_count = size*6;
        // face count * 6 vertices * 6 floats per vertex (3 floats for position, 3 for UV) * sizeof float (should be 4 bytes/32bits)
        size_t float_count = face_count * index_stride * vertex_stride;
        size_t byte_count = float_count*sizeof(float);
        printf("size of float: %zu\n",sizeof(float));
        printf("lattice chunk face count: %zu\n", face_count);
        printf("float count: %zu\n",float_count);
        printf("number of bytes for the lattice mesh: %zu\n", float_count*sizeof(float));
        
        *out = (float *) malloc(byte_count);
        *out_size = byte_count;

        if (*out == NULL)
        {
                printf("Unable to create lattice data heap.\n");
        }
        
        // Negative Z faces
        for (int z = 0 ; z < size ; z++)
        {
                // BOTTOM FACE
                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -z*voxel_scale;
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = -z*voxel_scale;
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = -z*voxel_scale;
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                // TOP FACE
                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = -z*voxel_scale;
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -z*voxel_scale;
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -z*voxel_scale;
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;
        }

        // POSITIVE Z FACES
        // TODO reorder vertices to go counter clockwise
        for (int z = 0 ; z < size ; z++)
        {
                // BOTTOM FACE
                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = -z*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = -z*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;


                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -z*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                // TOP FACE
                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -z*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -z*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = z;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = -z*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = z;
        
                vertex_offset+=vertex_stride;
        }

        // NEGATIVE X FACES
        for (int x = 0 ; x < size ; x++)
        {
                int reverse = (size-1)-x;
                // BOTTOM FACE
                *(*out+vertex_offset+0) = x*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = x*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = x*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                // TOP FACE
                *(*out+vertex_offset+0) = x*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = x*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = x*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = 0.0f;
        
                vertex_offset+=vertex_stride;
        }

        // POSITIVE X FACES
        for (int x = 0 ; x < size ; x++)
        {
                int reverse = (size-1)-x;
                // BOTTOM FACE
                *(*out+vertex_offset+0) = x*voxel_scale;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = x*voxel_scale;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = x*voxel_scale;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                // TOP FACE
                *(*out+vertex_offset+0) = x*voxel_scale;
                *(*out+vertex_offset+1) = 0.0f;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 0.0f;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = x*voxel_scale;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = x*voxel_scale;
                *(*out+vertex_offset+1) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = reverse;
                *(*out+vertex_offset+4) = 1.0f;
                *(*out+vertex_offset+5) = 1.0f;
        
                vertex_offset+=vertex_stride;
        }

        // NEGATIVE Y FACES
        for (int y = 0 ; y < size ; y++)
        {
                // BOTTOM FACE
                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = y*voxel_scale;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = y*voxel_scale;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = y*voxel_scale;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                // TOP FACE
                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = y*voxel_scale;
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = y*voxel_scale;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = y*voxel_scale;
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 1.0f;
                
                vertex_offset+=vertex_stride;
        }

        // POSITIVE Y FACES
        for (int y = 0 ; y < size ; y++)
        {
                // BOTTOM FACE
                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = y*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = y*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 0.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = y*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                // TOP FACE
                *(*out+vertex_offset+0) = 1.0f*voxel_scale*size;
                *(*out+vertex_offset+1) = y*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 0.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 1.0f;

                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = y*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+2) = -1.0f*voxel_scale*size+(1.0f*voxel_scale);
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 1.0f;
                
                vertex_offset+=vertex_stride;

                *(*out+vertex_offset+0) = 0.0f;
                *(*out+vertex_offset+1) = y*voxel_scale+(1.0f*voxel_scale);
                *(*out+vertex_offset+2) = 1.0f*voxel_scale;
                *(*out+vertex_offset+3) = 1.0f;
                *(*out+vertex_offset+4) = y;
                *(*out+vertex_offset+5) = 0.0f;
                
                vertex_offset+=vertex_stride;
        }

        printf("vertex_offset: %lld\n",vertex_offset);
}


void print_mat4(glm::mat4 mat)
{
        printf("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",mat[0][0], mat[0][1],mat[0][2],mat[0][3],mat[1][0],mat[1][1],mat[1][2],mat[1][3],mat[2][0],mat[2][1],mat[2][2],mat[2][3],mat[3][0],mat[3][1],mat[3][2],mat[3][3]);
}


int main(int argc, char* argv[])
{
        // Set minimum version of opengl
        // We are using at minimum version 4.4 which supports texture arrays
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window;
	    int width = 800,height = 600;

	    if (!glfwInit())
		        return -1;

	    window = glfwCreateWindow(width,height, "global lattice test", NULL, NULL);
	    if(!window)
	    {
	    	glfwTerminate();
	    	printf("unable to open a window context.");
	    	return -1;
	    }

	    glfwMakeContextCurrent(window);
	    gladLoadGL(glfwGetProcAddress);

        printf("OpenGL version: %s\n",glGetString(GL_VERSION));

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        int lattice_size = 3;
        float * lattice_data;
        size_t lattice_data_size;
        create_lattice_mesh_data(lattice_size, 0.1f, &lattice_data, &lattice_data_size);

        printf("lattice data size: %zu\n",lattice_data_size);

        struct Lattice chicken = create_lattice("resources/genericVertex.glsl", "resources/genericFragment.glsl", lattice_data, lattice_data_size);

        printf("passed the lattice data\n");

        free(lattice_data);

        int chunk_data_size = lattice_size*lattice_size*lattice_size;
        printf("chunk data size: %d\n",chunk_data_size);
        int* chunk_data = (int*) malloc(chunk_data_size*sizeof(int));
        printf("size of chunk_data: %zu\n",chunk_data_size*sizeof(int));

        for (int i = 0 ; i < chunk_data_size ; i++)
        {
                *(chunk_data+i) = rand()%2;
        }

        unsigned int lattice_chunk_albedo;
        
        glGenTextures(1, &lattice_chunk_albedo);
        glBindTexture(GL_TEXTURE_2D_ARRAY, lattice_chunk_albedo);

        unsigned char* stone = (unsigned char * ) malloc(4*sizeof(char));

        *(stone) = 0x00;
        *(stone+1) = 0x00;
        *(stone+2) = 0xFF;
        *(stone+3) = 0xFF;

        unsigned char* original = (unsigned char*) malloc(4*sizeof(char));

        *(original) = 0xFF;
        *(original+1) = 0x00;
        *(original+2) = 0x00;
        *(original+3) = 0xFF;
        
        unsigned char* zp = (unsigned char* ) malloc(4*sizeof(char));
        *(zp) = 0x00;
        *(zp+1) = 0xFF;
        *(zp+2) = 0x00;
        *(zp+3) = 0xFF;

        unsigned char * xp = (unsigned char *) malloc(4*sizeof(char));
        *(xp) = 0x90;
        *(xp+1) = 0xFF;
        *(xp+2) = 0x00;
        *(xp+3) = 0xFF;

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, lattice_size, lattice_size, lattice_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        float start_time = glfwGetTime();
        printf("texture packer start time: %f\n",start_time);
        for (int i  = 0 ; i < chunk_data_size ; i++)
        {
                int x=(i/lattice_size)%lattice_size,y=(i/lattice_size/lattice_size)%lattice_size,z=i%lattice_size;
                //printf("index: %d position: %d,%d,%d,  value: %d\n",i,x,y,z,*(chunk_data+i));
                switch (*(chunk_data+i))
                {
                        case 0:
                                break;
                        case 1:
                                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, x, y, z, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, stone);
                                break;
                }
        }
        printf("texture packer end time: %f\n",glfwGetTime()-start_time);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, original);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, zp);
       
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 1, 0, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, xp);

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        free(original);
        free(zp);
        free(xp);
        free(stone);

        if (argc == 2){
	    	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    }
        
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        float prev_frame_time = 0.0f;

        glBindTexture(GL_TEXTURE_2D_ARRAY, lattice_chunk_albedo);

        while(!glfwWindowShouldClose(window))
        {
                float frame_delta = glfwGetTime() - prev_frame_time;
                prev_frame_time = glfwGetTime();
                
                glClearColor(0.4f,0.5f,0.6f,1.0f);
		        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                input_process(window, &camera, frame_delta);

                camera_process(&camera);
                //printf("frame delta: %f ",frame_delta);
                //printf("position, x: %f, y: %f, z: %f\n",camera.position.x, camera.position.y, camera.position.z);
                /*printf("view matrix:\n");
                print_mat4(camera.view);
                printf("projection matrix:\n");
                print_mat4(camera.projection);*/
                draw_lattice(window, &chicken, &camera);

		        glfwSwapBuffers(window);

		        glfwPollEvents();
	    }

        free(chunk_data);

        glfwTerminate();

        return 0;
}
