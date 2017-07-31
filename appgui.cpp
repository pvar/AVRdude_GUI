#include "appgui.h"

using namespace std;

FuseWidget::FuseWidget()
{
        this->check = nullptr;
        this->combo = nullptr;
        this->reg_label = nullptr;
        this->combo_label = nullptr;
}

CBRecordModel::CBRecordModel()
{
        add(col_name);
        add(col_data);
}

gtkGUI::gtkGUI()
{
        get_executable_path();
        microcontroller = new Micro(exec_path);
        avrdude = new Dude();

        display_warnings = true;

        // connect signals to functions (add "listeners")
        // execution completed
        avrdude->signal_exec_done().connect(sigc::mem_fun(this, &gtkGUI::execution_done) );
        // execution started
        avrdude->signal_exec_started().connect(sigc::mem_fun(this, &gtkGUI::execution_started) );

        // create object builder
        Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();
        try {
                builder->add_from_file(exec_path + "dudegui.ui");
        } catch(const Glib::FileError& ex) {
                cout << "FileError: " << ex.what() << endl;
                return;
        } catch(const Glib::MarkupError& ex) {
                cout << "MarkupError: " << ex.what() << endl;
                return;
        } catch(const Gtk::BuilderError& ex) {
                cout << "BuilderError: " << ex.what() << endl;
                return;
        }

        Gtk::Button *btn_firm_read, *btn_firm_write, *btn_firm_verify;
        Gtk::Button *btn_erom_read, *btn_erom_write, *btn_erom_verify;
        Gtk::Button *btn_open_eeprom, *btn_open_flash;
        Gtk::TextView *tv_dude_output;

        // use builder to instantiate GTK widgets
        builder->get_widget("dude_auto_erase", auto_erase);
        builder->get_widget("dude_auto_verify", auto_verify);
        builder->get_widget("dude_auto_check", auto_check);
        builder->get_widget("main_window", main_window);
        builder->get_widget("combo_family", cb_family);
        builder->get_widget("combo_device", cb_device);
        builder->get_widget("combo_protocol", cb_protocol);
        builder->get_widget("signature_test", btn_check_sig);
        builder->get_widget("erase_device", btn_erase_dev);
        builder->get_widget("dev_xml_file", lbl_spec_xml);
        builder->get_widget("dev_mem_flash", lbl_spec_flash);
        builder->get_widget("dev_mem_sram", lbl_spec_sram);
        builder->get_widget("dev_mem_eeprom", lbl_spec_eeprom);
        builder->get_widget("dev_max_speed", lbl_spec_speed);
        builder->get_widget("dev_signature", lbl_signature);
        builder->get_widget("fuse_parameters", lbl_fusebytes);
        builder->get_widget("fuse_settings_grid", fuse_grid);
        builder->get_widget("fw_flash_box", box_flash_ops);
        builder->get_widget("fw_eeprom_box", box_eeprom_ops);
        builder->get_widget("avrdude_output", tv_dude_output);
        builder->get_widget("avrdude_command", lbl_dude_command);
        builder->get_widget("open_eeprom", btn_open_eeprom);
        builder->get_widget("open_flash", btn_open_flash);
        builder->get_widget("read_erom", btn_erom_read);
        builder->get_widget("write_erom", btn_erom_write);
        builder->get_widget("verify_eeprom", btn_erom_verify);
        builder->get_widget("read_flash", btn_firm_read);
        builder->get_widget("write_flash", btn_firm_write);
        builder->get_widget("verify_flash", btn_firm_verify);
        builder->get_widget("eeprom_file", ent_eeprom_file);
        builder->get_widget("flash_file", ent_flash_file);
        builder->get_widget("write_fuses", btn_fuse_write);
        builder->get_widget("default_fuses", btn_fuse_def);
        builder->get_widget("read_fuses", btn_fuse_read);

        // create empty text buffer and assign to text view
        dude_output_buffer = Gtk::TextBuffer::create();
        tv_dude_output->set_buffer(dude_output_buffer);

        // set custom style provider for application window
        Glib::ustring data = ".console {font: Monospace 9; color: #a88A85;}"
                             ".fuse_value {font: Monospace 9; color: #888A85;}"
                             ".fuse_name {font-weight: bold; color: #888A85;}";
        Glib::RefPtr<Gtk::CssProvider> css = Gtk::CssProvider::create();
        if (not css->load_from_data(data)) {
                cerr << "Failed to load css!\n";
                exit(1);
        }
        Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
        Glib::RefPtr<Gtk::StyleContext> win_context = main_window->get_style_context();
        win_context->add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_USER);

        // add style-class to textview displaying execution output
        Glib::RefPtr<Gtk::StyleContext> context;
        context = tv_dude_output->get_style_context();
        context->add_class("console");
        // add style-class to label displaying selected fuse values
        context = lbl_fusebytes->get_style_context();
        context->add_class("fuse_value");

        // create the tree-models
        tm_family = Gtk::ListStore::create(cbm_generic);
        tm_device = Gtk::ListStore::create(cbm_generic);
        tm_port = Gtk::ListStore::create(cbm_generic);
        tm_protocol = Gtk::ListStore::create(cbm_generic);

        // assign tree-models to combo-boxes
        cb_family->set_model(tm_family);
        cb_device->set_model(tm_device);
        cb_protocol->set_model(tm_protocol);

        // define visible columns
        cb_family->pack_start(cbm_generic.col_name);
        cb_device->pack_start(cbm_generic.col_name);
        cb_protocol->pack_start(cbm_generic.col_name);

        // add microcontroller families and programmer names to tree-models
        populate_static_treemodels();

        // connect signal handlers
        main_window->signal_delete_event().connect(sigc::mem_fun(*this, &gtkGUI::exit_application));
        btn_open_flash->signal_clicked().connect( sigc::bind<file_op>( sigc::mem_fun(*this, &gtkGUI::select_file), open_f) );
        btn_open_eeprom->signal_clicked().connect( sigc::bind<file_op>( sigc::mem_fun(*this, &gtkGUI::select_file), open_e) );
        btn_erom_read->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_eeprom_read));
        btn_erom_write->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_eeprom_write));
        btn_erom_verify->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_eeprom_verify));
        btn_firm_read->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_flash_read));
        btn_firm_write->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_flash_write));
        btn_firm_verify->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_flash_verify));
        btn_fuse_write->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_fuse_write));
        btn_fuse_read->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_fuse_read));
        btn_fuse_def->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_fuse_default));
        btn_check_sig->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_check_signature));
        btn_erase_dev->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_erase_devive));
        cb_family->signal_changed().connect(sigc::mem_fun(*this, &gtkGUI::cb_new_family));
        dev_combo_signal = cb_device->signal_changed().connect(sigc::mem_fun(*this, &gtkGUI::cb_new_device));
        dev_combo_programmer = cb_protocol->signal_changed().connect(sigc::mem_fun(*this, &gtkGUI::cb_dude_settings));
        check_button_erase = auto_erase->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_dude_settings));
        check_button_check = auto_check->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_dude_settings));
        check_button_verify = auto_verify->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::cb_dude_settings));
}

