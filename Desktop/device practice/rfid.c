#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define CS_PIN 10          // Chip Select pin
#define RST_PIN 6          // Reset pin
#define SPI_CHANNEL 0      // SPI channel
#define SPI_SPEED 13560000 // 13.56 MHz

// 상태 코드 정의
#define MI_OK 0
#define MI_NOTAGERR 1
#define MI_ERR 2

// RC522 명령어 정의
#define PCD_IDLE 0x00
#define PCD_AUTHENT 0x0E
#define PCD_RECEIVE 0x08
#define PCD_TRANSMIT 0x04
#define PCD_TRANSCEIVE 0x0C
#define PCD_RESETPHASE 0x0F
#define PCD_CALCCRC 0x03

// RC522 레지스터 정의
#define CommandReg 0x01
#define CommIEnReg 0x02
#define DivIEnReg 0x03
#define CommIrqReg 0x04
#define DivIrqReg 0x05
#define ErrorReg 0x06
#define Status1Reg 0x07
#define Status2Reg 0x08
#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define ControlReg 0x0C
#define BitFramingReg 0x0D
#define CollReg 0x0E
#define ModeReg 0x11
#define TxControlReg 0x14
#define TxAutoReg 0x15
#define TModeReg 0x2A
#define TPrescalerReg 0x2B
#define TReloadRegL 0x2D
#define TReloadRegH 0x2C
#define VersionReg 0x37

#define MAX_LEN 16 // 최대 버퍼 길이 정의

// 함수 프로토타입 선언
uint8_t communicateWithRC522(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint8_t *backLen);
void writeToRC522(uint8_t reg, uint8_t value);
uint8_t readFromRC522(uint8_t reg);
void initRC522();
void antennaOn();
void resetRC522();
uint8_t requestTag(uint8_t *bufferATQA);
uint8_t anticollision(uint8_t *bufferUID);

// 비트 마스크를 클리어하는 함수
void clearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t value = readFromRC522(reg);
    writeToRC522(reg, value & (~mask));
}

// 비트 마스크를 설정하는 함수
void setBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t value = readFromRC522(reg);
    writeToRC522(reg, value | mask);
}

// RC522에 데이터를 쓰는 함수
void writeToRC522(uint8_t reg, uint8_t value)
{
    digitalWrite(CS_PIN, LOW);                 // CS 핀을 LOW로 설정하여 통신 시작
    wiringPiSPIDataRW(SPI_CHANNEL, &reg, 1);   // 레지스터 주소 전송
    wiringPiSPIDataRW(SPI_CHANNEL, &value, 1); // 데이터 전송
    digitalWrite(CS_PIN, HIGH);                // CS 핀을 HIGH로 설정하여 통신 종료
}

// RC522에서 데이터를 읽는 함수
uint8_t readFromRC522(uint8_t reg)
{
    uint8_t data;

    reg |= 0x80;                              // 레지스터 주소에 읽기 비트 설정
    digitalWrite(CS_PIN, LOW);                // CS 핀을 LOW로 설정하여 통신 시작
    wiringPiSPIDataRW(SPI_CHANNEL, &reg, 1);  // 레지스터 주소 전송
    wiringPiSPIDataRW(SPI_CHANNEL, &data, 1); // 데이터 수신
    digitalWrite(CS_PIN, HIGH);               // CS 핀을 HIGH로 설정하여 통신 종료

    return data;
}

