#include <iostream>
#include <bitset>
#include <string>
#include <vector>
using namespace std;

typedef bitset<4> uint4_t; // NIBBLE (4 bits)
typedef unsigned char uint8_t;  // BYTE (8 bits)
typedef unsigned short uint16_t; // WORD (16 bits)
typedef unsigned long long uint128_t; // 128 bits

/*################ Constants ################*/

// Round constants for key expansion
const uint8_t R_CON[2] = { 0x80, 0x30 };

// Matrix for MixColumns
const uint8_t MIX[2][2] = {{0x1, 0x4}, {0x4, 0x1}};

// Substitution box (S-Box)
const uint8_t S_BOX[16] = {
    0x9, 0x4, 0xA, 0xB,
    0xD, 0x1, 0x8, 0x5,
    0x6, 0x2, 0x0, 0x3,
    0xC, 0xE, 0xF, 0x7
};

// Lookup table for multiplication by 4
const uint8_t GF_4[16] = { 0x0, 0x4, 0x8, 0xC, 0x3, 0x7, 0xB, 0xF, 0x6, 0x2, 0xE, 0xA, 0x5, 0x1, 0xD, 0x9 }; 

// Base64 characters
const string BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*################ Global Variables ################*/

// Bloco de 16 bits (4 nibbles)
uint4_t stateArray[2][2] = { 0 }; // 2x2 matrix to store nibbles
uint16_t roundKeys[3] = { 0 };    // Placeholder for round keys
vector<uint16_t> encryptedBlocks; // Vector to store encrypted blocks
vector<uint16_t> plainTextBlocks; // Vector to store plaintext blocks


/*################ Functions ################*/

// Function to encode data to Base64
string base64Encode(const vector<uint8_t>& data) {
    string encoded;
    int val = 0;
    int bits = -6;
    const int b63 = 0x3F;

    for (uint8_t byte : data) {
        val = (val << 8) + byte;
        bits += 8;
        while (bits >= 0) {
            encoded.push_back(BASE64_CHARS[(val >> bits) & b63]);
            bits -= 6;
        }
    }

    if (bits > -6) {
        encoded.push_back(BASE64_CHARS[((val << 8) >> (bits + 8)) & b63]);
    }

    while (encoded.size() % 4) {
        encoded.push_back('=');
    }

    return encoded;
}

uint8_t SubWord(uint8_t input) {
    return S_BOX[input];          // Perform substitution using S-Box
}

uint8_t RotWord(uint8_t input) {
    return (input << 4) | ((input >> 4) & 0xF); // Rotate nibbles
}

void ExpandKey(uint16_t masterKey, uint16_t roundKeys[]) {

    uint8_t b[6] = { 0 }; // 6 bytes

    // Key expansion
    roundKeys[0] = masterKey; // Master key
    b[0] = (masterKey >> 8) & 0xFF; // First uint8_t
    b[1] = masterKey & 0xFF;  // Second uint8_t

    b[2] = b[0] ^ SubWord(RotWord(b[1])) ^ R_CON[0]; // Substituition of the first uint8_t
    b[3] = b[2] ^ b[1];
    roundKeys[1] = (b[2] << 8) | b[3]; // First round key
    
    b[4] = b[2] ^ SubWord(RotWord(b[3])) ^ R_CON[1]; // Substitution of the second uint8_t
    b[5] = b[4] ^ b[3];
    roundKeys[2] = (b[4] << 8) | b[5]; // Second round key

    for (int i = 0; i < 3; i++) {
        cout << "Round Keys " << i << ": " << hex << roundKeys[i] << endl;
    }

    cout << endl;

}

void AddRoundKey(uint4_t stateArray[2][2], uint16_t roundKey) {

    uint8_t matrix_roundKey[2][2] = { 0 }; // 2x2 matrix for round key
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            matrix_roundKey[i][j] = (roundKey >> ((1 - i) * 4 + (1 - j) * 8)) & 0xF; // Extracting nibbles from the round key
        }
    }
    // XOR operation between the stateArray and the round key
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            stateArray[i][j] ^= matrix_roundKey[i][j]; // XOR with the round key
        }
    }

}

void SubNibbles(uint4_t stateArray[2][2]) {

    // Substitution of nibbles using S-Box
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            stateArray[i][j] = S_BOX[stateArray[i][j].to_ulong()];
        }
    }

}

void ShiftRows(uint4_t stateArray[2][2]) {

    // Shift rows operation
    uint4_t temp = stateArray[1][0];
    stateArray[1][0] = stateArray[1][1];
    stateArray[1][1] = temp;

}

