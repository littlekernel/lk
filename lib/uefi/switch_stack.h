
#ifdef __cplusplus
extern "C" {

#endif

int call_with_stack(void *stack, int (*fp)(void *, void *), void *param1,
                    void *param2);
#ifdef __cplusplus
}
#endif