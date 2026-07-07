#include <stdio.h>
#include <pthread.h>

__thread int tls_init = 42;
__thread int tls_zero;

static void *thread_func(void *arg)
{
    (void)arg;

    printf("%d\n", tls_init);
    if (tls_init != 42) return (void *)1;

    printf("%d\n", tls_zero);
    if (tls_zero != 0) return (void *)1;

    tls_init = 100;
    tls_zero = 200;

    printf("%d\n", tls_init);
    printf("%d\n", tls_zero);

    return (void *)0;
}

int main()
{
    pthread_t t;
    void *ret;
    int errors = 0;

    printf("%d\n", tls_init);
    if (tls_init != 42) errors = 1;

    printf("%d\n", tls_zero);
    if (tls_zero != 0) errors = 1;

    pthread_create(&t, NULL, thread_func, NULL);
    pthread_join(t, &ret);

    if (ret) errors = 1;

    printf("%d\n", tls_init);
    if (tls_init != 42) errors = 1;

    printf("%d\n", tls_zero);
    if (tls_zero != 0) errors = 1;

    return errors;
}
