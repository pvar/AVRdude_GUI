#ifndef DUDE_H
#define DUDE_H

#include <sigc++/sigc++.h>
#include <iostream>
#include <thread>
#include <glibmm.h>

class Dude
{
        public:
                Dude();
                virtual ~Dude();

                void setup (bool auto_erase, bool auto_verify, bool auto_check,
                            std::string programmer, std::string microcontroller);

                // basic avrdude operations
                void do_clear_device (void);
                void do_read_signature (void);
                void do_eeprom_write (std::string file);
                void do_eeprom_read (std::string file);
                void do_eeprom_verify (std::string file);
                void do_flash_write (std::string file);
                void do_flash_read (std::string file);
                void do_flash_verify (std::string file);
                void do_fuse_write (int bytes_count, int low, int high, int ext);
                void do_fuse_read (int bytes_count);

                // possible states after execution
                enum exec_status {
                        no_error,
                        init_error,
                        invalid_signature,
                        unknown_device,
                        cannot_read_signature,
                        command_not_found,
                        insufficient_permissions,
                        verification_error,
                        programmer_not_found
                };

                // device fuse settings (as read from device memory)
                int fusebytes_ondevice[3] = {255, 255, 255};
                // console output from command execution
                std::string last_command;
                // console output from command execution
                std::string raw_exec_output;
                // usefull part of output -- according to nature of executed command
                std::string processed_output;
                // error code that occured during last execution
                exec_status execution_status;
                // wether a working thread is active or not
                bool working;

                // custom signal types
                typedef sigc::signal<void> type_sig_exec_done;
                typedef sigc::signal<void> type_sig_exec_started;
                type_sig_exec_done signal_exec_done();
                type_sig_exec_started signal_exec_started();

        protected:
                std::thread* avrdude_thread;

                // execution start/stop
                // this signal is meant to be connected to a function of another object
                // (the connection will be initialized by that other object)
                type_sig_exec_done sig_exec_done;
                type_sig_exec_started sig_exec_started;

                // execution completion
                // this is meant to be used internally -- to notify a function of this object
                // (dispatcher is like sigc<void>, but is suitable for inter-thread communication)
                Glib::Dispatcher local_sig_exec_done;

                std::string oneliner;
                std::string protocol;
                std::string options;
                std::string device;
                std::string command;

                // array af strings that indicate an error has occured
                // (the order of appearance in this array corresponds to the severity of the error)
                const std::vector<std::string>
                error_strings = {
                        "command not found",
                        "target doesn't answer",
                        "Valid parts are:",
                        "can't open device",
                        "could not find",
                        "did not find any USB device",
                        "error reading signature data",
                        "Double check chip, or use -F to override this check.",
                        "content mismatch",
                        "verification error"
                };

                // array af the correspsonding error-codes
                // (these two arrays should always have the same number of elements)
                const std::vector<exec_status>
                error_codes = {
                        command_not_found,
                        init_error,
                        unknown_device,
                        programmer_not_found,
                        programmer_not_found,
                        programmer_not_found,
                        cannot_read_signature,
                        invalid_signature,
                        verification_error,
                        verification_error
                };

                // utility functions
                void execute (void);
                void execution_begin (void);
                void execution_end (void);
                void check_for_errors (void);
};

#endif
