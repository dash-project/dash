#############################################################
# TARGETS
#############################################################

all: lib$(LIB_NAME).a
    
src/%.o: ../src/%.c
	$(CC) -I$(DASH_INCLUDES) -D_GNU_SOURCE=1 $(OPTFLAGS) $(CFLAGS) -c -fmessage-length=0 -fPIC -o "$@" "$<"
	@echo ' '

src/%.o: ../src/%.cpp
	$(CC) -I$(DASH_INCLUDES) -D_GNU_SOURCE=1 $(OPTFLAGS) $(CFLAGS) -c -fmessage-length=0 -fPIC -o "$@" "$<"
	@echo ' '

lib$(LIB_NAME).a: $(OBJS)
	ar -cq "lib$(LIB_NAME).a" $(OBJS)
	@echo ' '

clean:
	-@echo '--- clean'
	-$(RM) $(OBJS) $(C_DEPS) $(LIBRARIES) lib$(LIB_NAME).a
	-@echo '--- DONE clean'
	-@echo ' '

clean-deploy:
	-@echo '--- clean-deploy'
	-rm -rf $(DASH_INCLUDES)/$(LIB_NAME)
	-@echo '--- DONE clean-deploy'
	-@echo ' '

ccd: clean clean-deploy deploy

deploy: lib$(LIB_NAME).a
	-@echo '--- deploy'
	-mkdir -p $(DASH_INCLUDES)/$(LIB_NAME)    
	-cp ../src/*.h $(DASH_INCLUDES)/$(LIB_NAME)
	-@echo '--- DONE deploy'
	-@echo ' '

