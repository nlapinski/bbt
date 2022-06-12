#include <math.h>
#ifdef __linux__ 
    /* mraa header */
    #include "mraa/spi.h"
    //#include "pthread.h"
#endif
#ifdef __MINGW32__
    #include "winmraa.h"
    #include "pthread.h"
#endif

#include <chrono>
#include "plf_nanotimer.h"

//global spi context
extern mraa_spi_context spi;
extern bool reset;
extern plf::nanotimer timer;
extern bool pin_lock;
char* mitoa(int val, int base){
    
    static char buf[256] = {0};
    
    int i = 30;
    
    for(; val && i ; --i, val /= base)
    
        buf[i] = "0123456789abcdef"[val % base];
    
    return &buf[i+1];
    
}

//double fit range
double flt_map(double x, double in_min, double in_max, double out_min, double out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

double round(double d)
{
    return floor(d + 0.5);
}

uint32_t int_map(double input, double input_start, double input_end, double output_start, double output_end){
    double slope = 1.0 * (output_end - output_start) / (input_end - input_start);
    //output = output_start + slope * (input - input_start)
    return (output_start + round(slope * (input - input_start)));
}


int write_pin(mraa_spi_context spi,int pin,int val){
    if(pin_lock==true){
        return 0;
    }
    pin_lock=true;
    //printf("CURRENT STATE OF PIN >>>> %d \n", pin_lock);
    uint8_t low = val & 0xff;
    uint8_t high=(val>>8) & 0xff;
    //uint8_t p1= 0x30 | pin;
    uint8_t pat[4];
    pat[0] =0x00;
    pat[1]=0x30 | pin;
    pat[2]=high;
    pat[3]=low;
    mraa_spi_write_buf(spi, pat, 4);
    pin_lock=false;
    return 1;

}


//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Console / ShowExampleAppConsole()
//-----------------------------------------------------------------------------
char* replace_str(const char* s, const char* oldW, const char* newW)
{
    char* result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);
  
    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;
  
            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }
  
    // Making new string of enough length
    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1);
  
    i = 0;
    while (*s) {
        // compare the substring with the result
        if (strstr(s, oldW) == s) {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }
  
    result[i] = '\0';
    return result;
}


int stringify(char *dest, const char *target, const char *replace) {
    char *p = strstr(dest, target);
    if (p == NULL) {
        /* no replacement */
        return 0;
    }
    size_t len1 = strlen(target);
    size_t len2 = strlen(replace);
    if (len1 != len2) {
        /* move the remainder of the string into the right place */
        memmove(p + len2, p + len1, strlen(p + len1) + 1);
    }
    memcpy(p, replace, len2);
    return 1;
}

char *stristr4(const char *haystack, const char *needle) {
    int c = tolower((unsigned char)*needle);
    if (c == '\0')
        return (char *)haystack;
    for (; *haystack; haystack++) {
        if (tolower((unsigned char)*haystack) == c) {
            for (size_t i = 0;;) {
                if (needle[++i] == '\0')
                    return (char *)haystack;
                if (tolower((unsigned char)haystack[i]) != tolower((unsigned char)needle[i]))
                    break;
            }
        }
    }
    return NULL;
}

/*
void posix_nano(int microseconds){
    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);

    // Add the time you want to sleep
    deadline.tv_nsec += microseconds*1000;

    // Normalize the time to account for the second boundary
    if(deadline.tv_nsec >= 1000000000) {
       deadline.tv_nsec -= 1000000000;
        deadline.tv_sec++;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
}


void std_sleep_us(int microseconds)
{
 
    bool sleep = true;
    auto start = std::chrono::steady_clock::now();
    while(sleep)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
        if ( elapsed.count() > microseconds )
            sleep = false;
    }
}
*/


