#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "button.h"

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

#define PIN 20
#define POUT2 21
#define VALUE_MAX 40
#define DIRECTION_MAX 40

static int GPIOExport(int pin)
{
#define BUFFER_MAX 3
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open export for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

static int GPIOUnexport(int pin)
{
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

static int GPIODirection(int pin, int dir)
{
  static const char s_directions_str[] = "in\0out";
  char path[DIRECTION_MAX];
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
  fd = open(path, O_WRONLY);
  if (fd == -1)
  {
    fprintf(stderr, "Failed to open gpio direction for writing! Pin: %d, Error: %s\n", pin, strerror(errno));
    return -1;
  }

  if (write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3) == -1)
  {
    fprintf(stderr, "Failed to set direction! Pin: %d, Direction: %s, Error: %s\n", pin, (IN == dir) ? "in" : "out", strerror(errno));
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

static int GPIORead(int pin)
{
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return (-1);
  }

  if (-1 == read(fd, value_str, 3))
  {
    fprintf(stderr, "Failed to read value!\n");
    return (-1);
  }

  close(fd);

  return (atoi(value_str));
}

static int GPIOWrite(int pin, int value)
{
  static const char s_values_str[] = "01";

  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return (-1);
  }

  if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
  {
    fprintf(stderr, "Failed to write value!\n");
    return (-1);
  }

  close(fd);
  return (0);
}

// 쓰레드 cancel시 호출될 함수
void dispose(void *arg)
{
  // 할당한 자원 해제
  BUTTON *button = (BUTTON *)arg;
  GPIOUnexport(button->pin);
  free(button);
}
// 폴링으로 버튼의 이벤트를 처리하는 함수
void *event_routine(void *arg)
{
  BUTTON *button = (BUTTON *)arg;
  clock_t start, end;
  int isPressed = 0;
  int longPressTriggered = 0;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_cleanup_push(dispose, arg);

  while (1)
  {
    GPIOWrite(button->pout, 1);

    int pinState = GPIORead(button->pin);
    // printf("%d", pinState);
    if (pinState == 0)
    {
      if (!isPressed)
      {
        isPressed = 1;
        longPressTriggered = 0;
        start = clock(); // 버튼을 누른 순간의 시간 측정
        button->onPressDown();
      }
      else
      {
        end = clock();                                                    // 버튼을 뗀 순간의 시간 측정
        double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000; // ms 단위로 변환
        if (elapsed >= 800 && !longPressTriggered)
        {
          longPressTriggered = 1;
          button->onLongClick();
        }
      }
    }
    else
    {
      if (isPressed)
      {
        isPressed = 0;
        button->onPressUp();
      }
    }
    usleep(1000000 / button->polling_rate); // 폴링 주기 설정
  }

  pthread_cleanup_pop(0);
  return NULL;
}
pthread_t *initButton(BUTTON button)
{
  // 필요한 자원 할당
  // thread create
  pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
  BUTTON *button_copy = (BUTTON *)malloc(sizeof(BUTTON));
  if (button_copy == NULL || thread == NULL)
  {
    free(thread);
    free(button_copy);
    return NULL;
  }
  *button_copy = button;
  if (GPIOExport(button_copy->pin) == -1 || GPIOExport(button_copy->pout) == -1)
  {
    free(thread);
    free(button_copy);
    return NULL;
  }
  if (GPIODirection(button_copy->pin, IN) == -1 || GPIODirection(button_copy->pout, OUT) == -1)
  {
    GPIOUnexport(button_copy->pin);
    GPIOUnexport(button_copy->pout);
    free(thread);
    free(button_copy);
    return NULL;
  }
  if (pthread_create(thread, NULL, event_routine, button_copy))
  {
    GPIOUnexport(button_copy->pin);
    GPIOUnexport(button_copy->pout);
    free(thread);
    free(button_copy);
    return NULL;
  }
  return thread;
}