gtkGUI::~gtkGUI()
{
}

bool gtkGUI::exit_application (GdkEventAny* any_event)
{
        if (avrdude->working) {
                //cout << "avrdude is still working...!" << endl;
                message_popup ("Sorry...", "The programm cannot exit at this moment; Avrdude is currently performing an operation on your device...");
                return true;
        }

        cout << "Bye bye!" << endl;
        return false;
}

void gtkGUI::populate_static_treemodels (void)
{
        Gtk::TreeModel::Row row;
        // populate tree-model with device families
        //row = *(tm_family->append());
        //row[cbm_generic.col_name] = "AT Xmega";
        //row[cbm_generic.col_data] = "ATxmega";
        row = *(tm_family->append());
        row[cbm_generic.col_name] = "AT mega";
        row[cbm_generic.col_data] = "ATmega";
        row = *(tm_family->append());
        row[cbm_generic.col_name] = "AT tiny";
        row[cbm_generic.col_data] = "ATtiny";
        row = *(tm_family->append());
        row[cbm_generic.col_name] = "AT 90S xxxx";
        row[cbm_generic.col_data] = "AT90S";
        row = *(tm_family->append());
        row[cbm_generic.col_name] = "AT 90 USB";
        row[cbm_generic.col_data] = "AT90USB";
        row = *(tm_family->append());
        row[cbm_generic.col_name] = "AT 90 CAN";
        row[cbm_generic.col_data] = "AT90CAN";
        row = *(tm_family->append());
        row[cbm_generic.col_name] = "AT 90 PWM";
        row[cbm_generic.col_data] = "AT90PWM";
        cb_family->set_active(0);

        // populate tree-model with supported protocols
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "USBasp programmer";
        row[cbm_generic.col_data] = "usbasp";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "\"usbtiny\" type programmer";
        row[cbm_generic.col_data] = "usbtiny";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Arduino programmer";
        row[cbm_generic.col_data] = "arduino";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel STK200";
        row[cbm_generic.col_data] = "stk200";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel STK500";
        row[cbm_generic.col_data] = "stk500";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel STK500 HV Serial";
        row[cbm_generic.col_data] = "stk500hvsp";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel STK500 Parallel";
        row[cbm_generic.col_data] = "stk500pp";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel STK600";
        row[cbm_generic.col_data] = "stk600";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel STK600 HV Serial";
        row[cbm_generic.col_data] = "stk600hvsp";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel STK600 Parallel";
        row[cbm_generic.col_data] = "stk600pp";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel Butterfly";
        row[cbm_generic.col_data] = "butterfly";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel AVR ISP";
        row[cbm_generic.col_data] = "avrisp";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel AVR ISP mkII ";
        row[cbm_generic.col_data] = "avrisp2";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel AVR ISP V2";
        row[cbm_generic.col_data] = "avrispv2";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel AppNote AVR109 Boot Loader";
        row[cbm_generic.col_data] = "avr109";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel Low Cost Serial Programmer";
        row[cbm_generic.col_data] = "avr910";
        row = *(tm_protocol->append());
        row[cbm_generic.col_name] = "Atmel AppNote AVR911 AVROSP";
        row[cbm_generic.col_data] = "avr911";
        cb_protocol->set_active(0);
}

