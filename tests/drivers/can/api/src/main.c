/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <can.h>
#include <ztest.h>
#include <strings.h>

/*
 * @addtogroup t_can_driver
 * @{
 * @defgroup t_can_basic test_basic_can
 * @brief TestPurpose: verify can driver works
 * @details
 * - Test Steps
 *   -# Set driver to loopback mode
 *   -# Try to send a message.
 *   -# Attach a filter from every kind
 *   -# Test if message reception is really blocking
 *   -# Send and receive a message with standard id without masking
 *   -# Send and receive a message with extended id without masking
 *   -# Send and receive a message with standard id with masking
 *   -# Send and receive a message with extended id with masking
 *    -# Send and message with different id that should not be received.
 * - Expected Results
 *   -# All tests MUST pass
 * @}
 */

#define TEST_SEND_TIMEOUT    K_MSEC(100)
#define TEST_RECEIVE_TIMEOUT K_MSEC(100)

#define TEST_CAN_STD_ID      0x555
#define TEST_CAN_STD_MASK_ID 0x55A
#define TEST_CAN_STD_MASK    0x7F0
#define TEST_CAN_SOME_STD_ID 0x123

#define TEST_CAN_EXT_ID      0x15555555
#define TEST_CAN_EXT_MASK_ID 0x1555555A
#define TEST_CAN_EXT_MASK    0x1FFFFFF0

CAN_DEFINE_MSGQ(can_msgq, 5);
struct k_sem rx_isr_sem;

struct can_msg test_std_msg = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.std_id  = TEST_CAN_STD_ID,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

struct can_msg test_std_mask_msg = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.std_id  = TEST_CAN_STD_MASK_ID,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

struct can_msg test_ext_msg = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.ext_id  = TEST_CAN_EXT_ID,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

struct can_msg test_ext_mask_msg = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr     = CAN_DATAFRAME,
	.ext_id  = TEST_CAN_EXT_MASK_ID,
	.dlc     = 8,
	.data    = {1, 2, 3, 4, 5, 6, 7, 8}
};

const struct can_filter test_std_filter = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.std_id = TEST_CAN_STD_ID,
	.rtr_mask = 1,
	.std_id_mask = CAN_STD_ID_MASK
};

const struct can_filter test_std_masked_filter = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.std_id = TEST_CAN_STD_ID,
	.rtr_mask = 1,
	.std_id_mask = TEST_CAN_STD_MASK
};

const struct can_filter test_ext_filter = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.ext_id = TEST_CAN_EXT_ID,
	.rtr_mask = 1,
	.ext_id_mask = CAN_EXT_ID_MASK
};

const struct can_filter test_ext_masked_filter = {
	.id_type = CAN_EXTENDED_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.ext_id = TEST_CAN_EXT_ID,
	.rtr_mask = 1,
	.ext_id_mask = TEST_CAN_EXT_MASK
};

const struct can_filter test_std_some_filter = {
	.id_type = CAN_STANDARD_IDENTIFIER,
	.rtr = CAN_DATAFRAME,
	.std_id = TEST_CAN_SOME_STD_ID,
	.rtr_mask = 1,
	.std_id_mask = CAN_STD_ID_MASK
};

static inline void check_msg(struct can_msg *msg1, struct can_msg *msg2,
			     u32_t mask)
{
	int cmp_res;

	zassert_equal(msg1->id_type, msg2->id_type,
		      "ID type does not match");

	zassert_equal(msg1->rtr, msg2->rtr,
		      "RTR bit does not match");

	if (msg2->id_type == CAN_STANDARD_IDENTIFIER) {
		zassert_equal(msg1->std_id | mask, msg2->std_id | mask,
			      "ID does not match");
	} else {
		zassert_equal(msg1->ext_id | mask, msg2->ext_id | mask,
			      "ID does not match");
	}

	zassert_equal(msg1->dlc, msg2->dlc,
		      "DLC does not match");

	cmp_res = memcmp(msg1->data, msg2->data, msg1->dlc);
	zassert_equal(cmp_res, 0, "Received data differ");
}

static void tx_isr(u32_t error_flags)
{

}

static void rx_std_isr(struct can_msg *msg)
{
	check_msg(msg, &test_std_msg, 0);
	k_sem_give(&rx_isr_sem);
}

static void rx_std_mask_isr(struct can_msg *msg)
{
	check_msg(msg, &test_std_msg, 0x0F);
	k_sem_give(&rx_isr_sem);
}

