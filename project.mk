#############################################################
# TARGETS
#############################################################

all: $(PROJECT_NAME)
    
$(PROJECT_NAME): $(OBJS)
	$(LD) -L$(DASH_LIBS) -o "$(PROJECT_NAME)" $(OBJS) $(LIBS)
	@echo ' '

src/%.o: ../src/%.c
	$(CC) -I$(DASH_INCLUDES) -D_GNU_SOURCE=1 $(OPTFLAGS) $(CFLAGS) -c -fmessage-length=0 -fPIC -o "$@" "$<"
	@echo ' '

src/%.o: ../src/%.cpp
	$(CC) -I$(DASH_INCLUDES) -D_GNU_SOURCE=1 $(OPTFLAGS) $(CFLAGS) -c -fmessage-length=0 -fPIC -o "$@" "$<"
	@echo ' '

clean:
	-@echo '--- clean'
	-$(RM) $(OBJS) $(C_DEPS) $(LIBRARIES) $(PROJECT_NAME)
	-@echo '--- DONE clean'
	-@echo ' '