// RC522와 통신하는 함수
uint8_t communicateWithRC522(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint8_t *backLen)
{
    uint8_t status = MI_ERR; // 기본적으로 오류로 초기화

    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;

    switch (command)
    {
    case PCD_AUTHENT:
        irqEn = 0x12;
        waitIRq = 0x10;
        break;
    case PCD_TRANSCEIVE:
        irqEn = 0x77;
        waitIRq = 0x30;
        break;
    default:
        break;
    }

    writeToRC522(CommIEnReg, irqEn | 0x80); // 중단 요청 활성화
    clearBitMask(CommIrqReg, 0x80);         // 중단 요청 라인 클리어
    setBitMask(FIFOLevelReg, 0x80);         // FIFO 버퍼 플러시

    writeToRC522(CommandReg, PCD_IDLE); // 초기화

    // FIFO 버퍼에 데이터 쓰기
    for (int i = 0; i < sendLen; i++)
    {
        writeToRC522(FIFODataReg, sendData[i]);
    }

    // 명령 전송 시작
    writeToRC522(CommandReg, command);
    if (command == PCD_TRANSCEIVE)
    {
        setBitMask(BitFramingReg, 0x80); // StartSend=1, transmission of data starts
    }

    // 중단 요청 대기
    int i = 2000;
    do
    {
        n = readFromRC522(CommIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    clearBitMask(BitFramingReg, 0x80); // StartSend=0

    if (i != 0)
    {
        if (!(readFromRC522(ErrorReg) & 0x1B))
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {
                status = MI_NOTAGERR;
            }

            if (command == PCD_TRANSCEIVE)
            {
                n = readFromRC522(FIFOLevelReg);
                lastBits = readFromRC522(ControlReg) & 0x07;
                if (lastBits)
                {
                    *backLen = (n - 1) * 8 + lastBits;
                }
                else
                {
                    *backLen = n * 8;
                }

                if (n == 0)
                {
                    n = 1;
                }
                if (n > MAX_LEN)
                {
                    n = MAX_LEN;
                }

                for (int i = 0; i < n; i++)
                {
                    backData[i] = readFromRC522(FIFODataReg);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
    }

    return status;
}

// resetRC522 함수 추가
void resetRC522()
{
    digitalWrite(RST_PIN, LOW);
    delay(50);
    digitalWrite(RST_PIN, HIGH);
    delay(50);
}

// initRC522 함수 추가
void initRC522()
{
    writeToRC522(CommandReg, PCD_RESETPHASE);
    writeToRC522(TModeReg, 0x8D);
    writeToRC522(TPrescalerReg, 0x3E);
    writeToRC522(TReloadRegL, 30);
    writeToRC522(TReloadRegH, 0);
    writeToRC522(TxAutoReg, 0x40);
    writeToRC522(ModeReg, 0x3D);
    writeToRC522(TxControlReg, 0x83);
}

// antennaOn 함수 추가
void antennaOn()
{
    uint8_t temp = readFromRC522(TxControlReg);
    if ((temp & 0x03) != 0x03)
    {
        writeToRC522(TxControlReg, temp | 0x03);
    }
}

// requestTag 함수 추가
uint8_t requestTag(uint8_t *bufferATQA)
{
    writeToRC522(BitFramingReg, 0x07);
    bufferATQA[0] = 0x26; // Request command for ISO14443A
    uint8_t backLen = 2;
    uint8_t result = communicateWithRC522(PCD_TRANSCEIVE, bufferATQA, 1, bufferATQA, &backLen);
    if (result != MI_OK || backLen != 2)
    {
        return MI_ERR;
    }
    return MI_OK;
}

// anticollision 함수 추가
uint8_t anticollision(uint8_t *bufferUID)
{
    writeToRC522(BitFramingReg, 0x00);
    bufferUID[0] = 0x93; // Anti-collision command
    bufferUID[1] = 0x20;
    uint8_t backLen = 5;
    uint8_t result = communicateWithRC522(PCD_TRANSCEIVE, bufferUID, 2, bufferUID, &backLen);
    if (result != MI_OK)
    {
        return MI_ERR;
    }
    return MI_OK;
}

// RC522가 연결되어 있는지 확인하는 함수
int isRC522Connected()
{
    if (wiringPiSetup() == -1)
    {
        printf("WiringPi setup failed.\n");
        return 0;
    }

    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1)
    {
        printf("SPI setup failed.\n");
        return 0;
    }

    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, HIGH);

    resetRC522();
    initRC522();
    antennaOn();

    // RC522 초기화 후 상태 레지스터의 값 확인
    uint8_t version = readFromRC522(VersionReg);
    if (version == 0x92 || version == 0x91)
    {
        printf("RC522 connected (Version: 0x%02X).\n", version);
        return 1;
    }
    else
    {
        printf("RC522 not connected (Version: 0x%02X).\n", version);
        return 0;
    }
}

// 나머지 코드는 이전에 작성한 코드와 동일하게 유지됩니다.

int main()
{
    if (!isRC522Connected())
    {
        printf("RC522 not connected. Exiting...\n");
        return 1;
    }

    // RC522가 연결되어 있다면 이후 코드 실행
    while (1)
    {
        printf("Scanning for RFID tags...\n");

        uint8_t bufferATQA[2];
        if (requestTag(bufferATQA) == MI_OK)
        {
            printf("Card detected.\n");
            uint8_t bufferUID[5];
            if (anticollision(bufferUID) == MI_OK)
            {
                printf("Card UID: %02X %02X %02X %02X\n", bufferUID[0], bufferUID[1], bufferUID[2], bufferUID[3]);
            }
            else
            {
                printf("Collision detected or no UID received.\n");
            }
        }
        else
        {
            printf("No card detected.\n");
        }

        delay(1000);
    }

    return 0;
}
