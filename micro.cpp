#include "micro.h"

DeviceFuseSettings::DeviceFuseSettings()
{
        this->fuse_settings = new list<FuseSetting>;
        this->option_lists = new map <Glib::ustring, list<OptionEntry>*>;
}

DeviceFuseSettings::~DeviceFuseSettings()
{
        /* iterate through map and delete embedded lists */
        map<Glib::ustring, list<OptionEntry>*>::iterator iter = option_lists->begin();
        for (; iter!=option_lists->end(); ++iter)
                delete iter->second;

        /* delete the whole map and the extra list */
        delete option_lists;
        delete fuse_settings;
}

Micro::Micro (Glib::ustring path, Glib::ustring xml)
{
        //cout << "EXE PATH = [" << path << "]" << endl;
        //cout << "XML FILE = [" << xml << "]" << endl;
        exec_path = path;
        device_xml = xml;
}

Micro::~Micro()
{
        if (specifications)
                delete specifications;
        if (warnings)
                delete warnings;
        if (settings)
                delete settings;
}

/*******************************************************************************
 * Create a map with device names and the corresponding files
 *******************************************************************************/

map <Glib::ustring, Glib::ustring>* Micro::get_device_list (void)
{
        map <Glib::ustring, Glib::ustring> *device_map = new map <Glib::ustring, Glib::ustring>;
        DIR *directory;
        struct dirent *entry;

        /* prepare path to XML files... */
        Glib::ustring xml_dir = ("xml-files/");
        Glib::ustring whole_path = this->exec_path + xml_dir;
        gchar *path_xml_files = new char[whole_path.size() + 1];
        // !! Result is not safe, if path uses multi-byte characters (because of copy)
        copy(whole_path.begin(), whole_path.end(), path_xml_files);
        path_xml_files[whole_path.size()] = '\0';

        /* parse xml files one by one */
        if ((directory = opendir ((const gchar *)path_xml_files)) != NULL) {

                /* check every XML file in given directory */
                while ((entry = readdir(directory)) != NULL) {
                        const Glib::ustring filename = entry->d_name;
                        /* check for appropriate filename-length */
                        if (filename.size() < 4)
                                continue;
                        /* look for expected filename-extension */
                        if ((filename.compare(filename.size() - 3, 3, "xml") != 0) &&
                            (filename.compare(filename.size() - 3, 3, "XML") != 0))
                                continue;
                        /* process specific XML file */
                        try {
                                xmlpp::DomParser parser;
                                parser.parse_file((const gchar *)path_xml_files + filename);
                                if (parser) {
                                        xmlpp::Node *xml_node = parser.get_document()->get_root_node();
                                        const Glib::ustring node_name = xml_node->get_name();
                                        /* check if valid part-description file */
                                        if (node_name.compare("AVRPART") != 0)
                                                continue;
                                        /* go to ADMIN node */
                                        xml_node = xml_node->get_first_child("ADMIN");
                                        /* go to PART_NAME node */
                                        xml_node = xml_node->get_first_child("PART_NAME");
                                        /* get node content */
                                        Glib::ustring device_name = this->get_txt_value(xml_node);
                                        /* insert new entry in device map */
                                        (*device_map)[device_name] = filename;

                                }
                        } catch(const exception& ex) {
                                cerr << "Exception caught: " << ex.what() << endl;
                                continue;
                        }
                }
                closedir (directory);
        } else {
                delete path_xml_files;
                return nullptr;
        }
        delete path_xml_files;

        //cout << "device XML-files: " << device_map->size() << endl;
        return device_map;

}

/*******************************************************************************
 * Parse data from specified file
 *******************************************************************************/

