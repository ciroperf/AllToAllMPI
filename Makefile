CC = mpic++

  CFLAGS  = -g -Wall

  TARGET = test

  all: $(TARGET)

  $(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o a.out $(TARGET).cpp

  clean:
	$(RM) $(TARGET)
  
