#include "appgui.h"

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
        microcontroller = new Micro(exec_path, "");

        // create object builder
        Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();
        try {
                builder->add_from_file(exec_path + "avrprog.glade");
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

        // use builder to instantiate GTK widgets
        builder->get_widget("main_window", main_window);
        builder->get_widget("combo_family", cb_family);
        builder->get_widget("combo_device", cb_device);
        builder->get_widget("combo_speed", cb_speed);
        builder->get_widget("combo_protocol", cb_protocol);
        builder->get_widget("signature_test", btn_check_sig);
        builder->get_widget("erase_device", btn_erase_dev);
        builder->get_widget("signature_test_result", lbl_sig_tst);
        builder->get_widget("dev_mem_flash", lbl_spec_flash);
        builder->get_widget("dev_mem_sram", lbl_spec_sram);
        builder->get_widget("dev_mem_eeprom", lbl_spec_eeprom);
        builder->get_widget("dev_max_speed", lbl_spec_speed);
        builder->get_widget("dev_signature", lbl_signature);
        builder->get_widget("fuse_parameters", ent_fusebytes);
        builder->get_widget("fuse_settings_grid", fuse_grid);

        /* create the tree-models */
        tm_family = Gtk::ListStore::create(cbm_generic);
        tm_device = Gtk::ListStore::create(cbm_generic);
        tm_speed = Gtk::ListStore::create(cbm_generic);
        tm_port = Gtk::ListStore::create(cbm_generic);
        tm_protocol = Gtk::ListStore::create(cbm_generic);

        /* assign tree-models to combo boxes*/
        cb_family->set_model(tm_family);
        cb_device->set_model(tm_device);
        cb_speed->set_model(tm_speed);
        cb_protocol->set_model(tm_protocol);

        /* define visible columns */
        cb_family->pack_start(cbm_generic.col_name);
        cb_device->pack_start(cbm_generic.col_name);
        cb_speed->pack_start(cbm_generic.col_name);
        cb_protocol->pack_start(cbm_generic.col_name);

        /* populate tree-model with device families */
        Gtk::TreeModel::Row row = *(tm_family->append());
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
        row = *(tm_family->append());
        row[cbm_generic.col_name] = "AT mega";
        row[cbm_generic.col_data] = "ATmega";
        row = *(tm_family->append());
        row[cbm_generic.col_name] = "AT tiny";
        row[cbm_generic.col_data] = "ATtiny";
        //row = *(tm_family->append());
        //row[cbm_generic.col_name] = "AT Xmega";
        //row[cbm_generic.col_data] = "ATxmega";
        cb_family->set_active(5);

        // connect signal handlers
        dev_combo_signal = cb_device->signal_changed().connect(sigc::mem_fun(*this, &gtkGUI::cb_new_device));
        cb_family->signal_changed().connect(sigc::mem_fun(*this, &gtkGUI::cb_new_family));
}

