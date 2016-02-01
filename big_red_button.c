/*
,* Based on original code by Malcolm Sparks <malcolm@congreve.com> provided at http://blog.opensensors.io/blog/2013/11/25/the-big-red-button/
,*
,* A program to convert USB firing events from the Dream Cheeky 'Big Red Button' to MQTT events.
,*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/utsname.h>

#define LID_CLOSED 21
#define BUTTON_PRESSED 22
#define BUTTON_PRESSED2 30
#define BUTTON_NOT_PRESSED2 31
#define LID_OPEN 23

#define MAXRCVLEN 500
#define PORTNUM 53

typedef enum { false, true } bool;

struct utsname unameData;

// bool connectionLive() {
//   struct hostent *he;

//   if ((he = gethostbyname("google.com")) == NULL) {  // get the host info
//       return false;
//   }

//   return true;
// }

// void setConnectionStatusLight(bool hasConnection) {
//   if (hasConnection) {
//         system("sudo /usr/local/bin/blink1-tool -g --rgb 0,255,0 &>/dev/null");
//   } else {
// 	system("sudo /usr/local/bin/blink1-tool -g --rgb 255,0,0 &>/dev/null");
//   }
// }

int main(int argc, char **argv)
{
  char command_activate[1024];
  char command_deactivate[1024];
  char command_launch[1024];
  int fd;
  int i, res, desc_size = 0;
  char buf[256];
  time_t last_time = time(0);
  // time_t last_connection_check = time(0);
  bool fired = false;
  // bool connection = connectionLive();  
  
  // setConnectionStatusLight(connection);

  uname(&unameData);
  sprintf((char*)command_activate, "mosquitto_pub -h localhost -t 'button/action' -m '{\"type\": \"activate\", \"id\":\"%s\"}'", unameData.nodename);
  sprintf((char*)command_launch, "mosquitto_pub -h localhost -t 'button/action' -m '{\"type\": \"launch\", \"id\":\"%s\"}'", unameData.nodename);
  sprintf((char*)command_deactivate, "mosquitto_pub -h localhost -t 'button/action' -m '{\"type\": \"deactivate\", \"id\":\"%s\"}'", unameData.nodename);

  while (true) {
    /* Use a udev rule to make this device */
    while (true) {
      fd = open("/dev/big_red_button", O_RDWR|O_NONBLOCK);

      if (fd < 0) {
        perror("Unable to open device");
        perror("waiting 5s");
        sleep(5);
      }
      else {
        break;
      }
    }

    // system("amixer sset PCM,0 90% > /dev/null");

    int prior = LID_CLOSED;

    while (true) {
      // if (time(0) - last_connection_check > 5) {
      //   connection = connectionLive();
      //   last_connection_check = time(0);
      //   // setConnectionStatusLight(connection);
      // }

      // if (connection) {
        memset(buf, 0x0, sizeof(buf));
        buf[0] = 0x08;
        buf[7] = 0x02;

        res = write(fd, buf, 8);
        if (res < 0) {
          perror("write");
          break;
        }

        memset(buf, 0x0, sizeof(buf));
        res = read(fd, buf, 8);
        if (res >= 0) {
          if (!fired && (buf[0] == LID_OPEN) && (time(0) - last_time > 2)){
            // system("omxplayer /usr/local/lib/big_red_button/danger.wav >/dev/null");
            last_time = time(0);
          }

          if (prior == LID_CLOSED && buf[0] == LID_OPEN) {
           /*printf("Ready to fire!\n");
           fflush(stdout);*/
            system (command_activate);
          } else if (!fired && ((prior != BUTTON_PRESSED && buf[0] == BUTTON_PRESSED) || (prior == BUTTON_NOT_PRESSED2 && buf[0] == BUTTON_PRESSED2))) {
          /*printf("Fire!\n");
          fflush(stdout);*/
            fired = true;
            // system("killall omxplayer > /dev/null");
            // system("omxplayer /usr/local/lib/big_red_button/countdown.mp4 > /dev/null");
            system (command_launch);
            //system("curl -s 'https://zapier.com/hooks/catch/okfq8r/' >/dev/null");
            /*system("espeak -ven+f3 -k5 '10. 9. 8. 7. 6. 5. 4. 3. 2. 1.'");*/
          } else if ((prior != LID_CLOSED && buf[0] == LID_CLOSED) || (prior == BUTTON_PRESSED2 && buf[0] == BUTTON_NOT_PRESSED2)) {
          /*printf("Stand down!\n");
          fflush(stdout);*/
            fired = false;
            if (buf[0] == LID_CLOSED) {
              system (command_deactivate);
            }
          }
          prior = buf[0];
        }
      // }
      usleep(20000); /* Sleep for 20ms*/
    }
  }
}