void MixColumns(uint4_t stateArray[2][2]) {
    // MixColumns operation using the matrix {{1,4},{4,1}}
    const uint8_t n00 = stateArray[0][0].to_ulong();
    const uint8_t n01 = stateArray[0][1].to_ulong();
    const uint8_t n10 = stateArray[1][0].to_ulong();
    const uint8_t n11 = stateArray[1][1].to_ulong();

    // Performing the matrix multiplication
    stateArray[0][0] = uint4_t(n00 ^ GF_4[n10]);  // 1*n00 + 4*n10
    stateArray[0][1] = uint4_t(n01 ^ GF_4[n11]);  // 1*n01 + 4*n11
    stateArray[1][0] = uint4_t(GF_4[n00] ^ n10);  // 4*n00 + 1*n10
    stateArray[1][1] = uint4_t(GF_4[n01] ^ n11);  // 4*n01 + 1*n11
}

// Function to print the state array
void printState(){

    cout << "+---+---+" << endl;
    for (int i = 0; i < 2; i++) {
        cout << "| ";
        for (int j = 0; j < 2; j++) {
            cout << hex << uppercase << stateArray[i][j].to_ulong() << " | ";
        }
        cout << endl;
        cout << "+---+---+" << endl;
    }
    cout << endl;

}

void encrypt_saes_ecb(const uint16_t roundKeys[], uint16_t plainText) {
    
    // Load the plaintext block into the state array
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            stateArray[i][j] = (plainText >> ((1 - i) * 4 + (1 - j) * 8)) & 0xF;
        }
    }

    // Round 0: AddRoundKey[0]
    AddRoundKey(stateArray, roundKeys[0]);

    // Round 1: SubNibbles, ShiftRows, MixColumns, AddRoundKey[1]
    SubNibbles(stateArray);
    ShiftRows(stateArray);
    MixColumns(stateArray);
    AddRoundKey(stateArray, roundKeys[1]);

    // Round 2: SubNibbles, ShiftRows, AddRoundKey[2]
    SubNibbles(stateArray);
    ShiftRows(stateArray);
    AddRoundKey(stateArray, roundKeys[2]);

    // Combine the state array into a single 16-bit encrypted block
    uint16_t encryptedBlock = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            encryptedBlock |= (stateArray[i][j].to_ulong() << ((1 - i) * 4 + (1 - j) * 8));
        }
    }

    encryptedBlocks.push_back(encryptedBlock); // Store the encrypted block

}

int main () {
    
    cout << "ECB Demonstration!" << endl;
    cout << "The plainText: 'Veni, Vidi, Vici'" << endl;
    cout << "The key tested here is '0x7149' (16 bits)" << endl;
    cout << endl;

    string plainText = "Veni, Vidi, Vici"; // 16-bit plaintext
    uint16_t key = 0x7149; // 16-bit key

    // Convert the plaintext to a vector of uint16_t plainTextBlocks
    for (size_t i = 0; i < plainText.length(); i += 2) {
        uint16_t block = (static_cast<uint8_t>(plainText[i]) << 8) | (i + 1 < plainText.length() ? static_cast<uint8_t>(plainText[i + 1]) : 0);
        plainTextBlocks.push_back(block);
    }

    // Print the plaintext blocks
    cout << "##### Plaintext Blocks #####" << endl;
    cout << endl;
    for (size_t i = 0; i < plainTextBlocks.size(); i++) {
        cout << "Block " << i + 1 << ": " << hex << plainTextBlocks[i] << endl;
    }
    cout << endl;

    cout << "##### Round Keys #####" << endl;
    cout << endl;

    // Expand the key
    ExpandKey(key, roundKeys);

    // Encrypt each block
    for (size_t i = 0; i < plainTextBlocks.size(); i++) {
        encrypt_saes_ecb(roundKeys, plainTextBlocks[i]);
    }

    // Print the encrypted blocks
    cout << "##### Encrypted Blocks #####" << endl;
    cout << endl;
    for (size_t i = 0; i < encryptedBlocks.size(); i++) {
        cout << "Block " << i + 1 << ": " << hex << encryptedBlocks[i] << endl;
    }
    cout << endl;

    // Convert the encryptedBlocks to Base64
    vector<uint8_t> encryptedData;
    for (uint16_t block : encryptedBlocks) {
        encryptedData.push_back((block >> 8) & 0xFF); // First byte
        encryptedData.push_back(block & 0xFF);        // Second byte
    }
    string base64Encoded = base64Encode(encryptedData);
    cout << "Base64 Encoded: " << base64Encoded << endl;
    cout << endl;
    
    return 0;

}