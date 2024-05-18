#ifndef MY_FUNCTIONS_H
#define MY_FUNCTIONS_H


#include <stdio.h>
#include <stack>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstring>
#define ImageBufferSize 1474560
#define RootEntrySize 32
#define RootEntryNum 224
#define FATBeginIndex 512
#define rootEntryBeginIndex 9728
#define dataBeginIndex 16896
#define ClusterSize 512
#define MaxDirEntryNumPerCluster 16
struct RootEntry
{
    char DIR_Name[11];
    __uint8_t DIR_Attr; // 文件属性
    char reserve[10];
    __uint16_t DIR_WrtTime; // 最后一次写入时间
    __uint16_t DIR_WrtDate; // 最后一次写入日期
    __uint16_t DIR_FstClus; // 开始簇号
    __uint32_t DIR_FileSize;
};

enum class inputType
{
    LS,
    CAT,
    EXIT
};

struct InputInfo
{
    inputType type;
    std::string fileName;
    bool isVerbose;
};

struct DirectAndFileRes
{
    int dirNum;
    int fileNum;
};

// 用于存储镜像的缓冲区
char ImageBuffer[ImageBufferSize];

// 栈，用来暂存目录
std::stack<RootEntry> getRootStack;
std::stack<RootEntry> lsStack;

// 栈，用来保存递归关系
// push的时候，会push当前rootEntryStack栈顶Entry的子目录数量，当这个栈的栈顶为0的时候，就说明Entry的子目录已经全部访问完毕（Path也就不再需要这个dirName了，就subPath（EntryName.top））
std::stack<int> sonDirNumStack;

// 栈，用来保存压进去的entry的名称，用于后续subPath操作
std::stack<std::string> rootEntryNameStack;

// 字符串，用来存储当前访问的目录是哪一个
std::string dirPath;

extern "C"
{
    void my_print(const char *, const int);
}

void InitializeImageBuffer();
RootEntry InitializeRootEntry(char buffer[32]);
RootEntry InitializeRootEntry(int startIndex, int endIndex);
InputInfo AnalyzingInput(std::string input);
std::string GetInput();
RootEntry GetRoot(std::string dirName);
void ls(RootEntry rootEntry, bool isVerbose);
int GetNextCluster(int curCluster);
void ClearStack(std::stack<RootEntry> stack);
std::vector<std::string> stringSplit(const std::string &str, char delim);
bool isLegalL(std::string str);
int GetBeginIndexByRootEntry(RootEntry rootEntry);
void addPath(std::string dirName);
void Print(std::string msg);
void subPath(std::string dirName);
bool isParentOrCurDir(RootEntry rootEntry);
std::string getDirname(RootEntry rootEntry);
DirectAndFileRes getDirectSonDirAndFileNum(RootEntry rootEntry);
void PrintDirOrFile(RootEntry rootEntry, bool isVerbose);
void cat(RootEntry rootEntry);
int min(int a,int b);
// 真正的根
RootEntry root = {
    {'/', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    0x10,      // DIR_Attr 初始化为目录属性
    {0},       // reserve 初始化为全零
    0x0000,    // DIR_WrtTime 初始化为最后一次写入时间
    0x0000,    // DIR_WrtDate 初始化为最后一次写入日期
    0x0000,    // 0簇特殊標記，代表是根目錄，getIndex的時候特判就好了
    0x00000000 // DIR_FileSize 初始化为文件大小
};

#endif