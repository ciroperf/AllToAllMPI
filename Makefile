CC = mpic++

  CFLAGS  = -g -Wall

  TARGET = alltoall_datatype

  all: $(TARGET)

  $(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o a.out $(TARGET).cpp 

  clean:
	$(RM) $(TARGET)
  
