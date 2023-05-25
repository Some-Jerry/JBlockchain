#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "picosha2.h"
#include "rapidJSON/include/rapidjson/document.h"
#include "rapidJSON/include/rapidjson/stringbuffer.h"
#include "rapidJSON/include/rapidjson/writer.h"
#include "rapidJSON/include/rapidjson/filereadstream.h"

using namespace std;
using namespace rapidjson;

// --- BLOCKCHAIN VARIABLES --- //
struct Block {
    string hash = "0";
    string data = "";
    string previoushash = "";
    string sender = "";
    string recipient = "";
    long long int nonce = 1;
};

vector<Block> blockchain;
vector<int> difficulty_list;
int current_difficulty = 2;
unordered_map<string, vector<string>> sender_map;
unordered_map<string, vector<string>> receiver_map;
string chainhash;

// --- END BLOCKCHAIN VARIABLES --- //

// --- BLOCKCHAIN METHODS --- //

// Return true if a string s begins with n zeros, 
bool starts_with_n_zeros(string s, int n) {
    // Make sure string is long enough to hold our zeros
    if (s.length() < n) {
        return false;
    }
    string zeros(n, '0');
    return s.substr(0, n) == zeros;
}

// Return true if the sender exists in sender_map
bool senderExists(string sender) {
    bool ret = false;
    if (sender_map.find(sender) != sender_map.end()) {
        ret = true;
    }
    return ret;
}

// Return true if the recipient exists in receiver_map
bool recipientExists(string recipient) {
    bool ret = false;
    if (sender_map.find(recipient) != sender_map.end()) {
        ret = true;
    }
    return ret;
}

// Hash a block based on a passed difficulty
string hashBlock(Block& block, int diff) {
    // Concat our string and generate a hash based on that string
    // data + prevHash + nonce + sender + recipient
    string string_to_hash = block.data + block.previoushash + to_string(block.nonce) + block.sender
        + block.recipient;
    string hashed_string;

    // Hash the string to hex using SHA256
    picosha2::hash256_hex_string(string_to_hash, hashed_string);

    // Hash until we have our requisite number of leading zeros
    while (!starts_with_n_zeros(hashed_string, diff)) {
        block.nonce++;
        string_to_hash = block.data + block.previoushash + to_string(block.nonce) + block.sender
            + block.recipient;
        picosha2::hash256_hex_string(string_to_hash, hashed_string);
    }
    return hashed_string;
}

// Generate a Genesis block for the blockchain
void generateGenesisBlock() {
    // Create our first block for all blockchains
    // only contains data, all other values set to "", nonce = 1

    Block block;

    block.data = "Leeroy Jenkins";
    difficulty_list.push_back(2);

    string newhash = hashBlock(block, difficulty_list[0]);

    block.hash = newhash;
    chainhash = newhash;

    blockchain.push_back(block);
}

// Add a block to the blockchain
void addBlock() {

    Block block;
    string data, sender, recipient, diff;

    // If the blockchain is new, create the first block
    if (blockchain.empty()) {
        generateGenesisBlock();
    }

    // Take in all values from user and store in the block
    cout << "Enter data: ";
    getline(cin, data);
    block.data = data;

    cout << "Enter sender: ";
    getline(cin, sender);
    block.sender = sender;

    cout << "Enter recipient: ";
    getline(cin, recipient);
    block.recipient = recipient;

    // Update some global variables (maps, difficulty list)

    // Add sender to sender_map 
    // Add recipient as value to sender
    if (!senderExists(sender)) {
        sender_map[sender] = {};
    }
    sender_map[sender].push_back(recipient);

    // Add recipient to receiver_map
    // Add sender as value to recipient
    if (!recipientExists(recipient)) {
        receiver_map[recipient] = {};
    }
    receiver_map[recipient].push_back(sender);

    // Prompt user for difficulty of block (if it differs from the default)
    cout << "Enter difficulty (Leave empty to assign default difficulty [" << current_difficulty << "]): ";
    getline(cin, diff);
    if (diff == "") { diff = to_string(current_difficulty); }
    difficulty_list.push_back(stoi(diff));

    // Set previoushash of current block to hash of previous block
    // Recall, this is before we add the current block to the blockchain
    Block prevBlock = blockchain[0 + (blockchain.size() - 1)];
    block.previoushash = prevBlock.hash;

    // Hash the block, update in block struct and chainhash
    cout << "\nBegin hashing new block...\n";

    string newhash = hashBlock(block, stoi(diff));

    cout << "\nFinished hashing new block!\n";

    block.hash = newhash;
    chainhash = newhash;

    // Finally, add block to blockchain
    blockchain.push_back(block);
}

