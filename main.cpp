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

#define AUDIO_ON 1
#define AUDIO_OFF 0
#define AUDIO AUDIO_OFF

#include <sys/time.h>



//so we can build on mingw+linux
#ifdef __linux__ 
    #include <sys/resource.h>
#endif

#ifdef __MINGW32__

#endif

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <string.h>
#include <thread>
#include <time.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include <chrono>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "calculator.h"
#include "console.h"
#include "theme.h"

#if AUDIO == AUDIO_ON
    #include "SDL_mixer.h"
#endif


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

bool reset = true;




//////////////////////////////////////////////////////////////////////////////////////////
//audio globals
int current_channel = 0;

//////////////////////////////////////////////////////////////////////////////////////////
#define NUM_WAVEFORMS 5

#if AUDIO == AUDIO_ON
    const char* _waveFileNames[] =
    {
    "kit/kick.wav",
    "kit/clap.wav",
    "kit/open_hat.wav",
    "kit/closed_hat.wav",
    "kit/snare.wav",
    };

    Mix_Chunk* _sample[5];
#endif



//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


void init_dac(){

    //sleep_us(10000);
    //zero out dacs
    for(int i=0;i<16;i++){
        mraa_spi_write(spi,0x00);
        mraa_spi_write(spi,0x30+i);
        mraa_spi_write(spi,0x00);
        mraa_spi_write(spi,0x00);
        //sleep_us(10000);
    }

}

static void ShowExampleAppConsole(bool* p_open, bool* reset)
{
    static ExampleAppConsole console1;
    static ExampleAppConsole console2;
    static ExampleAppConsole console3;
    static ExampleAppConsole console4;
    static ExampleAppConsole console5;
    static ExampleAppConsole console6;
    static ExampleAppConsole console7;
    static ExampleAppConsole console8;

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
        *reset = false;
    }
}

void textbox(){
    static char name[128] = ""; 
    ImGui::Text("Nome: "); ImGui::SameLine(); ImGui::InputText("", name, IM_ARRAYSIZE(name));
}
#if AUDIO == AUDIO_ON
    void init_audio(){
        memset(_sample, 0, sizeof(Mix_Chunk*) * 5);

        // Set up the audio stream
        Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512);
        //SDL_PauseAudioDevice(deviceId, 0);
        Mix_AllocateChannels(32);

        // Load waveforms
        for( int i = 0; i < NUM_WAVEFORMS; i++ )
        {
            _sample[i] = Mix_LoadWAV(_waveFileNames[i]);
            if( _sample[i] == NULL )
            {
                fprintf(stderr, "Unable to load wave file: %s\n", _waveFileNames[i]);
            }
        }
    }
#endif
// Main code
int main(int, char**)
//int main( int argc, char** argv )
{
    /*
    setpriority(PRIO_PROCESS, 0, -20);
    struct sched_param param;
    param.sched_priority = 1;
    int canSetRealTimeThreadPriority = (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) == 0);
    */

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

    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    #if AUDIO == AUDIO_ON
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
        {
            printf("Error: %s\n", SDL_GetError());
            return -1;
        }
    #endif
        
    #if AUDIO == AUDIO_OFF
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            printf("Error: %s\n", SDL_GetError());
            return -1;
        }
    #endif


    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    //8 or 16? not sure which or if any faster
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    //SDL_DisplayMode DM;
    //SDL_GetCurrentDisplayMode(0, &DM);
    //auto swidth = DM.w;
    //auto sheight = DM.h;

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL |  SDL_WINDOW_BORDERLESS |  SDL_WINDOW_INPUT_GRABBED );
    //SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Window* window = SDL_CreateWindow("bytebeat cv", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 600, window_flags);
    
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    //SDL_GL_SetSwapInterval(1); // Enable vsync

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
    
    //ImGuiContext& g = *GImGui;    
    //g.NavDisableHighlight = false;
    //disable ini
    
    int cur_mod=0;
    

    // Main loop
    #if AUDIO == AUDIO_ON
        init_audio();    
    #endif

    

    bool done = false;
    


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
        bool console1=true;
        //ImGui::ShowDemoWindow();
        ShowExampleAppConsole(&console1,&reset);

        //debug 
        //ImGui::ShowStackToolWindow();
        char focus_window[128];

        //simple keyboard controls
        if(ImGui::IsKeyPressed(ImGuiKey_Tab) && !io.KeyShift){
            cur_mod+=1;

            if(cur_mod>8){
                cur_mod=0;
            }
            snprintf(focus_window, 128, "mod %d", cur_mod);
            ImGui::SetWindowFocus((const char*)focus_window);
        }
        if(ImGui::IsKeyPressed(ImGuiKey_Tab) && io.KeyShift){
            cur_mod-=1;
            if(cur_mod<0){
                cur_mod=8;
            }
            snprintf(focus_window, 128, "mod %d", cur_mod);
            ImGui::SetWindowFocus((const char*)focus_window);
        }

        /*

        if(ImGui::IsKeyPressed(ImGuiKey_RightArrow)){
            //SDL_QueueAudio(deviceId, wavBuffer, wavLength);
            
            Mix_PlayChannel(current_channel, _sample[0], 0);
            current_channel+=1;
            if(current_channel>4){
                current_channel=0;
            }
        }
        if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow)){
            //SDL_QueueAudio(deviceId, wavBuffer, wavLength);
            Mix_PlayChannel(current_channel, _sample[1], 0);
            current_channel+=1;
            if(current_channel>4){
                current_channel=0;
            }
        }
        */

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        
    }

    // Cleanup
    mraa_spi_stop(spi);
    mraa_deinit();

    #if AUDIO == AUDIO_ON
        Mix_CloseAudio();
    #endif

    

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
