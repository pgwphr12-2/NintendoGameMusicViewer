#include "app/App.h"

#include <iostream>
#include <string>

int main()
{
    App app;
    std::string error;
    if (!app.initialize(error)) {
        std::cerr << "NintendoGameMusicViewer: " << error << '\n';
        return 1;
    }
    const int result = app.run();
    app.shutdown();
    return result;
}
