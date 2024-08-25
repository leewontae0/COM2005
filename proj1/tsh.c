/*
 * Copyright(c) 2023-2024 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80             /* 명령어의 최대 길이 */

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다. 
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */
static void cmdexec(char *cmd)
{
    char *argv[MAX_LINE/2+1];                   /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;                               /* 인자의 개수 */
    char *p, *q;                                /* 명령어를 파싱하기 위한 변수 */
    int idx_l = -1, idx_r = -1, idx_p = -1;     /* 파싱된 인자 내 기호 '<', '>', '|' 의 인덱스를 저장하기 위한 변수 */
    char temp[MAX_LINE];                        /* 함수 cmdexec의 인자를 저장하기 위한 변수 */
    strcpy(temp, cmd);

    /*
     * 명령어 앞부분 공백문자를 제거하고 인자를 하나씩 꺼내서 argv에 차례로 저장한다.
     * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
     */
    p = cmd; p += strspn(p, " \t");
    do {
        /*
         * 공백문자, 큰 따옴표, 작은 따옴표가 있는지 검사한다.
         */ 
        q = strpbrk(p, " \t\'\"");
        /*
         * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
         */
        if (q == NULL || *q == ' ' || *q == '\t') {
            q = strsep(&p, " \t");
            if (*q) argv[argc++] = q;
        }
        /*
         * 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else if (*q == '\'') {
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
        }
        /*
         * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else {
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
        }

    } while (p);
    
    argv[argc] = NULL;

    /*
     * 파싱된 인자들 중에 기호 '|', '<', '>'와 일치하는 것이 있으면, 인덱스를 저장한다.
     */
    for (int i = 0; i < argc; i++) {

        if (strcmp(argv[i], "|") == 0) {
            idx_p = i;
            break; /* 파이프가 있다면, 반복문에서 나간다. */
        }

        if (strcmp(argv[i], "<") == 0) {
            idx_l = i;
        }

        if (strcmp(argv[i], ">") == 0) {
            idx_r = i;
        }
    }


    /*
     * 기호 '|'가 있으면, 파이프 기능을 수행한다.
     * 파이프 기능을 수행하기 위해, 부모와 자식 프로세스를 생성한다.
     * 각 프로세스는 파이프를 통해 연결된다.
     * 자식 프로세스는 기호 '|' 이전의 명령어를 처리하고, 부모 프로세스는 기호 '|' 이후의 명령어를 처리한다.
     * 분할된 각 명령어는 재귀를 통해 처리한다.
     * 파이프를 통해 자식 프로세스의 출력을 부모 프로세스의 입력으로 향하게 한다.
     */

    if (idx_p >= 0)
    {

        int idxOfPipe = 0; /* 함수 cmdexec의 인자 내, 기호 '|'의 인덱스를 저장하기 위한 변수 */

        /*
         * 함수 cmdexec의 인자를 차례로 돌며, 기호 '|'의 인덱스를 저장한다.
         */
        for (int i = 0; i < strlen(temp); i++) {

            if (temp[i] == '|') {
                idxOfPipe = i;
                break; /* 기호 '|'가 있다면, 반복문에서 나간다. */
            }

        }

        int fd[2]; /* 파이프를 할당하기 위한 변수 */

        if (pipe(fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
         
        pid_t pid;
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);

        /*
         * 자식 프로세스
         * 기호 '|' 이전의 명령어를 파싱해서 실행한다.
         */
        } else if (pid == 0) {

            close(fd[0]);

            dup2(fd[1], STDOUT_FILENO); /* 표준 출력이 파이프를 향하게 한다. */

            char command_1[MAX_LINE]; /* 기호 '|' 이전의 명령어를 저장하기 위한 배열 */
            
            strncpy(command_1, temp, idxOfPipe); /* 함수 cmdexec의 인자 내에서, 기호 '|' 이전부분만 파싱한다. */

            command_1[idxOfPipe] = '\0';
            
            cmdexec(command_1);
        
        /*
         * 부모 프로세스
         * 기호 '|' 이후의 명령어를 파싱해서 실행한다.
         */
        } else {

            close(fd[1]);
            
            dup2(fd[0], STDIN_FILENO); /* 표준 입력이 파이프를 향하게 한다. */

            char command_x[MAX_LINE]; /* 기호 '|' 이후의 명령어를 저장하기 위한 배열 */

            strcpy(command_x, temp + idxOfPipe + 1); /* 함수 cmdexec의 인자 내에서, 기호 '|' 이후부분만 파싱한다. */

            cmdexec(command_x);

        }

    }
            

    /*
     * 기호 '<'가 있으면, 표준 입력 리다이렉션 기능을 수행한다.
     * 입력받은 인자로 파일 디스크립터를 생성하는데, 
     * 이때 파일이 존재하지 않는다면 파일을 새로 생성한다.
     * 파일이 표준입력을 향하게 한다.
     */
    if (idx_l >= 0)
    {
        /*
         * 명령어를 실행할 때 기호 '<' 이전의 명령어만 실행한다.            
         */
        argv[idx_l] = NULL;

        /*
         * 읽을 파일을 지정한다.
         * 만약 파일이 존재하지 않는다면, 새로 생성한다.
         * 읽을 파일의 이름은 기호 '<' 이후에 있다.
         */
        int fd_in = open(argv[idx_l+1], O_RDONLY | O_CREAT, 0666);

        dup2(fd_in, STDIN_FILENO); /* 표준입력이 파일을 향하게 한다. */
        close(fd_in);
                
    }

       
    /*
     * 기호 '>'가 있으면, 표준 출력 리다이렉션 기능을 수행한다.
     * 입력받은 인자로 파일 디스크립터를 생성하는데, 
     * 이때 파일이 존재하지 않는다면 파일을 새로 생성한다.
     * 표준 출력을 파일으로 향하게 한다.
     */
    if (idx_r >= 0)
    {

        /*
         * 명령어를 실행할 때 기호 '>' 이전의 명령어만 실행한다.
         */
        argv[idx_r] = NULL; 


        /*
        * 작성할 파일을 지정한다.
        * 만약 파일이 존재하지 않는다면, 새로 생성한다.
        * 입력받을 파일의 이름은 기호 '>' 이후에 있다.
        */
        int fd_out = open(argv[idx_r+1], O_WRONLY | O_CREAT, 0666);

        dup2(fd_out, STDOUT_FILENO); /* 표준출력이 파일을 향하게 한다. */
        close(fd_out);

    }


    /*
     * argv에 저장된 명령어를 실행한다.
     */
    if (argc > 0) {
        execvp(argv[0], argv);
    }

}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void)
{
    char cmd[MAX_LINE+1];       /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    bool background;            /* 백그라운드 실행 유무 */
    
    /*
     * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
     */
    while (true) {
        /*
         * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
        /*
         * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
         */
        printf("tsh> "); fflush(stdout);
        /*
         * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
         * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
         * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
        /*
         * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
        /*
         * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
         */
        char *p = strchr(cmd, '&');
        if (p != NULL) {
            background = true;
            *p = '\0';
        }
        else
            background = false;
        /*
         * 자식 프로세스를 생성하여 입력된 명령어를 실행하게 한다.
         */
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        /*
         * 자식 프로세스는 명령어를 실행하고 종료한다.
         */
        else if (pid == 0) {
            cmdexec(cmd);
            exit(EXIT_SUCCESS);
        }
        /*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        else if (!background)
            waitpid(pid, NULL, 0);
    }
    return 0;
}
