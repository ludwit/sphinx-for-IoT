#include "sphinx.h"

int main(void)
{
    puts("Generated RIOT application: 'sphinx-networking'");

    /* start sphinx immediately */
    sphinx_start();

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
