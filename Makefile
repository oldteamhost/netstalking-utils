CC = gcc
CFLAGS = -Wall
SRC_DIRS = icmpflood imghst lasth
LDFLAGS = -lcurl
OBJ_DIR = build

ICMPFLOOD_SRC = icmpflood/icmpflood.c
IMGHST_SRC = imghst/imghst.c
LASTH_SRC = lasth/lasth6.c

ICMPFLOOD_EXEC = $(OBJ_DIR)/icmpflood
IMGHST_EXEC = $(OBJ_DIR)/imghst
LASTH_EXEC = $(OBJ_DIR)/lasth

all: build $(ICMPFLOOD_EXEC) $(IMGHST_EXEC) $(LASTH_EXEC)

build:
	@mkdir -p $(OBJ_DIR)

$(ICMPFLOOD_EXEC): $(ICMPFLOOD_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(IMGHST_EXEC): $(IMGHST_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(LASTH_EXEC): $(LASTH_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(ICMPFLOOD_EXEC) $(IMGHST_EXEC) $(LASTH_EXEC)

.PHONY: all clean build
