#include "parser.h"

using namespace std;

Parser::status Parser::process_file (string filename, DeviceDescription &description)
{
        if (filename == "") {
                cout << "\nno device-description file specified" << endl;
                return status::io_error;
        }

        xmlpp::DomParser parser;

        status read_file = get_content (filename, parser);
        if (read_file != status::success)
                return read_file;

        xmlpp::Node *root_node = parser.get_document()->get_root_node();

        get_specs (root_node, description);
        get_settings (root_node, description);
        get_warnings (root_node, description);
        get_default (root_node, description);

        return status::success;
}


Parser::status Parser::get_content (std::string filename, xmlpp::DomParser &parser)
{
        // attempt to open specified file
        try {
                parser.parse_file(filename);
        } catch(const exception& ex) {
                cout << "Exception caught, trying to read file: " << ex.what() << endl;
                return status::io_error;
        }

        // check if content found
        if (!parser)
                return status::no_content;

        // check if content has expected structure
        xmlpp::Node *root_node = parser.get_document()->get_root_node();
        if (is_valid (root_node))
                return status::success;
        else
                return status::doc_error;
}


void Parser::print_warnings (DeviceDescription &description)
{
        list<DeviceDescription::FuseWarning>::iterator iter;
        for (iter = description.warnings->begin(); iter != description.warnings->end(); iter++) {
                cout << endl;
                cout << "    byte: " << iter->fbyte << endl;
                cout << "    mask: " << iter->fmask << endl;
                cout << "   value: " << iter->fresult << endl;
                cout << "    text: " << iter->warning << endl;
        }
}


void Parser::print_defaults (DeviceDescription &description)
{
        for (int i = 0; i < 3; i++)
                cout << "fuse byte " << i + 1 << " : " << hex << description.fusebytes_default[i] << endl;
}


void Parser::print_settings (DeviceDescription &description)
{
        list<DeviceDescription::FuseSetting>::iterator iter;
        for (iter = (description.fuse_settings)->begin(); iter != (description.fuse_settings)->end(); ++iter) {
                cout << endl;
                cout << (*iter).fname << " ";
                cout << "mask[" << (*iter).fmask << "] ";
                cout << "offset[" << (*iter).offset << "] ";
                cout << "enum[" << (*iter).fenum << "] ";
                cout << (*iter).fdesc << endl;
        }
}


void Parser::print_options (DeviceDescription &description)
{
        map<string, list<DeviceDescription::OptionEntry>*>::iterator iter;
        for (iter = (description.option_lists)->begin(); iter != (description.option_lists)->end(); ++iter) {
                cout << iter->first << " => " << endl;
                list<DeviceDescription::OptionEntry>::iterator iter2;
                for (iter2 = (*iter->second).begin(); iter2 != (*iter->second).end(); ++iter2)
                                cout << (*iter2).ename << endl;
        }
}
