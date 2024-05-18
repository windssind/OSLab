#include<stdio.h>
int fun(int a[][5]){
    
}
int main(){
    int a = 126;
    int *a_p = &a;
    int a_p_1 = a_p;
    printf("%c\n",*((int*)(a_p_1 - 1)));
    
}