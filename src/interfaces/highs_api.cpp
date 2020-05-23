#include <interfaces/highs_api.h>

#include <vector>

struct MyThing {
private:
    int property;
    std::vector<MyThingCallback> callbacks;
    // functions are allowed here
};