void spi_task(int* ms, int* next_time,char* cmd, int *pin, char* ResultBuf, char* ResultValue, char* LastCommand, unsigned long long *CurrentFrame, float* adc1arr,float* adc2arr, double* imin, double* imax, double* omin, double* omax){
    //int t = 0;
    int IDX=0;
    
    //linux prioritiy
    /*
    struct sched_param sp;
    sp.sched_priority = 90;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)){
        printf("WARNING: Failed to set bbt WORKER thread to real-time priority \n");
    }
    */
    
    double results = timer.get_elapsed_ns();
    double end = results+((double)(*ms)*1.0);

    while(true){
        results = timer.get_elapsed_ns();
    
        if(results<end){
            continue;
        }
        else{

            *CurrentFrame+=1;
            int t = (int)*CurrentFrame;
            char temp_str[256];
            strcpy(temp_str,LastCommand);
            char time_str[256];
            snprintf(time_str,256,"%d",t);
            char replace[2] = "t";
            char* result;
            result = replace_str((char*)temp_str, (char*)replace, (char*)time_str);
            //stringify(temp_str, replace, time_str);
            uint8_t res = (uint8_t)calc((char*)result);
            strcpy(ResultBuf,result);
            double voltage = flt_map((double)res,*imin,*imax,*omin,*omax);
            voltage = clamp(voltage,*omin,*omax);
            uint32_t dac_voltage = int_map(clamp(voltage,-10,10),-10.0,10.0,0.0,65535.0);
            snprintf(ResultValue,256,"%6.2fv",voltage);

            
            

            //}
            IDX+=1;
            if(IDX>200){
                IDX=0;
            }
            adc1arr[IDX] = (float)((int)res*256);
            adc2arr[IDX] = voltage;
            *ms = *next_time;
            //timer.start();
            results = timer.get_elapsed_ns();
            end = results+((double)(*ms)*1.0);

            bool sucess = false; 
            while(!sucess){
                sucess = write_pin(spi,*pin,(int)(dac_voltage));
            };
        }
    }
}

std::vector<double> split_args(char* args){
    std::vector <double> values;
    char delim[] = " ";
    char *ptr = strtok(args, delim);
    while(ptr != NULL)
    {
        values.push_back(strtod(ptr,NULL));
        ptr = strtok(NULL, delim);   
    }
    return values;
}

// Demonstrate creating a simple console window, with scrolling, filtering, completion and history.
// For the console example, we are using a more C++ like approach of declaring a class to hold both data and functions.
struct ExampleAppConsole
{
    char                  InputBuf[256];
    ImVector<char*>       Items;
    ImVector<const char*> Commands;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;
    char                  LastCommand[256];
    char                  ResultBuf[256];
    char                  ResultValue[256];
    unsigned long long    CurrentFrame;
    int                   IDX;
    float                 adc1arr[200];
    float                 adc2arr[200];
    std::thread           Worker;
    double                TimeMs;
    double                IMin;
    double                IMax;
    double                OMin;
    double                OMax;
    char                  Cmd[256];
    int                   Pin;
    int                   Focused;
    bool                  init;
    double                LastTime;

    long long             cret;
    calculator::stacks     cs;
    calculator::operators  cb;
   //pegtl::memory_input in( argv, "input" );
   //pegtl::parse< calculator::grammar, calculator::action >( in, b, s );
   //ret = (long long)s.finish();
   //return ret;

