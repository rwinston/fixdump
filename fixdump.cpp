#include<string>
#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<iomanip>

#include<boost/program_options.hpp>

#include<quickfix/Application.h>
#include<quickfix/MessageCracker.h>

#ifdef WIN32
#include <windows.h>
HANDLE stdout_handle;
CONSOLE_SCREEN_BUFFER_INFO console_info;
#endif

using namespace boost;
using namespace std;
namespace po = boost::program_options;

const string RED("\033[0;31m");  
const string RED_BOLD("\033[1;31m");    
const string YELLOW("\033[0;33m");
const string YELLOW_BOLD("\033[1;33m"); 
const string CYAN("\033[0;36m"); 
const string WHITE("\033[0;37m");
const string GREEN("\033[0;32m");
const string GREEN_BOLD("\033[1;32m");  

const int PAD = 32;
bool interrupted(false);

enum FIELDTYPE {
    FIELD_NAME,
    FIELD_VALUE,
    FIELD_SEPARATOR,
    SECTION_SEPARATOR,
    SENDER,
    TARGET,
    TYPE,
    MESSAGE_SEPARATOR
};


#ifdef WIN32
BOOL break_handler(DWORD type) {
    switch (type) {
        case CTRL_C_EVENT:
            interrupted = true;
            SetConsoleTextAttribute(stdout_handle, console_info.wAttributes);
            return TRUE;
            break;
    }
    return TRUE;
}
#endif