void gtkGUI::get_executable_path (void)
{
        pid_t pid = getpid();
        gchar pid_str[20] = {0};
        sprintf(pid_str, "%d", pid);

        Glib::ustring path = "";
        Glib::ustring _link = "/proc/";
        _link.append( pid_str );
        _link.append( "/exe");

        char proc[512];
        int chars_read = readlink(_link.c_str(), proc, 512);
        if (chars_read != -1) {
                proc[chars_read] = 0;
                path = proc;
                Glib::ustring::size_type len = path.find_last_of("/");
                path = path.substr(0, len + 1);
        }

        this->exec_path = path;
}

void gtkGUI::data_prep (void)
{
        Glib::signal_timeout().connect( sigc::mem_fun(*this, &gtkGUI::data_prep_start), 100 );
}


void gtkGUI::prep_fuse_meta (void)
{
        string warning;
        gint count = 0;
        // count fuse bytes with default values
        for (int i = 0; i < 3; i++)
                if (microcontroller->description->fusebytes_default[i] != 255)
                        count++;
        if (count == 0)
                warning.append("The device description does not specify default fuse settings.\n\n");

        // check if fuse warnings have been specified
        if (microcontroller->description->warnings->size() == 0)
                warning.append("The device description provides no warnings, regarding configuration of fuse settings.");

        // display messages, if any...
        if (!warning.empty())
                message_popup ("Warning!", warning);

        // copy default fuse values over device current fuse values
        for (int i = 0; i < 3; i++)
                avrdude->fusebytes_ondevice[i] = microcontroller->description->fusebytes_default[i];
}

bool gtkGUI::data_prep_start (void)
{
        //cout << "DATA PREPARATION..." << endl;

        // get supported devices
        microcontroller->get_device_list();
        // check if device-to-xml map has been populated
        if (microcontroller->device_map == nullptr) {
                // war user if device-to-xml map is still empty
                message_popup ("Failure!", "Program failed to locate directory with XML files "
                                           "or configuration file with device to XML mapping." );
        } else {
                // update device combo box
                this->cb_new_family();
        }
        // do not repeat timer
        return FALSE;
}

void gtkGUI::cb_new_family (void)
{
        Gtk::TreeModel::Row row;

        // get selected family
        Glib::ustring family;
        Gtk::TreeModel::iterator selected_family = cb_family->get_active();
        if (selected_family) {
                row = *selected_family;
                if (row) {
                        family = row[cbm_generic.col_data];
                        //cout << "selected family: " << family << endl;
                }
        }

        // exit if device_map is not yet populated
        if (microcontroller->device_map == nullptr)
                return;

        // block on-change signals
        dev_combo_signal.block(true);
        dev_combo_programmer.block(true);
        // lock controls and reset settings
        controls_lock();
        device_data_clear();
        // clear device tree-view
        tm_device->clear();
        // insert family members
        row = *(tm_device->append());
        row[cbm_generic.col_name] = "None";
        row[cbm_generic.col_data] = "";
        map<string, string>::iterator iter;
        for (iter = microcontroller->device_map->begin(); iter != microcontroller->device_map->end(); ++iter) {
                // ignore devices with irrelevant names
                string name_part = iter->first.substr (0, family.size());
                if (name_part != family)
                        continue;
                // add new entry to treeview of device combo box
                row = *(tm_device->append());
                row[cbm_generic.col_name] = iter->first;
                row[cbm_generic.col_data] = iter->second;
        }
        // set default selected entry
        cb_device->set_active(0);
        // unblock on-change signals
        dev_combo_signal.unblock();
        dev_combo_programmer.unblock();
}

void gtkGUI::cb_new_device (void)
{
        // get selected device
        Glib::ustring device;
        Gtk::TreeModel::iterator selected_device = cb_device->get_active();
        if (selected_device) {
                Gtk::TreeModel::Row row = *selected_device;
                if (row) {
                        device = row[cbm_generic.col_data];
                        //cout << "selected device: " << device << endl;
                }
        }

        // lock controls and reset settings
        controls_lock();
        device_data_clear();
        // do not proceed if "None" device
        if (device.size() < 1)
                return;
        // prepare data for selected device
        microcontroller->parse_data(device);
        // prepare default settings and check if warnings were found
        prep_fuse_meta();
        // copy default fuse values over device current fuse values
        for (int i = 0; i < 3; i++)
                avrdude->fusebytes_ondevice[i] = microcontroller->description->fusebytes_default[i];
        // unlock controls and update labels
        controls_unlock();
        display_warnings = false;
        device_data_show();
        process_fuse_values();
        display_warnings = true;
}

void gtkGUI::controls_lock (void)
{
        // disable avrdude operations
        btn_check_sig->set_sensitive(false);
        btn_erase_dev->set_sensitive(false);
        btn_fuse_read->set_sensitive(false);
        btn_fuse_write->set_sensitive(false);
        btn_fuse_def->set_sensitive(false);
        box_flash_ops->set_sensitive(false);
        box_eeprom_ops->set_sensitive(false);
}

void gtkGUI::controls_unlock (void)
{
        // enable avrdude operations
        btn_check_sig->set_sensitive(true);
        btn_erase_dev->set_sensitive(true);
        btn_fuse_read->set_sensitive(true);
        btn_fuse_write->set_sensitive(true);
        btn_fuse_def->set_sensitive(true);
        box_flash_ops->set_sensitive(true);
        if (microcontroller->description->eeprom_exists)
                box_eeprom_ops->set_sensitive(true);
}