    ExampleAppConsole()
    {
        init = true;
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos = -1;

        // "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
        Commands.push_back("HELP");
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
        Commands.push_back("CLASSIFY");
        Commands.push_back("CALC");
        Commands.push_back("TIME");
        Commands.push_back("FIT");

        AutoScroll = true;
        ScrollToBottom = false;
        AddLog("CV calc");
        ExecCommand("calc (t*5)%256");
        //ExecCommand("fit 0.0 256.0 -2.0 2.0");
        OMin =-10.0;
        OMax = 10.0;
        IMin =0.0;
        IMax = 256.0;
        TimeMs = 4000.0;
        Focused = 0;
        IDX=0;
        //Worker = std::thread(spi_task, &TimeMs, &NextTimeMs,Cmd,&Pin, ResultBuf,ResultValue,LastCommand,&CurrentFrame, adc1arr,adc2arr, &IMin,&IMax,&OMin,&OMax);
        //Worker.detach();
        //sched_param sch;
        //int policy; 
        //pthread_getschedparam(Worker.native_handle(), &policy, &sch);
        //sch.sched_priority = 0;
        //if (pthread_setschedparam(Worker.native_handle(), SCHED_FIFO, &sch)) {
         //   fprintf(stderr,"WARNING: Failed to set worker bbt thread"
         //   "to real-time priority\n");
        //  }

        
    }
    ~ExampleAppConsole()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            free(History[i]);
    }

    // Portable helpers
    static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    static char* Strdup(const char* s)                           { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    void spi_update(double results){
    
        if(results>LastTime){
            CurrentFrame+=1;

            strcpy(ResultBuf,LastCommand);
            //char time_str[256];

            char* time_str = mitoa(CurrentFrame, 10);

            char replace[2] = "t";
            //stringify(ResultBuf, replace, time_str);        
            char* result = replace_str((char*)ResultBuf, (char*)replace, (char*)time_str);
            //char* result = replace_all((const char*)ResultBuf, (const char*)replace, (const char*)time_str);

            strcpy(ResultBuf,result);
            //return;
            //uint8_t res = (uint8_t)calc(ResultBuf);
            pegtl::memory_input in( ResultBuf, "input" );
            pegtl::parse< calculator::grammar, calculator::action >( in, cb, cs );
            uint8_t res = (uint8_t)cs.finish();
            double voltage = flt_map((double)res,IMin,IMax,OMin,OMax);

            voltage = clamp(voltage,OMin,OMax);
            uint32_t dac_voltage = int_map(clamp(voltage,-10,10),-10.0,10.0,0.0,65535.0);
            snprintf(ResultValue,256,"%6.2fv",voltage);
            //write_pin(spi,Pin,(int)(dac_voltage));
            IDX+=1;
            if(IDX>200){
                IDX=0;
            }
            adc1arr[IDX] = (float)((int)res*256);
            adc2arr[IDX] = voltage;

            LastTime = results+(TimeMs*1000);

            bool sucess = false; 
            while(!sucess){
                sucess = write_pin(spi,Pin,(int)(dac_voltage));
            };
        }
    }

    void    ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            free(Items[i]);
        Items.clear();
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf)-1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    void    Draw(const char* title, bool* p_open,int x , int y, int pin)
    {
        Pin = pin;                     
        if(init){
            ImVec2 new_size = ImVec2(256,300);
            ImGui::SetNextWindowSize(new_size, ImGuiCond_Once);
        
            float ox = new_size.x * x;
            float oy = new_size.y * y;
            ImVec2 new_p = ImVec2(ox,oy);                
            ImGui::SetNextWindowPos(new_p, ImGuiCond_Once);
            init = false;
        }

        if (!ImGui::Begin(title, p_open,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize  ))
        {
            ImGui::End();
            return;
        } 
        //////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////
        if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)){
            Focused=1;
        }
        else{
            Focused=0;
        }
        //////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////
        
        
        

        //manage expression replacement with time 
        if(!Focused){
            
            ImGui::BeginChild("graph", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::PlotLines("ADC1", adc1arr, IM_ARRAYSIZE(adc1arr), 0, NULL, 0.0, 65535.0, ImVec2(256,120));
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 0.90f, 0.72f, 1.00f));
            ImGui::PlotLines("ADC2", adc2arr, IM_ARRAYSIZE(adc2arr), 0, NULL, -10.0, 10.0, ImVec2(256,120));
            ImGui::PopStyleColor();
            ImGui::EndChild();

        }

        if(Focused){
            ImGui::PlotLines("ADC1", adc1arr, IM_ARRAYSIZE(adc1arr), 0, NULL, 0.0, 65535.0, ImVec2(256,70));
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 0.90f, 0.72f, 1.00f));
            ImGui::PlotLines("ADC2", adc2arr, IM_ARRAYSIZE(adc2arr), 0, NULL, -10.0, 10.0, ImVec2(256,70));
            ImGui::PopStyleColor();

            // Reserve enough left-over height for 1 separator + 1 input text
            const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve*2), false, ImGuiWindowFlags_NoScrollbar);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

            for (int i = 0; i < Items.Size; i++)
            {
                const char* item = Items[i];
                if (!Filter.PassFilter(item))
                    continue;

                // Normally you would store more information in your item than just a string.
                // (e.g. make Items[] an array of structure, store color/type etc.)
                ImVec4 color;
                bool has_color = false;
                if (strstr(item, "[error]"))          { ; has_color = true; }
                else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
                else if (strncmp(item, "! ", 2) == 0) { color = ImVec4(0.0f, 0.9f, 0.9f, 1.0f); has_color = true; }
                if (has_color)
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(item);
                if (has_color)
                    ImGui::PopStyleColor();
            }


            if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
                ImGui::SetScrollHereY(1.0f);
            ScrollToBottom = false;
            ImGui::PopStyleVar();
            ImGui::EndChild();
            ImGui::Separator();

            // Command-line
            bool reclaim_focus = false;
            ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;        

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0,0.9f,.9f,1.0f));
            if (ImGui::InputText("##Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
            {
                char* s = InputBuf;
                Strtrim(s);
                if (s[0])
                    ExecCommand(s);
                strcpy(s, "");
                reclaim_focus = true;
            }
            ImGui::PopStyleColor();
            if(Focused){
                ImGui::SetKeyboardFocusHere(-1);    
            }

            // Demonstrate keeping auto focus on the input box    
            if (reclaim_focus)
                ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

            ImGui::Text("Expanded: %s", ResultBuf); 
            ImGui::Text("Value: %s | Time: %llu", ResultValue, CurrentFrame);
        }
        ImGui::End();

    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back.
        // This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = History.Size - 1; i >= 0; i--)
            if (Stricmp(History[i], command_line) == 0)
            {
                free(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(Strdup(command_line));

        // Process command
        if (Stricmp(command_line, "CLEAR") == 0)
        {
            ClearLog();
        }
        else if (Stricmp(command_line, "HELP") == 0)
        {
            AddLog("Commands:");
            for (int i = 0; i < Commands.Size; i++)
                AddLog("- %s", Commands[i]);
        }
        else if (Stricmp(command_line, "HISTORY") == 0)
        {
            int first = History.Size - 10;
            for (int i = first > 0 ? first : 0; i < History.Size; i++)
                AddLog("%3d: %s\n", i, History[i]);
        }
        else if (stristr4(command_line, "RESET") != NULL)
        {
            //CurrentFrame=0;
            reset = true;
            AddLog("! reset frame counter to zero");
        }
        else if (stristr4(command_line, "TIME") != NULL)
        {
            TimeMs = strtod(command_line+5,NULL);
            AddLog("! set new time constant %s", command_line+5);
        }
        else if (stristr4(command_line, "FIT") != NULL)
        {
            std::vector<double> split = split_args((char*)command_line+4);
            IMin = split[0];
            IMax = split[1];
            OMin = split[2];
            OMax = split[3];
            AddLog("! fit range %f %f >> %f %f", IMin, IMax, OMin,OMax);
        }
        else if (stristr4(command_line, "CALC") != NULL)
        {
            strcpy(LastCommand,command_line+5);
            AddLog("! set new expr %s", command_line+5);
        }
        else
        {
            AddLog("Unknown command: '%s'\n", command_line);
        }

        // On command input, we scroll to bottom even if AutoScroll==false
        ScrollToBottom = true;
    }

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        ExampleAppConsole* console = (ExampleAppConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiInputTextCallbackData* data)
    {
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
        case ImGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION
                return 0;

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates;
                for (int i = 0; i < Commands.Size; i++)
                    if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.Size == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
                }
                else if (candidates.Size == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can..
                    // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
        case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = History.Size - 1;
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= History.Size)
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
        }
        return 0;
    }
};

