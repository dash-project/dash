#############################################################
# TARGETS
#############################################################

all: lib$(LIB_NAME).so
    
src/%.o: ../src/%.c
	$(CC) -I$(DASH_INCLUDES) -D_GNU_SOURCE=1 $(OPTFLAGS) $(CFLAGS) -c -fmessage-length=0 -fPIC -o "$@" "$<"
	@echo ' '

src/%.o: ../src/%.cpp
	$(CC) -I$(DASH_INCLUDES) -D_GNU_SOURCE=1 $(OPTFLAGS) $(CFLAGS) -c -fmessage-length=0 -fPIC -o "$@" "$<"
	@echo ' '

lib$(LIB_NAME).so: $(OBJS)
	$(LD) -L$(DASH_LIBS) -shared -o "lib$(LIB_NAME).so" $(OBJS) $(LIBS)
	@echo ' '

clean:
	-@echo '--- clean'
	-$(RM) $(OBJS) $(C_DEPS) $(LIBRARIES) lib$(LIB_NAME).so
	-@echo '--- DONE clean'
	-@echo ' '

clean-deploy:
	-@echo '--- clean-deploy'
	-rm -rf $(DASH_INCLUDES)/$(LIB_NAME)
	-rm -rf $(DASH_LIBS)/lib$(LIB_NAME).so
	-@echo '--- DONE clean-deploy'
	-@echo ' '

ccd: clean clean-deploy deploy

deploy: lib$(LIB_NAME).so
	-@echo '--- deploy'
	-mkdir -p $(DASH_LIBS)
	-mkdir -p $(DASH_INCLUDES)/$(LIB_NAME)    
	-cp lib$(LIB_NAME).so $(DASH_LIBS)
	-cp ../src/*.h $(DASH_INCLUDES)/$(LIB_NAME)
	-@echo '--- DONE deploy'
	-@echo ' '

