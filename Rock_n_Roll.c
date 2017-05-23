#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define DIRE "/dev/hbridge"
#define CLOCKID CLOCK_REALTIME
#define SIG SIGUSR1
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

int setPWM(char ch, int value);
int setupPWM();
int command(char *com, int pwm);
void stop();
bool issafe();
static void handler(int sig, siginfo_t *si, void *uc);
int timer();

// global variables
timer_t timerid;
int counter = 1, sum_front_left = 0, sum_front_right = 0;

int main() {
  setupPWM();
  // "X011" means counter clockwise
  // Further functions concerning movement to be implemented in Lab5
  command("A101", 1);
  command("B101", 1);
  timer();
  // Stop the tank before the end of the program. To be changed in Lab5
  stop();
  return 0;
}

// Checks if the distance is safe
bool issafe(int sum) {
  if ((sum/2) > 1700) return false;
  return true;
}

// Stops the wheels
void stop() {
  command("A001", 1);
  command("B001", 1);
}

// Function that uses the kernel module to open the devfile and write to it
// Ultimately, the driver will take care of translating the command given
int command(char *com, int pwm) {
  int fd = open(DIRE, O_RDWR);
  if (fd < 0) {
    printf("File %s cannot be opened\n", DIRE);
    exit(1);
  }
  write(fd, com, sizeof(com + 1));
  setPWM(com[0], pwm);
  close(fd);
  return 0;
}

// Enables or disables the PWM
int setPWM(char ch, int value) {
    if (ch == 'A') {
    FILE *fd = fopen("/sys/devices/ocp.3/pwm_test_P9_14.13/run", "w");
    if (fd == NULL) return EXIT_FAILURE;
    fprintf(fd, "%d", value);
    fclose (fd);
  }
  else if (ch == 'B') {
    FILE *fd = fopen("/sys/devices/ocp.3/pwm_test_P8_19.12/run", "w");
    if (fd == NULL) return EXIT_FAILURE;
    fprintf(fd, "%d", value);
    fclose (fd);
  }
  return 0;
}

// Configure PWM pins
int setupPWM() {
  char path[35] = "/sys/devices/bone_capemgr.9/slots";
  FILE *fd = fopen(path, "w");
  if (fd == NULL) {
    fprintf(stderr, "Could not open file: %s\n", path);
    return EXIT_FAILURE;
  }
  fprintf(fd, "%s", "am33xx_pwm");
  fclose(fd);

  // ChannelB : EHRPWM2A
  fd = fopen(path, "w");
  if (fd == NULL) {
    fprintf(stderr, "Could not open file: %s\n", path);
    return EXIT_FAILURE;
  }
  fprintf(fd, "%s", "bone_pwm_P8_19");
  fclose (fd);

  // ChannelA : EHRPWM1A
  fd = fopen(path, "w");
  if (fd == NULL) {
    fprintf(stderr, "Could not open file: %s\n", path);
    return EXIT_FAILURE;
  }
  fprintf(fd, "%s", "bone_pwm_P9_14");
  fclose (fd);
  return 0;
}

// Timer interrupt handler: after 300 executions, it ignores the signal
// Reads from the APC and gives the average dynamically
static void handler(int sig, siginfo_t *si, void *uc) {
  int data;
  FILE *fd = fopen("/sys/devices/ocp.3/helper.12/AIN6", "r");
  if (fd == NULL) exit(1);
  fscanf(fd, "%d", &data);
  fclose(fd);
  sum_front_left += data;
  fd = fopen("/sys/devices/ocp.3/helper.12/AIN2", "r");
  if (fd == NULL) exit(1);
  fscanf(fd, "%d", &data);
  fclose(fd);
  sum_front_right += data;
  printf("Averages:\t\tLeft distance: %d\tRight distance: %d\n", 
           sum_front_left/counter, sum_front_right/counter);
  if(counter++ == 300) signal(sig, SIG_IGN);
}

// Timer interrupt
int timer() {
  timer_t timerid;
  struct sigevent sev;
  struct itimerspec its;
  long long freq_nanosecs;
  sigset_t mask;
  struct sigaction sa;

  /* Establish handler for timer signal */
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIG, &sa, NULL) == -1)
  errExit("sigaction");

  /* Block timer signal temporarily */
  sigemptyset(&mask);
  sigaddset(&mask, SIG);
  if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
  errExit("sigprocmask");

  /* Create the timer */
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIG;
  sev.sigev_value.sival_ptr = &timerid;
  int timer_value = timer_create(CLOCKID, &sev, &timerid);
  if (timer_value == -1)
    errExit("timer_create");

  /* Start the timer */
  freq_nanosecs = 300;
  its.it_value.tv_sec = freq_nanosecs / 1000000000;
  its.it_value.tv_nsec = freq_nanosecs % 1000000000;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  if (timer_settime(timerid, 0, &its, NULL) == -1)
  errExit("timer_settime");

  /* Unlock the timer signal, so that timer notification can be delivered */
  if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    errExit("sigprocmask");

  return EXIT_SUCCESS;
}

