#include "headers.h"




int validaComandosUser(char *comando);
void validaUser(int argc, char **argv);
void *fEnviaServer (void *loginData);
void *fRecebeServer (void *loginData);
void signalHandler(int sig);