void gtkGUI::device_data_clear (void)
{
        // clear old labels and fuses
        display_fuse_settings(false);
        display_specifiactions(false);
        // reset avrdude settings
        auto_verify->set_active(true);
        auto_erase->set_active(true);
        auto_check->set_active(true);
        // update object settings
        cb_dude_settings ();
}

void gtkGUI::device_data_show (void)
{
        // display specifications
        display_specifiactions(true);
        // display fuse settings
        display_fuse_settings(true);
        // display fuse bytes
        display_fuse_values();
}

void gtkGUI::cb_dude_settings (void)
{
        gboolean auto_erase_flag, auto_verify_flag, auto_check_flag;
        Glib::ustring microcontroller, programmer;
        Gtk::TreeModel::iterator selection;

        // get selected microcontroller
        selection = cb_device->get_active();
        if (selection) {
                Gtk::TreeModel::Row row = *selection;
                if (row)
                        microcontroller = row[cbm_generic.col_name];
        }

        // do not proceed if invalid or no microcontroller was selected
        if ((microcontroller.size() < 1) || (microcontroller == "None"))
                return;

        // get selected programmer
        selection = cb_protocol->get_active();
        if (selection) {
                Gtk::TreeModel::Row row = *selection;
                if (row)
                        programmer = row[cbm_generic.col_data];
        }

        // do not proceed if invalid programmer was selected
        if (programmer.size() < 1)
                return;

        // read checkbuttons' state
        if (auto_erase->get_active())
                auto_erase_flag = true;
        else
                auto_erase_flag = false;

        if (auto_verify->get_active())
                auto_verify_flag = true;
        else
                auto_verify_flag = false;

        if (auto_check->get_active())
                auto_check_flag = true;
        else
                auto_check_flag = false;

        avrdude->setup( auto_erase_flag, auto_verify_flag, auto_check_flag, programmer, microcontroller );
}

void gtkGUI::cb_check_signature (void)
{
        // read signature from device
        avrdude->do_read_signature();
        // display console-output and executed command
        update_console_view();
        // get actuall signature from processed output
        Glib::ustring actual_signature = avrdude->processed_output;
        // get expected signature from specifications
        Glib::ustring selected_signature = microcontroller->description->signature.substr(2,7);
        // check execution outcome for errors
        if (avrdude->execution_status == Dude::exec_status::no_error) {
                // check for signature match
                if (actual_signature.uppercase() == selected_signature)
                        message_popup ("Success!", "Matching device detected.");
                else
                        message_popup ("Failure!", "Unexpected device signature.");
        } else {
                // display appropriate error message
                execution_outcome(false);
        }
}

void gtkGUI::cb_erase_devive (void)
{
        // execute chip-erase
        avrdude->do_clear_device();

}

void gtkGUI::display_specifiactions (gboolean have_specs)
{
        if (have_specs) {
                Glib::ustring xmlfile;
                xmlfile = microcontroller->description->xml_filename;
                xmlfile.append(" (build ");
                xmlfile.append(microcontroller->description->xml_version);
                xmlfile.append(")");
                lbl_spec_flash->set_label(microcontroller->description->flash_size);
                lbl_spec_eeprom->set_label(microcontroller->description->eeprom_size);
                lbl_spec_sram->set_label(microcontroller->description->sram_size);
                lbl_spec_speed->set_label(microcontroller->description->max_speed);
                lbl_signature->set_label(microcontroller->description->signature);
                lbl_spec_xml->set_label(xmlfile);
        } else {
                lbl_spec_flash->set_label("NA");
                lbl_spec_eeprom->set_label("NA");
                lbl_spec_sram->set_label("NA");
                lbl_spec_speed->set_label("NA");
                lbl_spec_xml->set_label("NA");
                lbl_signature->set_label("0x000000");
        }
}
void gtkGUI::clear_fuse_widget (FuseWidget* settings_widget)
{
        if (settings_widget->combo) {
                (settings_widget->callback)->disconnect();
                (settings_widget->combo_label)->hide();
                (settings_widget->combo)->hide();
                (settings_widget->model)->clear();
                delete settings_widget->combo_label;
                delete settings_widget->combo;
                fuse_grid->remove_row(0);
                fuse_grid->remove_row(0);
        } else {
                if (settings_widget->check) {
                        (settings_widget->callback)->disconnect();
                        (settings_widget->check)->hide();
                        delete settings_widget->check;
                } else {
                        (settings_widget->reg_label)->hide();
                        delete settings_widget->reg_label;
                }
                fuse_grid->remove_row(0);
        }
}

void gtkGUI::display_fuse_settings (gboolean have_fuses)
{
        if (!have_fuses) {
                // return if widget-list is uninitialized
                if (fuse_tab_widgets == nullptr)
                        return;

                // loop through fuse-widget list: hide and delete corresponding widgets
                list<FuseWidget>::iterator iter;
                for (iter = fuse_tab_widgets->begin(); iter != fuse_tab_widgets->end(); ++iter)
                        clear_fuse_widget(&(*iter));

                // delete fuse-widget list and "mark" as empty
                delete fuse_tab_widgets;
                fuse_tab_widgets = nullptr;
                return;
        }

        // used to populate tree-models
        Gtk::TreeModel::Row row;
        // index to next grid-line
        guint grid_line = 1;
        // create list of fuse-widgets
        fuse_tab_widgets = new list<FuseWidget>;
        // loop through fuse-options: create and display corresponding widgets
        list<DeviceDescription::FuseSetting>::iterator iter;
        for (iter = (microcontroller->description->fuse_settings)->begin(); iter != (microcontroller->description->fuse_settings)->end(); ++iter) {
                // create a new instance of FuseWidget
                FuseWidget *widget_entry = new FuseWidget;
                // create new instance of signal connection
                widget_entry->callback = new sigc::connection;
                // decide what kind of widget is needed
                if ((*iter).single_option) {
                        // check if "single setting" or "register name"
                        if ((*iter).offset == 512) {
                                // REGISTER NAME
                                // create pointer to label
                                widget_entry->reg_label = new Gtk::Label((*iter).fdesc);
                                // set align, indent and width of the widgets
                                (widget_entry->reg_label)->set_halign(Gtk::Align::ALIGN_START);
                                (widget_entry->reg_label)->set_margin_start(5);
                                (widget_entry->reg_label)->set_margin_top(20);
                                (widget_entry->reg_label)->set_margin_bottom(12);
                                // add style-class to label
                                Glib::RefPtr<Gtk::StyleContext> context = (widget_entry->reg_label)->get_style_context();
                                context->add_class("fuse_name");
                                // put label in grid
                                fuse_grid->attach(*(widget_entry->reg_label), 0, grid_line++, 1, 1);
                                // show widget
                                (widget_entry->reg_label)->show();
                        } else {
                                // SINGLE OPTION
                                // create pointer to checkbutton
                                widget_entry->check = new Gtk::CheckButton((*iter).fdesc);
                                // put checkbutton in grid
                                fuse_grid->attach(*(widget_entry->check), 0, grid_line++, 1, 1);
                                // show widget
                                (widget_entry->check)->show();
                                // add callback for on_change event
                                *(widget_entry->callback) = (widget_entry->check)->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::calculate_fuse_values));
                        }
                } else {
                        // OPTION SELECTION
                        // create pointer to treemodel
                        widget_entry->model = Gtk::ListStore::create(cbm_generic);
                        // prepare to loop through the enumerator members
                        list<DeviceDescription::OptionEntry>* this_enum_list = (*(microcontroller->description)->option_lists)[(*iter).fenum];
                        list<DeviceDescription::OptionEntry>::iterator enum_iter = this_enum_list->begin();
                        // get value from first entry (this is a pseudo entry with the maximum value of all entries)
                        widget_entry->max_value = enum_iter->value;
                        // proceed to next entry
                        enum_iter++;
                        // loop through the rest of the entries...
                        for (; enum_iter != this_enum_list->end(); ++enum_iter) {
                                row = *(widget_entry->model->append());
                                // add setting description
                                row[cbm_generic.col_name] = enum_iter->ename;
                                // add setting values
                                row[cbm_generic.col_data] = to_string(enum_iter->value);
                        }
                        // create pointer to label
                        widget_entry->combo_label = new Gtk::Label((*iter).fdesc);
                        // create pointer to combobox
                        widget_entry->combo = new Gtk::ComboBox();
                        // assign treemodel to combobox
                        (widget_entry->combo)->set_model(widget_entry->model);
                        // define visible columns
                        (widget_entry->combo)->pack_start(cbm_generic.col_name);
                        // set align, indent and width of the widgets
                        (widget_entry->combo_label)->set_halign(Gtk::Align::ALIGN_START);
                        (widget_entry->combo_label)->set_margin_start(24);
                        (widget_entry->combo)->set_margin_start(24);
                        // put widgets in grid
                        fuse_grid->attach(*(widget_entry->combo_label), 0, grid_line++, 1, 1);
                        fuse_grid->attach(*(widget_entry->combo), 0, grid_line++, 1, 1);
                        // select first entry
                        (widget_entry->combo)->set_active(0);
                        // show widgets
                        (widget_entry->combo_label)->show();
                        (widget_entry->combo)->show();
                        // add callback for on_change event
                        *(widget_entry->callback) = (widget_entry->combo)->signal_changed().connect(sigc::mem_fun(*this, &gtkGUI::calculate_fuse_values));
                }
                // copy bit-mask and fuse-byte offset
                widget_entry->bitmask = (*iter).fmask;
                widget_entry->bytenum = (*iter).offset;
                // put widget_entry in fuse_tab_widgets
                fuse_tab_widgets->push_back(*widget_entry);
        }
}

