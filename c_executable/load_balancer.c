#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>    // for struct timeval
#include <sys/select.h>  // for fd_set, FD_ZERO, FD_SET
#include <time.h>        // ADDED: for time()
#include "cJSON.h"

#define MAX_LINE 100

//#define I_UNDETECTED 15.0
#define I_LOW 7
#define I_MODERATE 15

int max_current = 0;

// Base folder for tmp files
#define TMP_FOLDER "/tmp/load_balancer"
#define FILE_READ  TMP_FOLDER "/charger_state"   // charger side write (mcu.js)
#define FILE_WRITE TMP_FOLDER "/control_current" // charger side read  (mcu.js)
#define FILE_LB_STATE TMP_FOLDER "/load_balancer_state"  // availability of the LB

// If no serial data for X seconds, set state to 0
#define SERIAL_TIMEOUT 6   // seconds

time_t last_serial_time = 0; // ADDED: track last serial communication

void write_file(const char *path, const char *data) {
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s", data);
        fclose(f);
    } else {
        perror("write_file");
    }
}

void read_file(const char *path, char *buf, size_t size) {
    FILE *f = fopen(path, "r");
    if (f) {
        fgets(buf, size, f);
        fclose(f);
    } else {
        perror("read_file");
        buf[0] = '\0';
    }
}





int setup_serial(const char *portname) {
    int fd = open(portname, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Serial open");
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD | CS8);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_lflag = 0;
    tty.c_iflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        close(fd);
        return -1;
    }

    return fd;
}

void handle_serial_line(const char *line) {
    char content[32];
    char lb_state[4];

    

   printf("Received Data: %s\n", line);
    if (strcmp(line, "U") == 0) { // U for Undetected
        snprintf(lb_state, sizeof(lb_state), "0\n");
        printf("load_balancer_undetected..\n");
        tcflush(setup_serial("/dev/ttyS2"), TCIFLUSH);
    }
    else if (strcmp(line, "L") == 0) { // Low current
        snprintf(content, sizeof(content), "%d", I_LOW);
        snprintf(lb_state, sizeof(lb_state), "1\n");
    }
    else if (strcmp(line, "M") == 0) { // Moderate current
        snprintf(content, sizeof(content), "%d", I_MODERATE);
        snprintf(lb_state, sizeof(lb_state), "1\n");
    }
    else {
        int val = atoi(line);
        printf("val%d\n",val);
        printf("line %s\n",line);
        if (val != 0 || line[0] == '0') {
            int Control_Current = val;
            snprintf(content, sizeof(content), "%d", Control_Current);
            snprintf(lb_state, sizeof(lb_state), "1\n");
            printf("content: %s\n", content);
            printf("%d\n",Control_Current);
        } 
        else 
        {
            snprintf(lb_state, sizeof(lb_state), "1\n");
            printf("Unknown Data: %s\n", line);
            return;
        }
    }
    

    write_file(FILE_WRITE, content);
    write_file(FILE_LB_STATE, lb_state);

    // Update last received serial time
    last_serial_time = time(NULL); // ADDED
}




// Read entire file into a string
char *read_file_entire(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        perror("File open failed");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *data = (char *)malloc(len + 1);
    if (!data)
    {
        fclose(f);
        return NULL;
    }
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);
    return data;
}


int read_mcu_value_int(const char *filename, const char *key)
{
    char *json_data = read_file_entire(filename);
    if (!json_data)
        return -1;

    cJSON *root = cJSON_Parse(json_data);
    free(json_data);
    if (!root)
        return -1;

    cJSON *m1 = cJSON_GetObjectItem(root, "M1");
    cJSON *mcuConfig = cJSON_GetObjectItem(m1, "mcuConfig");
    cJSON *item = cJSON_GetObjectItem(mcuConfig, key);

    int value = -1;
    if (item && cJSON_IsString(item))
    {
        value = atoi(item->valuestring);
    }

    cJSON_Delete(root);
    return value;
}

int main() {
    // Create the /tmp/load_balancer folder if it doesn't exist
    if (mkdir(TMP_FOLDER, 0777) != 0 && errno != EEXIST) {
        perror("mkdir");
        return 1;
    }
    
    int serial_fd = setup_serial("/dev/ttyS2");
    if (serial_fd < 0) {
        // If serial cannot be opened, mark LB as unavailable
        printf("serial port cannot open\n");
        write_file(FILE_LB_STATE, "0\n"); // ADDED
        sleep(1);
        return 1;
    }

    // write_file(FILE_READ, "a\n");
    write_file(FILE_WRITE, "15\n"); // set initial current as 15
    write_file(FILE_LB_STATE, "0\n"); // Initially mark as unavailable
    
   

    char serial_buf[MAX_LINE];
    int serial_index = 0;

    max_current = read_mcu_value_int("/chargerFiles/config.json", "max_current");

    last_serial_time = time(NULL); // start tracking

    while (1) {
        // Step 1: Check file and write to serial
        
        char state[32];
        read_file(FILE_READ, state, sizeof(state));
        state[strcspn(state, "\r\n")] = 0;

        if (strcmp(state, "STATE_C2") == 0)
            write(serial_fd, "c", 1);
        else if (strcmp(state, "STATE_A1") == 0)
            write(serial_fd, "i", 1);
        
        // Step 2: Check for incoming serial data
        fd_set readfds;
        struct timeval timeout = {0, 500000}; // 200ms

        FD_ZERO(&readfds);
        FD_SET(serial_fd, &readfds);
        
        if (select(serial_fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
           
            char ch;
            if (read(serial_fd, &ch, 1) > 0) {
                if (ch == '\n') {
                    serial_buf[serial_index] = '\0';
                    handle_serial_line(serial_buf);
                    serial_index = 0;
                } else if (serial_index < MAX_LINE - 1) {
                    serial_buf[serial_index] = ch;
                    serial_index++;
                
                }
                
            }
        }
        
        // Step 3: Check timeout (no data for too long)
        if (time(NULL) - last_serial_time > SERIAL_TIMEOUT) {
            write_file(FILE_LB_STATE, "0\n"); // ADDED: mark unavailable
            printf("RAK communication doesnt work\n");
        }

        

        usleep(200000);
    }

    close(serial_fd);
    return 0;
}