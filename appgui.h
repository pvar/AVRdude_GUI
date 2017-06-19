#ifndef PROGGUI_H
#define PROGGUI_H

#include <sigc++/sigc++.h>
#include <gtkmm.h>
#include <iostream>
#include <iomanip>
#include "micro.h"
#include "dude.h"

using namespace std;

enum file_op { open_f, save_f, open_e, save_e };
enum op_code { sig_check, dev_erase, eeprom_w, eeprom_r, eeprom_v, flash_w, flash_r, flash_v, fuse_w, fuse_r };

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
                // path to executable
                Glib::ustring exec_path;
                // device to XML-file mapping
                map <Glib::ustring, Glib::ustring> *device_map = nullptr;
                // list of widgets used for fuse-settings
                list<FuseWidget> *fuse_tab_widgets = nullptr;
                // object for retrieving microcontroller data
                Micro *microcontroller;
                // object for interfacing with avrdude
                Dude *avrdude;
                // record-model for the tree-models used by combo boxes
                CBRecordModel cbm_generic;

                // signal connection handlers
                sigc::connection dev_combo_signal;
                sigc::connection dev_combo_programmer;
                sigc::connection check_button_erase;
                sigc::connection check_button_check;
                sigc::connection check_button_verify;

                // tree-models for combo boxes
                Glib::RefPtr<Gtk::ListStore> tm_family;
                Glib::RefPtr<Gtk::ListStore> tm_device;
                Glib::RefPtr<Gtk::ListStore> tm_port;
                Glib::RefPtr<Gtk::ListStore> tm_protocol;

                // text buffer for diaplsying avrdude output
                Glib::RefPtr<Gtk::TextBuffer> dude_output_buffer;

                // widgets
                Gtk::CheckButton *auto_erase   = nullptr;
                Gtk::CheckButton *auto_verify  = nullptr;
                Gtk::CheckButton *auto_check   = nullptr;
                Gtk::ComboBox *cb_family       = nullptr;
                Gtk::ComboBox *cb_device       = nullptr;
                Gtk::ComboBox *cb_protocol     = nullptr;
                Gtk::Grid *fuse_grid           = nullptr;
                Gtk::Box *box_flash_ops        = nullptr;
                Gtk::Box *box_eeprom_ops       = nullptr;

                Gtk::Button *btn_fuse_read     = nullptr;
                Gtk::Button *btn_fuse_write    = nullptr;
                Gtk::Button *btn_check_sig     = nullptr;
                Gtk::Button *btn_erase_dev     = nullptr;
                Gtk::Label *lbl_sig_tst        = nullptr;
                Gtk::Label *lbl_spec_flash     = nullptr;
                Gtk::Label *lbl_spec_sram      = nullptr;
                Gtk::Label *lbl_spec_eeprom    = nullptr;
                Gtk::Label *lbl_spec_speed     = nullptr;
                Gtk::Label *lbl_spec_xml       = nullptr;
                Gtk::Label *lbl_signature      = nullptr;
                Gtk::Label *lbl_fusebytes      = nullptr;
                Gtk::Entry *ent_flash_file     = nullptr;
                Gtk::Entry *ent_eeprom_file    = nullptr;
                Gtk::FileChooserDialog *browser= nullptr;

                // signal handlers
                void cb_new_device (void);
                void cb_new_family (void);
                void cb_dude_settings (void);
                void display_specs (gboolean have_specs);
                void display_fuses (gboolean have_fuses);
                void display_fuse_bytes ();
                void calculate_fuses (void);
                void check_sig (void);
                void erase_dev (void);
                void eeprom_read (void);
                void eeprom_write (void);
                void eeprom_verify (void);
                void flash_read (void);
                void flash_write (void);
                void flash_verify (void);
                void fuse_write (void);
                void fuse_read (void);
                void select_file (file_op action);

                // utilities
                void populate_static_treemodels (void);
                void get_executable_path (void);
                void clear_fuse_widget(FuseWidget* settings_widget);
                bool data_prep_start (void);
                void update_console_view (void);
                void lock_and_clear (void);
                void unlock_and_update (void);
                void execution_done (void);
                //void msg_popup (Glib::ustring title, Glib::ustring message);
};

#endif
