/*
 * Tests set for BCM2838 mailbox property interface.
 *
 * Copyright (c) 2022 Auriga
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "libqtest-single.h"
#include "bcm2838-mailbox.h"

#define MBOX_TEST_MESSAGE_ADDRESS 0x10000000

#define TEST_TAG(x) TAG_ ## x
#define TEST_TAG_TYPE(x) TAG_##x##_t

#define TEST_FN_NAME(test, subtest) \
    test ## _ ## subtest ## _test

#define SETUP_FN_NAME(test, subtest) \
    test ## _ ## subtest ## _setup

#define CHECK_FN_NAME(test, subtest) \
    test ## _ ## subtest ## _spec_check

#define DECLARE_TEST_CASE_SETUP(testname, ...)              \
    void SETUP_FN_NAME(testname, __VA_ARGS__)               \
                             (TEST_TAG_TYPE(testname) * tag)

/*----------------------------------------------------------------------------*/
#define DECLARE_TEST_CASE(testname, ...)                                       \
    __attribute__((weak))                                                      \
    void SETUP_FN_NAME(testname, __VA_ARGS__)                                  \
                      (TEST_TAG_TYPE(testname) * tag);                         \
    static void CHECK_FN_NAME(testname, __VA_ARGS__)                           \
                             (TEST_TAG_TYPE(testname) *tag);                   \
    static void TEST_FN_NAME(testname, __VA_ARGS__)(void) {                    \
        struct {                                                               \
            MboxBufHeader header;                                              \
            TEST_TAG_TYPE(testname) tag;                                       \
            uint32_t end_tag;                                                  \
        } mailbox_buffer = { 0 };                                              \
                                                                               \
        QTestState *qts = qtest_init("-machine raspi4b2g");                    \
                                                                               \
        mailbox_buffer.header.size = sizeof(mailbox_buffer);                   \
        mailbox_buffer.header.req_resp_code = MBOX_PROCESS_REQUEST;            \
                                                                               \
        mailbox_buffer.tag.id = TEST_TAG(testname);                            \
        mailbox_buffer.tag.value_buffer_size = MAX(                            \
            sizeof(mailbox_buffer.tag.request.value),                          \
            sizeof(mailbox_buffer.tag.response.value));                        \
        mailbox_buffer.tag.request.zero = 0;                                   \
                                                                               \
        mailbox_buffer.end_tag = TAG_END;                                      \
                                                                               \
        if (SETUP_FN_NAME(testname, __VA_ARGS__)) {                            \
            SETUP_FN_NAME(testname, __VA_ARGS__)(&mailbox_buffer.tag);         \
        }                                                                      \
                                                                               \
        qtest_memwrite(qts, MBOX_TEST_MESSAGE_ADDRESS,                         \
                    &mailbox_buffer, sizeof(mailbox_buffer));                  \
        qtest_mbox1_write_message(qts, MBOX_CHANNEL_ID_PROPERTY,               \
                            MBOX_TEST_MESSAGE_ADDRESS);                        \
                                                                               \
        qtest_mbox0_read_message(qts, MBOX_CHANNEL_ID_PROPERTY,                \
                            &mailbox_buffer, sizeof(mailbox_buffer));          \
                                                                               \
        g_assert_cmphex(mailbox_buffer.header.req_resp_code, ==, MBOX_SUCCESS);\
                                                                               \
        g_assert_cmphex(mailbox_buffer.tag.id, ==, TEST_TAG(testname));        \
        g_assert_cmpint(mailbox_buffer.tag.response.size, ==,                  \
                    sizeof(mailbox_buffer.tag.response.value));                \
        g_assert_cmpint(mailbox_buffer.tag.response.success, ==, 1);           \
                                                                               \
        CHECK_FN_NAME(testname, __VA_ARGS__)(&mailbox_buffer.tag);             \
                                                                               \
        qtest_quit(qts);                                                       \
    }                                                                          \
    static void CHECK_FN_NAME(testname, __VA_ARGS__)                           \
                             (TEST_TAG_TYPE(testname) * tag)

/*----------------------------------------------------------------------------*/