void color_code(int code) {
#ifdef WIN32
    switch (code) {
        case FIELD_NAME:
            SetConsoleTextAttribute(stdout_handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            break;
        case FIELD_SEPARATOR:
            SetConsoleTextAttribute(stdout_handle, 15 | FOREGROUND_INTENSITY);
            break;
        case FIELD_VALUE:
            SetConsoleTextAttribute(stdout_handle, 14);
            break;
        case TYPE:
            SetConsoleTextAttribute(stdout_handle, 14 | FOREGROUND_INTENSITY);
            break;
        case SENDER:
            SetConsoleTextAttribute(stdout_handle, 3 | FOREGROUND_INTENSITY);
            break;
        case MESSAGE_SEPARATOR:
            SetConsoleTextAttribute(stdout_handle, 14);
            break;
        case SECTION_SEPARATOR:
            SetConsoleTextAttribute(stdout_handle, 3);
            break;
    }
#else
    switch (code) {
        case FIELD_NAME:
            cout << GREEN_BOLD;
            break;
        case FIELD_SEPARATOR:
            cout <<  WHITE;
            break;
        case FIELD_VALUE:
            cout << YELLOW;
            break;
        case TYPE:
            cout << YELLOW_BOLD;
            break;
        case SENDER:
            cout << CYAN;
            break;
        case MESSAGE_SEPARATOR:
            cout << YELLOW;
            break;
        case SECTION_SEPARATOR:
            cout << CYAN;
            break;
    }
#endif
}

void print_section(const FIX::FieldMap& map, FIX::DataDictionary& dictionary) {
    FIX::FieldMap::iterator i = map.begin();
    for (; i != map.end(); ++i) {
        string name;
        dictionary.getFieldName((*i).first, name);
        if (name.empty())  {
            stringstream ss;
            ss << "Unknown (" << (*i).first << ")";
            name = ss.str();
        }

        string value_name;
        dictionary.getValueName((*i).first, map.getField( (*i).first), value_name);
        if (!value_name.empty()) value_name = " (" + value_name + ")";
        color_code(FIELD_NAME); cout <<  left << setw(PAD) << name;
        color_code(FIELD_SEPARATOR); cout  << ": ";
        color_code(FIELD_VALUE); cout  << (*i).second <<  value_name << endl;
    }

    FIX::FieldMap::g_iterator j;
    for(j = map.g_begin(); j != map.g_end(); ++j)
    {
        vector<FIX::FieldMap*>::const_iterator k;
        for(k = j->second.begin(); k != j->second.end(); ++k) {
            print_section( (*(*k)), dictionary);
        }
    }
}

void process(string& s, FIX::DataDictionary& dictionary, bool validate, bool exclude_admin) { 
    try {
        const FIX::Message m(s, dictionary, validate);

        FIX::MsgType msg_type;
        const string messageType = m.getHeader().getField( msg_type ).getString() ;

        if (exclude_admin && FIX::Message::isAdminMsgType( msg_type ))
            return;      

        FIX::TargetCompID target_id;
        const string target = m.getHeader().getField( target_id ).getString();

        FIX::SenderCompID sender_id;
        const string sender = m.getHeader().getField( sender_id ).getString();

        string n;
        dictionary.getValueName( msg_type.getField(), msg_type, n);
        if (n.empty()) n = "Unknown (" + messageType + ")";

        color_code(TYPE);
        cout << left << setw(28) << n << "\t";
        color_code(SENDER);
        cout << sender << " -> " << target << endl;   

        color_code(SECTION_SEPARATOR);
        cout << "----" << endl;
        print_section(m.getHeader(), dictionary);

        color_code(SECTION_SEPARATOR);
        cout << "----" << endl;
        print_section(m, dictionary);

        color_code(SECTION_SEPARATOR);
        cout << "----" << endl; 
        print_section(m.getTrailer(), dictionary);

        color_code(MESSAGE_SEPARATOR);
        cout << "================================" << endl; 

    } catch (FIX::Exception& e) {
        if (!interrupted) cerr << "Caught exception: " << e.detail << endl;
    }
};

void usage(const char* progname) {
    cout << "Usage: " << progname << " [-d dictionary] [-f fixversion] -h -v\n"
        "-d: dictionary: path to a dictionary file. If the path is not specified, a dictionary is searched using the following rules:\n"
        "\t* if the FIXBASE variable is set, read dictionary files from this directory;\n"
        "\t* if the FIXBASE variable is not set, read dictionary files from the current directory.\n"
        "-f: fixversion - Load a specific dictionary version. fixversion can be 42,42,44,50SP1, or 50SP2. This parameter is only useful when the -d parameter is not set.\n"
        "-x: Exclude admin messages\n"
        "-v: Validate FIX  messages on creation\n";
    exit(0);
}


int main(int argc, char* argv[]) {
    std::vector<string> lines;

    bool validate(false);
    string dictionary_path;
    string fix_version("44"); 
    bool exclude_admin(false);
    bool highlight(true);

    po::options_description desc("fix2json options");
    desc.add_options()
        ("help,h", "produce help message")
        ("validate,V", po::value<bool>(&validate)->default_value(false), "validate FIX messages")
        ("sync,s", "dont sync with stdio (may speed up intensive I/O)")
        ("exclude-admin,x", po::value<bool>(&exclude_admin)->default_value(false), "exclude admin messages")
        ("no-highlight,n", po::value<bool>(&highlight)->default_value(true), "dont highlight output")
        ("fix-version,f", po::value<string>(&fix_version)->default_value("44"), "load a specific dictionary version")
        ("dictionary,d", po::value<string>(&dictionary_path), "use custom FIX dictionary");


    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);   

    if (vm.count("help")) {
        usage(argv[0]);
    }

#ifdef WIN32
    stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(stdout_handle, &console_info);
    SetConsoleCtrlHandler( (PHANDLER_ROUTINE) break_handler, TRUE );
#endif

    if (dictionary_path.empty()) {
        string basedir = "./";
        if (getenv("FIXBASE") != NULL) { basedir = getenv("FIXBASE"); }
        dictionary_path = basedir + string("/FIX") + fix_version + string(".xml");
    }

    FIX::DataDictionary dictionary(dictionary_path);

    string line;
    while (std::getline(std::cin, line)) {
        size_t pos = line.find("8=FIX");
        if (pos != string::npos && pos >= 0) {
            line = pos == 0 ? line : line.substr(pos, string::npos);
            process(line, dictionary, validate, exclude_admin);
        }
    }

}
