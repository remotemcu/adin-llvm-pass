typedef struct test_struct {
    void*   point;               
} test_t;

static test_t test_ex;

int ** pointer_int;

int* q(int* a){
	pointer_int[0] = a;
	test_ex.point = a;
	return test_ex.point;
}