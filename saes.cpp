#include <iostream>
#include <bitset>
#include <string>
#include <vector>
using namespace std;

typedef bitset<4> uint4_t; // NIBBLE (4 bits)
typedef unsigned char uint8_t;  // BYTE (8 bits)
typedef unsigned short uint16_t; // WORD (16 bits)

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
uint4_t stateArray[2][2] = { 0 }; // 2x2 matrix for 4 nibbles
uint16_t roundKeys[3] = { 0 }; // Placeholder for round keys


/*################ Functions ################*/

uint8_t gFunction(uint8_t byte){

    uint8_t rotByte = (S_BOX[byte >> 4] << 4) | S_BOX[byte & 0x0F]; // RotWord functionality
    uint8_t subByte = (rotByte << 4) | ((rotByte >> 4) & 0xF);      // SubWord functionality

    return subByte;
    
}

void ExpandKey(uint16_t masterKey, uint16_t roundKeys[]) {

    uint8_t b[6] = { 0 }; // 6 bytes

    // Key expansion
    roundKeys[0] = masterKey; // Master key
    b[0] = (masterKey >> 8) & 0xFF; // First uint8_t
    b[1] = masterKey & 0xFF;  // Second uint8_t

    b[2] = b[0] ^ (gFunction(b[1]) ^ R_CON[0]); // Substituition of the first uint8_t
    b[3] = b[2] ^ b[1];
    roundKeys[1] = (b[2] << 8) | b[3]; // First round key
    
    b[4] = b[2] ^ (gFunction(b[3]) ^ R_CON[1]); // Substitution of the second uint8_t
    b[5] = b[4] ^ b[3];
    roundKeys[2] = (b[4] << 8) | b[5]; // Second round key

    for (int i = 0; i < 3; i++) {
        cout << "Round Keys " << i << ": 0x" << hex << roundKeys[i] << endl;
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
            cout << "SubNibbles stateArray[" << i << "][" << j << "]: 0x" << hex << stateArray[i][j].to_ulong() << endl;
        }
    }

    cout << endl;

}

void ShiftRows(uint4_t stateArray[2][2]) {

    // Shift rows operation
    uint4_t temp = stateArray[1][0];
    stateArray[1][0] = stateArray[1][1];
    stateArray[1][1] = temp;

    cout << "ShiftRows stateArray[1][0]: 0x" << hex << stateArray[1][0].to_ulong() << endl;
    cout << "ShiftRows stateArray[1][1]: 0x" << hex << stateArray[1][1].to_ulong() << endl;
    cout << endl;

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

int main () {
    
    cout << "This is a Simplified AES Implementation at UnB 2025.1!" << endl;
    cout << "The plainText tested here is 'cd' (16 bits)" << endl;
    cout << "The key tested here is '0x7144' (16 bits)" << endl;
    cout << endl;

    

    // string plainText = "CAFE"; // 32-bit plaintext
    // uint16_t plainTextHex = 0x3F1B; // For testing purposes
    // uint16_t key = "0xF0CA"; // For testing purposes
    string plainText = "cd"; // 16-bit plaintext
    uint16_t key = 0x7144; // 16-bit key

    // Convert hex string to unsigned short
    //unsigned short int plainTextHex = static_cast<unsigned short int>(stoul(plainText, nullptr, 16));

    // Convert hex string to unsigned short
    uint16_t plainTextHex = 0;
    for (char c : plainText) {
        plainTextHex = (plainTextHex << 8) | static_cast<uint8_t>(c);
    }

    // Split the 16-bit plaintext into 4 nibbles and assign to the stateArray matrix
    stateArray[0][0] = (plainTextHex >> 12) & 0xF; // First uint4_t (most significant)
    stateArray[1][0] = (plainTextHex >> 8) & 0xF;  // Second uint4_t
    stateArray[0][1] = (plainTextHex >> 4) & 0xF;  // Third uint4_t
    stateArray[1][1] = plainTextHex & 0xF;         // Fourth uint4_t (least significant)

    cout << "##### Initial State - Plain Text #####" << endl;
    cout << endl;
    printState();
    cout << endl;

    cout << "##### Key Expansion ##### " << endl;
    cout << endl;
    ExpandKey(key, roundKeys);

    cout << "### AddRoundKey [0] ###" << endl;
    cout << endl;
    AddRoundKey(stateArray, roundKeys[0]);
    printState();

    cout << "### SubNibbles State - Round 1 ###" << endl;
    cout << endl;
    SubNibbles(stateArray);
    printState();

    cout << "### ShiftRows State - Round 1 ###" << endl;
    cout << endl;
    ShiftRows(stateArray);
    printState();

    cout << "### MixColumns State - Round 1 ###" << endl;
    cout << endl;
    MixColumns(stateArray);
    printState();

    cout << "### AddRoundKey [1] ###" << endl;
    cout << endl;
    AddRoundKey(stateArray, roundKeys[1]);
    printState();

    cout << "### SubNibbles State - Round 2 ###" << endl;
    cout << endl;
    SubNibbles(stateArray);
    printState();

    cout << "### ShiftRows State - Round 2 ###" << endl;
    cout << endl;
    ShiftRows(stateArray);
    printState();

    cout << "### AddRoundKey [2] ###" << endl;
    cout << endl;
    AddRoundKey(stateArray, roundKeys[2]);
    printState();

    cout << "##### Final State - Encrypted Text #####" << endl;
    cout << endl;
    printState();

    cout << "Encrypted Hex: 0x" << hex << uppercase << (stateArray[0][0].to_ulong() << 12 | stateArray[1][0].to_ulong() << 8 | stateArray[0][1].to_ulong() << 4 | stateArray[1][1].to_ulong()) << endl;
    cout << "Base64 Encoded: ";
    vector<uint8_t> encryptedData = { static_cast<uint8_t>(stateArray[0][0].to_ulong()), static_cast<uint8_t>(stateArray[1][0].to_ulong()), static_cast<uint8_t>(stateArray[0][1].to_ulong()), static_cast<uint8_t>(stateArray[1][1].to_ulong()) };
    string base64Encoded = base64Encode(encryptedData);
    cout << base64Encoded << endl;
    cout << endl;
    
    return 0;

}