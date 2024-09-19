#include "app/app.h"
App::App(void) {
    rooms=new std::unordered_map<std::string,Room *>;
}

