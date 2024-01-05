#include "glm/fwd.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <fstream>
#include <filesystem>


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
                printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform1f(location, value);
}


void set_shader_value_vec2(const char * loc, glm::vec2 value, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform2f(location, value.x, value.y);
}


void set_shader_value_float_array(const char * loc, float* value, int size, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform1fv(location,size,value);
}


void set_shader_value_matrix4(const char * loc, glm::mat4 value, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
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


int load_texture(const char* path, unsigned int* out)
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


int load_texture_array(const char** path, size_t path_size, unsigned int* out_texture_id)
{
        glGenTextures(1,out_texture_id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, *out_texture_id);

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 16,16,path_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        // load textures and bind them to the array
        for (int i = 0 ; i < path_size ; i++)
        {
                printf("%d path: \"%s\"\n",i,path[i]);
                int width,height,nr_channels;
                unsigned char* texture = stbi_load(path[i],&width,&height,&nr_channels,4);
                printf("number of channels: %d\n",nr_channels);
                printf("width: %d, height: %d\n",width,height);

                if (texture)
                        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0,0,i, width,height,1, GL_RGBA, GL_UNSIGNED_BYTE, texture);
                else
                {
                        printf("Unable to load texture from path: %s\n",path[i]);
                        stbi_image_free(texture);
                        return -1;
                }
                stbi_image_free(texture);
        }

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        printf("finished loading textures\n");
        return 0;
}


// creates the textures to be displayed on chunk lattices
int texture_packer(const char** pathes, size_t pathes_size, int** chunk_data, int chunk_width, int chunk_height, int chunk_depth, unsigned int* out_texture_id)
{
        glGenTextures(1, out_texture_id);
        glBindTexture(GL_TEXTURE_2D, *out_texture_id);

        int texture_width = chunk_width*6*chunk_depth;

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, chunk_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        for(int i = 0 ; i < 1024 ; i ++)
        {
                
        }
        return 0;
}


float vertex_data[] = {
        // VERTEX           UV
        -1.0f, -1.0f, 0.0f, 1.0f,1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,0.0f,
         1.0f,  1.0f, 0.0f, 0.0f,0.0f,

        -1.0f, -1.0f, 0.0f, 1.0f,1.0f,
         1.0f, -1.0f, 0.0f, 0.0f,1.0f,
         1.0f,  1.0f, 0.0f, 0.0f,0.0f
};


// Update the framebuffer size to the window size 
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
        glViewport(0,0,width,height);
}


typedef struct Camera
{
        const float fov = 60.0f;
        const float speed = 1.0f;
        const float sensitivity = 0.05f;
        float yaw,pitch;
        glm::vec3 position = glm::vec3(0.0f,0.0f,-5.0f);
        //glm::vec3 target = glm::vec3(0.0f,0.0f,0.0f);
        glm::vec3 direction;// = glm::normalize(position - target);
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection;
}Camera;


// global client camera
struct Camera camera;


void camera_process(struct Camera* camera)
{
        camera->front = glm::normalize(camera->direction);

        camera->right = glm::normalize(glm::cross(glm::vec3(0.0f,1.0f,0.0f), camera->direction));
        camera->up = glm::cross(camera->direction, camera->right);

        camera->view = glm::lookAt(camera->position, camera->position+camera->front, camera->up);

        camera->view = glm::translate(camera->view, camera->position);
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
        camera.pitch += y_offset;

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
                glfwTerminate();
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camera->position += camera->speed * frame_delta * camera->front;
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camera->position -= camera->speed * frame_delta * camera->front;
        if(glfwGetKey(window, GLFW_KEY_A))
                camera->position += glm::normalize(glm::cross(camera->front, camera->up)) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_D))
                camera->position += glm::normalize(glm::cross(camera->front, camera->up)) * camera->speed * frame_delta;
}


typedef struct Mesh
{
        unsigned int vbo;
        unsigned int vao;
        unsigned int shader_program;
        int vbo_size;
        glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,1.0f));
}Mesh;


struct Mesh create_mesh(const char* vertexPath, const char* fragmentPath, unsigned int vao_id, float* vbo_data, int vbo_size)
{
        struct Mesh result;
        result.vao = vao_id;
        result.vbo_size = vbo_size;
        // Create new individual Vertex Buffer Object  
        glGenBuffers(1, &result.vbo);
        // Unbind the current buffers just in case
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        // Bind the requested VAO
        glBindVertexArray(result.vao);
                               
        // set the current VBO
        glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
        // set the vertex data
        glBufferData(GL_ARRAY_BUFFER, vbo_size, vbo_data, GL_STATIC_DRAW);
        
        //  Configure the vertex data attributes
        
        //Vertex Postion
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);
               
        //UV Postion
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
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


void draw_mesh(GLFWwindow* window, struct Mesh* mesh, struct Camera* camera)
{
        glBindVertexArray(mesh->vao);

        glUseProgram(mesh->shader_program);
        set_shader_value_float("TIME", (float) glfwGetTime(), mesh->shader_program);

        int width,height;
        glfwGetWindowSize(window, &width, &height);
        glm::vec2 resolution = glm::vec2(width, height);
        set_shader_value_vec2("RESOLUTION", resolution, mesh->shader_program);

        camera->projection = glm::perspective(glm::radians(camera->fov), (float)width/height, 0.1f, 100.0f);

        set_shader_value_matrix4("model", mesh->model_matrix, mesh->shader_program);
        set_shader_value_matrix4("view", camera->view, mesh->shader_program);
        set_shader_value_matrix4("projection", camera->projection, mesh->shader_program);

        glDrawArrays(GL_TRIANGLES, 0, mesh->vbo_size);
}


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


