CC = mpic++

  CFLAGS  = -g -Wall

  TARGET = alltoallNBC

  all: $(TARGET)

  $(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o a.out $(TARGET).cpp 

  clean:
	$(RM) $(TARGET)
  