void Micro::parse_data ()
{
        if (this->device_xml == "") {
                cout << "\nXML device description file not specified!" << endl;
                return;
        }

        /* prepare data structures */
        if (specifications)
                delete specifications;
        if (warnings)
                delete warnings;
        if (settings) {
                if (settings->fuse_settings)
                        delete settings->fuse_settings;
                if (settings->option_lists)
                        delete settings->option_lists;
                delete settings;
        }
        specifications = new DeviceSpecifications;
        settings = new DeviceFuseSettings;
        warnings = new list<FuseWarning>;

        /* prepare path to specified XML file... */
        Glib::ustring xml_dir = ("xml-files/");
        Glib::ustring path_to_file = this->exec_path + xml_dir + this->device_xml;

        /* process specific XML file */
        try {
                xmlpp::DomParser parser;
                parser.parse_file(path_to_file);
                if (parser) {
                        /* go to root node */
                        xmlpp::Node *root_node = parser.get_document()->get_root_node();
                        /* check if a valid description file */
                        const Glib::ustring node_name = root_node->get_name();
                        if (node_name.compare("AVRPART") != 0) {
                                cout << "Not a valid description file!" << endl;
                                return;
                        }

                        /* should check return status! */
                        this->parse_specifications(root_node);
                        this->parse_settings(root_node);
                        this->parse_warnings(root_node);
                        /* --------------------------- */
                }
        } catch(const exception& ex) {
                cout << "Exception caught, trying to parse device description file: " << ex.what() << endl;
                return;
        }

        return;
}


DeviceSpecifications* Micro::get_specifications (void)
{
        return this->specifications;
}


DeviceFuseSettings* Micro::get_fuse_settings (void)
{
        return this->settings;
}


list<FuseWarning>* Micro::get_fuse_warnings (void)
{
        return this->warnings;
}

// ============================= PROTECTED METHODS =============================



/*******************************************************************************
 * Parse fuse settings
 *******************************************************************************/

int Micro::parse_settings (xmlpp::Node *root_node)
{
        //cout << "PARSING FUSES..." << endl;

        DeviceFuseSettings* settings = new DeviceFuseSettings;

        xmlpp::Node::NodeList children;
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = nullptr;

        /* go to V2 node */
        xml_node = xml_node->get_first_child("V2");
        /* if no such node was found, it must be a device without fuses... */
        if (!xml_node)
                return 0;
        /* go to templates node */
        xml_node = xml_node->get_first_child("templates");
        /* if no such node was found, it must be a device without fuses... */
        if (!xml_node)
                return 0;
        /* go to node with attribute: class = "FUSE" */
        tmp_node = this->get_child_with_attr(xml_node,  "class", "FUSE");

        /* if no such node was found, check if it's a device with NVM... */
        if (!tmp_node) {
                /* go to node with attribute: class = "NVM" */
                tmp_node = this->get_child_with_attr(xml_node,  "class", "NVM");
                /* if no such node was found, it must be a device without fuses... */
                if (!tmp_node)
                        return 0;
                /* go to node with attribute: name = "NVM_FUSES" */
                tmp_node = this->get_child_with_attr(tmp_node,  "name", "NVM_FUSES");
                /* if no such node was found, there must be a problem... */
                if (!tmp_node)
                        return -1;
                /* use default node pointer */
                xml_node = tmp_node;
        } else {
                /* go to registers node */
                tmp_node = tmp_node->get_first_child("registers");
                /* use default node pointer */
                xml_node = tmp_node;
        }

        guint fuse_byte_counter = 0;
        /* loop through children (reg nodes) */
        children = xml_node->get_children();
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                /* increase fuse-byte counter */
                fuse_byte_counter++;
                /* get offset value for this reg node */
                Glib::ustring regoffset = this->get_att_value((*node_iterator), "offset");
                /* if no offset attribute was found, proceed to next iteration */
                if (regoffset.size() < 1)
                        continue;
                guint offset = ::atoi((regoffset.substr(2, 2)).c_str());
                /* get name for this reg node */
                Glib::ustring regname = this->get_att_value((*node_iterator), "name");
                /* prepare pseudo-setting with fuse-byte name */
                FuseSetting* entry = new FuseSetting;
                entry->single_option = true;
                entry->fdesc = "FUSE REGISTER:  " + regname;
                entry->offset = 512; // special value that denotes a pseudo-entry
                /* prepare list of settings from this node */
                list<FuseSetting> *fuse_listing = new list<FuseSetting>;
                fuse_listing = this->get_lst_fuse ((*node_iterator), offset);
                /* add pseudo-setting in this list of settings */
                fuse_listing->push_front(*entry);
                /* add this settings list to the rest */
                list<FuseSetting>::iterator pos = (settings->fuse_settings)->begin();
                (settings->fuse_settings)->splice(pos, *fuse_listing);
                delete entry;
        }
        settings->fuse_bytes = fuse_byte_counter;

        /* print fuse setting list ---------------------------------------------------------------*/
        //cout << endl;
        //list<FuseSetting>::iterator iter0;
        //for (iter0 = (settings->fuse_settings)->begin(); iter0 != (settings->fuse_settings)->end(); ++iter0) {
        //                cout << (*iter0).fname << " ";
        //                cout << "mask[" << (*iter0).fmask << "] ";
        //                cout << "offset[" << (*iter0).offset << "] ";
        //                cout << "enum[" << (*iter0).fenum << "] ";
        //                cout << (*iter0).fdesc << endl;
        //}
        /*----------------------------------------------------------------------------------------*/

        /* go up to parent node */
        xml_node = xml_node->get_parent();
        /* loop through children (registers and enumerator nodes) */
        children = xml_node->get_children();
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                /* skip irrelevant nodes (namely registers) */
                if (((*node_iterator)->get_name()).compare("enumerator") != 0)
                        continue;
                /* prepare list with this enumerator's entries */
                list<OptionEntry> *enum_listing = new list<OptionEntry>;
                enum_listing = this->get_lst_enum ((*node_iterator));
                /* get enumerator name */
                Glib::ustring enum_name = this->get_att_value ((*node_iterator), "name");
                /* add new entry in map */
                (*settings->option_lists)[enum_name] = enum_listing;
        }

        /* print option_lists map -----------------------------------------------------------------*/
        //map<Glib::ustring, list<OptionEntry>*>::iterator iter1;
        //for (iter1 = (settings->option_lists)->begin(); iter1 != (settings->option_lists)->end(); ++iter1) {
        //        cout << iter1->first << " => " << endl;
        //        list<OptionEntry>::iterator iter2;
        //        for (iter2 = (*iter1->second).begin(); iter2 != (*iter1->second).end(); ++iter2)
        //                        cout << (*iter2).ename << endl;
        //}
        /*----------------------------------------------------------------------------------------*/

        this->settings = settings;
        return 1;
}

