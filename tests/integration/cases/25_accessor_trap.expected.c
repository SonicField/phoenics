#ifndef abort
extern void abort(void);
#endif
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
    Shape_Circle,
    Shape_Rectangle,
    Shape__COUNT
} Shape_Tag;

typedef struct {
    double radius;
} Shape_Circle_t;

typedef struct {
    double width;
    double height;
} Shape_Rectangle_t;

typedef struct Shape {
    Shape_Tag tag;
    union {
        Shape_Circle_t Circle;
        Shape_Rectangle_t Rectangle;
    };
} Shape;

static inline Shape Shape_mk_Circle(double radius) {
    Shape _v;
    _v.tag = Shape_Circle;
    _v.Circle.radius = radius;
    return _v;
}

static inline Shape Shape_mk_Rectangle(double width, double height) {
    Shape _v;
    _v.tag = Shape_Rectangle;
    _v.Rectangle.width = width;
    _v.Rectangle.height = height;
    return _v;
}

static inline Shape_Circle_t Shape_as_Circle(Shape v) {
    if (v.tag != Shape_Circle) abort();
    return v.Circle;
}

static inline Shape_Rectangle_t Shape_as_Rectangle(Shape v) {
    if (v.tag != Shape_Rectangle) abort();
    return v.Rectangle;
}
#line 11

int main(void) {
    /* Verify correct accessor works */
    Shape s = Shape_mk_Rectangle(3.0, 4.0);
    Shape_Rectangle_t r = Shape_as_Rectangle(s);
    printf("correct accessor: %g %g\n", r.width, r.height);

    /* Test wrong accessor aborts — run in child process */
    pid_t pid = fork();
    if (pid == 0) {
        /* Child: call wrong accessor — must abort */
        Shape_as_Circle(s);
        /* If we reach here, trap failed */
        _exit(99);
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) {
        printf("wrong accessor: aborted (SIGABRT) — correct\n");
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 99) {
        printf("ERROR: wrong accessor did NOT abort!\n");
        return 1;
    } else {
        printf("wrong accessor: terminated by signal %d\n", WTERMSIG(status));
    }

    /* Test another wrong accessor direction */
    Shape s2 = Shape_mk_Circle(2.5);
    pid = fork();
    if (pid == 0) {
        Shape_as_Rectangle(s2);
        _exit(99);
    }
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) {
        printf("reverse wrong accessor: aborted (SIGABRT) — correct\n");
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 99) {
        printf("ERROR: reverse wrong accessor did NOT abort!\n");
        return 1;
    } else {
        printf("reverse wrong accessor: terminated by signal %d\n", WTERMSIG(status));
    }

    return 0;
}
