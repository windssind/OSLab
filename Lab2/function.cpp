#include "function.h"
void InitializeImageBuffer()
{
    //
    FILE *file;

    // 打开镜像文件
    file = fopen("a.img", "rb");
    if (file == NULL)
    {
        printf("无法打开文件。\n");
        return;
    }

    // 从文件中读取内容到缓冲区
    size_t bytesRead = fread(ImageBuffer, sizeof(unsigned char), ImageBufferSize, file);
    if (bytesRead < ImageBufferSize)
    {
        perror("未能读取1440个字符!");
    }

    // 处理读取的数据
    // 在这里可以对缓冲区中的数据进行处理或其他操作

    // 关闭文件
    fclose(file);
}

/// @brief 对于给定的[startIndex,endIndex)，获取这个范围内的RootEntry返回
/// @param startIndex
/// @param endIndex
/// @return
RootEntry InitializeRootEntry(int startIndex, int endIndex)
{
    char rootEntryCharArray[32];
    std::copy(ImageBuffer + startIndex, ImageBuffer + endIndex, rootEntryCharArray);
    return InitializeRootEntry(rootEntryCharArray);
}
RootEntry InitializeRootEntry(char buffer[32])
{
    RootEntry *rootEntry = new RootEntry();
    std::memcpy(rootEntry, buffer, 32);
    return *rootEntry;
}

InputInfo AnalyzingInput(std::string input)
{
    InputInfo inputInfo;
    inputInfo.isVerbose = false;
    std::vector<std::string> argv = stringSplit(input, ' ');
    if (argv.at(0) == "")
    {
        std::cout << "请输入文件名!" << std::endl;
        exit(0);
    }
    else if (argv[0] == "cat")
    {
        if (argv.size() > 2)
        {
            std::cout << "cat: too many arguments" << std::endl;
            exit(0);
        }
        else if (argv.size() == 1)
        {
            std::cout << "cat: missing file operand" << std::endl;
            exit(0);
        }
        inputInfo.type = inputType::CAT;
        inputInfo.fileName = argv[1];
    }
    else if (argv[0] == "ls")
    {
        inputInfo.type = inputType::LS;
        inputInfo.fileName = "/"; // 默认为主目录
        bool hasFileName = false; // 用于判断是否已经有文件名输入了
        for (int i = 1; i < argv.size(); i++)
        {
            if (argv[i][0] == '-')
            {
                if (!isLegalL(argv[i]))
                {
                    std::cout << "ls: invalid option -- '" << argv[i][1] << "'" << std::endl;
                    exit(0);
                }
                else
                {
                    inputInfo.isVerbose = true;
                }
            }
            else
            {
                inputInfo.fileName = argv[i];
                if (hasFileName)
                {
                    std::cout << "ls: cannot access '" << argv[i] << "': Too many arguments" << std::endl;
                    exit(0);
                }
                hasFileName = true;
            }
        }
    }
    else if (argv[0] == "exit")
    {
        inputInfo.type = inputType::EXIT;
    }
    return inputInfo;
}

// 网上复制的split方法
std::vector<std::string> stringSplit(const std::string &str, char delim)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delim))
    {
        if (token != "")
            tokens.push_back(token);
    }

    return tokens;
}

bool isLegalL(std::string str)
{
    if (str.length() < 2)
    {
        return false;
    }

    if (str[0] != '-')
    {
        return false;
    }
    size_t count = str.length();

    for (size_t i = 1; i < count; i++)
    {
        if (str[i] != 'l')
        {
            return false;
        }
    }
    return true;
}

std::string GetInput()
{
    std::string input;
    std::getline(std::cin, input);
    return input;
}

