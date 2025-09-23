CXX = g++
CXXFLAGS = -g -Wall -Wextra -MMD -MP
TARGET = bin/cssh

SRC = main.cpp
OBJ = $(SRC:%.cpp=build/%.o)
DEPS = $(OBJ:build/%.o=build/%.d)

.PHONY: all clean clean-data

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@if [ ! -d "$(HOME)/cssh/" ]; then \
		mkdir -p "$(HOME)/cssh/"; \
	fi
	
	@if [ ! -f "$(HOME)/cssh/friendly_names.config" ]; then \
		cp ./friendly_names.config "$(HOME)/cssh/"; \
	fi

	cp ./bin/cssh $(HOME)/cssh/

build/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
-include $(DEPS)

clean:
	rm -f $(OBJ) $(DEPS) $(TARGET)

clean-data:
	rm -f $(HOME)/cssh/device_login_record.csv $(HOME)/cssh/device_login_record_sno.txt $(HOME)/cssh/device_being_used.dat $(HOME)/cssh/device_scanned.dat 