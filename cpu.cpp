#include "c8common.hpp"


//typedef void (*FuncPtr)();

int hash_opcode(BYTE_16 opc){
    BYTE n1 = nth_nibble(opc, 1) >> 12;
    BYTE n4 = nth_nibble(opc, 4);

    if((n1==0x00)||(n1==0xe)||(n1==0xf)||(n1==0x8)){
        BYTE n3 = nth_nibble(opc, 3) >> 4;
        if(n4==5){
            if((n1==0xf)&&((n3==0x6)||(n3==0x1))){
                return(n1 << 4 | n3);
            }
        }
        return n1 << 4 | n4; 
    }
    return n1;
}

class CPU{
    public:
    typedef void(CPU::*FuncPtr)();
    BYTES_2 ir{};
    BYTE_8 registers[16]{};
	BYTE_8 memory[4096]{};
	BYTE_16 index{};
	BYTE_16 pc{};
	BYTE_16 stack[16]{};
	BYTE_8 sp{};
    BYTE_16 opcode{};
    BYTE_8 keypad[16];
    BYTE_8 dt{};
    BYTE_8 st{};
    uint32_t video[64 * 32]{};
    BYTE_8 delaytimer{}, soundtimer{};
    FuncPtr table[256]{};


    CPU(){
        pc = 0x200;
        //load font
        

        for(BYTE i = 0; i < 80; i++) 
            memory[i] = fontset[i];
        
        for(int i = 0; i < 256; i++){
            table[i] = &voidfunc;
        }


        
        //entries of format Axyz -> hashed to 0xA

        table[0x0] = &OP_00E0_CLS;
		table[0x1] = &OP_1nnn_JP_addr;
		table[0x2] = &OP_2nnn_CALL_addr;
		table[0x3] = &OP_3xkk_SEVx;
		table[0x4] = &OP_4xkk_SENVx;
		table[0x5] = &OP_5xy0_SEVxVy;
		table[0x6] = &OP_6xkk_LDVx;
		table[0x7] = &OP_7xkk_LDVx;
		
        table[0x9] = &OP_9xy0_SNEVxVy;
		table[0xA] = &OP_Annn_LDi_addr;
		table[0xB] = &OP_Bnnn_JPV0_addr;
		table[0xC] = &OP_Cxkk_RNDVx;
		table[0xD] = &OP_Dxyn_DRW;
        
        //entries of format AxyZ -> hashed to 0xAZ except 0xfx65 & 0xfx15

        table[0x80] = &OP_8xy0_SEVxVy;
        table[0x81] = &OP_8xy1_VxorVy;
        table[0x82] = &OP_8xy2_VxandVy;
        table[0x83] = &OP_8xy3_VxxorVy;
        table[0x84] = &OP_8xy4_VxplusVy;
        table[0x85] = &OP_8xy5_VxminusVy;
        table[0x86] = &OP_8xy6_VxSHR;
        table[0x87] = &OP_8xy7_VyminusVx;
        table[0x8e] = &OP_8xyE_SHLVx;

        table[0xe1] = &OP_ExA1_SKNPVx;
        table[0xee] = &OP_Ex9E_SKPVx;
        table[0xf7] = &OP_Fx07_LDVx;
        table[0xfa] = &OP_Fx0A_LDVxK;
        table[0xf1] = &OP_Fx15_LDTDx; //different hash
        table[0xf8] = &OP_Fx18_LDTDx;
        table[0xfe] = &OP_Fx1E_ADDIVx;
        table[0xf9] = &OP_Fx29_LDFVx;
        table[0xf3] = &OP_Fx33;
        table[0xf5] = &OP_Fx55_LDIVx;
        table[0xf6] = &OP_8xy7_VyminusVx; //different hash
        
        


    }

    FuncPtr get_func_ptr(BYTE_16 opcode){
        int index = hash_opcode(opcode);
	    return table[index];
    }

