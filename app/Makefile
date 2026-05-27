# Nom du programme final généré dans /bin
TARGET_EXEC ?= simulateur_reseau

# Dossiers pour organiser le projet
BUILD_DIR ?= ./build
TARGET_DIR ?= ./bin
SRC_DIRS ?= ./src

# Trouve automatiquement tous les fichiers .c dans le dossier ./src
SRCS := $(shell find $(SRC_DIRS) -name "*.c")
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Inclusion des fichiers .h
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# Options de compilation : Wall/Wextra pour les avertissements, -g pour le débuggage (Valgrind/GDB)
CFLAGS ?= $(INC_FLAGS) -MMD -MP -g -Wall -Wextra -Wno-unused-parameter

# Règle principale : compilation de l'exécutable final
$(TARGET_DIR)/$(TARGET_EXEC): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Règle pour compiler chaque fichier .c en fichier objet .o
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Cibles "virtuelles"
.PHONY: clean

# Nettoyage des dossiers temporaires build/ et bin/
clean:
	rm -rf $(BUILD_DIR) $(TARGET_DIR)

-include $(DEPS)