// Return true if blockchain has no corruption
bool verifyBlockchain() {

    // Tracker variables
    string currHash;
    string prevHash;
    int diff_tracker = 0;

    // Check that each block hash is correct
    // Hash the block, then compare that hash to the current hash
    for (Block& block : blockchain) {
        currHash = hashBlock(block, difficulty_list[diff_tracker]);
        if (currHash != block.hash) { return false; }
        diff_tracker++;
    }

    // Check that each block prevHash is correct
    // Iterate through the blockchain comparing each current block's prevHash to the previous block's hash
    auto previt = blockchain.begin();
    for (auto it = blockchain.begin() + 1; it != blockchain.end(); ++it) {
        currHash = it->previoushash;
        prevHash = previt->hash;
        if (currHash != prevHash) { return false; }
        ++previt;
    }


    return true;
}

// Print each block's data/sender/recipient to the terminal
void viewBlockchain() {

    for (const Block& block : blockchain) {
        cout << "Data: " << block.data << " | " << " Sender: " << block.sender << " | " <<
            " Recipient: " << block.recipient << endl << "----------" << endl;
    }
}

// Choose a block and modify its data field
void corruptBlock() {

    string newdata, blockNum;

    // Select a block from the blockchain based on index
    cout << "Select a block in the range 0 to " << blockchain.size() - 1 << " (0 being the genesis block): ";
    getline(cin, blockNum);

    // Enter new data for the block and update the struct
    cout << "Enter the new data field for block " << blockNum << ":";
    getline(cin, newdata);

    blockchain[stoi(blockNum)].data = newdata;


}

// Rehash and update hashes for all blocks in the blockchain
void fixCorruption() {

    string currHash, prevHash;

    // Rehash genesis block
    currHash = hashBlock(blockchain[0], difficulty_list[0]);
    blockchain[0].hash = currHash;

    // Update previoushash for current block, then rehash each block
    int diff_tracker = 1; // Start at 1 since we hashed the Genesis block above
    auto previt = blockchain.begin();
    for (auto it = blockchain.begin() + 1; it != blockchain.end(); ++it) {
        prevHash = previt->hash;
        it->previoushash = prevHash;
        currHash = hashBlock(*it, difficulty_list[diff_tracker]);
        it->hash = currHash;

        ++previt;
        diff_tracker++;

    }
}

