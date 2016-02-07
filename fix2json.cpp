#include<string>
#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<iomanip>
#include<unistd.h>

#include<quickfix/Application.h>
#include<quickfix/MessageCracker.h>

using namespace std;

void wrap(const string& value, FIX::TYPE::Type& datatype) {
	switch (datatype) {
		case FIX::TYPE::Price:
		case FIX::TYPE::Int:
		case FIX::TYPE::Amt:
		case FIX::TYPE::Qty:
		case FIX::TYPE::SeqNum:
		case FIX::TYPE::Length:
		case FIX::TYPE::Float:
			cout << value;
		break;
		default:
			cout <<  "\"" << value <<  "\""; 
		break;
	}
}

void print_section(const FIX::FieldMap& map, FIX::DataDictionary& dictionary) {
	FIX::FieldMap::iterator i = map.begin();
	int fields = const_cast<FIX::FieldMap&>(map).getFieldCount(); int count = 0;
	for (; i != map.end(); ++i) {
		string name;
		dictionary.getFieldName((*i).first, name);
		if (name.empty())  {
			stringstream ss;
			ss << (*i).first;
			name = ss.str();
		}

		// Get the field type
		FIX::TYPE::Type type = FIX::TYPE::Unknown;
		dictionary.getFieldType( (*i).first, type);
	
		cout << "\"" << name << "\":";
		wrap((*i).second.getString(), type);
		cout << "" << (++count < fields ? "," : ""); 
	}


	FIX::FieldMap::g_iterator j = map.g_begin();
	for(; j != map.g_end(); ++j)
	{
		cout << ",\"" << j->first << "\":{";
		vector<FIX::FieldMap*>::const_iterator k;
		for(k = j->second.begin(); k != j->second.end(); ++k) {
			print_section( (*(*k)), dictionary);
		}
		cout << "}";
	}

}

void process(string& s, FIX::DataDictionary& dictionary, bool validate, bool use_namespace) {  
	try {
		const FIX::Message m(s, dictionary, validate);

		FIX::MsgType msg_type;
		const string messageType = m.getHeader().getField( msg_type ).getString() ;

		FIX::TargetCompID target_id;
		const string target = m.getHeader().getField( target_id ).getString();

		FIX::SenderCompID sender_id;
		const string sender = m.getHeader().getField( sender_id ).getString();

		string n;
		dictionary.getValueName( msg_type.getField(), msg_type, n);
		if (n.empty()) n = "Unknown (" + messageType + ")";

		cout << "{ \"type\":\"" << messageType << "\",\"sender\":\"" << sender << "\",\"target\":\"" << target << "\"";

		cout << "," << (use_namespace ? "\"header\":{" : "");
		print_section(m.getHeader(), dictionary);
		cout << (use_namespace ? "}" : "");

		cout << (m.getHeader().totalFields() > 0 ? "," :  "") << (use_namespace ? "\"fields\":{" : "");
		print_section(m, dictionary);
		cout << (use_namespace ? "}" : "");

		cout << (static_cast<FIX::FieldMap>(m).getFieldCount() > 0 ? "," : "") << (use_namespace ? "\"trailer\":{" : "");
		print_section(m.getTrailer(), dictionary);
		cout << (use_namespace ? "}}" : "}") << endl;

	} catch (FIX::Exception& e) {
		cerr << "Caught exception: " << e.detail << endl;
	}
};

void usage() {
	cout << "fix2json [options]" << endl;
	cout << "where options are: " << endl;
	cout << "-d <dictionary path> : use specified FIX dictionary" << endl;
	cout << "-v : enable validation" << endl;
	cout << "-s : disable stdio syncing (experimental)" << endl;
	cout << "-n : dont namespace fix header/fields/trailer" << endl;
	cout << "-h : print this message()";
	exit(1);
}

int main(int argc, char* argv[]) {
	std::vector<string> lines;
	const string opts("spd:nh");
	bool validate(false);
	bool use_namespace(true);
	string dictionary_path;

	int opt = 0;
	opt = getopt(argc, argv, opts.c_str());
	while (opt != -1) {	
		switch (opt) {
			case 'd':
				dictionary_path = optarg;
				break;
			case 'v':
				validate = true;
				break;
			case 's':
				::std::ios_base::sync_with_stdio(false);
				break;
			case 'n':
				use_namespace = false;
				break;
			case 'h':
				usage();
				break;

		}
		opt = getopt(argc, argv, opts.c_str());
	}	


	if (dictionary_path.empty())
		dictionary_path = "./FIX44.xml";
	FIX::DataDictionary dictionary(dictionary_path);
	
	string line;
	while (getline(cin, line)) {
		size_t pos  = line.find("8=FIX");
		if (pos != string::npos && pos > 0) {
			line = line.substr(pos, string::npos);
			process(line, dictionary, validate, use_namespace);
		}
	}

}