#define QTEST_ADD_TEST_CASE(testname, ...)                                     \
    qtest_add_func(stringify(/bcm2838/mbox/property/                           \
                   TEST_FN_NAME(testname, __VA_ARGS__)-test),                  \
                   TEST_FN_NAME(testname, __VA_ARGS__))

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(GET_FIRMWARE_REVISION) {
    g_assert_cmpint(tag->response.value.revision, ==, FIRMWARE_REVISION);
}

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(GET_BOARD_REVISION) {
    g_assert_cmpint(tag->response.value.revision, ==, BOARD_REVISION);
}

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(GET_ARM_MEMORY) {
    g_assert_cmphex(tag->response.value.base, ==, ARM_MEMORY_BASE);
    g_assert_cmphex(tag->response.value.size, ==, ARM_MEMORY_SIZE);
}

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(GET_VC_MEMORY) {
    g_assert_cmphex(tag->response.value.base, ==, VC_MEMORY_BASE);
    g_assert_cmphex(tag->response.value.size, ==, VC_MEMORY_SIZE);
}

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(SET_DEVICE_POWER_STATE) {
    g_assert_cmphex(tag->response.value.device_id, ==, DEVICE_ID_UART0);
    g_assert_cmpint(tag->response.value.enabled, ==, 1);
    g_assert_cmpint(tag->response.value.wait, ==, 0);
}
DECLARE_TEST_CASE_SETUP(SET_DEVICE_POWER_STATE) {
    tag->request.value.device_id = DEVICE_ID_UART0;
    tag->request.value.enabled = 1;
    tag->request.value.wait = 1;
}

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(GET_CLOCK_STATE) {
    g_assert_cmphex(tag->response.value.clock_id, ==, CLOCK_ID_CORE);
    g_assert_cmphex(tag->response.value.enabled, ==, 1);
    g_assert_cmphex(tag->response.value.not_present, ==, 0);
}
DECLARE_TEST_CASE_SETUP(GET_CLOCK_STATE) {
   tag->request.value.clock_id = CLOCK_ID_CORE;
}

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(GET_CLOCK_RATE, EMMC) {
    g_assert_cmphex(tag->response.value.clock_id, ==, CLOCK_ID_EMMC);
    g_assert_cmphex(tag->response.value.rate, ==, CLOCK_RATE_EMMC);
}
DECLARE_TEST_CASE_SETUP(GET_CLOCK_RATE, EMMC) {
    tag->request.value.clock_id = CLOCK_ID_EMMC;
}

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(GET_MAX_CLOCK_RATE, EMMC) {
    g_assert_cmphex(tag->response.value.clock_id, ==, CLOCK_ID_EMMC);
    g_assert_cmphex(tag->response.value.rate, ==, CLOCK_RATE_EMMC);
}
DECLARE_TEST_CASE_SETUP(GET_MAX_CLOCK_RATE, EMMC) {
    tag->request.value.clock_id = CLOCK_ID_EMMC;
}

/*----------------------------------------------------------------------------*/
DECLARE_TEST_CASE(GET_MIN_CLOCK_RATE, EMMC) {
    g_assert_cmphex(tag->response.value.clock_id, ==, CLOCK_ID_EMMC);
    g_assert_cmphex(tag->response.value.rate, ==, CLOCK_RATE_EMMC);
}
DECLARE_TEST_CASE_SETUP(GET_MIN_CLOCK_RATE, EMMC) {
    tag->request.value.clock_id = CLOCK_ID_EMMC;
}

/*----------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    QTEST_ADD_TEST_CASE(GET_FIRMWARE_REVISION);
    QTEST_ADD_TEST_CASE(GET_BOARD_REVISION);
    QTEST_ADD_TEST_CASE(GET_ARM_MEMORY);
    QTEST_ADD_TEST_CASE(GET_VC_MEMORY);
    QTEST_ADD_TEST_CASE(SET_DEVICE_POWER_STATE);
    QTEST_ADD_TEST_CASE(GET_CLOCK_STATE);
    QTEST_ADD_TEST_CASE(GET_CLOCK_RATE, EMMC);
    QTEST_ADD_TEST_CASE(GET_MAX_CLOCK_RATE, EMMC);
    QTEST_ADD_TEST_CASE(GET_MIN_CLOCK_RATE, EMMC);

    return g_test_run();
}
