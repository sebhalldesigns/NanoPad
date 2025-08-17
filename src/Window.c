#include <Window.xml.h>

#include <stdio.h>
#include <string.h>

#if _WIN32
    #include <windows.h>
    #include <shobjidl.h> // Header for IFileDialog
#elif __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>
#endif

char* currentFile = NULL;
size_t currentFileSize = 0;

nkLabel_t* labels = NULL;
size_t labelCount = 0;

extern Window_t generatedWindow;

void OpenFileButtonClick(nkButton_t *button)
{

    
    /* check for labels */
    if (labels != NULL) {
        for (size_t i = 0; i < labelCount; i++) {
            nkView_RemoveChildView(&generatedWindow.FileStackPanel.view, &labels[i].view);
        }

        free(labels);
        labels = NULL;
        labelCount = 0;
    }

    #if _WIN32
    printf("Open file clicked\n");

    // 1. Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        printf("Failed to initialize COM.\n");
        return;
    }

    // 2. Create an IFileOpenDialog object
    IFileOpenDialog *pFileOpen;
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL, &IID_IFileOpenDialog, (void**)&pFileOpen);

    if (SUCCEEDED(hr)) {
        // 3. Configure the dialog (optional but good practice)
        COMDLG_FILTERSPEC rgSpec[] = {
            {L"Text Files", L"*.txt;*.log"},
            {L"All Files", L"*.*"}
        };
        pFileOpen->lpVtbl->SetFileTypes(pFileOpen, ARRAYSIZE(rgSpec), rgSpec);
        pFileOpen->lpVtbl->SetFileTypeIndex(pFileOpen, 2);
        pFileOpen->lpVtbl->SetTitle(pFileOpen, L"Select a File");

        // 4. Show the dialog and get the result
        hr = pFileOpen->lpVtbl->Show(pFileOpen, NULL);

        if (SUCCEEDED(hr)) {
            IShellItem *pItem;
            hr = pFileOpen->lpVtbl->GetResult(pFileOpen, &pItem);

            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath;
                hr = pItem->lpVtbl->GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr)) {
                    // 5. Convert UTF-16 (Windows native) -> UTF-8 for printing
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    char *utf8 = malloc(size_needed);

                    if (utf8) {

                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, utf8, size_needed, NULL, NULL);
                        printf("Selected file (UTF-8): %s\n", utf8);

                        /* now measure the file size and load it into memory */
                        FILE *file = fopen(utf8, "rb");
                        if (file) {
                            fseek(file, 0, SEEK_END);
                            currentFileSize = ftell(file);
                            fseek(file, 0, SEEK_SET);
                            currentFile = malloc(currentFileSize + 1); // +1 for null terminator
                            
                            if (currentFile) {
                                fread(currentFile, 1, currentFileSize, file);
                                currentFile[currentFileSize] = '\0'; // null-terminate the string
                                printf("File loaded successfully, size: %zu bytes\n", currentFileSize);
                            }

                            fclose(file);
                        }

                        free(utf8);
                    }
                    
                    CoTaskMemFree(pszFilePath); // Free the string from GetDisplayName
                }
                pItem->lpVtbl->Release(pItem); // 6. Release COM objects
            }
        }
        pFileOpen->lpVtbl->Release(pFileOpen); // 6. Release COM objects
    }
    CoUninitialize(); // 7. Uninitialize COM

    printf("Open file dialog completed.\n");
    printf("Current file: %s\n", currentFile ? currentFile : "No file selected");

    if (currentFile) 
    {
        // Make a copy of the file content for counting lines
        char *fileContentCopy = strdup(currentFile);
        if (!fileContentCopy) {
            printf("Failed to allocate memory for file content copy.\n");
            return;
        }

        size_t lineCount = 0;
        char *line = strtok(fileContentCopy, "\n");
        while (line != NULL) 
        {
            lineCount++;
            line = strtok(NULL, "\n");
        }

        free(fileContentCopy); // Free the copy used for counting

        printf("File contains %zu lines.\n", lineCount);

        labels = malloc(lineCount * sizeof(nkLabel_t));
        if (labels)
        {
            labelCount = lineCount;
            size_t index = 0;
            line = strtok(currentFile, "\n");
            while (line != NULL && index < labelCount) 
            {
                nkLabel_t *label = &labels[index++];
                nkLabel_Create(label);
                label->text = strdup(line); // Duplicate the line text
                label->foreground = NK_COLOR_WHITE; // Set default foreground color
                label->background = NK_COLOR_BLACK; // Set default background color
                label->padding = nkThickness_FromConstant(5.0f); // Set padding
                line = strtok(NULL, "\n");

                nkView_AddChildView(&generatedWindow.FileStackPanel.view, &label->view);
                printf("Label created for line: %s\n", label->text);
            }   
            printf("Labels created for each line in the file.\n");
        }

        nkWindow_LayoutViews(&generatedWindow.super);
        nkWindow_RequestRedraw(&generatedWindow.super);
    }
    #elif __EMSCRIPTEN__

    printf("Open file clicked (Web version)\n");

    // The script creates a hidden <input type="file"> element,
    // simulates a click on it, and then sets up an event listener
    // to handle the selected file.
    emscripten_run_script(R"(
        var input = document.createElement('input');
        input.type = 'file';
        input.style.display = 'none';
        document.body.appendChild(input);

        input.onchange = function(event) {
            var file = event.target.files[0];
            if (file) {
                var reader = new FileReader();
                reader.onload = function(e) {
                    var fileData = new Uint8Array(e.target.result);
                    var fileName = file.name;

                    // Pass the data back to C++
                    // We need to pass the file data and name to a C function
                    // using Emscripten's C-to-JS and JS-to-C bridging.
                    
                    // Allocate memory in Emscripten's heap
                    var fileNamePtr = Module._malloc(fileName.length + 1);
                    Module.stringToUTF8(fileName, fileNamePtr, fileName.length + 1);

                    var fileDataPtr = Module._malloc(fileData.length);
                    HEAPU8.set(fileData, fileDataPtr);

                    // Call the C++ function
                    Module._AsyncWebFileCallback(fileNamePtr, fileDataPtr, fileData.length);

                    // Free the allocated memory
                    Module._free(fileNamePtr);
                    Module._free(fileDataPtr);
                };
                reader.readAsArrayBuffer(file);
            }
        };
        input.click();
        document.body.removeChild(input);
    )");

    #endif
}