/*******************************************************************************
 * Parse fuse warnings
 *******************************************************************************/

int Micro::parse_warnings (xmlpp::Node *root_node)
{
        //cout << "PARSING WARNINGS..." << endl;

        list<FuseWarning>* warnings = new list<FuseWarning>;

        this->warnings = warnings;
        return 1;
}

/*******************************************************************************
 * Parse specifications
 *******************************************************************************/

int Micro::parse_specifications (xmlpp::Node *root_node)
{
        //cout << "PARSING SPECIFICATIONS..." << endl;

        DeviceSpecifications* specs = new DeviceSpecifications;
        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = nullptr;
        Glib::ustring txtvalue;
        gfloat numvalue;

        /* go to ADMIN node */
        xml_node = xml_node->get_first_child("ADMIN");
        /* go to SPEED node */
        xml_node = xml_node->get_first_child("SPEED");
        /* get node content */
        txtvalue = this->get_txt_value(xml_node);
        txtvalue.resize(txtvalue.size() - 3);
        specs->max_speed = txtvalue + " MHz";
        //cout << "speed: " << specs->max_speed << endl;
        /* go up to ADMIN node */
        xml_node = xml_node->get_parent();
        /* go to SIGNATURE node */
        xml_node = xml_node->get_first_child("SIGNATURE");
        /* get node content */
        specs->signature = this->get_signature_bytes(xml_node);
        //cout << "signature: \"" << specs->signature << "\"" << endl;
        /* go up to AVRPART node */
        xml_node = xml_node->get_parent();
        xml_node = xml_node->get_parent();
        /* go to MEMORY node */
        xml_node = xml_node->get_first_child("MEMORY");
        /* go to PROG_FLASH node */
        xml_node = xml_node->get_first_child("PROG_FLASH");
        /* get node content */
        txtvalue = this->get_txt_value(xml_node);
        numvalue = ::atof(txtvalue.c_str()) / 1024;
        txtvalue = float_to_string(numvalue);
        specs->flash_size = txtvalue + " KB";
        //cout << "flash: " << specs->flash_size << endl;
        /* go up to MEMORY node */
        xml_node = xml_node->get_parent();
        /* go to EEPROM node */
        tmp_node = xml_node->get_first_child("EEPROM");
        /* if no such node was found, it must be a device without EEPROM */
        if (!tmp_node) {
                specs->eeprom_exists = false;
                txtvalue = "0 Bytes";
                specs->sram_size = txtvalue;
        } else {
                specs->eeprom_exists = true;
                xml_node = tmp_node;
                /* get node content */
                txtvalue = this->get_txt_value(xml_node);
                numvalue = ::atof(txtvalue.c_str()) / 1024;
                /* check for zero eeprom size */
                if (numvalue == 0)
                        specs->eeprom_exists = false;
                /* express size in KBytes */
                if (numvalue > 1)
                        txtvalue = float_to_string(numvalue) + " KB";
                else
                        txtvalue += " Bytes";
                specs->eeprom_size = txtvalue;
                //cout << "eeprom: " << specs->eeprom_size << endl;
                /* go up to MEMORY node */
                xml_node = xml_node->get_parent();
        }
        /* go to INT_SRAM node */
        tmp_node = xml_node->get_first_child("INT_SRAM");
        /* if no such node was found, it must be a device without SRAM! */
        if (!tmp_node) {
                txtvalue = "0 Bytes";
                specs->sram_size = txtvalue;
        } else {
                xml_node = tmp_node;
                /* go to SIZE node */
                xml_node = xml_node->get_first_child("SIZE");
                /* get node content */
                txtvalue = this->get_txt_value(xml_node);
                numvalue = ::atof(txtvalue.c_str()) / 1024;
                if (numvalue > 1)
                        txtvalue = float_to_string(numvalue) + " KB";
                else
                        txtvalue += " Bytes";
                specs->sram_size = txtvalue;
                //cout << "sram_size: " << specs->eeprom_size << endl;
        }

        this->specifications = specs;
        return 1;
}