static void rx_ext_isr(struct can_msg *msg)
{
	check_msg(msg, &test_ext_msg, 0);
	k_sem_give(&rx_isr_sem);
}

static void rx_ext_mask_isr(struct can_msg *msg)
{
	check_msg(msg, &test_ext_msg, 0x0F);
	k_sem_give(&rx_isr_sem);
}

static void send_test_msg(struct device *can_dev, struct can_msg *msg)
{
	int ret;

	ret = can_send(can_dev, msg, TEST_SEND_TIMEOUT, NULL);
	zassert_not_equal(ret, CAN_TX_ARB_LOST,
			  "Arbitration though in loopback mode");
	zassert_equal(ret, CAN_TX_OK, "Can't send a message. Err: %d", ret);
}

static void send_test_msg_nowait(struct device *can_dev, struct can_msg *msg)
{
	int ret;

	ret = can_send(can_dev, msg, TEST_SEND_TIMEOUT, tx_isr);
	zassert_not_equal(ret, CAN_TX_ARB_LOST,
			  "Arbitration though in loopback mode");
	zassert_equal(ret, CAN_TX_OK, "Can't send a message. Err: %d", ret);
}

static inline int attach_msgq(struct device *can_dev,
			      const struct can_filter *filter)
{
	int filter_id;

	filter_id = can_attach_msgq(can_dev, &can_msgq, filter);
	zassert_not_equal(filter_id, CAN_NO_FREE_FILTER,
			  "Filter full even for a single one");
	zassert_true((filter_id >= 0), "Negative filter number");

	return filter_id;
}

static inline int attach_isr(struct device *can_dev,
			     const struct can_filter *filter)
{
	int filter_id;

	k_sem_reset(&rx_isr_sem);

	if (filter->id_type == CAN_STANDARD_IDENTIFIER) {
		if (filter->std_id_mask == CAN_STD_ID_MASK) {
			filter_id = can_attach_isr(can_dev, rx_std_isr, filter);
		} else {
			filter_id = can_attach_isr(can_dev, rx_std_mask_isr, filter);
		}
	} else {
		if (filter->ext_id_mask == CAN_EXT_ID_MASK) {
			filter_id = can_attach_isr(can_dev, rx_ext_isr, filter);
		} else {
			filter_id = can_attach_isr(can_dev, rx_ext_mask_isr, filter);
		}
	}

	zassert_not_equal(filter_id, CAN_NO_FREE_FILTER,
			  "Filter full even for a single one");
	zassert_true((filter_id >= 0), "Negative filter number");

	return filter_id;
}

static void send_receive(const struct can_filter *filter, struct can_msg *msg)
{
	struct device *can_dev;
	int ret, filter_id;
	struct can_msg msg_buffer;
	u32_t mask = 0;

	can_dev = device_get_binding(DT_CAN_1_NAME);
	zassert_not_null(can_dev, "Device not not found");

	filter_id = attach_msgq(can_dev, filter);

	send_test_msg(can_dev, msg);

	ret = k_msgq_get(&can_msgq, &msg_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, 0, "Receiving timeout");

	if (filter->id_type == CAN_STANDARD_IDENTIFIER) {
		if (filter->std_id_mask != CAN_STD_ID_MASK) {
			mask = 0x0F;
		}
	} else {
		if (filter->ext_id_mask != CAN_EXT_ID_MASK) {
			mask = 0x0F;
		}
	}

	check_msg(&msg_buffer, msg, mask);

	can_detach(can_dev, filter_id);

	send_test_msg_nowait(can_dev, msg);

	filter_id = attach_isr(can_dev, filter);

	ret = k_sem_take(&rx_isr_sem, TEST_RECEIVE_TIMEOUT);

	can_detach(can_dev, filter_id);
}

/*
 * Set driver to loopback mode
 * The driver stays in loopback mode after that.
 * The controller can now be tested against itself
 */
static void test_set_loopback(void)
{
	struct device *can_dev;
	int ret;

	can_dev = device_get_binding(DT_CAN_1_NAME);
	zassert_not_null(can_dev, "Device not not found");

	ret = can_configure(can_dev, CAN_LOOPBACK_MODE, 0);
	zassert_equal(ret, 0, "Can't set loopback-mode. Err: %d", ret);
}

