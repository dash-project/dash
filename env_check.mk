
env_check:
ifndef DASH_INCLUDES
	@echo 'DASH_INCLUDES not set!'
	exit 1
endif
ifndef DASH_LIBS
	@echo 'DASH_LIBS not set!'
	exit 1
endif

.PHONY: env_check
	