/*******************************************************************************
 * Go to the specified node (by attribute)
 *******************************************************************************/

xmlpp::Node* Micro::get_child_with_attr (xmlpp::Node* starting_node, Glib::ustring att_name, Glib::ustring att_value)
{
        xmlpp::Node *target = nullptr;
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        xmlpp::Node::NodeList children = starting_node->get_children();

        /* loop through child-nodes */
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        /* loop through attribute-nodes */
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                /* break inner loop if target has been pinpointed */
                                if ((*att_iterator)->get_name().compare(att_name) == 0)
                                        if ((*att_iterator)->get_value().compare(att_value) == 0) {
                                                //cout << (*att_iterator)->get_name() << " = " << (*att_iterator)->get_value() << endl;
                                                target = *node_iterator;
                                                break;
                                        }
                        }
                        /* break outer loop if target has been defined */
                        if (target != nullptr)
                                break;
                }
        }
        return target;
}

/*******************************************************************************
 * Get text value in specified node
 *******************************************************************************/

Glib::ustring Micro::get_txt_value (xmlpp::Node* starting_node)
{
        const xmlpp::TextNode *text = NULL;
        xmlpp::Node::NodeList::iterator iter;
        xmlpp::Node::NodeList children = starting_node->get_children();

        /* loop through child-nodes */
        for (iter = children.begin(); iter != children.end(); ++iter) {
                text = dynamic_cast<const xmlpp::TextNode*>(*iter);
                if (text)
                        break;
                else
                        continue;
        }
        return text->get_content();
}

/*******************************************************************************
 * Get the value of specified attribute
 *******************************************************************************/

Glib::ustring Micro::get_att_value (xmlpp::Node* xml_node, Glib::ustring att_name)
{
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        Glib::ustring data_string;

        if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(xml_node)) {
                /* loop through attribute-nodes */
                const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                        /* break inner loop if target has been pinpointed */
                        if ((*att_iterator)->get_name().compare(att_name) == 0) {
                                //cout << (*att_iterator)->get_name() << " = " << (*att_iterator)->get_value() << endl;
                                data_string = (*att_iterator)->get_value();
                                break;
                        }
                }
        }
        return data_string;
}

