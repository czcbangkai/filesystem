CC = g++ 
OBJDIR = obj
OBJS = mysh.o tokenizer.o builtin.o vfs.o fslib.o utilities.o
DEPS = mysh.hpp tokenizer.hpp builtin.hpp vfs.hpp fslib.hpp utilities.hpp

.PHONY: all clean

all: $(OBJDIR) shell 

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< -std=c++11

shell: $(OBJDIR)/mysh.o $(OBJDIR)/tokenizer.o $(OBJDIR)/builtin.o $(OBJDIR)/vfs.o $(OBJDIR)/fslib.o $(OBJDIR)/utilities.o
	$(CC) -o shell $(OBJDIR)/mysh.o $(OBJDIR)/tokenizer.o $(OBJDIR)/builtin.o $(OBJDIR)/vfs.o $(OBJDIR)/fslib.o $(OBJDIR)/utilities.o -std=c++11

clean:
	@rm -f $(TARGET) $(wildcard *.o)
	@rm -rf $(OBJDIR) 