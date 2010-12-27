#include "test.h"
#include "assertions.h"
#include "../sha1.h"

#define FIPS1_IN "abc"
#define FIPS2_IN "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"

#define FIPS1_OUT "a9993e36" "4706816a" "ba3e2571" "7850c26c" "9cd0d89d"
#define FIPS2_OUT "84983e44" "1c3bd26e" "baae4aa1" "f95129e5" "e54670f1"
#define FIPS3_OUT "34aa973c" "d4c4daa4" "f61eeb2b" "dbad2731" "6534016f"

void test_sha1_FIPS()
{
	sha1 cksum;
	sha1_ctx ctx;
	int i;

	test("SHA1: FIPS Pub 180-1 test vectors");

	sha1_init(&cksum, NULL);
	sha1_data(FIPS1_IN, strlen(FIPS1_IN), &cksum);
	assert_str_equals("sha1(" FIPS1_IN ")", cksum.hex, FIPS1_OUT);

	sha1_init(&cksum, NULL);
	sha1_data(FIPS2_IN, strlen(FIPS2_IN), &cksum);
	assert_str_equals("sha1(" FIPS2_IN ")", cksum.hex, FIPS2_OUT);

	/* it's hard to define a string constant for 1 mil 'a's... */
	sha1_ctx_init(&ctx);
	for (i = 0; i < 1000000; ++i) {
		sha1_ctx_update(&ctx, (unsigned char *)"a", 1);
	}
	sha1_ctx_final(&ctx, cksum.raw);
	sha1_hexdigest(&cksum);
	assert_str_equals("sha1(<1,000,000 x s>)", cksum.hex, FIPS3_OUT);
}

void test_sha1_init()
{
	sha1 calc;
	sha1 init;

	sha1_init(&calc, NULL);
	assert_str_equals("sha1()", calc.hex, "");
	/* Borrow from FIPS checks */
	sha1_data(FIPS1_IN, strlen(FIPS1_IN), &calc);
	assert_str_equals("sha1(" FIPS1_IN ")", calc.hex, FIPS1_OUT);

	sha1_init(&init, calc.hex);
	assert_str_equals("init.hex == calc.hex", calc.hex, init.hex);

	unsigned int i;
	char buf[256];
	for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
		snprintf(buf, 256, "octet[%i] equality", i);
		assert_int_equals(buf, calc.raw[i], init.raw[i]);
	}
}

void test_suite_sha1()
{
	test_sha1_FIPS();
	test_sha1_init();
}