    void romload(std::string filename){
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (file.is_open()){
            std::streampos size = file.tellg();
            BYTE* buffer = new BYTE[size];

            file.seekg(0, std::ios::beg);
            file.read(buffer, size);
            file.close();

            
            for (long i = 0; i < size; ++i){
                memory[0x200 + i] = buffer[i];
            }

            
            delete[] buffer;
	    }

    }
    
    void voidfunc(){};

    //OPCODES
    
    void OP_0nnn_SYS_addr(){
        pc = last_n_bits(opcode, 12);
    }

    void OP_00E0_CLS(){
        for(int i = 0; i < 32 * 64; i++){
            video[i] = 0;}
    }

    void OP_0EE_RET(){
        pc = stack[sp];
        sp--;
    }

    void OP_1nnn_JP_addr(){
        pc = last_n_bits(opcode, 12);
    }

    void OP_2nnn_CALL_addr(){
        sp++;
        stack[sp] = pc;
        pc = last_n_bits(opcode, 12);
    }

    void OP_3xkk_SEVx(){
        BYTE_16 addr = last_n_bits(opcode, 12);
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_16 kk = nth_nibble(opcode, 3) | nth_nibble(opcode, 4);
        if(registers[x] == kk){
            pc += 2;
        }
    }

    void OP_4xkk_SENVx(){
        BYTE_16 addr = last_n_bits(opcode, 12);
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_16 kk = nth_nibble(opcode, 3) | nth_nibble(opcode, 4);
        if(registers[x] != kk){
            pc += 2;
        }
    }

    void OP_5xy0_SEVxVy(){
        BYTE_16 addr = last_n_bits(opcode, 12);
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        if(registers[x] == registers[y]){
            pc += 2;
        }
    }

    void OP_6xkk_LDVx(){
        BYTE_16 addr = last_n_bits(opcode, 12);
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_16 kk = nth_nibble(opcode, 3) | nth_nibble(opcode, 4);
        registers[x] = kk;
    }

    void OP_7xkk_LDVx(){
        BYTE_16 addr = last_n_bits(opcode, 12);
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_16 kk = nth_nibble(opcode, 3) | nth_nibble(opcode, 4);
        registers[x] += kk;
    }

    void OP_8xy0_SEVxVy(){
        
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        
        registers[x] = registers[y];
    }

    void OP_8xy1_VxorVy(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        
        registers[x] |= registers[y];
    }

    void OP_8xy2_VxandVy(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        
        registers[x] &= registers[y];
    }

    void OP_8xy3_VxxorVy(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        
        registers[x] ^= registers[y];
    }

    
    void OP_8xy4_VxplusVy(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        if(registers[x]+ registers[y] > 255){
            registers[0xf] = 1;}
        else{
            registers[0xf] = 0;
        }
        registers[x] += registers[y];
    }

    void OP_8xy5_VxminusVy(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        if(registers[x] > registers[y]){
            registers[0xf] = 1;}
        else{
            registers[0xf] = 0;
        }
        registers[x] -= registers[y];
    }

    void OP_8xy6_VxSHR(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        registers[x] >>= 1;
    }

