//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// Dear ImGui: standalone example application for SDL2 + OpenGL                         //
// (SDL is a cross-platform general purpose library for handling windows,               //
// inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)                         //
// If you are new to Dear ImGui, read documentation from the                            //
// docs/ folder + read the top of imgui.cpp.                                            //
// Read online: https://github.com/ocornut/imgui/tree/master/docs                       //
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

#include "plf_nanotimer.h"

#include <sys/time.h>
#include <sched.h>


//so we can build on mingw+linux
#ifdef __linux__ 
    #include <sys/resource.h>
#endif

#include <string.h>
#include <thread>
#include <time.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>

#include <SDL_opengl.h>
#define IMGUI_DEFINE_MATH_OPERATORS

#include "calculator.h"
#include "console.h"
#include "theme.h"

/* SPI declaration */
#define SPI_BUS 0
/* SPI frequency in Hz */
//15mhz
//#define SPI_FREQ 15000000
//#define SPI_FREQ 10000000

#define SPI_FREQ 50000000
//#define SPI_FREQ 2147773188

//global spi context
mraa_spi_context spi;
bool pin_lock =false;
bool draw_lock = false;

bool reset = true;

plf::nanotimer timer;

static ExampleAppConsole console1;
static ExampleAppConsole console2;
static ExampleAppConsole console3;
static ExampleAppConsole console4;
static ExampleAppConsole console5;
static ExampleAppConsole console6;
static ExampleAppConsole console7;
static ExampleAppConsole console8;

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void init_dac(){

    //zero out dacs
    for(int i=0;i<16;i++){
        mraa_spi_write(spi,0x00);
        mraa_spi_write(spi,0x30+i);
        mraa_spi_write(spi,0x00);
        mraa_spi_write(spi,0x00);
    }
}

void spi_manager_A(){
    double results;
    /*
    struct sched_param sp;
    sp.sched_priority = 80;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)){
        printf("WARNING: Failed to set bbt MANAGER thread to real-time priority \n");
    }*/
    

    while(true){
        results = timer.get_elapsed_ns();
        console1.spi_update(results);
        console2.spi_update(results);    
        console3.spi_update(results);    
        console4.spi_update(results);    
        //console5.spi_update(results);    
        //console6.spi_update(results);    
        //console7.spi_update(results);    
        //console8.spi_update(results);
        //printf("exe time in miliseconds %f \n", (timer.get_elapsed_ns()-results)/1000);    

    }
    
}

void spi_manager_B(){
    double results;
    /*
    struct sched_param sp;
    sp.sched_priority = 90;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)){
        printf("WARNING: Failed to set bbt MANAGER thread to real-time priority \n");
    }*/
    

    while(true){
        results = timer.get_elapsed_ns();
        //console1.spi_update(results);
        //console2.spi_update(results);    
        //console3.spi_update(results);    
        //console4.spi_update(results);    
        console5.spi_update(results);    
        console6.spi_update(results);    
        console7.spi_update(results);    
        console8.spi_update(results);
        //printf("exe time in miliseconds %f \n", (timer.get_elapsed_ns()-results)/1000);    

    }
    
}


static void ShowExampleAppConsole(bool* p_open, bool* reset)
{


    console1.Draw("mod 1", p_open,0,0,0);
    console2.Draw("mod 2", p_open,1,0,1);
    console3.Draw("mod 3", p_open,2,0,2);
    console4.Draw("mod 4", p_open,3,0,3);
    console5.Draw("mod 5", p_open,0,1,4);
    console6.Draw("mod 6", p_open,1,1,5);
    console7.Draw("mod 7", p_open,2,1,6);
    console8.Draw("mod 8", p_open,3,1,7);
    
    if(*reset){
        console1.CurrentFrame=0;
        console2.CurrentFrame=0;
        console3.CurrentFrame=0;
        console4.CurrentFrame=0;
        console5.CurrentFrame=0;
        console6.CurrentFrame=0;
        console7.CurrentFrame=0;
        console8.CurrentFrame=0;
        console1.IDX=0;
        console2.IDX=0;
        console3.IDX=0;
        console4.IDX=0;
        console5.IDX=0;
        console6.IDX=0;
        console7.IDX=0;
        console8.IDX=0;
        *reset = false;
    }
}



// Main 
int main(int, char**)
{

    //mraa SPI setup
    mraa_result_t status = MRAA_SUCCESS;
    status = mraa_init();
    if(status != MRAA_SUCCESS){
        printf("MRAA init error \n");
    }
    
    //mraa init stuff
    spi = mraa_spi_init(SPI_BUS);
    if(status!= MRAA_SUCCESS){
            printf("SPI init error \n");
    }

    /* set SPI frequency */
    status = mraa_spi_frequency(spi, SPI_FREQ);

    if(status!= MRAA_SUCCESS){
            printf("SPI clock error \n");
    }

    /* set big endian mode */
    //lt2668 is MSB LSB - this causes errors but is needed?
    status = mraa_spi_lsbmode(spi, 1);

    if(status!= MRAA_SUCCESS){
            printf("SPI lsb error \n");
    }

    init_dac();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    
    //SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS );
    SDL_Window* window = SDL_CreateWindow("ByteBeat", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 600, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |=  ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;     // Enable Keyboard Controls
    io.IniFilename = NULL;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    imtheme();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.05f, .05f, 0.05f, 1.00f);
    int cur_mod=0;

    // Main loop
    bool done = false;
    char focus_window[16];
    bool console1=true;    
    glViewport(0, 0, 1024, 600);

    //disable stdout buff
    std::setvbuf(stdout, NULL, _IONBF, 0);

    
    
    //linux prioritiy
    /*
    struct sched_param sp;
    sp.sched_priority = 80;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)){
        printf("WARNING: Failed to set bbt MAIN thread to real-time priority \n");
    }*/


    timer.start();
    std::thread           ManagerA;
    ManagerA = std::thread(spi_manager_A);
    ManagerA.detach();

    std::thread           ManagerB;
    ManagerB = std::thread(spi_manager_B);
    ManagerB.detach();

    while (!done)
    {
        //printf("%f \n",ImGui::GetIO().Framerate);
        SDL_Event event;

        if(ImGui::IsKeyPressed(ImGuiKey_Escape))
            done = true;    

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;        
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        //simple keyboard controls
        if(ImGui::IsKeyPressed(ImGuiKey_Tab) && !io.KeyShift){
            cur_mod+=1;

            if(cur_mod>8){
                cur_mod=0;
            }
            snprintf(focus_window, 16, "mod %d", cur_mod);
            ImGui::SetWindowFocus((const char*)focus_window);
        }
        if(ImGui::IsKeyPressed(ImGuiKey_Tab) && io.KeyShift){
            cur_mod-=1;
            if(cur_mod<0){
                cur_mod=8;
            }
            snprintf(focus_window, 16, "mod %d", cur_mod);
            ImGui::SetWindowFocus((const char*)focus_window);
        }

        
        ShowExampleAppConsole(&console1,&reset);
        // Rendering
        ImGui::Render();
        
        //??
        //glBlendFunc( GL_ONE,  GL_SRC1_ALPHA);
        glClearColor(clear_color.x , clear_color.y , clear_color.z , clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        fflush(stdout);


    }

    // Cleanup
    mraa_spi_stop(spi);
    mraa_deinit();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
