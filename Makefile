CC = g++ 
OBJDIR = obj
OBJS = mysh.o tokenizer.o builtin.o vfs.o fslib.o utilities.o format.o
DEPS = mysh.hpp tokenizer.hpp builtin.hpp vfs.hpp fslib.hpp utilities.hpp

.PHONY: all clean

all: $(OBJDIR) shell format

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< -std=c++11

shell: $(OBJDIR)/mysh.o $(OBJDIR)/tokenizer.o $(OBJDIR)/builtin.o $(OBJDIR)/vfs.o $(OBJDIR)/fslib.o $(OBJDIR)/utilities.o
	$(CC) -o shell $(OBJDIR)/mysh.o $(OBJDIR)/tokenizer.o $(OBJDIR)/builtin.o $(OBJDIR)/vfs.o $(OBJDIR)/fslib.o $(OBJDIR)/utilities.o -std=c++11

format: $(OBJDIR)/format.o $(OBJDIR)/vfs.o
	$(CC) -o format $(OBJDIR)/format.o $(OBJDIR)/vfs.o

clean:
	@rm -f $(TARGET) $(wildcard *.o)
	@rm -rf $(OBJDIR) 