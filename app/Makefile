# Nom des programmes
TARGET_STP = ./bin/simulateur_stp
TARGET_SANS_STP = ./bin/simulateur_sans_stp

# Dossiers
BUILD_DIR = ./build
TARGET_DIR = ./bin
SRC_DIRS = ./src

# Flags
INC_FLAGS = -I$(SRC_DIRS)
CFLAGS = $(INC_FLAGS) -g -Wall -Wextra -Wno-unused-parameter

# Cibles principales
all: $(TARGET_STP) $(TARGET_SANS_STP)

# Compilation de l'exécutable STP
$(TARGET_STP): $(BUILD_DIR)/graphe.o $(BUILD_DIR)/reseau.o $(BUILD_DIR)/main_stp.o
	mkdir -p $(TARGET_DIR)
	$(CC) $^ -o $@

# Compilation de l'exécutable SANS STP
$(TARGET_SANS_STP): $(BUILD_DIR)/graphe.o $(BUILD_DIR)/reseau.o $(BUILD_DIR)/main_sans_stp.o
	mkdir -p $(TARGET_DIR)
	$(CC) $^ -o $@

# Règle générique pour compiler chaque fichier .c en fichier objet .o
$(BUILD_DIR)/%.o: $(SRC_DIRS)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET_DIR)
