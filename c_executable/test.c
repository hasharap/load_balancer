
Conversation opened. 1 unread message.

Skip to content
Using Vega Innovation Mail with screen readers
1 of 4
serial
External
Spam
buddhika ranathunga <buddhika93n@gmail.com>
	
Attachments2:50 PM (12 minutes ago)
	
to me
This message might be dangerous

Messages like this one were used to steal personal information. Don't click links, download attachments, or reply with personal information.

 2 Attachments  •  Scanned by Gmail
Downloading these attachments is disabled.

This email has been identified as phishing. If you want to download these and you trust this message, click "Not spam" in the banner above.
	

#include "serialCom.h"
#include "HAL_uart.h"
#include "mainStateMachine.h"
#include <time.h>
struct SerialStatus
{
    uint8_t serial_state;
    // uint8_t update_base_config_values;

    uint8_t send45Counter;
    uint8_t send46Counter;
    uint8_t send47Counter;
    uint8_t send48Counter;
    uint8_t verifyParameters;
    uint8_t serial_error;
    uint8_t serial_error_timeout;
    uint8_t error_occurred_count;

    struct
    {
        uint8_t error_report;
        uint8_t messege_id;
        uint8_t control_side_current_state_prev;
        uint8_t send_heart_beat;

        float voltage_L1;
        float voltage_L2;
        float voltage_L3;

        float currentL1;
        float currentL2;
        float currentL3;

        float power;
        long energy;
        uint8_t stm32_version_received_counter;
        uint8_t frequency;
        uint8_t temperature;
        uint16_t cp_value;
        uint16_t stm32_version_num;
        uint16_t uv_upper_stm32;
        uint16_t uv_lower_stm32;
        uint16_t ov_upper_stm32;
        uint16_t ov_lower_stm32;
        uint16_t freq_upper_stm32;
        uint16_t freq_lower_stm32;

        union
        {
            uint8_t all;
            struct
            {
                uint8_t current_state : 5;
                uint8_t charging_active : 1;
                uint8_t cpPWM_active : 1;
                uint8_t connector_state : 1;
            } bits;

        } controlSideState;

        union
        {
            uint8_t all;
            struct
            {
                uint8_t power_side_state : 5;
                uint8_t power_side_ready : 1;
                uint8_t contactor_state : 2;
            } bits;
        } powerSideState;

        union
        {
            uint8_t all;
            struct
            {
                uint8_t start_charge : 1;
                uint8_t stop_charge : 1;
                uint8_t schedule_charge : 1;
                uint8_t vehicle_check : 1;
                uint8_t charge_pause : 1;
                uint8_t rtc_update_complete : 1;
                uint8_t rtc_update_alarm_complete : 1;
            } bits;
        } networkSideRequestStruct;

        union
        {
            uint8_t all;
            struct
            {
                uint8_t serial_A_error : 1;
                uint8_t fault_level : 2;
                uint8_t diode_check_failed : 1;
                uint8_t CP_fault : 1;
            } bits;

        } controlSideError;

        union
        {
            uint8_t all;
            struct
            {
                uint8_t error_OV : 1;
                uint8_t error_UV : 1;
                uint8_t error_SR_C : 1;
                uint8_t error_GFI_test : 1;
                uint8_t error_Freq : 1;
                uint8_t error_OT : 1;
                uint8_t error_PL : 1;

            } bits;

        } powerSideError;

        union
        {
            uint8_t all;
            struct
            {
                uint8_t trip_OC_L1 : 1;
                uint8_t trip_GFI : 1;
            } bits;

        } tripBits;

        union
        {
            uint16_t all;
            struct
            {
                uint16_t LB : 8;
                uint16_t HB : 8;
            } bytes;
        } val1;

        union
        {
            uint16_t all;
            struct
            {
                uint16_t LB : 8;
                uint16_t HB : 8;
            } bytes;
        } val2;
        union
        {
            uint16_t all;
            struct
            {
                uint16_t LB : 8;
                uint16_t HB : 8;
            } bytes;
        } val3;

        union
        {
            uint16_t all;
            struct
            {
                uint8_t uv_en : 2; // Least significant byte (LSB)
                uint8_t ov_en : 2;
                uint8_t sc_en : 2;
                uint8_t cpf_en : 2;

                uint8_t gfi_en : 2; // Most significant byte (MSB)
                uint8_t gfit_en : 2;
                uint8_t pl_en : 2;
                uint8_t freq_en : 2;
            } bits;
        } en_1;

        union
        {
            uint16_t all; // Access the entire 16-bit value
            struct
            {
                uint8_t modbus_en : 2;
                uint8_t penf_en : 2;
                uint8_t ot_en : 2;
                uint8_t resv_2 : 2;
                uint8_t resv_3 : 2;
                uint8_t resv_4 : 2;
                uint8_t resv_5 : 2;
                uint8_t resv_6 : 2;
            } bits;
        } en_2;

    } serialReceive;
    struct
    {
        uint8_t current_state;
        uint8_t stop_charge;
        uint8_t enable_schedule_charge;
        uint8_t charger_lock_status;
        uint8_t schedule_charge_start;
        uint8_t is_internet_available;
        uint8_t messege_id;
        uint8_t maximum_current_request;

