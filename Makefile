
# --> PROGRAM ------------------------------------------------------------------
PROGRAM = ft_ping

# ~~~~~~~~~~~~~~~~ SOURCES ~~~~~~~~~~~~~~~~

SRCS_PATH		=	srcs
SRCS    =	main.c \
			icmp.c \
			utils.c
			

# ~~~~~~~~~~~~~~~~ OBJECTS ~~~~~~~~~~~~~~~~

OBJS_PATH		=	objs
OBJS			=	$(addprefix $(OBJS_PATH)/, $(SRCS:.c=.o))

# ~~~~~~~~~~~~~~~~ INCLUDES ~~~~~~~~~~~~~~~~

INCLUDES_PATH = includes

INCLUDES		=	ft_ping.h
INCLUDES_FILES		=	$(addprefix $(INCLUDES_PATH)/, $(INCLUDES))

# --> COMPILER AND FLAGS -------------------------------------------------------

CC				= gcc

FLAGS			= -I./$(INCLUDES_PATH)
# FLAGS			+= -Wall -Wextra -Werror
FLAGS			+= -fsanitize=address -g3

# --> RULES --------------------------------------------------------------------

all:   header $(PROGRAM)

# ~~~~~~~~~~~~ COMPILING IN .o ~~~~~~~~~~~~

$(OBJS_PATH)/%.o:	$(SRCS_PATH)/%.c $(INCLUDES_FILES) Makefile
	mkdir -p $(dir $@);
	printf "%-62b%b" "$(CYAN)$(BOLD)compiling $(END)$<"
	 ${CC} ${FLAGS} -I$(INCLUDES_PATH) -c $< -o $@							
	printf "$(GREEN)[✓]$(END)\n"

# ~~~~~~~ COMPILING THE EXECUTABLE ~~~~~~~~

$(PROGRAM): $(OBJS)
	printf "%-63b%b" "\n$(BOLD)$(GREEN)creating$(END) $@"
	$(CC) $(FLAGS) $(OBJS) -o $(PROGRAM)
	printf "$(GREEN)[✓]$(END)\n\n\n"


# ~~~~~~~~~~~~ CLEANING RULES ~~~~~~~~~~~~~

clean:
	rm -rf $(OBJS_PATH)

fclean: clean
	rm -f $(PROGRAM)

# ~~~~~~~~~ REMAKE AND EXEC RULE ~~~~~~~~~~
re: fclean all


# --> HEADER -------------------------------------------------------------------

header :
	@printf "  __ _              _             \n"
	@printf " / _| |_      _ __ (_)_ __   __ _ \n"
	@printf "| |_| __|    | '_ \| | '_ \ / _\` |\n"
	@printf "|  _| |_     | |_) | | | | | (_| |\n"
	@printf "|_|  \__|____| .__/|_|_| |_|\__, |\n"
	@printf "        |____|_|            |___/ \n"
	@printf "\n\n"


# --> COLOR --------------------------------------------------------------------

BLACK		:= ""
RED			:= ""
GREEN		:= ""
YELLOW		:= ""
BLUE		:= ""
PURPLE		:= ""
CYAN		:= ""
WHITE		:= ""
END			:= ""
UNDER		:= ""
BOLD		:= ""
rev			:= ""
END			:= ""

ifneq (,$(findstring 256color, ${TERM}))
	BLACK		:= $(shell tput -Txterm setaf 0)
	RED			:= $(shell tput -Txterm setaf 1)
	GREEN		:= $(shell tput -Txterm setaf 2)
	YELLOW		:= $(shell tput -Txterm setaf 3)
	BLUE		:= $(shell tput -Txterm setaf 4)
	PURPLE		:= $(shell tput -Txterm setaf 5)
	CYAN		:= $(shell tput -Txterm setaf 6)
	WHITE		:= $(shell tput -Txterm setaf 7)
	END			:= $(shell tput -Txterm sgr0)
	UNDER		:= $(shell tput -Txterm smul)
 	BOLD		:= $(shell tput -Txterm bold)
	rev			:= $(shell tput -Txterm rev)
endif


.PHONY: all clean fclean re header exec
.SILENT:
