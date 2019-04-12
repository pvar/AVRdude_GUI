#include "main.h"

using namespace std;

int main(int argc, char* argv[])
{
        // create application
        Glib::RefPtr<Gtk::Application> application = Gtk::Application::create(argc, argv);

        // set global C++ locale to user-configured locale
        std::locale::global(std::locale(""));

        // create gtkGUI instance
        gtkGUI gui;

        // add callback to activate signal
        application->signal_activate().connect(sigc::mem_fun(gui, &gtkGUI::data_prep));

        // run window
        application->run(*(gui.main_window));

        return EXIT_SUCCESS;
}