void gtkGUI::calculate_fuse_values ()
{
        /*
        How to calculate fuse-bytes from settings:

        1) loop through fuse-widgets...
                a) if it's a multiple (combo box) setting:
                        factor = bitmask / MAX(enum_value)
                        final_value = selected_value * factor
                        final_value = final_value XOR bitmask
                b) if it's a simple (check button) setting:
                        final_value = bitmask

        2) fuse-byte value = get final_values of releated settings and OR them
        */

        // clear fuse-byte values
        microcontroller->fusebytes_custom[0] = 0;
        microcontroller->fusebytes_custom[1] = 0;
        microcontroller->fusebytes_custom[2] = 0;

        Gtk::TreeModel::Row selected_row;
        Gtk::TreeModel::iterator selected;

        // loop through fuse widgets and get state
        list<FuseWidget>::iterator fwidget = fuse_tab_widgets->begin();
        for (fwidget++; fwidget != fuse_tab_widgets->end(); ++fwidget) {
                // make sure it is related to a valid fuse-byte
                if ((fwidget->bytenum >= 0) && (fwidget->bytenum <= 2)) {
                        // check if combo-box or check button
                        if (fwidget->combo) {
                                selected = (fwidget->combo)->get_active();
                                selected_row = *selected;
                                guint selected_value = atoi(string((selected_row[cbm_generic.col_data])).c_str());
                                guint final_value = (selected_value * fwidget->bitmask) / fwidget->max_value;
                                microcontroller->fusebytes_custom[fwidget->bytenum] |= (final_value ^ fwidget->bitmask);
                        } else {
                                if ((fwidget->check)->get_active())
                                        microcontroller->fusebytes_custom[fwidget->bytenum] |= fwidget->bitmask;
                        }
                }
        }

        // negate calculated values (they are expected this way)
        microcontroller->fusebytes_custom[0] ^= 255;
        microcontroller->fusebytes_custom[1] ^= 255;
        microcontroller->fusebytes_custom[2] ^= 255;

        // check if warnings are enabled or not
        if (display_warnings)
                display_fuse_warnings();

        // display fuse bytes
        display_fuse_values();
}

void gtkGUI::process_fuse_values (void)
{
        /*
        How to find fuse-settings from fuse-byte values:
        1) get fuse byte values
        2) loop through fuse-widgets...
                a) If it's a simple (check button) setting:
                        get relative fuse-byte value
                        tmp_value = fuse-byte XOR bitmask
                        tmp_value = fuse-byte AND bitmask
                        selected_value = (tmp_value * MAX(enum_value)) / bitmask
                        display option with selected_value

                b) if it's a simple (check button) setting:
                        get relative fuse-byte value
                        tmp_result = fuse-byte AND bitmask
                        if tmp_result NOT zero
                                setting -> active
                        else
                                setting -> not active
        */

        // get fuse values from device
        gint tmp_fusebytes[3];
        for (int i = 0; i < 3; i++)
                tmp_fusebytes[i] = avrdude->fusebytes_ondevice[i];

        // loop through fuse widgets and set state
        guint temp_value = 0;
        guint selected_value = 0;
        list<FuseWidget>::iterator fwidget = fuse_tab_widgets->begin();
        for (fwidget++; fwidget != fuse_tab_widgets->end(); ++fwidget) {
                // make sure it is related to a valid fuse-byte
                if ((fwidget->bytenum >= 0) && (fwidget->bytenum <= 2)) {
                        // check if combo-box or check button
                        if (fwidget->combo) {
                                temp_value = tmp_fusebytes[fwidget->bytenum] ^ fwidget->bitmask;
                                temp_value = tmp_fusebytes[fwidget->bytenum] & fwidget->bitmask;
                                selected_value = (temp_value * fwidget->max_value) / fwidget->bitmask;
                                // iterate through treemodel entries, tofind the one with selected_value
                                Glib::RefPtr<Gtk::TreeModel> refModel = (fwidget->combo)->get_model();
                                Gtk::TreeModel::Children children = refModel->children();
                                Gtk::TreeModel::Children::iterator iter;
                                gint index = 0;
                                for(iter = children.begin(); iter != children.end(); ++iter) {
                                        index++;
                                        Gtk::TreeModel::Row row = *iter;
                                        guint this_value = atoi(string(row[cbm_generic.col_data]).c_str());
                                        if (this_value == selected_value)
                                                break;
                                }
                                //cout << "selected entry: " << index - 1 << endl;
                                (fwidget->combo)->set_active(index - 1);
                        } else {
                                guint temp_value = tmp_fusebytes[fwidget->bytenum] & fwidget->bitmask;
                                if (temp_value) {
                                        //cout << "OFF" << endl;
                                        (fwidget->check)->set_active(false); // fuse values are negated
                                } else {
                                        //cout << "ON" << endl;
                                        (fwidget->check)->set_active(true); // fuse values are negated
                                }
                        }
                }
        }
}

void gtkGUI::display_fuse_warnings (void)
{
        gboolean warnings_exist = false;
        Glib::ustring warnings_text;

        // loop through warnings list
        list<DeviceDescription::FuseWarning>::iterator iter;
        for (iter = (microcontroller->description->warnings)->begin(); iter != (microcontroller->description->warnings)->end(); iter++) {
                // check if warning applies...
                guint check_result = microcontroller->fusebytes_custom[iter->fbyte] & iter->fmask;
                if (check_result == iter->fresult) {
                        warnings_exist = true;
                        warnings_text += iter->warning;
                        warnings_text += "\n";
                }
        }

        if (warnings_exist)
                message_popup("Warning!", warnings_text);
}

