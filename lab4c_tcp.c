//
// NAME: Zhengyuan Liu
// EMAIL: zhengyuanliu@ucla.edu
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mraa/aio.h>

# define Fahrenheit 0
# define Centigrade 1
# define AIO_PIN 1
# define GPIO_PIN 60

int id = -1;  // 9-digit-number
char * hostname = NULL;  // name or address
int port = -1;  // port number

int period = 1;  // a sampling interval in seconds
int scale = Fahrenheit;  // temperature scale, default: Fahrenheit
int log_opt = 0;  // option of log
int log_fd = -1;  // fd of the logfile
int sockfd;  // fd of the socket
mraa_aio_context temp_sensor;
int run_flag = 1;
int report_flag = 1;
char timestr[32];  // format time string

// print an error message on stderr when system call fails and exit with a return code of 1
void system_call_fail(char *name) {
    fprintf(stderr, "Error: system call %s fails, info: %s\n", name, strerror(errno));
    exit(2);
}

void print_usage() {
    fprintf(stderr, "Usage: lab4c_tcp --id=9-digit-number --host=name-or-address --log=filename port_number [--period=#] [--scale=C/F]\n");
}

// Centigrade to Fahrenheit
float c2f(float c) {
    return c * 9.0 / 5.0 + 32.0;
}

float get_temperature(int value) {
    const int B = 4275;               // B value of the thermistor
    const int R0 = 100000;            // R0 = 100k
    float R = 1023.0/value-1.0;
    R = R0*R;
    float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
    if (scale == Fahrenheit)
        temperature = c2f(temperature);
    return temperature;
}

void get_current_timestr() {
    time_t t = time(NULL);
    struct tm *tmp;
    tmp = localtime(&t);
    if (tmp == NULL)
        system_call_fail("localtime");
    if (strftime(timestr, sizeof(timestr), "%H:%M:%S", tmp) == 0) {
        fprintf(stderr, "strftime returned 0");
        exit(1);
    }
}

void report_temperature(float temperature) {
    get_current_timestr();
    dprintf(sockfd, "%s %.1f\n", timestr, temperature);
    if (log_opt)
        dprintf(log_fd, "%s %.1f\n", timestr, temperature);
}

void shut_down() {
    run_flag = 0;
    get_current_timestr();
    dprintf(sockfd, "%s SHUTDOWN\n", timestr);
    if (log_opt)
        dprintf(log_fd, "%s SHUTDOWN\n", timestr);
}

// initialize sensors
void init_sensors() {
    temp_sensor = mraa_aio_init(AIO_PIN);
    if (temp_sensor == NULL) {
        fprintf(stderr, "Failed to initialize AIO\n");
        mraa_deinit();
        exit(1);
    }
}

void close_sensors() {
    int status;
    status = mraa_aio_close(temp_sensor);
    if (status != MRAA_SUCCESS) {
        exit(1);
    }
}

int has_prefix(char *s, char *pre) {
    if (strlen(pre) > strlen(s))
        return 0;
    int i;
    int len = (int)strlen(pre);
    for(i = 0; i < len; i++)
        if (s[i] != pre[i])
            return 0;
    return 1;
}

void substr(char *s, char *sub, int start, int end) {
    int i;
    for(i = start; i < end; i++)
        sub[i - start] = s[i];
    sub[end - start] = '\0';
}

void process_command(char * command) {
    if (log_opt)
        dprintf(log_fd, "%s\n", command);
    if (strcmp(command, "SCALE=F") == 0) {
        scale = Fahrenheit;
    }
    else if (strcmp(command, "SCALE=C") == 0) {
        scale = Centigrade;
    }
    else if (strcmp(command, "STOP") == 0) {
        report_flag = 0;
    }
    else if (strcmp(command, "START") == 0) {
        report_flag = 1;
    }
    else if (strcmp(command, "OFF") == 0) {
        shut_down();
    }
    else if (has_prefix(command, "PERIOD=")) {
        char periodstr[100];
        substr(command, periodstr, (int)strlen("PERIOD="), (int)strlen(command));
        int tmp = atoi(periodstr);
        if (tmp > 0)
            period = tmp;
    }
    else if (has_prefix(command, "LOG ")){
        ;  // do nothing because all the commands should log to the logfile
    }
}

