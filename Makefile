XC="C:/Program Files (x86)/Microchip/xc8/v2.10"
CC=$(XC)/bin/xc8.exe

NAME     = pic-async
MCU      = 16f18326
FOSC     = 8000000UL
OBJ_DIR  = ./obj
DIST_DIR = ./dist

SRCS     = main.c loop.c


OBJS     = $(SRCS:.c=.p1)
DEPS     = $(SRCS:.c=.d)
CFLAGS   = -Q --chip=$(MCU) -D_XTAL_FREQ=$(FOSC) -DFW_VERSION=$(VERSION) --std=c99 --objdir=$(OBJ_DIR) --dep=gcc $(FLAGS) -g
LDFLAGS  = -Q --chip=$(MCU) --outdir=$(OBJ_DIR)


all: $(DIST_DIR)/$(NAME).hex $(DIST_DIR)/$(NAME).cof


$(DIST_DIR)/$(NAME).hex $(DIST_DIR)/$(NAME).cof: $(OBJ_DIR) $(OBJS)
	@echo "Build dist ..."
	@mkdir -p $(DIST_DIR)
	@$(CC) $(LDFLAGS) --output=inhx32,mcof --asmlist -O$(notdir $(NAME)) $(OBJS)
	cp $(OBJ_DIR)/$(notdir $(NAME)).hex $(OBJ_DIR)/$(notdir $(NAME)).cof $(DIST_DIR)


%.p1: %.c
%.p1: %.c %.d
	@echo "Build $< ..."
	@$(CC) $(CFLAGS) --pass1 -C -O$@ $<


%.d: ;
.PRECIOUS: %.d


$(OBJ_DIR):
	@mkdir -p $@

clean:
	rm -rf $(DEPS) $(OBJS) $(OPTS:.c=.p1) $(OPTS:.c=.d) $(OBJ_DIR) $(DIST_DIR)

include $(DEPS)