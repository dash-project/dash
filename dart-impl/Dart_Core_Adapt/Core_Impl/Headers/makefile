RM := rm -rf

all: deploy

clean-deploy:
	-$(RM) $(DASH_INCLUDES)/dart
	-mkdir -p $(DASH_INCLUDES)/dart
	-@echo ' '

deploy: clean-deploy
	-cp *.h $(DASH_INCLUDES)/dart
	-@echo ' '
