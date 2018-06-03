/*************************************************************************
	> File Name: test.c
	> Author: 
	> Mail: 
	> Created Time: Thu 19 Apr 2018 04:19:49 PM CST
 ************************************************************************/

#include<stdio.h>
int main()
{
    FILE *fd = fopen("Makefile","r+");

    int myfd_id = fileno(fd);
    printf("fd:%d\n",myfd_id);
    
    return 0;
}
