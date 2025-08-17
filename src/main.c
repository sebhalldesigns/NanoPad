#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <nanowin.h>

#include <Window.xml.h>

Window_t generatedWindow = {0}; // Static instance of Window_t

/* IMPORTANT: ON WEB, MAKE SURE NOTHING IS LOCAL TO THE MAIN FUNCTION AS IT IS TEMPORARY */
//DWORD WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
int main()
{

    Window_Create(&generatedWindow);

    while (nkWindow_PollEvents())
    {
        //nkWindow_RequestRedraw(&window); // Request a redraw for the window
    }

    return 0;
}