        union
        {
            uint16_t all;
            struct
            {
                uint16_t LB : 8;
                uint16_t HB : 8;
            } bytes;
        } val1;
        union
        {
            uint16_t all;
            struct
            {
                uint16_t LB : 8;
                uint16_t HB : 8;
            } bytes;
        } val2;
        union
        {
            uint16_t all;
            struct
            {
                uint16_t LB : 8;
                uint16_t HB : 8;
            } bytes;
        } val3;

    } serialSend;
};

struct SerialStatus *serial_status_new(void)
{
    return malloc(sizeof(struct SerialStatus));
}

void serial_status_free(SerialStatus **self)
{
    free((void *)(*self));
    *self = NULL;
}

void serial_status_init(SerialStatus *self)
{
    memset((void *)self, 0, sizeof(*self));
}
void *serial_thread_function(void *arg)
{
    SharedContext *args = (SharedContext *)arg;
    int fd = args->fd;
    SerialStatus *heap_serial = args->heap_serial;
    ChargerStatus *heap = args->heap;
    uint8_t error_matrix[ERROR_COUNT][2];
    struct MqttParams *mqttParams = args->mqttParams;
    time_t T0, T1;
    double tDiff;

    time(&T0);
    while (1)
    {
        time(&T1);                // Get current time
        tDiff = difftime(T1, T0); // Calculate the difference

        if (tDiff >= 0.5)
        { // Wait for 2 seconds
            // printf("2 seconds elapsed\n");
            T0 = T1;                          // Reset the timer
            pthread_mutex_lock(&args->mutex); // Lock before accessing heap_serial
            serial_com_send(fd, heap_serial, heap, mqttParams);
            pthread_mutex_unlock(&args->mutex);
            usleep(50000); // Sleep for 100ms to allow serial communication to complete
            pthread_mutex_lock(&args->mutex);
            serial_com_receive(fd, heap_serial, heap, 20, mqttParams); // Receive serial data

            setup_error_flags(heap_serial, heap, error_matrix);
            uint8_t current_error = find_highest_priority_error(error_matrix, ERROR_COUNT);
            set_error_occurred_count(heap_serial, current_error);
#ifdef DEBUG
            printf("Error Occurred Count: %d\n", get_error_occurred_count(heap_serial));
#endif
            pthread_mutex_unlock(&args->mutex); // Unlock after access

            // printf("Serial Data Updated\n");
        }
        // usleep(500000); // Sleep for 500ms
        // fflush(stdout);
    }

    return NULL;
}
void store_and_send_uart_log_pipe(uint8_t *uart_buffer, size_t length, char *type, SerialStatus *self, ChargerStatus *ChargerStatus, struct MqttParams *mqttParams) // today 2025-06-05 working before removing colors
{
    char buffer[600] = {0}; // Increased buffer size
    char temp[20];

    // Begin log line with color + label

    snprintf(buffer, sizeof(buffer), "%s", type);

    // Append hex bytes
    for (size_t i = 0; i < length; i++)
    {
        snprintf(temp, sizeof(temp), "%02x", uart_buffer[i]);
        strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
        if (i < length - 1)
        {
            strncat(buffer, ",", sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    // Append diagnostic only for "serial in"
    if (strcmp(type, "serial in: ") == 0)
    {
        char temp2[400];
        snprintf(temp2, sizeof(temp2), "\ncharger_id:%s,serial_no:%s,control_state:%d,network_state:%d,error_count:%d,wifi_strength:%d,mac_address:%s,load_balancing_current:%d,using_load_balancing:%d\n",
                 get_charger_id(ChargerStatus),
                 mqttParams->subscribed_topic,
                 self->serialReceive.controlSideState.bits.current_state,
                 get_current_state_(ChargerStatus),
                 get_error_occurred_count(self),                    // Add error count to the log
                 get_wifi_strength_level("wlan0", ChargerStatus),   // Add wifi strength to the log
                 mqttParams->mac_address,                           // Add MAC address to the logs
                 get_load_balancing_current_setting(ChargerStatus), // Add load balancing current to the log
                 get_using_load_balancing(ChargerStatus)

        );
        strncat(buffer, temp2, sizeof(buffer) - strlen(buffer) - 1);
    }

    // Final newline to separate each log entry
    strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
    // Ensure the buffer is null-terminated
    buffer[sizeof(buffer) - 1] = '\0'; // Safety null-termination
    // Print and send to log pipe
    printf("CSV: %s", buffer); // No extra newline
    send_data_to_log_pipe(buffer);
}

/*void store_and_send_uart_log_pipe(uint8_t *uart_buffer, size_t length, char *type, SerialStatus *self, ChargerStatus *ChargerStatus, struct MqttParams *mqttParams) //today 2025-06-05 working before removing colors
{
    char buffer[600] = {0}; // Increased buffer size
    char temp[20];
    const char *color;
    // Reset color
    //strncat(buffer, COLOR_RESET, sizeof(buffer) - strlen(buffer) - 1);
    if (strcmp(type, "serial in: ") == 0)
        color = COLOR_GREEN;
    else if (strcmp(type, "serial out: ") == 0)
        color = COLOR_YELLOW;
    else
        color = COLOR_CYAN;

    // Begin log line with color + label

    snprintf(buffer, sizeof(buffer), "%s%s", color, type);

    // Append hex bytes
    for (size_t i = 0; i < length; i++)
    {
        snprintf(temp, sizeof(temp), "%02x", uart_buffer[i]);
        strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
        if (i < length - 1)
        {
            strncat(buffer, ",", sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    // Reset color
    strncat(buffer, COLOR_RESET, sizeof(buffer) - strlen(buffer) - 1);

    // Append diagnostic only for "serial in"
    if (strcmp(type, "serial in: ") == 0)
    {
        char temp2[200];
        snprintf(temp2, sizeof(temp2), "\n%sCharger Id:%s, Reference No: %s, Control state:%d, Network state:%d%s\n",
                 HIGHLIGHT_RED_BG,
                 get_charger_id(ChargerStatus),
                 mqttParams->subscribed_topic,
                 self->serialReceive.controlSideState.bits.current_state,
                 get_current_state_(ChargerStatus),
                 COLOR_RESET);
        strncat(buffer, temp2, sizeof(buffer) - strlen(buffer) - 1);
    }

    // Final newline to separate each log entry
    strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
    // Ensure the buffer is null-terminated
    buffer[sizeof(buffer) - 1] = '\0'; // Safety null-termination
    // Print and send to log pipe
    printf("CSV: %s", buffer); // No extra newline
    send_data_to_log_pipe(buffer);
}*/
/*void store_and_send_uart_log_pipe(uint8_t *uart_buffer, size_t length, char *type, SerialStatus *self, ChargerStatus *ChargerStatus, struct MqttParams *mqttParams)
{
    char buffer[500] = {0}; // Adjust size as needed
    char temp[20];
    char temp2[200]; // Enough to hold "255," + null terminator
    char *color = COLOR_CYAN;

    if (strcmp(type, "serial in: ") == 0)
        color = COLOR_GREEN;
    else if (strcmp(type, "serial out: ") == 0)
        color = COLOR_YELLOW;
    else
        color = COLOR_CYAN;

    snprintf(buffer, sizeof(buffer), "%s%s", color, type);

    for (size_t i = 0; i < length; i++)
    {
        snprintf(temp, sizeof(temp), "%x", uart_buffer[i]);
        strcat(buffer, temp);
        if (i < length - 1)
        {
            strcat(buffer, ",");
        }
    }
    snprintf(temp, sizeof(temp), "%s", COLOR_RESET);
    strcat(buffer, temp);
    if (strcmp(type, "serial in: ") == 0)
    {
        snprintf(temp2, sizeof(temp2), "\n%sCharger Id:%s,Reference No: %s Control side state:%d Network side state: %d %s",
                 HIGHLIGHT_RED_BG,
                 get_charger_id(ChargerStatus),
                 mqttParams->subscribed_topic,
                 self->serialReceive.controlSideState.bits.current_state,
                 get_current_state_(ChargerStatus),
                 COLOR_RESET);
        strcat(buffer, temp2);
    }
    // Print or use the CSV buffer
    printf("CSV: %s\n", buffer);
    send_data_to_log_pipe(buffer);
}*/

void serial_com_send(int fd, SerialStatus *self, ChargerStatus *ChargerStatus, struct MqttParams *mqttParams)
{

    // self->serial_state = 0;
    printf("serial_state:%d\n", self->serial_state);
    self->serialSend.current_state = get_current_state_(ChargerStatus);
    self->serialSend.stop_charge = get_stop_command(ChargerStatus);
    self->serialSend.enable_schedule_charge = get_schedule_charge_state(ChargerStatus);
    self->serialSend.is_internet_available = get_is_internet_available(ChargerStatus);
    self->serialSend.charger_lock_status = get_charger_lock_status(ChargerStatus);
    self->serialSend.schedule_charge_start = get_schedule_charge_start(ChargerStatus);
    const char *time_buf = get_curr_time(ChargerStatus);

    if (get_update_base_config_values(ChargerStatus) == 1)
    {
        self->serial_state = 45;
        // self->update_base_config_values = 2;
        set_update_base_config_values(ChargerStatus, 2);
    }
    switch (self->serial_state)
    {
    case 0:
        self->send45Counter = 0;
        self->send46Counter = 0;
        self->send47Counter = 0;
        self->send48Counter = 0;

        self->serialSend.messege_id = 0;
        // #ifdef singlePhase
        if (get_single_phase_status(ChargerStatus) == 1)
        {
            increase_load_balancing_time_counter(ChargerStatus);
            if (get_enable_load_balancing(ChargerStatus) == 1 && get_load_balancing_time_counter(ChargerStatus) < MAX_TIMEOUT_LOAD_BALANCING && get_using_load_balancing(ChargerStatus) == 1)
            {
                self->serialSend.val1.bytes.LB = get_load_balancing_current_setting(ChargerStatus);
            }
            else if (get_enable_load_balancing(ChargerStatus) == 0 || get_load_balancing_time_counter(ChargerStatus) > MAX_TIMEOUT_LOAD_BALANCING || get_using_load_balancing(ChargerStatus) == 0)
            {
                self->serialSend.val1.bytes.LB = get_maximum_current_setting(ChargerStatus);
                set_using_load_balancing(ChargerStatus, 0);
            }
        }
        else
        {
            self->serialSend.val1.bytes.LB = get_maximum_current_setting(ChargerStatus);
        }

        self->serialSend.val1.bytes.HB = 1; // add time_setup_success_flag
        self->serialSend.val2.bytes.LB = 0; // add UpdateAlarmSerialFlag

        char HH0[3];
        memcpy(HH0, &time_buf[11], 2);
        HH0[2] = '\0';
        self->serialSend.val2.bytes.HB = atoi(HH0);

        char MM0[3];
        memcpy(MM0, &time_buf[14], 2);
        MM0[2] = '\0';
        self->serialSend.val3.bytes.LB = atoi(MM0);
#ifdef DEBUG
        printf("HH0:%s MM0:%s\n", HH0, MM0);
#endif

        self->serialSend.val3.bytes.HB = get_enable_load_balancing(ChargerStatus);

        self->serial_state = 1;
        break;
    case 1:

        self->serialSend.messege_id = 1;
        const char *schedule_start_time_weekdays = get_schedule_start_time_week_days(ChargerStatus);
        char HH1[3];
        memcpy(HH1, &schedule_start_time_weekdays[0], 2);
        HH1[2] = '\0';

        char MM1[3];
        memcpy(MM1, &schedule_start_time_weekdays[3], 2);
        MM1[2] = '\0';

        self->serialSend.val1.bytes.LB = atoi(HH1);
        self->serialSend.val1.bytes.HB = atoi(MM1);

#ifdef DEBUG
        printf("HH1:%s MM1:%s\n", HH1, MM1);
#endif

        const char *schedule_stop_time_weekdays = get_schedule_stop_time_week_days(ChargerStatus);
        char HH2[3];
        memcpy(HH2, &schedule_stop_time_weekdays[0], 2);
        HH2[2] = '\0';

        char MM2[3];
        memcpy(MM2, &schedule_stop_time_weekdays[3], 2);
        MM2[2] = '\0';

        self->serialSend.val2.bytes.LB = atoi(HH2);
        self->serialSend.val2.bytes.HB = atoi(MM2);
#ifdef DEBUG
        printf("HH2:%s MM2:%s\n", HH2, MM2);
#endif
        char mm[3];
        memcpy(mm, &time_buf[5], 2);
        mm[2] = '\0';
        self->serialSend.val3.bytes.LB = atoi(mm);

        char dd[3];
        memcpy(dd, &time_buf[8], 2);
        dd[2] = '\0';
        self->serialSend.val3.bytes.HB = atoi(dd);
#ifdef DEBUG
        printf("mm:%s dd:%s\n", mm, dd);
#endif
        self->serial_state = 2;
        break;
    case 2:
        self->serialSend.messege_id = 2;
        const char *schedule_start_time_weekends = get_schedule_start_time_week_ends(ChargerStatus);
        char HH3[3];
        memcpy(HH3, &schedule_start_time_weekends[0], 2);
        HH3[2] = '\0';

        char MM3[3];
        memcpy(MM3, &schedule_start_time_weekends[3], 2);
        MM3[2] = '\0';

        self->serialSend.val1.bytes.LB = atoi(HH3);
        self->serialSend.val1.bytes.HB = atoi(MM3);

#ifdef DEBUG
        printf("HH3:%s MM3:%s\n", HH3, MM3);
#endif
        const char *schedule_stop_time_weekends = get_schedule_stop_time_week_ends(ChargerStatus);
        char HH4[3];
        memcpy(HH4, &schedule_stop_time_weekends[0], 2);
        HH4[2] = '\0';

        char MM4[3];
        memcpy(MM4, &schedule_stop_time_weekends[3], 2);
        MM4[2] = '\0';

        self->serialSend.val2.bytes.LB = atoi(HH4);
        self->serialSend.val2.bytes.HB = atoi(MM4);
#ifdef DEBUG
        printf("HH4:%s MM4:%s\n", HH4, MM4);
#endif
        char yy1[3];
        memcpy(yy1, &time_buf[0], 2);
        yy1[2] = '\0';
        self->serialSend.val3.bytes.LB = atoi(yy1);

        char yy2[3];
        memcpy(yy2, &time_buf[2], 2);
        yy2[2] = '\0';
        self->serialSend.val3.bytes.HB = atoi(yy2);
#ifdef DEBUG
        printf("yy1:%s yy2:%s\n", yy1, yy2);
#endif
        self->serial_state = 3;
        break;
    case 3:
        self->serialSend.messege_id = 3;

        if (self->serialReceive.stm32_version_received_counter < 5)
        {
            self->serial_state = 4;
        }
        else if (self->serialReceive.stm32_version_received_counter >= 5 && self->verifyParameters == 0)
        {
            self->verifyParameters = 1;
            self->serial_state = 45;
        }
        else
        {
            self->serial_state = 0;
        }
        break;
    case 4:
        self->serialSend.messege_id = 4;
        self->serial_state = 0;
        break;
    case 45:
        if (self->send45Counter < 6)
        {
            self->serialSend.messege_id = 45;
            if (get_update_base_config_values(ChargerStatus) == 2)
            {
                self->serialSend.val1.all = 45;
            }
            else
            {
                self->serialSend.val1.all = 0;
            }
            self->send45Counter++;
        }
        else
        {
            self->serial_state = 0;
#ifdef DEBUG
            printf("in case 45: data not received");
#endif
        }
        break;
    case 46:
        if (self->send46Counter < 6)
        {
            self->serialSend.messege_id = 46;
            if (get_update_base_config_values(ChargerStatus) == 2)
            {
                self->serialSend.val1.all = get_en1_val_all(ChargerStatus);
                self->serialSend.val2.all = get_en2_val_all(ChargerStatus);
                self->serialSend.val3.all = get_uv_upper(ChargerStatus);
            }
            else
            {
                self->serialSend.val1.all = 0;
                self->serialSend.val2.all = 0;
                self->serialSend.val3.all = 0;
            }
            self->send46Counter++;
        }
        else
        {
            self->serial_state = 0;
#ifdef DEBUG
            printf("in case 46: data not received");
#endif
        }
        break;
    case 47:
        if (self->send47Counter < 6)
        {
            self->serialSend.messege_id = 47;
            if (get_update_base_config_values(ChargerStatus) == 2)
            {
                self->serialSend.val1.all = get_uv_lower(ChargerStatus);
                self->serialSend.val2.all = get_ov_upper(ChargerStatus);
                self->serialSend.val3.all = get_ov_lower(ChargerStatus);
            }
            else
            {
                self->serialSend.val1.all = 0;
                self->serialSend.val2.all = 0;
                self->serialSend.val3.all = 0;
            }
            self->send47Counter++;
        }
        else
        {
            self->serial_state = 0;
#ifdef DEBUG
            printf("in case 47: data not received");
#endif
        }
        break;
    case 48:
        if (self->send48Counter < 6)
        {
            self->serialSend.messege_id = 48;
            if (get_update_base_config_values(ChargerStatus) == 2)
            {
                self->serialSend.val1.all = get_freq_upper(ChargerStatus);
                self->serialSend.val2.all = get_freq_lower(ChargerStatus);
                self->serialSend.val3.all = 0;
            }
            else
            {
                self->serialSend.val1.all = 0;
                self->serialSend.val2.all = 0;
                self->serialSend.val3.all = 0;
            }
            self->send48Counter++;
        }
        else
        {
            self->serial_state = 0;
#ifdef DEBUG
            printf("in case 48: data not received");
#endif
        }
        break;
    }

    uint8_t uart_sendBuffer[20] = {
        0x23,
        0x4D,
        (uint8_t)self->serialSend.current_state,
        (uint8_t)self->serialSend.stop_charge,
        (uint8_t)self->serialSend.enable_schedule_charge,
        (uint8_t)self->serialSend.is_internet_available,
        (uint8_t)0,
        (uint8_t)self->serialSend.charger_lock_status,
        (uint8_t)self->serialSend.schedule_charge_start,
        (uint8_t)self->serialSend.messege_id,
        (uint8_t)self->serialSend.val1.bytes.LB,
        (uint8_t)self->serialSend.val1.bytes.HB,
        (uint8_t)self->serialSend.val2.bytes.LB,
        (uint8_t)self->serialSend.val2.bytes.HB,
        (uint8_t)self->serialSend.val3.bytes.LB,
        (uint8_t)self->serialSend.val3.bytes.HB,
        (uint8_t)0xff,
        (uint8_t)0xff,
        0x2A,
        0x0A};

    uint16_t buffer16 = (ModRTU_CRCA((uint8_t *)(uart_sendBuffer + 1), 15)) & 0xff;
    uart_sendBuffer[16] = (uint8_t)buffer16;
    uart_sendBuffer[17] = (ModRTU_CRCA((uint8_t *)(uart_sendBuffer + 1), 15)) >> 8;
#ifdef DEBUG
    printf("serial send:");
    for (int i = 0; i < 20; i++)
    {
        printf("%x ", uart_sendBuffer[i]);
    }
    printf("\n");
#endif

    store_and_send_uart_log_pipe(uart_sendBuffer, 20, "serial out: ", self, ChargerStatus, mqttParams);
    // send_data_to_log_pipe("lets' out this is a long sentence let's see how this prints let's increase more\n");
    serial_send_msg(fd, 2, (const char *)uart_sendBuffer, 20);
};

uint16_t ModRTU_CRCA(uint8_t *buf, int len)
{
    int pos, i;
    uint16_t crc = 0xFFFF;

    for (pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)buf[pos]; // XOR byte into least sig. byte of crc

        for (i = 8; i != 0; i--)
        { // Loop over each bit
            if ((crc & 0x0001) != 0)
            {              // If the LSB is set
                crc >>= 1; // Shift right and XOR 0xA001
                crc ^= 0xA001;
                // crc ^= 0x0190;
            }
            else
                // Else LSB is not set
                crc >>= 1; // Just shift right
        }
    }
    // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
    return crc;
}
void serial_com_receive(int fd, SerialStatus *self, ChargerStatus *ChargerStatus, int rx_buf_size, struct MqttParams *mqttParams)
{
    int len = 0;
    uint16_t crcReceived = 0;
    uint16_t txA_crc_calc = 0;
    uint8_t uart_data_header[2] = {0x23, 0x43};
    uint8_t crc_error_counter = 0;
    // ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_2, (size_t *)&len));

    // len = get_received_length(fd);

    uint8_t *data = (uint8_t *)malloc(rx_buf_size + 1);
    memset(data, 0, rx_buf_size + 1);
#ifdef DEBUG
    printf("before serial receive\n");
#endif
    self->serial_error_timeout++;
    len = serial_read_msg(fd, 2, (char *)data, rx_buf_size);
    /*if (len > 0)
    {
        printf("Received %d bytes:\n", len);
        for (int i = 0; i < len; i++)
        {
            printf("%02X ", (unsigned char)data[i]); // Print as hex
        }
        printf("\n");
    }
    else
    {
        perror("Read failed");
    }*/
    if (len >= 19)
    {
        crcReceived = ((data[17] << 8) | (data[16] & 0xff));
        txA_crc_calc = ModRTU_CRCA((uint8_t *)(data + 1), 15);
        if (memcmp(data, uart_data_header, 2) == 0 && data[18] == 0x2A && txA_crc_calc == crcReceived)
        {
            self->serialReceive.controlSideState.all = data[2];
            self->serialReceive.powerSideState.all = data[3];
            self->serialReceive.networkSideRequestStruct.all = data[4];
            self->serialReceive.controlSideError.all = data[5];
            self->serialReceive.error_report = data[6];
            self->serialReceive.tripBits.all = data[7];
            self->serialReceive.powerSideError.all = data[8];
            self->serialReceive.messege_id = data[9];
            self->serialReceive.val1.bytes.LB = data[10];
            self->serialReceive.val1.bytes.HB = data[11];
            self->serialReceive.val2.bytes.LB = data[12];
            self->serialReceive.val2.bytes.HB = data[13];
            self->serialReceive.val3.bytes.LB = data[14];
            self->serialReceive.val3.bytes.HB = data[15];
#ifdef DEBUG
            printf("serial In:");
            for (int i = 0; i < rx_buf_size; i++)
            {
                printf("%x ", data[i]);
            }
            printf("\n");
#endif
            store_and_send_uart_log_pipe(data, 20, "serial in: ", self, ChargerStatus, mqttParams);
            // send_data_to_log_pipe("serial in\n");
            self->serial_error = 0;
            self->serial_error_timeout = 0;
        }
        else
        {
            crc_error_counter++;

#ifdef DEBUG

            printf("crc_error_counter: %d\n", crc_error_counter);
#endif
        }
    }
    if (self->serial_error_timeout > MAX_SERIAL_WAIT_COUNT)
    {
        self->serial_error = 1;
        self->serial_error_timeout = 0;
    }

    free(data);
    int buffer_flushed = uart_flush_serial(fd, 2);
    if (buffer_flushed == -1)
    {
        printf("Buffer not flushed\n");
    }
    else
    {
        printf("Buffer flushed\n");
    }
    decode_received_serial_data(self, ChargerStatus);
}

void decode_received_serial_data(SerialStatus *self, ChargerStatus *ChargerStatus)
{
    if (self->serialReceive.control_side_current_state_prev != self->serialReceive.controlSideState.bits.current_state)
    {
        self->serialReceive.control_side_current_state_prev = self->serialReceive.controlSideState.bits.current_state;
        // self->serialReceive.send_heart_beat = 1;
        set_send_heartbeat_flag(ChargerStatus, 1);
    }
    if (get_charge_in_progress(ChargerStatus) == 0)
    {
        set_schedule_charge_start(ChargerStatus, self->serialReceive.networkSideRequestStruct.bits.schedule_charge);
#ifdef DEBUG
        printf("schedule_charge_start:%d\n", get_schedule_charge_start(ChargerStatus));
#endif
    }
    switch (self->serialReceive.messege_id)
    {
    case 0:
        self->serialReceive.voltage_L1 = (float)(self->serialReceive.val1.all);
        self->serialReceive.voltage_L2 = (float)(self->serialReceive.val2.all);
        self->serialReceive.voltage_L3 = (float)(self->serialReceive.val3.all);
#ifdef DEBUG
        printf("V1:%.2fV2:%.2fV3:%.2f\n", self->serialReceive.voltage_L1, self->serialReceive.voltage_L2, self->serialReceive.voltage_L3);
#endif
        break;

    case 1:
        self->serialReceive.currentL1 = (float)(self->serialReceive.val1.all) / 10;
        self->serialReceive.currentL2 = (float)(self->serialReceive.val2.all) / 10;
        self->serialReceive.currentL3 = (float)(self->serialReceive.val3.all) / 10;
#ifdef DEBUG
        printf("I1:%.2fI2:%.2fI3:%.2f\n", self->serialReceive.currentL1, self->serialReceive.currentL2, self->serialReceive.currentL3);
#endif
        break;
    case 2:
        self->serialReceive.power = (float)(self->serialReceive.val1.bytes.LB) * 0.1;
        self->serialReceive.energy = (long)(self->serialReceive.val2.bytes.HB << 16 | self->serialReceive.val2.bytes.LB << 8 | self->serialReceive.val1.bytes.HB) * 10;
        self->serialReceive.temperature = self->serialReceive.val3.bytes.LB;
        self->serialReceive.frequency = self->serialReceive.val3.bytes.HB;
#ifdef DEBUG
        printf("P:%.2fE:%ldT:%dF:%d\n", self->serialReceive.power, self->serialReceive.energy, self->serialReceive.temperature, self->serialReceive.frequency);
#endif
        break;
    case 3:
        self->serialReceive.cp_value = self->serialReceive.val1.all;
#ifdef DEBUG
        printf("CP:%d", self->serialReceive.cp_value);
#endif
        break;
    case 4:
        self->serialReceive.stm32_version_received_counter++;
        self->serialReceive.stm32_version_num = (100 * self->serialReceive.val1.bytes.LB) + (10 * self->serialReceive.val1.bytes.HB) + (self->serialReceive.val2.bytes.LB);
#ifdef DEBUG
        printf("STM32_V_NO:%d\n", self->serialReceive.stm32_version_num);
#endif
        break;
    case 45:
        self->serial_state = 46;
#ifdef DEBUG
        printf("in messageId:45 continue update or verifying parameters\n");
#endif
        break;
    case 46:
        self->serialReceive.en_1.all = self->serialReceive.val1.all;
        self->serialReceive.en_2.all = self->serialReceive.val2.all;
        self->serialReceive.uv_upper_stm32 = self->serialReceive.val3.all;
        /*if ((self->serialReceive.en_1.all == get_en1_val_all(ChargerStatus) && self->serialReceive.en_2.all == get_en2_val_all(ChargerStatus) && self->serialReceive.uv_upper_stm32 == get_uv_upper(ChargerStatus)) || !(get_config_flag(ChargerStatus) == 1))
        {
            set_update_base_config_values(ChargerStatus, 2);
            set_en1_val_all(ChargerStatus, self->serialReceive.en_1.all);
            set_en2_val_all(ChargerStatus, self->serialReceive.en_2.all);
            set_uv_upper(ChargerStatus, self->serialReceive.uv_upper_stm32);
        }*/
        if ((self->serialReceive.en_1.all == get_en1_val_all(ChargerStatus) && self->serialReceive.en_2.all == get_en2_val_all(ChargerStatus) && self->serialReceive.uv_upper_stm32 == get_uv_upper(ChargerStatus)) || !(get_config_flag(ChargerStatus) == 1))
        {
#ifdef DEBUG
            printf("in messageId:46 verification pass  or config flag zero continue updating parameters\n");
#endif
#ifdef DEBUG
            printf("in messageId:46 config_flag:%d\n", get_config_flag(ChargerStatus));
#endif
            self->serial_state = 47;
        }
        else
        {
#ifdef DEBUG
            printf("in messageId:46 error:verification failed");
#endif
            set_update_base_config_values(ChargerStatus, 1);
        }

        break;
    case 47:
        self->serialReceive.uv_lower_stm32 = self->serialReceive.val1.all;
        self->serialReceive.ov_upper_stm32 = self->serialReceive.val2.all;
        self->serialReceive.ov_lower_stm32 = self->serialReceive.val3.all;
        if ((self->serialReceive.uv_lower_stm32 == get_uv_lower(ChargerStatus) && self->serialReceive.ov_upper_stm32 == get_ov_upper(ChargerStatus) && self->serialReceive.ov_lower_stm32 == get_ov_lower(ChargerStatus)) || !(get_config_flag(ChargerStatus) == 1))
        {

#ifdef DEBUG
            printf("in messageId:47 verification pass or config flag zero continue updating parameters\n");
#endif
#ifdef DEBUG
            printf("in messageId:47 config_flag:%d\n", get_config_flag(ChargerStatus));
#endif
            self->serial_state = 48;
        }
        else
        {
#ifdef DEBUG
            printf("in messageId:47 error:verification failed");
#endif
            set_update_base_config_values(ChargerStatus, 1);
        }
        break;
    case 48:
        self->serialReceive.freq_upper_stm32 = self->serialReceive.val1.all;
        self->serialReceive.freq_lower_stm32 = self->serialReceive.val2.all;
        if ((self->serialReceive.freq_upper_stm32 == get_freq_upper(ChargerStatus) && self->serialReceive.freq_lower_stm32 == get_freq_lower(ChargerStatus)) || !(get_config_flag(ChargerStatus) == 1))
        {
#ifdef DEBUG
            printf("in messageId:48 verification pass  or config flag zero continue updating parameters\n");
#endif
            self->serial_state = 0;
            set_update_base_config_values(ChargerStatus, 0);
            set_config_flag(ChargerStatus, 0);
            uint8_t config_flag = get_config_flag(ChargerStatus);
#ifdef DEBUG
            printf("in messageId:48 config_flag:%d\n", config_flag);
#endif
            uint16_t err_config_flag = save_config_value("config-update-M1", &config_flag);
#ifdef DEBUG
            if (err_config_flag != 0)
            {
                printf("Error  saving to NVS err_config_flag : %d\n", err_config_flag);
            }
#endif
        }
        else
        {
#ifdef DEBUG
            printf("in messageId:48 error:verification failed");
#endif
            set_update_base_config_values(ChargerStatus, 1);
        }
        break;
    }
}

uint8_t get_control_side_state(SerialStatus *self)
{
    return self->serialReceive.controlSideState.bits.current_state;
}

uint8_t get_start_charge_status(SerialStatus *self)
{
    return self->serialReceive.networkSideRequestStruct.bits.start_charge;
}
uint8_t get_stop_charge_status(SerialStatus *self)
{
    return self->serialReceive.networkSideRequestStruct.bits.stop_charge;
}
uint8_t get_charging_active_status(SerialStatus *self)
{
    return self->serialReceive.controlSideState.bits.charging_active;
}

float get_current_L1(SerialStatus *self)
{
    return self->serialReceive.currentL1;
}
float get_current_L2(SerialStatus *self)
{
    return self->serialReceive.currentL2;
}
float get_current_L3(SerialStatus *self)
{
    return self->serialReceive.currentL3;
}
float get_voltage_L1(SerialStatus *self)
{
    return self->serialReceive.voltage_L1;
}
float get_voltage_L2(SerialStatus *self)
{
    return self->serialReceive.voltage_L2;
}
float get_voltage_L3(SerialStatus *self)
{
    return self->serialReceive.voltage_L3;
}
uint16_t get_stm32_v_num(SerialStatus *self)
{
    return self->serialReceive.stm32_version_num;
}
void set_stm32_v_num(SerialStatus *self, uint16_t value)
{
    self->serialReceive.stm32_version_num = value;
}

uint8_t get_temperature(SerialStatus *self)
{
    return self->serialReceive.temperature;
}
uint8_t get_frequency(SerialStatus *self)
{
    return self->serialReceive.frequency;
}
uint16_t get_cp_value(SerialStatus *self)
{
    return self->serialReceive.cp_value;
}
float get_power(SerialStatus *self)
{
    return self->serialReceive.power;
}
long get_energy(SerialStatus *self)
{
    return self->serialReceive.energy;
}

uint8_t get_error_report(SerialStatus *self)
{
    return self->serialReceive.error_report;
}

uint8_t get_uv_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_1.bits.uv_en;
}

uint8_t get_ov_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_1.bits.ov_en;
}
uint8_t get_sc_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_1.bits.sc_en;
}