gtkGUI::~gtkGUI()
{
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

bool gtkGUI::data_prep_start (void)
{
        //cout << "DATA PREPARATION..." << endl;

        /* get supported devices */
        device_map = microcontroller->get_device_list();
        /* update device combo box */
        this->cb_new_family();
        /* do not repeat timer */
        return FALSE;
}

void gtkGUI::cb_new_family (void)
{
        Gtk::TreeModel::Row row;

        /* get selected family */
        Glib::ustring family;
        Gtk::TreeModel::iterator selected_family = cb_family->get_active();
        if (selected_family) {
                row = *selected_family;
                if (row) {
                        family = row[cbm_generic.col_data];
                        //cout << "selected family: " << family << endl;
                }
        }

        /* exit if device_map is not yet populated */
        if (device_map == nullptr)
                return;

        /* disconnect on-change signal */
        dev_combo_signal.disconnect();
        /* clear device tree-view */
        tm_device->clear();
        /* insert family members */
        row = *(tm_device->append());
        row[cbm_generic.col_name] = "None";
        row[cbm_generic.col_data] = "";
        for (map<Glib::ustring, Glib::ustring>::iterator iter = device_map->begin(); iter != device_map->end(); ++iter) {
                /* ignore devices family-irrelevant names */
                string name_part = iter->first.substr (0, family.size());
                if (name_part != family)
                        continue;
                /* add new entry to treeview of device combo box */
                row = *(tm_device->append());
                row[cbm_generic.col_name] = iter->first;
                row[cbm_generic.col_data] = iter->second;
                //cout << iter->first << " => " << iter->second << endl;
        }

        /* set default selected entry */
        cb_device->set_active(0);
        /* reconnect on-change signal */
        dev_combo_signal = cb_device->signal_changed().connect(sigc::mem_fun(*this, &gtkGUI::cb_new_device));
}

void gtkGUI::cb_new_device (void)
{
        /*
          delete current instance of Micro and "mark" as empty
          (this will also delete specifications, warnings and settings,
          as they are references to structs created inside microcontroller)
        */
        if (microcontroller) {
                delete this->microcontroller;
                this->microcontroller = nullptr;
        }

        /* get selected device */
        Glib::ustring device;
        Gtk::TreeModel::iterator selected_device = cb_device->get_active();
        if (selected_device) {
                Gtk::TreeModel::Row row = *selected_device;
                if (row) {
                        device = row[cbm_generic.col_data];
                        //cout << "selected device: " << device << endl;
                }
        }

        /* clear old labels and fuses */
        this->display_fuses(false);
        this->display_specs(false);
        /* do not proceed if invalid device was selected */
        if (device.size() < 1)
                return;
        /* clear signature-test result */
        lbl_sig_tst->set_label("Unverified...");
        /* creatre new instance if Micro */
        microcontroller = new Micro(exec_path, device);
        /* prepare data for selected device */
        microcontroller->parse_data();
        /* get references to structs with microcontroller data */
        specifications = microcontroller->get_specifications();
        settings = microcontroller->get_fuse_settings();
        warnings = microcontroller->get_fuse_warnings();
        /* display specifications */
        this->display_specs(true);
        /* display fuse settings */
        this->display_fuses(true);
}

void gtkGUI::display_specs (gboolean have_specs)
{
        if (have_specs) {
                lbl_spec_flash->set_label(specifications->flash_size);
                lbl_spec_eeprom->set_label(specifications->eeprom_size);
                lbl_spec_sram->set_label(specifications->sram_size);
                lbl_spec_speed->set_label(specifications->max_speed);
                lbl_signature->set_label(specifications->signature);
        } else {
                lbl_spec_flash->set_label("NA");
                lbl_spec_eeprom->set_label("NA");
                lbl_spec_sram->set_label("NA");
                lbl_spec_speed->set_label("NA");
                lbl_signature->set_label("0x00 0x00 0x00");
        }
}
void gtkGUI::clear_fuse_widget(FuseWidget* settings_widget)
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

void gtkGUI::display_fuses (gboolean have_fuses)
{
        if (!have_fuses) {
                /* return if widget-list is uninitialized */
                if (fuse_tab_widgets == nullptr)
                        return;

                /* loop through fuse-widget list: hide and delete corresponding widgets */
                list<FuseWidget>::iterator iter;
                for (iter = fuse_tab_widgets->begin(); iter != fuse_tab_widgets->end(); ++iter)
                        clear_fuse_widget(&(*iter));

                /* delete fuse-widget list and "mark" as empty */
                delete fuse_tab_widgets;
                fuse_tab_widgets = nullptr;
                return;
        }

        /* used to populate tree-models */
        Gtk::TreeModel::Row row;
        /* index to next grid-line */
        guint grid_line = 1;
        /* create list of fuse-widgets */
        fuse_tab_widgets = new list<FuseWidget>;
        /* loop through fuse-options: create and display corresponding widgets */
        list<FuseSetting>::iterator iter;
        for (iter = ((this->settings)->fuse_settings)->begin(); iter != ((this->settings)->fuse_settings)->end(); ++iter) {
                /* create a new instance of FuseWidget */
                FuseWidget *widget_entry = new FuseWidget;
                /* create new instance of signal connection */
                widget_entry->callback = new sigc::connection;
                /* decide what kind of widget is needed */
                if ((*iter).single_option) {
                        /* check if name of register (not a normal setting) */
                        if ((*iter).offset == 512) {
                                /* widget is not a checkbutton (not a normal setting) */
                                /* create pointer to label */
                                widget_entry->reg_label = new Gtk::Label((*iter).fdesc);
                                /* set align, indent and width of the widgets */
                                (widget_entry->reg_label)->set_halign(Gtk::Align::ALIGN_START);
                                (widget_entry->reg_label)->set_margin_start(24);
                                (widget_entry->reg_label)->set_margin_top(16);
                                (widget_entry->reg_label)->set_margin_bottom(4);
                                /* put label in grid */
                                fuse_grid->attach(*(widget_entry->reg_label), 0, grid_line++, 1, 1);
                                /* show widget */
                                (widget_entry->reg_label)->show();
                        } else {
                                /* widget is a checkbutton (normal setting) */
                                /* create pointer to checkbutton */
                                widget_entry->check = new Gtk::CheckButton((*iter).fdesc);
                                /* add callback for on_change event */
                                *(widget_entry->callback) = (widget_entry->check)->signal_clicked().connect(sigc::mem_fun(*this, &gtkGUI::calculate_fuses));
                                /* put checkbutton in grid */
                                fuse_grid->attach(*(widget_entry->check), 0, grid_line++, 1, 1);
                                /* show widget */
                                (widget_entry->check)->show();
                        }
                } else {
                        /* create pointer to treemodel */
                        widget_entry->model = Gtk::ListStore::create(cbm_generic);
                        /* prepare to loop through the enumerator members */
                        list<OptionEntry>* this_enum_list = (*(this->settings)->option_lists)[(*iter).fenum];
                        list<OptionEntry>::iterator enum_iter = this_enum_list->begin();
                        /* prepare adjusted bit-mask with value of enumerator pseudo entry */
                        widget_entry->max_value = enum_iter->value;
                        enum_iter++;
                        /* loop through the rest of the entries */
                        for (; enum_iter != this_enum_list->end(); ++enum_iter) {
                                row = *(widget_entry->model->append());
                                row[cbm_generic.col_name] = enum_iter->ename;
                                row[cbm_generic.col_data] = to_string(enum_iter->value);
                        }
                        /* create pointer to label */
                        widget_entry->combo_label = new Gtk::Label((*iter).fdesc);
                        /* create pointer to combobox */
                        widget_entry->combo = new Gtk::ComboBox();
                        /* assign treemodel to combobox */
                        (widget_entry->combo)->set_model(widget_entry->model);
                        /* define visible columns */
                        (widget_entry->combo)->pack_start(cbm_generic.col_name);
                        /* add callback for on_change event */
                        *(widget_entry->callback) = (widget_entry->combo)->signal_changed().connect(sigc::mem_fun(*this, &gtkGUI::calculate_fuses));
                        /* set align, indent and width of the widgets */
                        (widget_entry->combo_label)->set_halign(Gtk::Align::ALIGN_START);
                        (widget_entry->combo_label)->set_margin_start(24);
                        (widget_entry->combo)->set_margin_start(24);
                        /* put widgets in grid */
                        fuse_grid->attach(*(widget_entry->combo_label), 0, grid_line++, 1, 1);
                        fuse_grid->attach(*(widget_entry->combo), 0, grid_line++, 1, 1);
                        /* select first entry */
                        (widget_entry->combo)->set_active(0);
                        /* show widgets */
                        (widget_entry->combo_label)->show();
                        (widget_entry->combo)->show();
                }
                /* copy bit-mask and fuse-byte offset */
                widget_entry->bitmask = (*iter).fmask;
                widget_entry->bytenum = (*iter).offset;
                /* put widget_entry in fuse_tab_widgets */
                fuse_tab_widgets->push_back(*widget_entry);
        }
}

void gtkGUI::calculate_fuses ()
{
        /* fuse-byte values */
        guint fusebytes[3] = {0, 0, 0};

        /* clear fuse-byte values */
        fusebytes[0] = 0;
        fusebytes[1] = 0;
        fusebytes[2] = 0;

        Gtk::TreeModel::iterator selected;
        Gtk::TreeModel::Row selected_row;

        /* calculate fuse-bytes from settings...

              combo setting: factor = bitmask / MAX(enum_value)
              combo setting: adjusted_value = selected_enum_value * factor
              combo setting: adjusted_value XOR bitmask

              single setting: just keep the bitmask

              fuse bytes: bitmasks are ORed and the result is negated
        */
        list<FuseWidget>::iterator fwidget = fuse_tab_widgets->begin();
        for (fwidget++; fwidget != fuse_tab_widgets->end(); ++fwidget) {
                if ((fwidget->bytenum >= 0) && (fwidget->bytenum <= 2)) {
                        if (fwidget->combo) {
                                selected = (fwidget->combo)->get_active();
                                selected_row = *selected;
                                guint this_value = atoi(string((selected_row[cbm_generic.col_data])).c_str());
                                guint adjusted_value = (fwidget->bitmask * this_value) / fwidget->max_value;
                                fusebytes[fwidget->bytenum] |= (adjusted_value ^ fwidget->bitmask);
                                //cout << "value: "<< this_value << "\t\tadj val: " << adjusted_value << "\t\tmask: " << (adjusted_value ^ fwidget->bitmask) << endl;
                        } else {
                                if ((fwidget->check)->get_active())
                                        fusebytes[fwidget->bytenum] |= fwidget->bitmask;
                        }
                }
        }

        /* negate calculated values (they are expected this way) */
        fusebytes[0] ^= 255;
        fusebytes[1] ^= 255;
        fusebytes[2] ^= 255;

        //-U lfuse:w:LOW_BYTE:m -U hfuse:w:HIGH_BYTE:m -U efuse:w:EXT_BYTE:m
        string fuse_parameters;
        stringstream converter_stream;

        fuse_parameters = "-U lfuse:w:0x";
        converter_stream << hex << setw(2) << setfill('0');
        converter_stream << fusebytes[0];
        fuse_parameters += converter_stream.str();
        converter_stream.str(string());
        fuse_parameters += ":m  -U hfuse:w:0x";
        converter_stream << hex << setw(2) << setfill('0');
        converter_stream << fusebytes[1];
        fuse_parameters += converter_stream.str();
        converter_stream.str(string());
        fuse_parameters += ":m  -U efuse:w:0x";
        converter_stream << hex << setw(2) << setfill('0');
        converter_stream << fusebytes[2];
        fuse_parameters += converter_stream.str();
        fuse_parameters += ":m";

        ent_fusebytes->set_text(fuse_parameters);
}