// Import blockchain from text file in JSON format
void importBlockchain() {

    // Prompt user for a valid import file name until it is provided
    string file_name;

    do
    {
        cout << "Enter the name of the input file: ";
        getline(cin, file_name);

        ifstream file(file_name); 

        if (!file) 
        {
            cout << "Input file doesn't exist or is not available for reading. Please try again." << std::endl;
        }
        else {
            break;
        }

    } while (true);

    ifstream file(file_name);

    string str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    // Parse the doc into str
    Document doc;
    doc.Parse(str.c_str());

    // Retrieve difficulty_list
    const Value& JSONdiff_list = doc["difficulty_list"];
    for (SizeType i = 0; i < JSONdiff_list.Size(); ++i) {
        difficulty_list.push_back(JSONdiff_list[i].GetInt());
    }

    // Retrieve receiver_map
    const Value& JSONreceiver_map = doc["receiver_map"];
    for (auto itr = JSONreceiver_map.MemberBegin(); itr != JSONreceiver_map.MemberEnd(); ++itr) {
        const string& receiver = itr->name.GetString();
        if (!recipientExists(receiver)) {
            receiver_map[receiver] = {};
        }

        const Value& senders = itr->value;
        for (SizeType i = 0; i < senders.Size(); ++i) {
            receiver_map[receiver].push_back(senders[i].GetString());
        }
    }

    // Retrieve sender_map
    const Value& JSONsender_map = doc["sender_map"];
    for (auto itr = JSONsender_map.MemberBegin(); itr != JSONsender_map.MemberEnd(); ++itr) {
        const string& sender = itr->name.GetString();
        if (!senderExists(sender)) {
            sender_map[sender] = {};
        }

        const Value& receivers = itr->value;
        for (SizeType i = 0; i < receivers.Size(); ++i) {
            sender_map[sender].push_back(receivers[i].GetString());
        }
    }

    // Retrieve chainhash
    const Value& JSONchainhash = doc["chainhash"];
    chainhash = JSONchainhash.GetString();

    // Retrieve blockchain
    const Value& JSONblockchain = doc["blockchain"];
    assert(JSONblockchain.IsArray());

    // For each block..
    for (SizeType i = 0; i < JSONblockchain.Size(); i++) {
        const Value& JSONblock = JSONblockchain[i];
        assert(JSONblock.IsObject());

        // Retreive block's struct variables
        const Value& previoushash = JSONblock["previoushash"];
        const Value& sender = JSONblock["sender"];
        const Value& recipient = JSONblock["recipient"];
        const Value& data = JSONblock["data"];
        const Value& nonce = JSONblock["nonce"];

        // Create the block
        Block block;

        // Assign values from text file to block
        block.previoushash = previoushash.GetString();
        block.sender = sender.GetString();
        block.recipient = recipient.GetString();
        block.data = data.GetString();
        block.nonce = nonce.GetInt();
        block.hash = hashBlock(block, difficulty_list[i]);

        // Add the block to the blockchain
        blockchain.push_back(block);
    }
}

// Export blockchain to text file in JSON format
void exportBlockchain() {

    // Prompt user for output file name
    string file_name;
    cout << "Please enter the name of the file (including .txt): ";
    getline(cin, file_name);

    // Create doc to be exported
    Document doc;
    doc.SetObject();

    // Add difficulty_list to doc
    Value diff_list(kArrayType);
    for (int i = 0; i < difficulty_list.size(); ++i) {
        diff_list.PushBack(difficulty_list[i], doc.GetAllocator());
    }
    doc.AddMember("difficulty_list", diff_list, doc.GetAllocator());

    // Add sender_map to doc
    Value sender_obj(kObjectType);

    for (auto& [key, value] : sender_map) {
        Value arr(kArrayType);
        for (int i = 0; i < value.size(); ++i) {
            Value str(value[i].c_str(), doc.GetAllocator());
            arr.PushBack(str, doc.GetAllocator());
        }
        Value k(key.c_str(), doc.GetAllocator());
        sender_obj.AddMember(k, arr, doc.GetAllocator());
    }
    doc.AddMember("sender_map", sender_obj, doc.GetAllocator());

    // Add receiver_map to doc
    Value receiver_obj(kObjectType);
    for (auto& [key, value] : receiver_map) {
        Value arr(kArrayType);
        for (int i = 0; i < value.size(); ++i) {
            Value str(value[i].c_str(), doc.GetAllocator());
            arr.PushBack(str, doc.GetAllocator());
        }
        Value k(key.c_str(), doc.GetAllocator());
        receiver_obj.AddMember(k, arr, doc.GetAllocator());
    }
    doc.AddMember("receiver_map", receiver_obj, doc.GetAllocator());

    // Add chainhash to doc
    Value chainhash_val(chainhash.c_str(), doc.GetAllocator());
    doc.AddMember("chainhash", chainhash_val, doc.GetAllocator());

    // Add blockchain to doc
    Value blockchain_arr(kArrayType);

    // For each block..
    for (int i = 0; i < blockchain.size(); ++i) {
        Value block_obj(kObjectType);

        // Add each block struct variable to the object
        Value prevhash_val(blockchain[i].previoushash.c_str(), doc.GetAllocator());
        block_obj.AddMember("previoushash", prevhash_val, doc.GetAllocator());

        Value sender_val(blockchain[i].sender.c_str(), doc.GetAllocator());
        block_obj.AddMember("sender", sender_val, doc.GetAllocator());

        Value recipient_val(blockchain[i].recipient.c_str(), doc.GetAllocator());
        block_obj.AddMember("recipient", recipient_val, doc.GetAllocator());

        Value data_val(blockchain[i].data.c_str(), doc.GetAllocator());
        block_obj.AddMember("data", data_val, doc.GetAllocator());

        block_obj.AddMember("nonce", blockchain[i].nonce, doc.GetAllocator());
        blockchain_arr.PushBack(block_obj, doc.GetAllocator());
    }
    doc.AddMember("blockchain", blockchain_arr, doc.GetAllocator());

    // Write JSON doc to a file
    ofstream ofs(file_name);
    if (ofs.is_open()) {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        ofs << buffer.GetString();
        ofs.close();
    }
    else {
        cerr << "Failed to open file for writing" << endl;
    }
}