uint8_t get_cpf_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_1.bits.cpf_en;
}

uint8_t get_gfi_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_1.bits.gfi_en;
}

uint8_t get_gfit_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_1.bits.gfit_en;
}

uint8_t get_pl_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_1.bits.pl_en;
}

uint8_t get_freq_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_1.bits.freq_en;
}
uint8_t get_modbus_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_2.bits.modbus_en;
}
uint8_t get_penf_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_2.bits.penf_en;
}
uint8_t get_ot_en_stm32(const SerialStatus *self)
{
    return self->serialReceive.en_2.bits.ot_en;
}

uint16_t get_uv_upper_limit_stm32(SerialStatus *self)
{
    return self->serialReceive.uv_upper_stm32;
}
uint16_t get_uv_lower_limit_stm32(SerialStatus *self)
{
    return self->serialReceive.uv_lower_stm32;
}
uint16_t get_ov_upper_limit_stm32(SerialStatus *self)
{
    return self->serialReceive.ov_upper_stm32;
}
uint16_t get_ov_lower_limit_stm32(SerialStatus *self)
{
    return self->serialReceive.ov_lower_stm32;
}
uint16_t get_freq_upper_stm32(SerialStatus *self)
{
    return self->serialReceive.freq_upper_stm32;
}
uint16_t get_freq_lower_stm32(SerialStatus *self)
{
    return self->serialReceive.freq_lower_stm32;
}