/*******************************************************************************
 * Get list of enumerator members
 *******************************************************************************/

list<OptionEntry>* Micro::get_lst_enum (xmlpp::Node* xml_node)
{
        list<OptionEntry> *enum_listing = new list<OptionEntry>;

        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        xmlpp::Node::NodeList children = xml_node->get_children();
        Glib::ustring tmp_string;
        guint max_value = 0;

        /* loop through child-nodes (enum nodes) */
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        /* loop through attribute-nodes */
                        OptionEntry* entry = new OptionEntry;
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                if ((*att_iterator)->get_name().compare("text") == 0) {
                                        entry->ename = (*att_iterator)->get_value();
                                        //cout << entry->ename << endl;
                                } else if ((*att_iterator)->get_name().compare("val") == 0) {
                                        string strvalue = ((*att_iterator)->get_value()).substr(2, 2);
                                        entry->value = stol (strvalue, nullptr, 16);
                                        if (max_value < entry->value)
                                                max_value = entry->value;
                                        //cout << entry->value << endl;
                                }
                        }
                        enum_listing->push_back(*entry);
                        delete entry;
                }
        }
        /* insert pseudo enumerator-entry with maximum value */
        OptionEntry* entry = new OptionEntry;
        entry->ename = "PSEUDO_ENTRY";
        entry->value = max_value;
        enum_listing->push_front(*entry);
        delete entry;
        return enum_listing;
}

/*******************************************************************************
 * Get list of settings
 *******************************************************************************/

list<FuseSetting>* Micro::get_lst_fuse (xmlpp::Node* xml_node, guint offset)
{
        list<FuseSetting> *fuse_listing = new list<FuseSetting>;

        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;

        xmlpp::Node::NodeList children = xml_node->get_children();
        Glib::ustring tmp_string;

        /* loop through child-nodes (bitfield nodes) */
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        /* loop through attribute-nodes */
                        FuseSetting* entry = new FuseSetting;
                        entry->fenum = ""; // must initialize to empty
                        entry->offset = offset;
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                if ((*att_iterator)->get_name().compare("name") == 0) {
                                        entry->fname = (*att_iterator)->get_value();
                                        //cout << entry->fname << endl;
                                } else if ((*att_iterator)->get_name().compare("text") == 0) {
                                        entry->fdesc = (*att_iterator)->get_value();
                                        //cout << entry->fdesc << endl;
                                } else if ((*att_iterator)->get_name().compare("enum") == 0) {
                                        entry->fenum = (*att_iterator)->get_value();
                                        //cout << entry->fenum << endl;
                                } else if ((*att_iterator)->get_name().compare("mask") == 0) {
                                        Glib::ustring strvalue = ((*att_iterator)->get_value()).substr(2, 2);
                                        entry->fmask = stol (strvalue, nullptr, 16);
                                        //cout << entry->fmask << endl;
                                }
                        }
                        if ((entry->fenum).size() == 0)
                                entry->single_option = true;
                        else
                                entry->single_option = false;
                        fuse_listing->push_back(*entry);
                        delete entry;
                }
        }
        return fuse_listing;
}

/*******************************************************************************
 * Get the device signature
 *******************************************************************************/

Glib::ustring Micro::get_signature_bytes (xmlpp::Node* signature_node)
{
        const xmlpp::TextNode *text = NULL;
        xmlpp::Node::NodeList::iterator iter;
        xmlpp::Node::NodeList children = signature_node->get_children();
        Glib::ustring signatrure;

        signatrure = "0x";
        for (iter = children.begin(); iter != children.end(); ++iter) {
                text = dynamic_cast<const xmlpp::TextNode*>(*iter);
                if (!text)
                        signatrure += (get_txt_value((*iter))).substr(1, 2);
        }
        return signatrure;
}

/*******************************************************************************
 * Crate the string representation of a float
 *******************************************************************************/

Glib::ustring Micro::float_to_string (gfloat number)
{
        string value(16, '\0');
        int chars_written = snprintf(&value[0], value.size(), "%.1f", number);
        value.resize(chars_written);

        return (Glib::ustring)value;
}