void draw_lattice(GLFWwindow* window, struct Lattice* lattice, struct Camera* camera)
{
        glBindVertexArray(lattice->vao);

        glUseProgram(lattice->shader_program);

        set_shader_value_float("TIME", (float) glfwGetTime(), lattice->shader_program);

        int width,height;
        glfwGetWindowSize(window, &width, &height);
        glm::vec2 resolution = glm::vec2(width, height);
        set_shader_value_vec2("RESOLUTION", resolution, lattice->shader_program);

        //set_shader_value_float_array("data", chunk_data, chunk_data_size, mesh->shader_program);
        
        //printf("width: %d, height: %d\n",width,height);
        
        camera->projection = glm::perspective(glm::radians(camera->fov), (float)width/height, 0.1f, 100.0f);

        //mesh->model_matrix = glm::rotate(mesh->model_matrix, (float) glfwGetTime()*glm::radians(1.0f), glm::vec3(0.0f,0.0f,1.0f));
        //mesh->model_matrix = glm::translate(mesh->model_matrix, glm::vec3(0.0f,0.0f,0.0f));
        
        set_shader_value_matrix4("model", lattice->model_matrix, lattice->shader_program);
        set_shader_value_matrix4("view", camera->view, lattice->shader_program);
        set_shader_value_matrix4("projection", camera->projection, lattice->shader_program);

        glDrawArrays(GL_TRIANGLES, 0, lattice->vbo_size);
}


struct Lattice create_lattice(const char* vertexPath, const char* fragmentPath, unsigned int vao_id, float* vbo_data, int vbo_size)
{
        return (struct Lattice) {};
}


void create_lattice_mesh_data(int width, int height, int depth, float voxel_scale, float** out, size_t* out_size)
{
        const int stride = 5;

        printf("width: %d, height: %d, depth: %d\n",width, height, depth);
        // 5 floats per vertex, width*2 + height*2 + depth*2 = face count, face_count * 6 * 5 * sizeof(float) = byte count
        size_t face_count = (width*2) + (height*2) + (depth*2);
        size_t byte_count = face_count * 6 * stride * sizeof(float);
        printf("lattice chunk face count: %lld\n", face_count);
        printf("number of bytes for the lattice: %lld", byte_count);
        
        *out = (float *) malloc(byte_count);
                
        // Negative Z faces
        for (int z = 0 ; z < depth ; z++)
        {
                // vectors
                *out[z*stride+0] = 0.0f*voxel_scale*depth;
                *out[z*stride+1] = 0.0f;
                *out[z*stride+2] = 0.0f*voxel_scale*depth;
                //*out+(x*stride)+1;
                //*out+(x*stride)+2;
                //*out+(x*stride)+3;
                //*out+(x*stride)+4;

                //*out+(x*stride+1) = -1.0f*voxel_scale*depth;
                //*out+(x*stride+1)+1;
                //*out+(x*stride+1)+2;
                //*out+(x*stride+1)+3;
                //*out+(x*stride+1)+4;

                //*out+(x*stride+2) = -1.0f*voxel_scale*depth;
                //*out+(x*stride+2)+1;
                //*out+(x*stride+2)+2;
                //*out+(x*stride+2)+3;
                //*out+(x*stride+2)+4;
        }

        // Negative X faces
        //for (int x = 0 ; x < width ; x++)
        //{
        //        *out;
        //}           
}


int main(int argc, char* argv[])
{
        // Set minimum version of opengl
        // We are using at minimum version 4.4 which supports texture arrays
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window;
	    int width,height;
	    width = 16*32;
	    height = 16*32;

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

	    unsigned int VAO;
	    glGenVertexArrays(1, &VAO);


        int lattice_width = 512;
        int lattice_height = 512;
        int lattice_depth = 512;
	    float * lattice_data;
        size_t lattice_data_size;
        //create_lattice_mesh_data(lattice_width, lattice_height, lattice_depth, 1.0f, &lattice_data, &lattice_data_size);

        struct Mesh test = create_mesh("resources/genericVertex.glsl", "resources/genericFragment.glsl", VAO, vertex_data, sizeof(vertex_data));

	    if (argc == 2){
	    	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    }

        /*
        float chunk_data[1024];

        for (int i = 0 ; i < 1024 ; i++)
        {
                chunk_data[i] = rand() % 2;
                //printf("chunk_data[%d] = %f\n", i, chunk_data[i]);
        }

        unsigned int textures;
        const char * pathes[3];
        pathes[0] = "resources/cobblestone.png";
        pathes[1] = "resources/stone.png";
        pathes[2] = "resources/oak_log.png";
        size_t size = sizeof(pathes)/sizeof(pathes[0]);
        
        int textures_success = load_texture_array(pathes, size, &textures);
        if (textures_success == -1)
                return -1;

        glBindTexture(GL_TEXTURE_2D_ARRAY, textures);
*/
        glEnable(GL_DEPTH_TEST);

        float prev_frame_time = 0.0f;

        while(!glfwWindowShouldClose(window))
        {
                float frame_delta = glfwGetTime() - prev_frame_time;
                prev_frame_time = glfwGetTime();
                glClearColor(0.4f,0.5f,0.6f,1.0f);
		        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                input_process(window, &camera, frame_delta);

                camera_process(&camera);
                printf("x: %f, y: %f, z: %f\n",camera.position.x, camera.position.y, camera.position.z);
                draw_mesh(window, &test, &camera);
                //draw_lattice(window, &test, &camera);

		        glfwSwapBuffers(window);

		        glfwPollEvents();
	    }

        glfwTerminate();

        return 0;
}
