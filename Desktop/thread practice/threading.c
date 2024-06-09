#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define POUT 23
#define PIN 24

#define PWM 0


static int GPIOExport(int pin) {
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

static int GPIOUnexport(int pin) {
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }
  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

static int GPIODirection(int pin, int dir) {
  static const char s_directions_str[] = "in\0out";
  char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio direction for writing!\n");
    return (-1);
  }

  if (-1 ==
      write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
    fprintf(stderr, "Failed to set direction!\n");
    return (-1);
  }

  close(fd);
  return (0);
}

static int GPIORead(int pin) {
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return (-1);
  }

  if (-1 == read(fd, value_str, 3)) {
    fprintf(stderr, "Failed to read value!\n");
    return (-1);
  }

  close(fd);

  return (atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
  static const char s_values_str[] = "01";
  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return (-1);
  }

  if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
    fprintf(stderr, "Failed to write value!\n");
    return (-1);

    close(fd);
    return (0);
  }
}

//초음파
void *t_ultra(void *data) {
  pid_t pid;
  pthread_t tid;

  pid = getpid();
  tid = pthread_self();

  char *thread_name = (char *)data;
  int i = 0;
// 실행부
  int repeat = 9;
  clock_t start_t, end_t;
  double time;

  // Enable GPIO pins
  if (-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN)) {
    printf("gpio export err\n");
    return (1);
  }
  // wait for writing to export file
  usleep(100000);

  // Set GPIO directions
  if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PIN, IN)) {
    printf("gpio direction err\n");
    return (2);
  }

  // init ultrawave trigger
  GPIOWrite(POUT, 0);
  usleep(10000);
  // start
  do {
    if (-1 == GPIOWrite(POUT, 1)) {
      printf("gpio write/trigger err\n");
      return (3);
    }

    // 1sec == 1000000ultra_sec, 1ms = 1000ultra_sec
    usleep(10);
    GPIOWrite(POUT, 0);

    while (GPIORead(PIN) == 0) {
      start_t = clock();
    }
    while (GPIORead(PIN) == 1) {
      end_t = clock();
    }

    time = (double)(end_t - start_t) / CLOCKS_PER_SEC;  // ms
    printf("time : %.4lf\n", time);
    printf("distance : %.2lfcm\n", time / 2 * 34000);

    usleep(2000000);
  } while (repeat--);

  // Disable GPIO pins
  if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN)) return (4);

  printf("complete\n");

}
//led
void *t_led(void *data) {
  pid_t pid;
  pthread_t tid;

  pid = getpid();
  tid = pthread_self();

  char *thread_name = (char *)data;
  int i = 0;
//실행부
  while (i < 3) {
    printf("[%s] pid:%u, tid:%x --- %d\n", thread_name, (unsigned int)pid,
           (unsigned int)tid, i);
    i++;
    sleep(1);
  }
}

static int PWMExport(int pwmnum) {
#define BUFFER_MAX 3
  char buffer[BUFFER_MAX];
  int fd, byte;

  // TODO: Enter the export path.
  fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for export!\n");
    return (-1);
  }

  byte = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, byte);
  close(fd);

  sleep(1);

  return (0);
}

static int PWMEnable(int pwmnum) {
  static const char s_enable_str[] = "1";

  char path[DIRECTION_MAX];
  int fd;

  // TODO: Enter the enable path.
  snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm0/enable", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in enable!\n");
    return -1;
  }

  write(fd, s_enable_str, strlen(s_enable_str));
  close(fd);

  return (0);
}

static int PWMWritePeriod(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  // TODO: Enter the period path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/period", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in period!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in period!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}

static int PWMWriteDutyCycle(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  // TODO: Enter the duty_cycle path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/duty_cycle", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in duty cycle!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in duty cycle!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}


// 스레드 두 개면 됨
int main() {
  pthread_t *p_thread;
  int thr_id;
  int status;
  char p1[] = "thread_1";
  char pM[] = "thread_m";

  thr_id = pthread_create(&p_thread, NULL, t_ultra, (void *)p1);
  if (thr_id < 0) {
    perror("thread create error : ");
    exit(0);
  }

  t_function((void *)pM);
  pthread_join(p_thread[0], (void **)&status);

  return 0;
}

