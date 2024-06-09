#include <stdio.h>
#include <stdlib.h>

int main()
{

        // Python 실행 파일을 호출하고 결과를 읽어옴
    FILE *fp;
    char path[1035];

    // "rfid_reader" 실행 파일을 호출하는 명령어
    fp = popen("./rfid_reader", "r");
    if (fp == NULL)
    {
        printf("Failed to run command\n");
        exit(1);
    }

    // 실행 결과를 읽어옴
    if (fgets(path, sizeof(path) - 1, fp) != NULL)
    {
        printf("RFID ID: %s", path);
    }

    // 프로세스 종료
    pclose(fp);

    return 0;
}
