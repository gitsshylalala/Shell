#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <myerr.h>

//文件重定向实现

void Run_ThisProgress(char *argv[],int argc,int IsNeedMF,int TheFile)
{
	if(argc != 0)
    {
        //让子进程去执行shell指令
		if(fork() == 0){
			//有 '>' 文件重定向
			if(IsNeedMF == 1)
            {
				close(1);
				open(argv[TheFile],O_RDWR | O_CREAT,0644);
			}
			if(-1 == execvp(argv[0],argv))
				err_msg("Pid Exec failed!\n");
		}

		wait(NULL);
	}	
}

int Judge_Rediect(char *argv[],int argc,int IsNeedMF,int TheFile)
{
	//没有标识符'>'
	//no '>'
	if(TheFile == 0 || IsNeedMF == 0)
		argv[argc] = NULL;

	//有多个'>'标识符
	//have more than one '>'
	else if(IsNeedMF > 1){
		err_msg("write in error!\n");
		return 1;
	}
	//有'>'却无文件
	else if(IsNeedMF == 1 && TheFile == argc)
	{	
		err_msg("The file can not be nameless!\n");
		return 1;
	}
	//不止一个文件
	else if(IsNeedMF == 1 && (TheFile + 1) != argc)
	{
		int j = 0;
		for(j = TheFile + 1;j <= argc;j++){
			err_msg("The file %s is needless!\n",argv[j]);	
		}
		err_msg("Leave The Procedure!\n");
		return 1;
	}

    return 0;
}

void AlterBuf(char *buf,char *argv[],int* argc,int* IsNeedMF,int* TheFile)//需要改变，所以传指针
{
    int i = 0;
    int status = 0;//0--空格 1--字符 
	while(buf[i]!='\0')
	{
		// is '>'
		if(buf[i] == '>' && isspace(buf[i+1]) && status == 0)
		{
			if (0 == i)
				err_quit("write in error!\n");
			else{
				argv[(*argc)] = NULL;
				(*argc)++;
				*TheFile = *argc;
				(*IsNeedMF) += 1;
			}
		}
		//space->char
		else if(!isspace(buf[i])&&status == 0){
			argv[(*argc)] = &buf[i];
			status = 1;
			(*argc)++;
		}
		//is space
		else if(isspace(buf[i]))
		{
			status = 0;
			buf[i] ='\0';
		}

		i++;
	}

}

void do_parse(char* buf)
{
	int argc = 0;
	char* argv[8];
	int IsNeedMoveFile = 0;//表示发现'>'的个数
	int TheTransferFile = 0;//内容转移的位置。
	
    //修改命令行。
	//暂时只考虑一个>的情况，不考虑 >>
    //这里需要改变argc IsNeedMoveFile TheTransferFile的值，所以要传地址
    AlterBuf(buf,argv,&argc,&IsNeedMoveFile,&TheTransferFile);

    //判断'>'的特殊情况
    if(1 == Judge_Rediect(argv,argc,IsNeedMoveFile,TheTransferFile) )
        return;
	
	//开始跑程序
    Run_ThisProgress(argv ,argc ,IsNeedMoveFile ,TheTransferFile);
	
    int i = 0;
	for(i=0;i<argc;i++){
		printf("%s ",argv[i]);
	}
	printf("\n");
}

//形成一个递归，从最后一个管道标识符开始往前
//比如 ls | grep test | wc
//肯定是执行ls 结果给 grep test 再给 wc
//最后一层递归处理ls 递归出来以后ls在父进程，用于输入，此时grep在子进程，用于输出
//然后下一层递归，输出结果给了上一层递归的输入，wc是最后的输出。
void DoJob(char *buf,int tmp[],int tmp_size)
{
    if(tmp_size > 0)
    {
        int fds[2];

        if(-1 == pipe(fds))
        {
            err_msg("pipe failed!\n");
            return;
        }
        pid_t pid = fork();
        if(pid == -1)
        {
            err_msg("fork failed!\n");
            return;
        }
        else if(pid == 0)//father--send
        {
            close(fds[1]);
            close(0);
            dup2(fds[0],0);
            close(fds[0]);
            DoJob(buf,tmp,tmp_size-1);
        } 
        else if(pid > 0)//child---recv
        {
            close(fds[0]);
            close(1);
            dup2(fds[1],1);
            close(fds[1]);
            wait(NULL);
            
            do_parse(buf+tmp[tmp_size-1]);
        }
    }
    else
        do_parse(buf);
}
        
int main()
{
	char buf[1024];

	while(1){
		memset(buf,0,sizeof(buf));
		printf("[ root@localhost file ]#");
		scanf("%[^\n]",buf);
		getchar();
		if(strncmp(buf,"exit",4) == 0)
			err_quit(NULL);
		//read buf
		else{
            //我需要去识别管道，但是还要保证能处理一个管道后的管道。
            int i = 0;
            int j = 0;
            int tmp[8] = {0};
            while(buf[i] != '\0')//将命令根据管道标识符分批次
            {
                if(buf[i] == '|')
                {
                    if(i == 0)
                    {
                        err_msg("| is not right!\n");
                        break;
                    }
                    buf[i] = '\0';//管道前内容为一次指令

                    if(buf[i+1] == '|') //管道标识符下一个还是管道标识符，错误
                    {
                        err_msg("Too many '|'\n");
                        break;
                    }

                    tmp[j] = i + 1;//记录管道所在位置的下一位置
                    j++;
                }
                i++;
            } 
            DoJob(buf,tmp,j);//根据管道开始工作。
        } 
	}
	return 0;
}
