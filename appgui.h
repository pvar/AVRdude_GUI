#ifndef PROGGUI_H
#define PROGGUI_H

#include <gtkmm.h>
#include <iostream>
#include <iomanip>
#include "micro.h"
//#include "dude.h"

using namespace std;

class CBRecordModel : public Gtk::TreeModel::ColumnRecord
{
        public:
                CBRecordModel();
                Gtk::TreeModelColumn<string> col_name;
                Gtk::TreeModelColumn<string> col_data;
};

struct FuseWidget {
        FuseWidget();

        Gtk::CheckButton *check;
        Gtk::ComboBox *combo;
        Gtk::Label *reg_label;
        Gtk::Label *combo_label;
        Glib::RefPtr<Gtk::ListStore> model;
        sigc::connection *callback;
        guint max_value;
        guint bitmask;
        guint bytenum;
};

class gtkGUI
{
        public:
                gtkGUI();
                virtual ~gtkGUI();

                Gtk::Window *main_window = nullptr;

                void data_prep (void);

        protected:
                /* references to structures holding microcontroller data */
                DeviceSpecifications *specifications = nullptr;
                DeviceFuseSettings *settings = nullptr;
                list<FuseWarning> *warnings = nullptr;
                /* path to executable */
                Glib::ustring exec_path;
                /* device to XML-file mapping */
                map <Glib::ustring, Glib::ustring> *device_map = nullptr;
                /* list of widgets used for fuse-settings */
                list<FuseWidget> *fuse_tab_widgets = nullptr;
                /* object for retrieving microcontroller data */
                Micro *microcontroller;
                /* record-model for the tree-models used by combo boxes */
                CBRecordModel cbm_generic;
                /* signal connection handler for temporarily disconnecting combo-box signals */
                sigc::connection dev_combo_signal;

                /* tree-models for combo boxes */
                Glib::RefPtr<Gtk::ListStore> tm_family;
                Glib::RefPtr<Gtk::ListStore> tm_device;
                Glib::RefPtr<Gtk::ListStore> tm_port;
                Glib::RefPtr<Gtk::ListStore> tm_protocol;

                // widgets
                Gtk::ComboBox *cb_family      = nullptr;
                Gtk::ComboBox *cb_device      = nullptr;
                Gtk::ComboBox *cb_protocol    = nullptr;
                Gtk::Button *btn_fuse_read    = nullptr;
                Gtk::Button *btn_fuse_write   = nullptr;
                Gtk::Button *btn_firm_read    = nullptr;
                Gtk::Button *btn_firm_write   = nullptr;
                Gtk::Button *btn_firm_verify  = nullptr;
                Gtk::Button *btn_firm_open    = nullptr;
                Gtk::Button *btn_erom_read    = nullptr;
                Gtk::Button *btn_erom_write   = nullptr;
                Gtk::Button *btn_erom_verify  = nullptr;
                Gtk::Button *btn_erom_open    = nullptr;
                Gtk::Button *btn_check_sig    = nullptr;
                Gtk::Button *btn_erase_dev    = nullptr;
                Gtk::Label *lbl_sig_tst       = nullptr;
                Gtk::Label *lbl_spec_flash    = nullptr;
                Gtk::Label *lbl_spec_sram     = nullptr;
                Gtk::Label *lbl_spec_eeprom   = nullptr;
                Gtk::Label *lbl_spec_speed    = nullptr;
                Gtk::Label *lbl_signature     = nullptr;
                Gtk::Entry *ent_fusebytes     = nullptr;
                Gtk::Grid *fuse_grid          = nullptr;
                Gtk::CheckButton *auto_erase  = nullptr;
                Gtk::CheckButton *auto_verify = nullptr;
                Gtk::CheckButton *auto_check  = nullptr;

                // signal handlers
                void cb_new_device (void);
                void cb_new_family (void);
                void cb_dude_settings (void);
                void display_specs (gboolean have_specs);
                void display_fuses (gboolean have_fuses);
                void calculate_fuses (void);

                // utilities
                void get_executable_path (void);
                void clear_fuse_widget(FuseWidget* settings_widget);
                bool data_prep_start (void);
};

#endif