// Set default difficult of the blockchain
void changeDifficulty() {

    // Take in difficulty from user, only accept values 1-7
    string new_difficulty;
    int new_diff;
    do {
        cout << "Enter a new difficulty value (1-7): ";
        getline(cin, new_difficulty);
        new_diff = stoi(new_difficulty);
    } while (new_diff < 1 || new_diff > 7);
    current_difficulty = new_diff;

}

// Print all recipients of a specified sender
void printRecipientsBySender() {

    // Request a sender name until a valid one is entered
    string sender_name;
    do {
        cout << "Enter the name of the sender (case sensitive!): ";
        getline(cin, sender_name);
    } while (!senderExists);

    cout << sender_name << ": ";

    // Output all recipients associated with that sender
    auto sender_it = sender_map.find(sender_name);
    const vector<string>& sender_values = sender_it->second;
    for (const string& value : sender_values) {
        cout << value << ", ";
    }
    cout << endl << "--------" << endl;


}

// Print all senders of a specified recipient
void printSendersByRecipient() {

    // Request a recipient name until a valid one is entered
    string recipient_name;
    do {
        cout << "Enter the name of the recipient (case sensitive!): ";
        getline(cin, recipient_name);
    } while (!recipientExists);

    cout << recipient_name << ": ";

    // Output all senders associated with that recipient
    auto recipient_it = receiver_map.find(recipient_name);
    const vector<string>& recipient_values = recipient_it->second;
    for (const string& value : recipient_values) {
        cout << value << ", ";
    }
    cout << endl << "--------" << endl;
}

// --- END BLOCKCHAIN METHODS --- //

// Main function
int main() {

    string choice;
    do {
        // print menu
        cout << "---------------------" << endl;
        cout << "0 - import blockchain" << endl;
        cout << "1 - add block" << endl;
        cout << "2 - verify blockchain" << endl;
        cout << "3 - view blockchain" << endl;
        cout << "4 - corrupt block" << endl;
        cout << "5 - fix corruption" << endl;
        cout << "6 - export blockchain" << endl;
        cout << "7 - change difficulty" << endl;
        cout << "8 - print all recipients by a sender" << endl;
        cout << "9 - print all senders by a recipient" << endl;
        cout << "10 - terminate program" << endl;
        cout << "Enter your choice: ";
        getline(cin, choice);

        switch (stoi(choice)) {
        case 0:
            importBlockchain();
            break;
        case 1:
            addBlock();
            break;
        case 2:
            if (verifyBlockchain()) {
                cout << "Blockchain is valid." << endl;
            }
            else {
                cout << "Blockchain is not valid." << endl;
            }
            break;
        case 3:
            viewBlockchain();
            break;
        case 4:
            corruptBlock();
            break;
        case 5:
            fixCorruption();
            break;
        case 6:
            exportBlockchain();
            break;
        case 7:
            changeDifficulty();
            break;
        case 8:
            printRecipientsBySender();
            break;
        case 9:
            printSendersByRecipient();
            break;
        case 10:
            cout << "Terminating program." << endl;
            break;
        default:
            cout << "Invalid choice. Please enter a number between 0 and 10." << endl;
            break;
        }
    } while (stoi(choice) != 10);

    return 0;
}
