################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: libdart-shmem-sysv.so

# Tool invocations
libdart-shmem-sysv.so: $(OBJS) $(USER_OBJS) env_check
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -shared -o "libdart-shmem-sysv.so" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(LIBRARIES) libdart-shmem-sysv.so
	-@echo ' '

post-build: env_check
	-mkdir -p $(DASH_LIBS)
	-cp libdart-shmem-sysv.so $(DASH_LIBS)
	-@echo ' '

-include ../../../env_check.mk

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
