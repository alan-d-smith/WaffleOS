#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* Include the actual production function */
#include "source/boot/stage2/string.c"

START_TEST(test_strcpy_no_buffer_overflow)
{
    /* Invariant: strcpy must never write beyond the bounds of the destination buffer */
    
    /* Adversarial payloads - 3 representative cases */
    const char *payloads[] = {
        "A",                    /* Minimal valid input */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ", /* Boundary: longer than typical small buffers */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" /* Exploit case: very long string */
    };
    
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);
    
    for (int i = 0; i < num_payloads; i++) {
        /* Create a guard page protected buffer */
        size_t page_size = getpagesize();
        char *guard_page = mmap(NULL, 3 * page_size, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        /* Mark the middle page as PROT_NONE to detect overflow */
        mprotect(guard_page + page_size, page_size, PROT_NONE);
        
        /* Destination buffer sits at end of first page */
        char *dst = guard_page + page_size - 16;
        size_t dst_size = 16;
        
        /* Clear buffer */
        memset(dst, 0, dst_size);
        
        /* Execute the actual production strcpy */
        char *result = strcpy(dst, payloads[i]);
        
        /* Verify result pointer is correct */
        ck_assert_ptr_eq(result, dst);
        
        /* Verify no overflow into protected page */
        /* If we get here without SIGSEGV, invariant holds */
        
        /* Cleanup */
        munmap(guard_page, 3 * page_size);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_strcpy_no_buffer_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}