void gtkGUI::display_fuse_values ()
{
        stringstream converter_stream;

        converter_stream << "LOW: 0x";
        converter_stream << hex << setw(2) << setfill('0') << microcontroller->fusebytes_custom[0];
        converter_stream << dec << " (" << microcontroller->fusebytes_custom[0] << ")";

        converter_stream << "    HIGH: 0x";
        converter_stream << hex << setw(2) << setfill('0') << microcontroller->fusebytes_custom[1];
        converter_stream << dec << " (" << microcontroller->fusebytes_custom[1] << ")";

        converter_stream << "    EXTENDED: 0x";
        converter_stream << hex << setw(2) << setfill('0') << microcontroller->fusebytes_custom[2];
        converter_stream << dec << " (" << microcontroller->fusebytes_custom[2] << ")";

        lbl_fusebytes->set_label(converter_stream.str());
}

void gtkGUI::select_file(file_op action)
{
        if (browser != nullptr)
                delete browser;

        // initialize file-chooser dialog
        if ((action == open_f) || (action == open_e))
                browser = new Gtk::FileChooserDialog("Choose firmware file to read from", Gtk::FILE_CHOOSER_ACTION_OPEN);
        else
                browser = new Gtk::FileChooserDialog("Choose or create file to write to", Gtk::FILE_CHOOSER_ACTION_SAVE);

        // prepare file browser popup
        browser->set_transient_for(*main_window);
        Gtk::Box* fb_box = browser->get_vbox();
        fb_box->set_margin_left (8);
        fb_box->set_margin_right (8);
        fb_box->set_margin_top (8);
        fb_box->set_margin_bottom (8);

        // add filters for certain file types
        Glib::RefPtr<Gtk::FileFilter> filter_all = Gtk::FileFilter::create();
        filter_all->set_name("All files");
        filter_all->add_pattern("*");
        browser->add_filter(filter_all);
        Glib::RefPtr<Gtk::FileFilter> filter_hex = Gtk::FileFilter::create();
        filter_hex->set_name("*.hex files");
        filter_hex->add_pattern("*.hex");
        browser->add_filter(filter_hex);
        Glib::RefPtr<Gtk::FileFilter> filter_bin = Gtk::FileFilter::create();
        filter_bin->set_name("*.bin files");
        filter_bin->add_pattern("*.bin");
        browser->add_filter(filter_bin);
        Glib::RefPtr<Gtk::FileFilter> filter_elf = Gtk::FileFilter::create();
        filter_elf->set_name("*.elf files");
        filter_elf->add_pattern("*.elf");
        browser->add_filter(filter_elf);
        Glib::RefPtr<Gtk::FileFilter> filter_obj = Gtk::FileFilter::create();
        filter_obj->set_name("*.o files");
        filter_obj->add_pattern("*.o");
        browser->add_filter(filter_obj);
        Glib::RefPtr<Gtk::FileFilter> filter_txt = Gtk::FileFilter::create();
        filter_txt->set_name("*.txt files");
        filter_txt->add_pattern("*.txt");
        browser->add_filter(filter_txt);

        // add buttons and responses
        browser->add_button("Cancel", Gtk::RESPONSE_CANCEL);
        browser->add_button("Select", Gtk::RESPONSE_OK);

        // open browser and close when a response is received
        int result = browser->run();
        browser->close();

        // check signal from file-chooser
        switch(result) {
                // get filename and put it in relevant entry
                case(Gtk::RESPONSE_OK): {
                        //cout << "RESPONSE OK" << endl;
                        string filename = browser->get_filename();
                        if (action == open_f)
                                ent_flash_file->set_text(filename);
                        else if (action == open_e)
                                ent_eeprom_file->set_text(filename);
                        break;
                }
                // do nothing...
                case(Gtk::RESPONSE_CANCEL): {
                        //cout << "RESPONSE CANCEL" << endl;
                        break;
                }
                // do nothing...
                default: {
                        //cout << "NO RESPONSE" << endl;
                        break;
                }
        }
}

void gtkGUI::cb_eeprom_read(void)
{
        // select or create file to save to
        select_file(save_e);
        // get file from file-chooser
        string filename = browser->get_filename();
        // warn and exit if no file specified
        if (filename.empty() == true)
                return;
        // read eeprom memory
        avrdude->do_eeprom_read(filename);

}

void gtkGUI::cb_eeprom_write(void)
{
        // get file from relevant entry
        Glib::ustring filename = ent_eeprom_file->get_text();
        // warn and exit if no file specified
        if (filename.empty() == true) {
                return;
        }
        // write eeprom memory
        avrdude->do_eeprom_write(filename);

}

void gtkGUI::cb_eeprom_verify(void)
{
        // get file from relevant entry
        Glib::ustring filename = ent_eeprom_file->get_text();
        // warn and exit if no file specified
        if (filename.empty() == true) {
                return;
        }
        // verify flash memory
        avrdude->do_eeprom_verify(filename);
}

void gtkGUI::cb_flash_read(void)
{
        // select or create file to save to
        select_file(save_f);
        // get file from file-chooser
        string filename = browser->get_filename();
        // exit if no file specified
        if (filename.empty() == true)
                return;
        // read flash memory
        avrdude->do_flash_read(filename);
}

void gtkGUI::cb_flash_write(void)
{
        // get file from relevant entry
        Glib::ustring filename = ent_flash_file->get_text();
        // warn and exit if no file specified
        if (filename.empty() == true) {
                return;
        }
        // write flash memory
        avrdude->do_flash_write(filename);
}