// str may have multiple lines (commands)
void process_commands(char * str) {
    int i;
    int start = 0;
    char command[100];
    int len = (int)strlen(str);
    for (i = 0; i < len; i++) {
        if (str[i] == '\n') {  // find a command
            substr(str, command, start, i);
            process_command(command);
            start = i + 1;
        }
    }
}

int main(int argc, char * argv[]) {
    int rc;  // return code
    int tmp;
    int i = 0;
    
    static const struct option long_options[] = {
        { "id",     required_argument, NULL, 'i' },
        { "host",   required_argument, NULL, 'h' },
        { "log",    required_argument, NULL, 'l' },
        { "period", required_argument, NULL, 'p' },
        { "scale",  required_argument, NULL, 's' },
        { 0,        0,                 0,     0  }
    };
    int opt = -1;
    const char *optstring = "i:h:l:p:s:";
    while ((opt = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                tmp = atoi(optarg);
                if (tmp <= 0)
                    fprintf(stderr, "Error: Invalid argument for temperature, using default period=1\n");
                else
                    period = tmp;
                break;
            case 's':
                if(strcmp("F", optarg) == 0) {
                    scale = Fahrenheit;
                }
                else if(strcmp("C", optarg) == 0) {
                    scale = Centigrade;
                }
                else {
                    fprintf(stderr, "Error: Invalid argument for temperature scale, using default scale=Fahrenheit\n");
                }
                break;
            case 'i':
                id = atoi(optarg);
                break;
            case 'h':
                hostname = optarg;
                break;
            case 'l':
                log_opt = 1;
                log_fd = creat(optarg, 0666);
                if (log_fd == -1){
                    fprintf(stderr, "Error: bad argument for log\n");
                    exit(1);
                }
                break;
            default:  // encounter an unrecognized argument
                fprintf(stderr, "Error: Unrecongized argument\n");
                print_usage();
                exit(1);
        }
    }
    if (optind < argc)
        port = atoi(argv[optind]);
    else {
        fprintf(stderr, "Error: lack argument for port number\n");
        print_usage();
        exit(1);
    }
    if (id == -1 || hostname == NULL || port == -1 || log_fd == -1) {  // lack mandatory parameters
        fprintf(stderr, "Error: lack argument(s)\n");
        print_usage();
        exit(1);
    }
    
    init_sensors();
    
    struct sockaddr_in server_addr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)  // error occurs
        system_call_fail("socket");
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Error: no such host\n");
        exit(1);
    }
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);  // set port
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    rc = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));  // connect to a server on sockfd
    if (rc == -1)  // error occurs
        system_call_fail("connect");
    
    // immediately send (and log) an ID terminated with a newline
    dprintf(sockfd, "ID=%d\n", id);
    dprintf(log_fd, "ID=%d\n", id);
    
    struct pollfd fds[1];
    fds[0].fd = sockfd;  // fd of the socket (server)
    fds[0].events = POLLIN | POLLHUP | POLLERR;// wait for either input (POLLIN) or error (POLLHUP, POLLERR) events
    
    while (run_flag) {
        rc = poll(fds, 1, 0);
        if (rc == -1)  // error occurs
            system_call_fail("poll");
        else if (rc == 0) {  // the call timed out and no file descriptors were ready
            if (i % period == 0) {  // every period second
                //int value = 400;  // debug mode in the laptop
                int value = mraa_aio_read(temp_sensor);
                float t = get_temperature(value);
                if (report_flag)
                    report_temperature(t);
            }
            i = (i + 1) % period;
            sleep(1);  // the sleep unit is one second
        }
        else {
            if (fds[0].revents & POLLIN) {  // fds[0] (stdin, keyboard) has data to read
                char buf[256];
                bzero((char *)buf, 256);
                ssize_t n_bytes = read(fds[0].fd, buf, 256);  // read input from the stdin
                if (n_bytes == -1)
                    system_call_fail("read");
                process_commands(buf);
            }
            if(fds[0].revents & (POLLHUP | POLLERR)) {  // fds[0] (stdin) has a hang up
                break;
            }
            if(fds[0].revents & POLLERR) {  // fds[0] (stdin) has an error
                fprintf(stderr, "An error occurs\n");
                close_sensors();
                exit(1);
            }
        }
    }
    
    close_sensors();
    return 0;
}
