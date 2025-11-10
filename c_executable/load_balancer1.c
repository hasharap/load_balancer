#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>

#define MAX_LINE 100

#define I_LOW 7
#define I_MODERATE 15

float max_current = 20;

// Base folder for tmp files
#define TMP_FOLDER "/tmp/load_balancer"
#define FILE_READ  TMP_FOLDER "/charger_state"   // charger side write (mcu.js)
#define FILE_WRITE TMP_FOLDER "/control_current" // charger side read  (mcu.js)
#define FILE_LB_STATE TMP_FOLDER "/load_balancer_state"  // availability of the LB

// If no serial data for X seconds, set state to 0
#define SERIAL_TIMEOUT 6   // seconds

time_t last_serial_time = 0;

void write_file(const char *path, const char *data) {
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s", data);
        fflush(f);  // Force write to disk
        fclose(f);
        printf("Written to %s: %s", path, data);
    } else {
        fprintf(stderr, "Error writing to %s: %s\n", path, strerror(errno));
    }
}

void read_file(const char *path, char *buf, size_t size) {
    FILE *f = fopen(path, "r");
    if (f) {
        if (fgets(buf, size, f) == NULL) {
            buf[0] = '\0';
        }
        fclose(f);
    } else {
        // Don't print error for non-existent files during startup
        buf[0] = '\0';
    }
}

