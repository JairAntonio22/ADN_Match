#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

typedef struct mySem {
#ifdef __APPLE__
	dispatch_semaphore_t sem;
#else
	sem_t sem;
#endif
} mySem;


void mySem_init(mySem *s, int value) {
#ifdef __APPLE__
	dispatch_semaphore_t *sem = &s->sem;
	*sem = dispatch_semaphore_create(value);
#else
	sem_init(&s->sem, 0, value);
#endif
}

void mySem_wait(mySem *s) {
#ifdef __APPLE__
	dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
#else
	int r;
	do {
		r = sem_wait(&s->sem);
	} while (r == -1 && errno == EINTR);
#endif
}

void mySem_signal(mySem *s) {
#ifdef __APPLE__
	dispatch_semaphore_signal(s->sem);
#else
	sem_post(&s->sem);
#endif
}
