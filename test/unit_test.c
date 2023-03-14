#include <stdio.h>
#include <string.h>
#include "CuTest.h"
#include "../conf.h"

void test_strtrim(CuTest* cutest){
    char cases[5][100] = {
        "  case 1",
        "case 2  ",
        "case 3",
        " case 4 \n",
        ""
    };

    char ans[5][100] = {
        "case 1",
        "case 2",
        "case 3",
        "case 4",
        ""
    };
    for(int i = 0; i < 5; i++){
        CuAssertStrEquals(cutest, ans[i], strtrim(cases[i]));
    }
}

void test_parse_conf_file(CuTest* cutest);

int main(){
    CuString *output = CuStringNew();
    CuSuite * suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_strtrim);
    CuSuiteRun(suite);
    CuSuiteDetails(suite, output);
    printf("%s", output->buffer);
    return 0;
}