void gtkGUI::cb_flash_verify(void)
{
        // get file from relevant entry
        Glib::ustring filename = ent_flash_file->get_text();
        // warn and exit if no file specified
        if (filename.empty() == true) {
                return;
        }
        // verify flash memory
        avrdude->do_flash_verify(filename);
}

void gtkGUI::cb_fuse_write(void)
{
        // write fuse bytes
        avrdude->do_fuse_write (microcontroller->description->fusebytes_count,
                                microcontroller->fusebytes_custom[0],
                                microcontroller->fusebytes_custom[1],
                                microcontroller->fusebytes_custom[2]);
}

void gtkGUI::cb_fuse_read(void)
{
        // read fuse bytes
        avrdude->do_fuse_read(microcontroller->description->fusebytes_count);
        // display console-output and executed command
        update_console_view();
        // check execution outcome and display proper message
        execution_outcome(true);
        // exit if outcome NOT successful
        if (avrdude->execution_status != Dude::exec_status::no_error)
                return;
        // apply fuse values read from device on fuse-widgets
        display_warnings = false;
        process_fuse_values();
        display_warnings = true;
}

void gtkGUI::cb_fuse_default (void)
{
        // disable fuse warnings
        display_warnings = false;
        // copy default fuse values over device current fuse values
        for (int i = 0; i < 3; i++)
                avrdude->fusebytes_ondevice[i] = microcontroller->description->fusebytes_default[i];
        // display updated (default) fuse settings
        process_fuse_values();
        // enable fuse warnings
        display_warnings = true;
}

void gtkGUI::execution_done (void)
{
        //cout << "APPGUI: execution thread stopped!" << endl;
        // enable controls
        controls_unlock();
        cb_device->set_sensitive(true);
        cb_family->set_sensitive(true);
        // change window cursor back
        Glib::RefPtr<Gdk::Window> window = main_window->get_window();
        window->set_cursor();
        // display console-output and executed command
        update_console_view();
        // check execution outcome and display proper message
        execution_outcome(true);
}

void gtkGUI::update_console_view (void)
{
        // display console-output
        dude_output_buffer->insert(dude_output_buffer->end(), avrdude->raw_exec_output);
        // remove first 128 lines, if more than 768
        if (dude_output_buffer->get_line_count() > 768) {
                Gtk::TextBuffer::iterator start = dude_output_buffer->begin();
                Gtk::TextBuffer::iterator end = dude_output_buffer->get_iter_at_line(128);
                dude_output_buffer->erase(start, end);
        }
        // display last executed command
        lbl_dude_command->set_label(avrdude->last_command);
}

void gtkGUI::execution_started (void)
{
        //cout << "APPGUI: execution thread initialized!" << endl;
        // disable controls
        cb_device->set_sensitive(false);
        cb_family->set_sensitive(false);
        controls_lock();
        // change window cursor
        Glib::RefPtr<Gdk::Window> window = main_window->get_window();
        Glib::RefPtr<Gdk::Display> display = main_window->get_display();
        Glib::RefPtr<Gdk::Cursor> cursor = Gdk::Cursor::create(display, "wait");
        window->set_cursor(cursor);
}

void gtkGUI::execution_outcome (gboolean show_success_message)
{
        switch (avrdude->execution_status) {

                case (Dude::exec_status::no_error): {
                        if (show_success_message)
                                message_popup("Success!", "Operation completed successfully.");
                        break;
                }
                case (Dude::exec_status::verification_error): {
                        message_popup("Failure!", "Mismatch: The contents of the specified file and the device memory differ.");
                        break;
                }
                case (Dude::exec_status::init_error): {
                        message_popup("Failure!", "Programmer failed to initialize the device. Check your device connection, clock source and fuse settings.");
                        break;
                }
                case (Dude::exec_status::invalid_signature): {
                        message_popup("Failure!", "Invalid device signature detected. Double check your device selection.");
                        break;
                }
                case (Dude::exec_status::unknown_device): {
                        message_popup("Failure!", "Unknown device name supplied. Avrdude does not seem to support the specified device (outdated version?).");
                        break;
                }
                case (Dude::exec_status::cannot_read_signature): {
                        message_popup("Failure!", "Cannot read device signature. Probable cause: Corrupted device memory.");
                        break;
                }
                case (Dude::exec_status::command_not_found): {
                        message_popup("Failure!", "Executable (avrdude) not found. Did you forget to install it?");
                        break;
                }
                case (Dude::exec_status::insufficient_permissions): {
                        message_popup("Failure!", "Cannot access programmer. Insufficient permissions.");
                        break;
                }
                case (Dude::exec_status::programmer_not_found): {
                        message_popup("Failure!", "Programmer not found. Check your cable connections.");
                        break;
                }
                default: {
                        message_popup("Failure!", "Unprecedented error...");
                        break;
                }
        }
}

void gtkGUI::message_popup (Glib::ustring title, Glib::ustring message)
{
        //cout << "\nTitle: " << title << endl;
        //cout << "Message: " << message << endl;

        Gtk::MessageDialog msg_box(*main_window, title);
        msg_box.set_secondary_text(message);
        msg_box.run();
}