    void OP_8xy7_VyminusVx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        if(registers[x] < registers[y]){
            registers[0xf] = 0;}
        else{
            registers[0xf] = 1;
        }
        registers[x] = registers[y] - registers[x];
    }

    void OP_8xyE_SHLVx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        registers[x] <<= 1;
    }

    void OP_9xy0_SNEVxVy(){

        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        
        if(registers[x] != registers[y]){
            pc += 2;
        }
    }

    void OP_Annn_LDi_addr(){
        ir = last_n_bits(opcode, 12);
    }

    void OP_Bnnn_JPV0_addr(){
        pc = last_n_bits(opcode, 12) + registers[0];
    }

    void OP_Cxkk_RNDVx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_16 kk = nth_nibble(opcode, 3) | nth_nibble(opcode, 4);
        
        int i = time(0);
        std::srand(i*i);
        registers[x] = std::rand()%255 & kk;

    }

    void OP_Dxyn_DRW(){

        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 y = nth_nibble(opcode, 3) >> 4;
        BYTE_8 height = nth_nibble(opcode, 4);

        BYTE_8 xPos = registers[x] % VIDEO_WIDTH;
        BYTE_8 yPos = registers[y] % VIDEO_HEIGHT;
        

        registers[0xF] = 0;

        for (unsigned int row = 0; row < height; ++row)
        {
            BYTE_8 spriteByte = memory[index + row];

            for (unsigned int col = 0; col < 8; ++col)
            {
                uint8_t spritePixel = spriteByte & (0x80u >> col);
                uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

                // Sprite pixel on
                if (spritePixel)
                {
                    // Screen pixel also on - collision
                    if (*screenPixel == 0xFFFFFFFF)
                    {
                        registers[0xF] = 1;
                    }

                    // Effectively XOR with the sprite pixel
                    *screenPixel ^= 0xFFFFFFFF;
                }
            }
        }

    }

    void OP_Ex9E_SKPVx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 down = registers[x];
        if(keypad[down]){
            pc += 2;
        }
    }

    void OP_ExA1_SKNPVx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        BYTE_8 down = registers[x];
        if(!keypad[down]){
            pc += 2;
        }
    }
    
    void OP_Fx07_LDVx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        registers[x] = delaytimer;
    }
    
    void OP_Fx0A_LDVxK(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        int j = 0;
        while(!keypad[j]){
            if(keypad[j]){
                registers[x] = j;
                break;
            }
            j = (j + 1)%16;
        }
    }

    void OP_Fx15_LDTDx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        dt = registers[x];
    }

    void OP_Fx18_LDTDx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        st = registers[x];
    }
    
    void OP_Fx1E_ADDIVx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        ir += registers[x];
    }

    void OP_Fx29_LDFVx(){
        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        
        ir = 0x200 + registers[x];
    }

    void OP_Fx33(){

        BYTE_8 x = nth_nibble(opcode, 2) >> 8;

        BYTE_8 val = registers[x];

        int hth = (val % 1000) / 100;
        int tth = (val % 100) / 10;
        int oth = (val % 10);

        registers[ir] = hth;
        registers[ir + 1] = tth;
        registers[ir + 2] = oth;
    }

    void OP_Fx55_LDIVx(){

        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        for(int i=0; i <= x; i++){
            registers[i] = memory[ir+i];
        }
    }


    void OP_Fx65_LDVxI(){

        BYTE_8 x = nth_nibble(opcode, 2) >> 8;
        for(int i=0; i <= x; i++){
            memory[ir+i] = registers[i];
        }
    }

    void Cycle();
    void PrintState();

};
void CPU::Cycle()
{
	// Fetching phase
	opcode = (memory[pc] << 8) | memory[pc + 1];

	pc += 2;

	// Decoding phase
    BYTE opcode_nibble = nth_nibble(opcode, 1) >> 12;

    FuncPtr decoded_function = get_func_ptr(opcode_nibble);

    // Execution phase
	((*this).*(decoded_function))();

	// Decrement the delay timer
	if (delaytimer > 0)
		--delaytimer;

	// Decrement the sound timer
	if (soundtimer > 0)
		--soundtimer;
}

void CPU::PrintState(){
    std::cout<<"Registers : ";
    for(int i = 0; i < 16; i++){
        std::cout<<(int)registers[i]<<" ";
    }
    std::cout<<"\n";
    std::cout<<"PC : "<<pc<<" IR : "<<ir<<std::endl;
    int j = 0;

    for(int a = 0; a < 32; a++){
        for(int b = 0; b < 64; b++){
            std::cout<<video[j];
            j++;
        }
        std::cout<<'\n';
    }
    system("cls");

}

typedef void(CPU::*FuncPtr)();

int main(){
    
    CPU C8 = CPU();
    std::string fname = std::string("pong.ch8");
    C8.romload(fname);
    
    for(int i = 0; i < 5000; i++){
        C8.Cycle();
        C8.PrintState();
    }

}
