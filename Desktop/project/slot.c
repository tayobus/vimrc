#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MSG_SIZE 256

// 전역 flag
volatile char isPresent = 0;
volatile char prevIsPresent = 0;
char timer = 0;

// 서버 정보
struct sockaddr_in serv_addr;

struct record
{
    char RFID[14];
    time_t start, end;
} record_instance;

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

char *readRFID()
{
    static char uid[14] = {0};

    // 파이프 생성
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("파이프 생성 오류");
        return NULL;
    }

    // 자식 프로세스 생성
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("프로세스 생성 오류");
        return NULL;
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
        ssize_t bytesRead = read(pipefd[0], uid, sizeof(uid) - 1); // 마지막에 NULL 추가를 위해 -1
        if (bytesRead == -1)
        {
            perror("읽기 오류");
            return NULL;
        }

        // NULL 종단 추가
        uid[bytesRead] = '\0';

        // 결과 출력
        printf("파이썬 스크립트의 출력: %s\n", uid);

        // 파이프 닫기
        close(pipefd[0]);
    }

    return uid;
}

// 소켓 만들고 connect
int sendRecord(struct sockaddr_in serv_addr)
{
    int sock;
    char msg[MSG_SIZE];
    char on[2] = "1";
    int str_len;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("Connection established\n");

    // 메세지 타입
    write(sock, "RECORD", MSG_SIZE);

    // record 전송
    read(sock, msg, MSG_SIZE);
    if (strcmp(msg, "OK") == 0)
        write(sock, &record_instance, sizeof(struct record));

    close(sock);

    return 0;
}

// RFID태그가 인식되는지 지속적으로 점검
void *t_RFID_handler(void *arg)
{
    while (1)
    {
        char *RFID = readRFID();
        prevIsPresent = isPresent;
        if (RFID != NULL && strlen(RFID) > 0)
        {
            isPresent = 1;
            printf("RFID: %s\n", RFID);
        }
        else
        {
            isPresent = 0;
            printf("notPresent\n");
        }

        if (prevIsPresent == 0 && isPresent == 1)
        {
            strncpy(record_instance.RFID, RFID, sizeof(record_instance.RFID) - 1);
            record_instance.start = time(NULL);
            record_instance.end = 0;
        }
        else if (prevIsPresent == 1 && isPresent == 0)
        {
            record_instance.end = time(NULL);
            sendRecord(serv_addr);
        }

        sleep(1); // 조금 대기하여 루프가 너무 빠르게 돌지 않도록 함
    }
    return NULL;
}

// LCD 출력
void *t_screen_handler(void *arg)
{
    time_t elapsed_time = 0;
    while (1)
    {
        if (prevIsPresent == 1 && isPresent == 1)
        {
            elapsed_time = time(NULL) - record_instance.start;
            printf("사용 경과 시간: %ld\n", elapsed_time);
        }
        else
        {
            printf("Screen Handler: No active RFID\n");
        }
        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    printf("started\n");
    if (argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    pthread_t t_screen;
    pthread_t t_RFID;

    if (pthread_create(&t_screen, NULL, t_screen_handler, NULL) != 0)
    {
        perror("Failed to create t_screen thread");
        exit(1);
    }
    if (pthread_create(&t_RFID, NULL, t_RFID_handler, NULL) != 0)
    {
        perror("Failed to create t_RFID thread");
        exit(1);
    }

    pthread_join(t_screen, NULL);
    pthread_join(t_RFID, NULL);

    return 0;
}
