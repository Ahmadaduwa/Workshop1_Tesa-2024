# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Libraries to link
LIBS = -lm -lpthread -lcurl -lpaho-mqtt3c -lcjson -lsqlite3

# Target application name
TARGET = iot_app

# Source files and object files
SRCS = main.c db_helper.c rest_thr.c mqtt_thr.c
OBJS = $(SRCS:.c=.o)

# Default target to build the application
all: $(TARGET)

# Rule to link object files and create the executable with libraries
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Rule to compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Run the application
run: $(TARGET)
	./$(TARGET) REST_ENDPOINT MQTT_TOPIC

# Phony targets
.PHONY: all clean run