void AsyncWebFileCallback(const char* fileName, const char* fileData, int fileSize) {
    printf("Selected file: %s\n", fileName);
    printf("File size: %d bytes\n", fileSize);

    if (currentFile) {
        free(currentFile); // Free previous file data if any
    }

    currentFile = malloc(fileSize + 1); // +1 for null terminator
    if (currentFile) {
        memcpy(currentFile, fileData, fileSize);    
        currentFile[fileSize] = '\0'; // null-terminate the string
        currentFileSize = fileSize;
        printf("File loaded successfully, size: %zu bytes\n", fileSize);
        
    }

    // Make a copy of the file content for counting lines
    char *fileContentCopy = strdup(currentFile);
    if (!fileContentCopy) {
        printf("Failed to allocate memory for file content copy.\n");
        return;
    }

    size_t lineCount = 0;
    char *line = strtok(fileContentCopy, "\n");
    while (line != NULL) 
    {
        lineCount++;
        line = strtok(NULL, "\n");
    }

    free(fileContentCopy); // Free the copy used for counting

    printf("File contains %zu lines.\n", lineCount);

    labels = malloc(lineCount * sizeof(nkLabel_t));
    if (labels)
    {
        labelCount = lineCount;
        size_t index = 0;
        line = strtok(currentFile, "\n");
        while (line != NULL && index < labelCount) 
        {
            nkLabel_t *label = &labels[index++];
            nkLabel_Create(label);
            label->text = strdup(line); // Duplicate the line text
            label->foreground = NK_COLOR_WHITE; // Set default foreground color
            label->background = NK_COLOR_BLACK; // Set default background color
            label->padding = nkThickness_FromConstant(5.0f); // Set padding
            line = strtok(NULL, "\n");

            nkView_AddChildView(&generatedWindow.FileStackPanel.view, &label->view);
            printf("Label created for line: %s\n", label->text);
        }   
        printf("Labels created for each line in the file.\n");
    }

    nkWindow_LayoutViews(&generatedWindow.super);
    nkWindow_RequestRedraw(&generatedWindow.super);

}
