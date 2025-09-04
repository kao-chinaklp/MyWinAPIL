#include "MyWindowX.h"

void test(UINT uMsg){
    if (uMsg==WM_PAINT)
    {
        printf("");
    }
    printf("%d\n", uMsg);    // Do nothing
    fflush(stdout);
}

int main(){
    auto* window=new MyWindowX("default", "test", test);
    window->show(SW_NORMAL);
    Sleep(3000);
    return 0;
}