/*
 * Sending a message to the wild should work because we are in loopback mode
 * and therfor ACK the frame ourselves
 */
static void test_send_and_forget(void)
{
	struct device *can_dev;

	can_dev = device_get_binding(DT_CAN_1_NAME);
	zassert_not_null(can_dev, "Device not not found");

	send_test_msg(can_dev, &test_std_msg);
}

/*
 * Test very basic filter attachment
 * Test each type but only one filter at a time
 */
static void test_filter_attach(void)
{
	struct device *can_dev;
	int filter_id;

	can_dev = device_get_binding(DT_CAN_1_NAME);
	zassert_not_null(can_dev, "Device not not found");

	filter_id = attach_isr(can_dev, &test_std_filter);
	can_detach(can_dev, filter_id);

	filter_id = attach_isr(can_dev, &test_ext_filter);
	can_detach(can_dev, filter_id);

	filter_id = attach_msgq(can_dev, &test_std_filter);
	can_detach(can_dev, filter_id);

	filter_id = attach_msgq(can_dev, &test_ext_filter);
	can_detach(can_dev, filter_id);

	filter_id = attach_isr(can_dev, &test_std_masked_filter);
	can_detach(can_dev, filter_id);

	filter_id = attach_isr(can_dev, &test_ext_masked_filter);
	can_detach(can_dev, filter_id);

	filter_id = attach_msgq(can_dev, &test_std_masked_filter);
	can_detach(can_dev, filter_id);

	filter_id = attach_msgq(can_dev, &test_ext_masked_filter);
	can_detach(can_dev, filter_id);
}

/*
 * Test if a message is received wile was sent.
 */
static void test_receive_timeout(void)
{
	struct device *can_dev;
	int ret, filter_id;
	struct can_msg msg;

	can_dev = device_get_binding(DT_CAN_1_NAME);
	zassert_not_null(can_dev, "Device not not found");

	filter_id = attach_msgq(can_dev, &test_std_filter);

	ret = k_msgq_get(&can_msgq, &msg, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, -EAGAIN, "Got a message without sending it");

	can_detach(can_dev, filter_id);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * Standard identifier
 */
void test_send_receive_std(void)
{
	send_receive(&test_std_filter, &test_std_msg);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout
 * Extended identifier
 */
void test_send_receive_ext(void)
{
	send_receive(&test_ext_filter, &test_ext_msg);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * The message ID is slightly different to the filter but should still
 * because of the mask settind in the filter.
 * Standard identifier
 */
void test_send_receive_std_masked(void)
{
	send_receive(&test_std_masked_filter, &test_std_mask_msg);
}

/*
 * Attach to a filter that should pass the message and send the message.
 * The massage should be received within a small timeout.
 * The message ID is slightly different to the filter but should still
 * because of the mask settind in the filter.
 * Extended identifier
 */
void test_send_receive_ext_masked(void)
{
	send_receive(&test_ext_masked_filter, &test_ext_mask_msg);
}

/*
 * Attach to a filter that should not pass the message and send a message
 * with a different id.
 * The massage should not be received.
 */
static void test_send_receive_wrong_id(void)
{
	struct device *can_dev;
	int ret, filter_id;
	struct can_msg msg_buffer;

	can_dev = device_get_binding(DT_CAN_1_NAME);
	zassert_not_null(can_dev, "Device not not found");

	filter_id = attach_msgq(can_dev, &test_std_filter);

	send_test_msg(can_dev, &test_std_mask_msg);

	ret = k_msgq_get(&can_msgq, &msg_buffer, TEST_RECEIVE_TIMEOUT);
	zassert_equal(ret, -EAGAIN,
		      "Got a message that should not pass the filter");

	can_detach(can_dev, filter_id);
}

void test_main(void)
{
	k_sem_init(&rx_isr_sem, 0, 1);

	ztest_test_suite(can_driver,
			 ztest_unit_test(test_set_loopback),
			 ztest_unit_test(test_send_and_forget),
			 ztest_unit_test(test_filter_attach),
			 ztest_unit_test(test_receive_timeout),
			 ztest_unit_test(test_send_receive_std),
			 ztest_unit_test(test_send_receive_ext),
			 ztest_unit_test(test_send_receive_std_masked),
			 ztest_unit_test(test_send_receive_ext_masked),
			 ztest_unit_test(test_send_receive_wrong_id));
	ztest_run_test_suite(can_driver);
}