void set_serial_error(SerialStatus *self, uint8_t error)
{
    self->serial_error = error;
}
uint8_t get_serial_error(SerialStatus *self)
{
    return self->serial_error;
}

void clear_serial_error(SerialStatus *self)
{
    self->serial_error = 0;
    self->serial_error_timeout = 0;
}
void increase_serial_error_timeout(SerialStatus *self)
{
    self->serial_error_timeout++;
}
uint8_t get_serial_error_timeout(SerialStatus *self)
{
    return self->serial_error_timeout;
}

void set_error_occurred_count(SerialStatus *self, uint8_t error)
{
    self->error_occurred_count = error;
}
uint8_t get_error_occurred_count(SerialStatus *self)
{
    return self->error_occurred_count;
}
void setup_error_flags(SerialStatus *self, ChargerStatus *chargerStatus, uint8_t errorsArray[][2])
{
    errorsArray[0][0] = 1;
    errorsArray[0][1] = (self->serialReceive.powerSideError.bits.error_SR_C == 1);

    errorsArray[1][0] = 2;
    errorsArray[1][1] = (self->serialReceive.powerSideError.bits.error_GFI_test == 1);

    errorsArray[2][0] = 3;
    errorsArray[2][1] = (self->serialReceive.tripBits.bits.trip_GFI == 1);

    errorsArray[3][0] = 4;
    errorsArray[3][1] = (self->serialReceive.controlSideError.bits.serial_A_error == 1);

    errorsArray[4][0] = 5;
    errorsArray[4][1] = (self->serialReceive.tripBits.bits.trip_OC_L1 == 1);

    errorsArray[5][0] = 6;
    errorsArray[5][1] = (self->serialReceive.powerSideError.bits.error_PL == 1);

    errorsArray[6][0] = 7;
    errorsArray[6][1] = (self->serialReceive.powerSideError.bits.error_OT == 1);

    errorsArray[7][0] = 8;
    errorsArray[7][1] = (self->serialReceive.controlSideError.bits.CP_fault == 1);

    errorsArray[8][0] = 9;
    errorsArray[8][1] = (self->serialReceive.controlSideError.bits.diode_check_failed == 1);

    errorsArray[9][0] = 10;
    errorsArray[9][1] = (self->serialReceive.powerSideError.bits.error_OV == 1);

    errorsArray[10][0] = 11;
    errorsArray[10][1] = (self->serialReceive.powerSideError.bits.error_UV == 1);

    errorsArray[11][0] = 12;
    errorsArray[11][1] = (self->serialReceive.powerSideError.bits.error_Freq == 1);

    errorsArray[12][0] = 13;
    errorsArray[12][1] = (get_serial_error(self) == 1 && get_ap_mode_initialised_flag(chargerStatus) == 0 && get_firmware_update_in_progress_flag(chargerStatus) == 0);
}

uint8_t find_highest_priority_error(uint8_t errors[][2], int errorCount)
{
    uint8_t highest_priority = errorCount + 1;
    for (int i = 0; i < errorCount; i++)
    {
        if (errors[i][1] && errors[i][0] < highest_priority)
        {
            highest_priority = errors[i][0];
        }
    }
    return (highest_priority == errorCount + 1) ? 0 : highest_priority;
}

serialCom.c
Displaying serialCom.c.