
#include "function.h"
int main(int argc, char *argv[])
{

    InitializeImageBuffer();
    while (true)
    {
        std::string input = GetInput();
        InputInfo inputInfo = AnalyzingInput(input);
        if (inputInfo.type == inputType::LS)
        {
            RootEntry rootEntry = GetRoot(inputInfo.fileName);
            ls(rootEntry, inputInfo.isVerbose);
        }else if (inputInfo.type == inputType::CAT){
            RootEntry rootEntry = GetRoot(inputInfo.fileName);
            cat(rootEntry);
        }
    }
}

