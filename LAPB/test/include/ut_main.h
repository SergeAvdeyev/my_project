#ifndef UT_MAIN_H
#define UT_MAIN_H

#include <syslog.h>		/* for syslog writing */
#include <time.h>
#include <stdarg.h>
#include <strings.h>
#include <unistd.h>

#include "lapb_iface.h"

struct lapb_cb * lapb_client = NULL;
char str_buf[1024];

char out_data[1024];
char in_data[1024];
int out_data_size = 0;
int in_data_size = 0;


enum {
    TEST_SABM_DTE,	/* Test SABM from DTE */
    TEST_DISC_SABM_DTE,	/* Test DISC from DTE */

    TEST_SABM_DCE,	/* Test SABM from DCE */
    TEST_SABME_DTE,	/* Test SABME from DTE*/
    TEST_SABME_DCE,	/* Test SABME from DTE*/
    TEST_DISC,		/* Test DISC */

    TEST_OK,		/* Finish testing OK*/
    TEST_BAD		/* One of the tests is bad */
};


#define TEST_SABM_DTE_1	    "013F"  /* SABM(DTE) */
#define TEST_SABM_DTE_2	    "01 73" /* UA(DTE) */

#define TEST_SABM_DCE_1	    "033F"  /* SABM(DCE) */
#define TEST_SABM_DCE_2	    "03 73" /* UA(DCE) */

#define TEST_SABME_DTE_1    "013F"  /* SABME(DTE) */
#define TEST_SABME_DTE_2    "01 73" /* UA(DTE) */

#define TEST_SABME_DCE_1    "033F"  /* SABME(DCE) */
#define TEST_SABME_DCE_2    "03 73" /* UA(DCE) */

#define TEST_DISC_DTE_1    "0153"  /* SABM(DTE) */
#define TEST_DISC_DTE_2    "01 73" /* UA(DTE) */

#endif // UT_MAIN_H