int setup_serial(const char *portname) {
    int fd = open(portname, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "Error opening serial port %s: %s\n", portname, strerror(errno));
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    // Get current attributes
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error getting serial attributes: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

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
        fprintf(stderr, "Error setting serial attributes: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    // Clear any existing data
    tcflush(fd, TCIOFLUSH);

    return fd;
}

void handle_serial_line(const char *line) {
    char content[64] = "";  // Initialize buffer
    char lb_state[8] = "";  // Initialize buffer
    
    // Remove any trailing whitespace/newlines from input
    char clean_line[MAX_LINE];
    strncpy(clean_line, line, sizeof(clean_line) - 1);
    clean_line[sizeof(clean_line) - 1] = '\0';
    
    // Remove trailing whitespace
    int len = strlen(clean_line);
    while (len > 0 && (clean_line[len-1] == ' ' || clean_line[len-1] == '\t' || 
                       clean_line[len-1] == '\r' || clean_line[len-1] == '\n')) {
        clean_line[--len] = '\0';
    }

    printf("Received Data: '%s'\n", clean_line);
    
    if (strcmp(clean_line, "U") == 0) { // U for Undetected
        snprintf(lb_state, sizeof(lb_state), "0\n");
        snprintf(content, sizeof(content), "0\n");
        printf("Load balancer undetected\n");
    }
    else if (strcmp(clean_line, "L") == 0) { // Low current
        snprintf(content, sizeof(content), "%.2f\n", I_LOW);
        snprintf(lb_state, sizeof(lb_state), "1\n");
        printf("Low current mode: %.2f A\n", I_LOW);
    }
    else if (strcmp(clean_line, "M") == 0) { // Moderate current
        snprintf(content, sizeof(content), "%.2f\n", I_MODERATE);
        snprintf(lb_state, sizeof(lb_state), "1\n");
        printf("Moderate current mode: %.2f A\n", I_MODERATE);
    }
    else {
        // Try to parse as integer
        char *endptr;
        long val = strtol(clean_line, &endptr, 10);
        
        // Check if conversion was successful
        if (*endptr == '\0' && endptr != clean_line) {
            int control_current = max_current - val;
            if (control_current < 0) control_current = 0;  // Prevent negative current
            
            snprintf(content, sizeof(content), "%.2f\n", control_current);
            snprintf(lb_state, sizeof(lb_state), "1\n");
            printf("Control current: %.2f A (max: %.2f, load: %ld)\n", 
                   control_current, max_current, val);
        } else {
            // Invalid input - mark as available but don't change current
            snprintf(lb_state, sizeof(lb_state), "1\n");
            printf("Invalid input received: '%s'\n", clean_line);
            return; // Don't write files for invalid input
        }
    }
    
    // Write files
    write_file(FILE_WRITE, content);
    write_file(FILE_LB_STATE, lb_state);

    // Update last received serial time
    last_serial_time = time(NULL);
}

int main() {
    printf("Starting load balancer service...\n");
    
    // Create the /tmp/load_balancer folder if it doesn't exist
    if (mkdir(TMP_FOLDER, 0777) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error creating directory %s: %s\n", TMP_FOLDER, strerror(errno));
        return 1;
    }
    
    printf("Created/verified directory: %s\n", TMP_FOLDER);

    int serial_fd = setup_serial("/dev/ttyS2");
    if (serial_fd < 0) {
        // If serial cannot be opened, mark LB as unavailable
        printf("Serial port cannot be opened, marking LB as unavailable\n");
        write_file(FILE_LB_STATE, "0\n");
        return 1;
    }

    printf("Serial port opened successfully\n");

    // Initialize files , 
    char init_current[4];
    char init_state[4];

    snprintf(init_current, sizeof(init_current), "15\n");
    write_file(FILE_WRITE, "0\n");
    write_file(FILE_LB_STATE, "0\n"); // Initially mark as unavailable until first data

    char serial_buf[MAX_LINE];
    int serial_index = 0;
    memset(serial_buf, 0, sizeof(serial_buf));

    last_serial_time = time(NULL);
    printf("Starting main loop...\n");

    while (1) {
        // Step 1: Check file and write to serial
        char state[64];
        read_file(FILE_READ, state, sizeof(state));
        
        // Remove newlines/carriage returns
        state[strcspn(state, "\r\n")] = '\0';
        
        if (strlen(state) > 0) {
            if (strcmp(state, "STATE_C2") == 0) {
                char msgc[] = "c\n";
                if (write(serial_fd, msgc, strlen(msgc)) < 0) {
                    fprintf(stderr, "Error writing to serial: %s\n", strerror(errno));
                }
                printf("Sent to serial: STATE_C2 -> 'c'\n");
            }
            else if (strcmp(state, "STATE_A1") == 0) {
                char msgi[] = "i\n";
                if (write(serial_fd, msgi, strlen(msgi)) < 0) {
                    fprintf(stderr, "Error writing to serial: %s\n", strerror(errno));
                }
                printf("Sent to serial: STATE_A1 -> 'i'\n");
            }
        }
        
        // Step 2: Check for incoming serial data with timeout
        fd_set readfds;
        struct timeval timeout = {0, 200000}; // 200ms

        FD_ZERO(&readfds);
        FD_SET(serial_fd, &readfds);

        int select_result = select(serial_fd + 1, &readfds, NULL, NULL, &timeout);
        if (select_result > 0 && FD_ISSET(serial_fd, &readfds)) {
            char ch;
            ssize_t bytes_read = read(serial_fd, &ch, 1);
            if (bytes_read > 0) {
                if (ch == '\n' || ch == '\r') {
                    if (serial_index > 0) {  // Only process if we have data
                        serial_buf[serial_index] = '\0';
                        handle_serial_line(serial_buf);
                        serial_index = 0;
                        memset(serial_buf, 0, sizeof(serial_buf));
                    }
                } else if (serial_index < MAX_LINE - 1) {
                    serial_buf[serial_index++] = ch;
                }
            } else if (bytes_read < 0) {
                fprintf(stderr, "Error reading from serial: %s\n", strerror(errno));
            }
        } else if (select_result < 0) {
            fprintf(stderr, "Select error: %s\n", strerror(errno));
        }

        // Step 3: Check timeout (no data for too long)
        time_t current_time = time(NULL);
        if (current_time - last_serial_time > SERIAL_TIMEOUT) {
            write_file(FILE_LB_STATE, "0\n");
            printf("Serial communication timeout - marking LB as unavailable\n");
            // Reset the timer to avoid spamming this message
            last_serial_time = current_time - SERIAL_TIMEOUT + 1;
        }

        usleep(100000); // 100ms sleep instead of 1 second for better responsiveness
    }

    close(serial_fd);
    return 0;
}