// 根据给定的文件名返回对应的目录项
RootEntry GetRoot(std::string dirName)
{
    if (dirName == "/")
    {
        return root;
    }
    ClearStack(getRootStack);
    std::vector<std::string> dirNames = stringSplit(dirName, '/');
    // 初始的时候栈为空，直接遍历一遍根目录并且压栈
    RootEntry rootEntry;
    RootEntry tmpEntry;
    std::stack<RootEntry> preEntrys;
    preEntrys.push(root);
    int beginIndex = rootEntryBeginIndex; // 记录了从哪个index开始搜
    addPath("/");
    for (int i = 0; i < dirNames.size(); ++i)
    {
        if (dirNames[i] == "..")
        {
            if (!preEntrys.empty()){
                subPath(getDirname(tmpEntry));
                tmpEntry = preEntrys.top();
                preEntrys.pop();
            }
            continue;
        }
        else if (dirNames[i] == ".")
        {
            continue;
        }
        for (int j = 0; j < RootEntryNum; ++j)
        {
            tmpEntry = InitializeRootEntry(beginIndex + j * RootEntrySize, beginIndex + (j + 1) * RootEntrySize);
            if (getDirname(tmpEntry) == dirNames[i])
            {
                if (i != dirNames.size() - 1)
                {
                
                    addPath(getDirname(tmpEntry));
                    preEntrys.push(tmpEntry);
                    beginIndex = GetBeginIndexByRootEntry(tmpEntry);
                }
                break;
            }
            if (j == RootEntryNum - 1)
            {
                std::cout << "No such directory!" << std::endl;
                exit(0);
            }
        }
    }
    return tmpEntry;
}

// 通过给出的目录项得到目录项在数据区的起始位置
int GetBeginIndexByRootEntry(RootEntry rootEntry)
{
    if (rootEntry.DIR_FstClus == 0X0000)
    {
        return rootEntryBeginIndex;
    }
    else
    {
        // 一个cluster占用512
        return dataBeginIndex + (rootEntry.DIR_FstClus - 2) * 512;
    }
}

/// @brief 通过给定的Cluster号得到下一个Cluster号,如果是坏簇或者最后一个簇，就返回-1
/// @return
int GetNextCluster(int curCluster)
{
    int cluster;
    int clusterGroup = curCluster / 2;
    int beginIndex = clusterGroup * 3;
    if (curCluster %2 ==1){
        cluster = (ImageBuffer[FATBeginIndex + beginIndex +2]) * 16 + (((ImageBuffer[ FATBeginIndex +  beginIndex + 1]) &0XF0 )>> 4); 
    }else {
        cluster = (ImageBuffer[FATBeginIndex + beginIndex]) + (ImageBuffer[FATBeginIndex + beginIndex + 1]) &0X0F; 
    }
    if (cluster >= 0XFF7)
    {
        return -1;
    }
    return cluster;
}

// 核心函数，用于展示文件和目录的信息
void ls(RootEntry rootEntry, bool isVerbose)
{
    if (rootEntry.DIR_Attr != 0X10)
    {
        std::cout << "ls: cannot access '" << getDirname(rootEntry) << "': Not a directory" << std::endl;
        exit(0);
    }
    ClearStack(lsStack);
    // 初始化栈
    int beginIndex;
    lsStack.push(rootEntry);
    sonDirNumStack.push(getDirectSonDirAndFileNum(rootEntry).dirNum);
    // 到这一步已经完全遍历了根目录了
    while (!lsStack.empty())
    {
        rootEntry = lsStack.top();
        lsStack.pop();
        beginIndex = GetBeginIndexByRootEntry(rootEntry); // 这个是弹出的目录项的起始byte号
        // 用于后续的
        addPath(getDirname(rootEntry));
        rootEntryNameStack.push(getDirname(rootEntry));
        Print(dirPath);
        if (isVerbose)
            Print(" " + std::to_string(getDirectSonDirAndFileNum(rootEntry).dirNum) + " " + std::to_string(getDirectSonDirAndFileNum(rootEntry).fileNum));
        int dirNum = 0;
        int fileNum = 0;
        Print(":\n");
        // 这里还需要借助FAT表获得全部内容
        for (int i = 0; i < MaxDirEntryNumPerCluster; ++i)
        {
            RootEntry tmpEntry = InitializeRootEntry(beginIndex + i * RootEntrySize, beginIndex + (i + 1) * RootEntrySize);
            if (tmpEntry.DIR_Attr == 0x10)
            {
                if (!isParentOrCurDir(tmpEntry))
                {
                    dirNum++;
                    lsStack.push(tmpEntry);
                }
                PrintDirOrFile(tmpEntry, isVerbose);
            }
            else if (tmpEntry.DIR_Attr == 0X20)
            {
                fileNum++;
                PrintDirOrFile(tmpEntry, isVerbose);
            }
            else
            {
                break;
            }

            if (i == MaxDirEntryNumPerCluster - 1)
            {
                int nextCluster;
                if (nextCluster = GetNextCluster(rootEntry.DIR_FstClus) != -1)
                {
                    beginIndex = dataBeginIndex + nextCluster * ClusterSize;
                    i = 0; // 进行重置
                }
            }
        }
        if (!isVerbose)
            Print("\n");
        // 遍历结束，将栈顶的数字-1
        int leftDirNum = sonDirNumStack.top() - 1;
        sonDirNumStack.pop();
        sonDirNumStack.push(leftDirNum);

        // 进入下一步递归,如果为0,就说明不需要遍历子目录，不需要压栈
        if (dirNum != 0)
            sonDirNumStack.push(dirNum);

        while (!sonDirNumStack.empty() && sonDirNumStack.top() == 0)
        {
            sonDirNumStack.pop();
            subPath(std::string(rootEntryNameStack.top()));
            rootEntryNameStack.pop();
        }
    }
}

