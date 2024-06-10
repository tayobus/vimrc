#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// #include <sys/types.h>

int main()
{
    // 파이프 생성
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("파이프 생성 오류");
        return 1;
    }

    // 자식 프로세스 생성
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("프로세스 생성 오류");
        return 1;
    }
    else if (pid == 0)
    { // 자식 프로세스
        // 파이프의 쓰기 파일 디스크립터를 표준 출력으로 복제
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); // 읽기 파일 디스크립터 닫기

        // 파이썬 스크립트 실행
        execlp("python3", "python3", "rfid_reader.py", NULL);
        perror("exec 오류");
        exit(EXIT_FAILURE);
    }
    else
    {                     // 부모 프로세스
        close(pipefd[1]); // 쓰기 파일 디스크립터 닫기

        // 파이프로부터 결과 읽기
        char buffer[1024];
        ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer));
        if (bytesRead == -1)
        {
            perror("읽기 오류");
            return 1;
        }

        // 결과 출력
        printf("파이썬 스크립트의 출력: %.*s\n", (int)bytesRead, buffer);

        // 파이프 닫기
        close(pipefd[0]);

        return 0;
    }
}