void ClearStack(std::stack<RootEntry> stack)
{
    while (!stack.empty())
    {
        stack.pop();
    }
}

void addPath(std::string dirName)
{
    if (dirName == "/")
    {
        dirPath = std::string("/");
        return;
    }
    dirPath = dirPath + dirName + std::string("/");
}

void subPath(std::string dirName)
{
    dirPath = dirPath.substr(0, dirPath.length() - dirName.length() - 1);
}

void Print(std::string msg)
{
    my_print(msg.c_str(), msg.length());
}

bool isParentOrCurDir(RootEntry rootEntry)
{
    return (getDirname(rootEntry) == "." || getDirname(rootEntry) == "..");
}

std::string getDirname(RootEntry rootEntry)
{
    std::string res = "";
    for (int i = 0; i < 8; i++)
    {
        if (rootEntry.DIR_Name[i] == ' ')
        {
            continue;
        }
        res += rootEntry.DIR_Name[i];
    }
    if (rootEntry.DIR_Attr == 0x20)
    {
        res += ".";
        for (int i = 8; i < 11; i++)
        {
            if (rootEntry.DIR_Name[i] == ' ')
            {
                continue;
            }
            res += rootEntry.DIR_Name[i];
        }
    }
    return res;
}

DirectAndFileRes getDirectSonDirAndFileNum(RootEntry rootEntry)
{
    DirectAndFileRes res = {0, 0};
    int beginIndex = GetBeginIndexByRootEntry(rootEntry);
    for (int i = 0; i <= MaxDirEntryNumPerCluster; i++)
    {
        RootEntry tmpEntry = InitializeRootEntry(beginIndex + i * RootEntrySize, beginIndex + (i + 1) * RootEntrySize);
        if (tmpEntry.DIR_Attr == 0x10)
        {
            if (!isParentOrCurDir(tmpEntry))
                res.dirNum++;
        }
        else if (tmpEntry.DIR_Attr == 0x20)
        {
            res.fileNum++;
        }
        else
        {
            break;
        }
        if (i == MaxDirEntryNumPerCluster - 1)
        {
            int nextCluster;
            if (nextCluster = GetNextCluster(rootEntry.DIR_FstClus) != -1)
            {
                beginIndex = dataBeginIndex + nextCluster * ClusterSize;
                i = 0; // 进行重置
            }
        }
    }
    return res;
}

void PrintDirOrFile(RootEntry rootEntry, bool isVerbose)
{
    if (rootEntry.DIR_Attr == 0X10)
    {
        Print("\033[31m" + getDirname(rootEntry) + "  " + "\033[0m" + (isVerbose ? (std::to_string(getDirectSonDirAndFileNum(rootEntry).dirNum) + " " + std::to_string(getDirectSonDirAndFileNum(rootEntry).fileNum)) : "") + (isVerbose ? "\n" : " "));
    }
    else
    {
        Print(getDirname(rootEntry) + "  " + (isVerbose ? std::to_string(rootEntry.DIR_FileSize) : "") + " " + (isVerbose ? "\n" : " "));
    }
}


void cat(RootEntry rootEntry){
    if (rootEntry.DIR_Attr != 0x20){
        std::cout<<"cat: "<<getDirname(rootEntry)<<" is not a file"<<std::endl;
    }

    int beginIndex = GetBeginIndexByRootEntry(rootEntry);

    int clusterNum = (rootEntry.DIR_FileSize / ClusterSize) + 1;

    int curCluster = rootEntry.DIR_FstClus;

    for (int i = 0; i <clusterNum;++i){
        Print(std::string(ImageBuffer + beginIndex,min(ClusterSize,rootEntry.DIR_FileSize - i * ClusterSize)));
        curCluster = GetNextCluster(curCluster);
        beginIndex = dataBeginIndex + (curCluster - 2) * ClusterSize;
    }

}

int min(int a,int b){
    return a<b?a:b;
}